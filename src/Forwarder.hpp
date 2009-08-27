#ifndef _CBR_FORWARDER_HPP_
#define _CBR_FORWARDER_HPP_



#include "Utility.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"

#include "ForwarderUtilityClasses.hpp"
#include "ObjectMessageQueue.hpp"


namespace CBR
{
  class Object;
  class ServerIDMap;
  class LocationService;
  class ObjectSegmentation;
  class CoordinateSegmentation;
  class ServerMessageQueue;
  class ObjectMessageQueue;
  class Network;
  class Trace;
  class LoadMonitor;
  class Proximity;
  class ObjectConnection;

class Forwarder : public MessageDispatcher, public MessageRouter, public MessageRecipient
  {
    private:
    //Unique to forwarder
      std::deque<SelfMessage> mSelfMessages; //will be used in route.
      std::deque<OutgoingMessage> mOutgoingMessages; //will be used in route.


    //Shared with server
      Trace* mTrace;  //will be used in several places.
      CoordinateSegmentation* mCSeg;
      ObjectSegmentation* mOSeg;
      LocationService* mLocationService;
      ObjectMessageQueue* mObjectMessageQueue;
      ServerMessageQueue* mServerMessageQueue;
      LoadMonitor* mLoadMonitor;
      Proximity* mProximity;
      ServerID m_serv_ID;//Keeps copy of server id on forwarder.  necessary for sending messages to objects.

      Time* mCurrentTime;

      typedef std::vector<CBR::Protocol::Object::ObjectMessage*> ObjectMessageList;
      std::map<UUID,ObjectMessageList> mObjectsInTransit;
      typedef std::vector<ObjMessQBeginSend> ObjMessQBeginSendList;
      std::map<UUID,ObjMessQBeginSendList> queueMap;

    
    
      Time mLastSampleTime;
      Duration mSampleRate;

      typedef std::map<UUID, ObjectConnection*> ObjectConnectionMap;
      ObjectConnectionMap mObjectConnections;

    //Private Functions
      void processChunk(const Sirikata::Network::Chunk&chunk, const ServerID& source_server, bool forwarded_self_msg);

      // Delivery interface.  This should be used to deliver received messages to the correct location -
      // the server or object it is addressed to.
      void deliver(Message* msg);

      ServerID lookup(const Vector3f&); //returns the server id that is responsible for the 3d point Vector3f
      ServerID lookup(const UUID&); //
      void tickOSeg(const Time&t);
      const ServerID& serv_id() const; //used to access server id of the processes



    public:
      Forwarder(ServerID id);
      ~Forwarder(); //D-E-S-T-R-U-C-T-O-R
      void initialize(Trace* trace, CoordinateSegmentation* cseg, ObjectSegmentation* oseg, LocationService* locService, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, Time* currTime, Proximity* prox);

      void tick(const Time&t);


      // Routing interface for servers.  This is used to route messages that originate from
      // a server provided service, and thus don't have a source object.  Messages may be destined
      // for either servers or objects.  The second form will simply automatically do the destination
      // server lookup.
      // if forwarding is true the message will be stuck onto a queue no matter what, otherwise it may be delivered directly
      void route(Message* msg, const ServerID& dest_server, bool is_forward = false);
      void route(CBR::Protocol::Object::ObjectMessage* msg, bool is_forward = false);
  private:
      // This version is provided if you already know which server the message should be sent to
      void route(CBR::Protocol::Object::ObjectMessage* msg, ServerID dest_serv, bool is_forward);
  public:
      bool routeObjectHostMessage(CBR::Protocol::Object::ObjectMessage* obj_msg);

      void receiveMessage(Message* msg);

      void addObjectConnection(const UUID& dest_obj, ObjectConnection* conn);
      ObjectConnection* removeObjectConnection(const UUID& dest_obj);
      ObjectConnection* getObjectConnection(const UUID& dest_obj);
  };//end class Forwarder

} //end namespace CBR


#endif //_CBR_FORWARDER_HPP_
