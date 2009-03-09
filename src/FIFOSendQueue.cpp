#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOSendQueue.hpp"
namespace CBR {

FIFOSendQueue::FIFOSendQueue(Network* net, uint32 bytes_per_second)
 : mNetwork(net),
   mRate(bytes_per_second),
   mRemainderBytes(0),
   mLastTime(0)
{
}

bool FIFOSendQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    mQueue.push(std::pair<ServerID,Network::Chunk>(destinationServer,msg));
    return true;
}
bool FIFOSendQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj){
    mQueue.push(std::pair<ServerID,Network::Chunk>(destinationServer,msg));
    return true;
}
void FIFOSendQueue::service(const Time& t){
    Duration sinceLast = t - mLastTime;
    uint32 free_bytes = mRemainderBytes + (uint32)(sinceLast.seconds() * mRate);

    while(!mQueue.empty() && mQueue.front().second.size() <= free_bytes) {
        bool ok=mNetwork->send(mQueue.front().first,mQueue.front().second,false,true,1);
        free_bytes -= mQueue.front().second.size();
        assert(ok&&"Network Send Failed");
        mQueue.pop();
    }

    mRemainderBytes = mQueue.empty() ? 0 : free_bytes;

    mLastTime = t;
}



}
