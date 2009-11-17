
#include "asyncConnectionGet.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <map>
#include <utility>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "../../SpaceContext.hpp"
#include <sirikata/network/IOStrandImpl.hpp>

#define NUM_MULTI 1

namespace CBR
{

//constructor
AsyncConnectionGet::AsyncConnectionGet(SpaceContext* con, IOStrand* str)
  : ctx(con),
    mStrand(str)
{
  mReady = NEED_NEW_SOCKET; //starts in the state that it's requesting a new socket.  Presumably asyncCraq reads that we need a new socket, and directly calls "initialize" on this class
 
}

int AsyncConnectionGet::numStillProcessing()
{
  return (int) (allOutstandingQueries.size());
}


AsyncConnectionGet::~AsyncConnectionGet()
{
  if (! NEED_NEW_SOCKET)
  {
    mSocket->close();
    delete mSocket;
  }
}



AsyncConnectionGet::ConnectionState AsyncConnectionGet::ready()
{
  return mReady;
}


//servicing function.  gets results of all the operations that we were processing.
void AsyncConnectionGet::tick(std::vector<CraqOperationResult*>&opResults_get, std::vector<CraqOperationResult*>&opResults_error, std::vector<CraqOperationResult*>&opResults_trackedSets)
{
  if (mPrevReadFrag.size() > 2000)
  {
    std::cout<<"\n\n\n\nHUGE mPrevReadFrag:  "<<mPrevReadFrag<<"\n\n";
    std::cout.flush();
  }
  
  if ((mOperationResultVector.size() !=0)||(mOperationResultErrorVector.size() !=0)||(mOperationResultTrackedSetsVector.size() !=0))
  {
    mTimesBetweenResults = 0;
  }

  if (allOutstandingQueries.size() != 0)
  {
    ++mTimesBetweenResults;
  }

  if (mTimesBetweenResults >  MAX_TIME_BETWEEN_RESULTS)
  {
    /*
    mSocket->close();
    delete mSocket;
    
    mReady = NEED_NEW_SOCKET;
    std::cout<<"\n\nReset: need new connection.  Times between: "<< mTimesBetweenResults <<" outstanding queries:  " <<allOutstandingQueries.size()  <<" \n\n\n";
    mTimesBetweenResults = 0;
    */
  }
  
  
  if (mOperationResultVector.size() != 0)
  {
    opResults_get.swap(mOperationResultVector);
    if (mOperationResultVector.size() != 0)
    {
      mOperationResultVector.clear();
    }
  }

  //  opResults_error = mOperationResultErrorVector;
  if (mOperationResultErrorVector.size() !=0)
  {
    opResults_error.swap( mOperationResultErrorVector);
    if (mOperationResultErrorVector.size() != 0)
    {
      mOperationResultErrorVector.clear();
    }
  }

  //  opResults_trackedSets = mOperationResultTrackedSetsVector;
  if (mOperationResultTrackedSetsVector.size() != 0)
  {
    opResults_trackedSets.swap(mOperationResultTrackedSetsVector);
    if (mOperationResultTrackedSetsVector.size() != 0)
    {
      mOperationResultTrackedSetsVector.clear();
    }
  }
}

//gives us a socket to connect to
void AsyncConnectionGet::initialize( boost::asio::ip::tcp::socket* socket,    boost::asio::ip::tcp::resolver::iterator it)
{
  mSocket = socket;
  mReady = PROCESSING;   //need to run connection routine.  so until we receive an ack that conn has finished, we stay in processing state.

  mHandlerState = false;
  mSocket->async_connect(*it, boost::bind(&AsyncConnectionGet::connect_handler,this,_1));  //using that tcp socket for an asynchronous connection.
  
  mPrevReadFrag = "";

  mTimesBetweenResults = 0;
  mAllResponseCount = 0;
  
}


//connection handler.
void AsyncConnectionGet::connect_handler(const boost::system::error_code& error)
{
  if (error)
  {
    mSocket->close();
    delete mSocket;
    mReady = NEED_NEW_SOCKET;

    std::cout<<"\n\nError in connection\n\n";
    return;
  }

  std::cout<<"\n\nbftm debug: asyncConnection: connected\n\n";

  mReady = READY;

  //  set_generic_read_result_handler();
  //  set_generic_read_error_handler();
  set_generic_stored_not_found_error_handler();


  //run any outstanding get queries.
  runReQuery();
}



//datakey should have a null termination character.
//public interface for the get command
bool AsyncConnectionGet::getMulti(CraqDataKey& dataToGet)
{
  static std::vector<IndividualQueryData*> tmpDataToGet;
    
  if (mReady != READY)
  {
    return false;
  }

  IndividualQueryData* iqd = new IndividualQueryData;
  iqd->is_tracking = false;
  iqd->tracking_number = 0;
  std::string tmpString = dataToGet;
  tmpString += STREAM_DATA_KEY_SUFFIX;
  strncpy(iqd->currentlySearchingFor,tmpString.c_str(),tmpString.size() + 1);
  iqd->gs = IndividualQueryData::GET;

  //need to add the individual query data to allOutstandingQueries.
  allOutstandingQueries.insert(std::pair<std::string, IndividualQueryData*> (tmpString, iqd));

  tmpDataToGet.push_back(iqd);
  if (tmpDataToGet.size() >= NUM_MULTI)
  {
    getMultiQuery(tmpDataToGet);
    tmpDataToGet.clear();
  }
  return true;
}



//bool AsyncConnectionGet::getMultiQuery(const CraqDataKey& dataToGet)
bool AsyncConnectionGet::getMultiQuery(const std::vector<IndividualQueryData*>& dtg)
{
  std::string query = "";

  static bool first = true;
  
  for (int s=0; s < (int)dtg.size(); ++s)
  {
    query.append(CRAQ_DATA_KEY_QUERY_PREFIX);
    query.append(dtg[s]->currentlySearchingFor); //this is the re
    query.append(CRAQ_DATA_KEY_QUERY_SUFFIX);
  }

//   struct timeval before, after;
//   gettimeofday(&before, NULL);

  if (first)
  {
    std::cout<<"\n\n\n"<<query<<"\n\n\n";
    first = false;
  }
  
  
  async_write((*mSocket),
              boost::asio::buffer(query),
              boost::bind(&AsyncConnectionGet::write_some_handler_get,this,_1,_2));

//   gettimeofday(&after, NULL);
//   double timeTaken = after.tv_sec - before.tv_sec;
//   timeTaken = timeTaken + (((double)(after.tv_usec - before.tv_usec))/1000000.0);
//   mTimesTaken.push_back(timeTaken);

  
  return true;
}


void AsyncConnectionGet::printStatisticsTimesTaken()
{
  double total = 0;

  for (int s=0; s< (int) mTimesTaken.size(); ++s)
  {
    total += mTimesTaken[s];
  }

  std::cout<<"\n\n\nTHIS IS TOTAL:   "<<total<<"\n\n";
  std::cout<<"\n\n\nTHIS IS AVG:     "<<total/((double) mTimesTaken.size())<<"\n\n";
  
}



//public interface for setting data in craq via this connection.
bool AsyncConnectionGet::set(CraqDataKey dataToSet, int  dataToSetTo, bool track, int trackNum)
{
  if (mReady != READY)
  {
    std::cout<<"\n\nbftm debug:  huge error\n\n";
    exit(1);
    return false;
  }

  IndividualQueryData* iqd    =    new IndividualQueryData;
  iqd->is_tracking            =                      track;
  iqd->tracking_number        =                   trackNum;
  std::string tmpStringData = dataToSet;
  strncpy(iqd->currentlySearchingFor,tmpStringData.c_str(),tmpStringData.size() + 1);
    //  iqd->currentlySearchingFor  =                  dataToSet;
  iqd->currentlySettingTo     =                dataToSetTo;
  iqd->gs                     =   IndividualQueryData::SET;
  
  std::string index = dataToSet;
  index += STREAM_DATA_KEY_SUFFIX;

  allOutstandingQueries.insert(std::pair<std::string,IndividualQueryData*> (index, iqd));  //logs that this 

  
  mReady = READY;

  //generating the query to write.
  std::string query;
  query.append(CRAQ_DATA_SET_PREFIX);
  query.append(dataToSet); //this is the re
  query += STREAM_DATA_KEY_SUFFIX; //bftm changed here.
  query.append(CRAQ_DATA_TO_SET_SIZE);
  query.append(CRAQ_DATA_SET_END_LINE);

  //convert from integer to string.
  std::stringstream ss;
  ss << dataToSetTo;
  std::string tmper = ss.str();
  for (int s=0; s< CRAQ_SERVER_SIZE - ((int) tmper.size()); ++s)
  {
    query.append("0");
  }
    
  query.append(tmper);
  query.append(STREAM_CRAQ_TO_SET_SUFFIX);
  
  query.append(CRAQ_DATA_SET_END_LINE);
  StreamCraqDataSetQuery dsQuery;    
  strncpy(dsQuery,query.c_str(), STREAM_CRAQ_DATA_SET_SIZE);

  
  //creating callback for write function
  mSocket->async_write_some(boost::asio::buffer(dsQuery,STREAM_CRAQ_DATA_SET_SIZE -2),
                            boost::bind(&AsyncConnectionGet::write_some_handler_set,this,_1,_2));

  return true;
}


//dummy handler for writing the set instruction.  (Essentially, if we run into an error from doing the write operation of a set, we know what to do.)
void AsyncConnectionGet::write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  static int thisWrite = 0;

  ++thisWrite;
  std::cout<<"\nwritten:  "<<thisWrite<<"\n";
  if (error)
  {
    printf("\n\nin write_some_handler_set\n\n");
    fflush(stdout);
    assert(false);
    killSequence();
  }
}

int AsyncConnectionGet::runReQuery()
{
  MultiOutstandingQueries::iterator it;

  int returner = 0;
  for (it = allOutstandingQueries.begin(); it != allOutstandingQueries.end(); ++it)
  {
    getQuery(it->second->currentlySearchingFor);
    ++ returner;
  }

  if (returner != 0)
    std::cout<<"\n\nThis is numToRequery: "<<returner<<"\n\n";

  
  if (! mHandlerState)
  {
    std::cout<<"\n\n***********************************HANDLER ISSUE*******************\n\n";
  }
  
  return returner;
}


//datakey should have a null termination character.
//public interface for the get command
bool AsyncConnectionGet::get(const CraqDataKey& dataToGet)
{
  if (mReady != READY)
  {
    return false;
  }

  IndividualQueryData* iqd = new IndividualQueryData;
  iqd->is_tracking = false;
  iqd->tracking_number = 0;
  std::string tmpString = dataToGet;
  tmpString += STREAM_DATA_KEY_SUFFIX;
  strncpy(iqd->currentlySearchingFor,tmpString.c_str(),tmpString.size() + 1);
  iqd->gs = IndividualQueryData::GET;

  //need to add the individual query data to allOutstandingQueries.
  allOutstandingQueries.insert(std::pair<std::string, IndividualQueryData*> (tmpString, iqd));

  return getQuery(dataToGet);
}



bool AsyncConnectionGet::getQuery(const CraqDataKey& dataToGet)
{
  //crafts query
  std::string query;
  query.append(CRAQ_DATA_KEY_QUERY_PREFIX);
  query.append(dataToGet); //this is the re
  query += STREAM_DATA_KEY_SUFFIX; //bftm changed here.
  query.append(CRAQ_DATA_KEY_QUERY_SUFFIX);

  StreamCraqDataKeyQuery dkQuery;
  strncpy(dkQuery,query.c_str(), STREAM_CRAQ_DATA_KEY_QUERY_SIZE);
    
  //sets write handler
  mSocket->async_write_some(boost::asio::buffer(dkQuery,STREAM_CRAQ_DATA_KEY_QUERY_SIZE-1),
                            boost::bind(&AsyncConnectionGet::write_some_handler_get,this,_1,_2));

  return true;
}


void AsyncConnectionGet::write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if (error)
  {
    printf("\n\nin write_some_handler_get\n\n");
    fflush(stdout);
    assert(false);
    killSequence();
  }
}


//This sequence needs to load all of its outstanding queries into the error results vector.
//
void AsyncConnectionGet::killSequence()
{
  mReady = NEED_NEW_SOCKET;
  mSocket->close();
  delete mSocket;

  
  printf("\n\n HIT KILL SEQUENCE \n\n");
  //  printf("\n\nHere is allResps:  %s\n\n ", mAllResps.c_str());
  
  fflush(stdout);
  assert(false);

  //  still need to load all the outstanding stuff into error vector
}



void AsyncConnectionGet::printOutstanding()
{
  MultiOutstandingQueries::iterator multiIter;

  std::cout<<"\n\n\n\n**************PRINTING OUTSTANDING*************************\n\n";
  
  for (multiIter = allOutstandingQueries.begin(); multiIter != allOutstandingQueries.end(); ++multiIter)
  {
    std::cout<<"\t"<<multiIter->first<<"      "<<multiIter->second<<"\n";
  }

  std::cout<<"\n\n";
  
}



//looks through the entire response string and processes out relevant information:
//  "ABCDSTORED000000000011000000000000000000000ZVALUE000000000000000000000000000000000Z120000000000YYSTORED000000000022000000000000000000000ZSTORED000000000000003300000000000000000ZNOT_FOUND000000000011000000000000000000000ZERROR000000000011000000000000000000000Z"
// returns true if anything matches the basic template.  false otherwise
bool AsyncConnectionGet::processEntireResponse(std::string response)
{
  //index from stored
  //not_found
  //value
  //error
  bool returner = false;
  bool firstTime = true;
  
  bool keepChecking = true;
  bool secondChecking;

  response = mPrevReadFrag + response;  //see explanation at end when re-setting mPrevReadFrag
  mPrevReadFrag = "";

  int numChecking = 0;
  
  while(keepChecking)
  {
    ++numChecking;

    if (numChecking > 100)
    {
      std::cout<<"\n\nThis is response:  "<<response<<"\n\n\n";
      std::cout.flush();
      assert(false);
    }
    
    std::cout<<"\n\nThis is response:   "<<response<<"\n\n\n";

    
    keepChecking   =                            false;

    //checks to see if there are any responses to get queries with data (also processes);
    secondChecking =             checkValue(response);  
    keepChecking   =   keepChecking || secondChecking;

    //checks to see if there are any responses to set responses that worked (stored) (also processes)
    secondChecking =            checkStored(response);  
    keepChecking   =   keepChecking || secondChecking;

    //checks to see if there are any responses to get queries that were not found  (also processes)
    secondChecking =          checkNotFound(response);  
    keepChecking   =   keepChecking || secondChecking;

    //checks to see if there are any error responses.  (also processes)
    secondChecking =             checkError(response); 
    keepChecking   =   keepChecking || secondChecking;

    if (firstTime)
    {
      returner  = keepChecking;  //states whether or not there were any full-formed expressions in this read
      firstTime =        false;
    }
  }

  mPrevReadFrag = response;  //apparently I've been running into the problem of what happens when data gets interrupted mid-stream
                             //The solution is to save the end bit of data that couldn't be parsed correctly (now in "response" variable and save it for appending to the next read.
  
  return returner;
}




/*
get abc
NOT_FOUND abc
set abc 3
def
STORED abc
get abc
VALUE abc 3
def
*/

// This function takes in a mutable string
// If there is a *full* not found message in response:
//  1) Removes it from response;
//  2) Add it to operation result queue
//  3) returns true
//
//Otherwise, returns false.
//
bool AsyncConnectionGet::checkNotFound(std::string& response)
{
  bool returner = false;
  size_t notFoundIndex = response.find(CRAQ_NOT_FOUND_RESP);
  
  std::string prefixed = "";
  std::string suffixed = "";
  
  
  if (notFoundIndex != std::string::npos)
  {
    prefixed = response.substr(0,notFoundIndex); //prefixed will be everything before the first STORED tag

    
    suffixed = response.substr(notFoundIndex); //first index should start with STORED_______

    size_t suffixNotFoundIndex = suffixed.find(CRAQ_NOT_FOUND_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP,STREAM_CRAQ_NOT_FOUND_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP));

    //Looking until we get to Z
    size_t endValueIndex = suffixed.find("YZ");
    if (endValueIndex != std::string::npos)
    {
      tmpSizeVec.push_back(suffixed.find("YZ") + 2);
    }


    
    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string notFoundPhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      notFoundPhrase = suffixed.substr(suffixNotFoundIndex, smallestNext);

      std::cout<<"\n\n\t\tSuffixed:  "<<suffixed<<"\n\n";
      
      suffixed = suffixed.substr(smallestNext);


      std::string dataKey;
      if ( parseValueNotFound(notFoundPhrase,dataKey))
      {
        //checks to make sure that the notFoundPhrase is fully and correctly formed
        processValueNotFound(dataKey);
        response = prefixed +suffixed;
        returner = true;
      }
      else
      {
        //the notfound phrase was incomplete.  we can't process it.  we return the response to its original state and return false immediately.
        response = response + notFoundPhrase + suffixed;
        return false;
      }
      
    }
    else
    {
      //means that the stored value is the last
      notFoundPhrase = suffixed.substr(suffixNotFoundIndex);
      response = prefixed;


      std::string dataKey;
      if ( parseValueNotFound(notFoundPhrase,dataKey))
      {
        //checks to make sure that the notFoundPhrase is fully and correctly formed
        processValueNotFound(dataKey);
        returner = true;
      }
      else
      {
        //the notfound phrase was incomplete.  we can't process it.  we return the response to its original state and return false immediately.
        response = response + notFoundPhrase;
        return false;
      }
      
    }
    //the above should have grabbed a phrase starting with "NOT_FOUND" from a sequence of characters
    //notFoundPhrase should store everything from NOT_FOUND up until either the end of response, or until a phrase keyed from a new keyword begins.  (eg. another "NOT_FOUND" or "STORED" or "VALUE" or "ERROR".)
        
    
  }
  return returner;
}

//checks a string to see if it's a correctly formatted not_found message.  If it is, grab data key from it, and return it in dataKey tab.
//if it is not formatted correctly, returns false
bool AsyncConnectionGet::parseValueNotFound(std::string response, std::string& dataKey)
{
  size_t notFoundIndex = response.find(CRAQ_NOT_FOUND_RESP);

  if (notFoundIndex == std::string::npos)
    return false;//means that there isn't actually a not found tag in this 

  if (notFoundIndex != 0)
    return false;//means that not found was in the wrong place.  return false so that can initiate kill sequence.

  //the not_found value was upfront.
  dataKey = response.substr(STREAM_CRAQ_NOT_FOUND_RESP_SIZE, CRAQ_DATA_KEY_SIZE);

  if ((int)dataKey.size() != CRAQ_DATA_KEY_SIZE)
    return false;  //didn't read enough of the key header

  return true;
}



//takes the data key associated with a not found message, and loads it into operation result vector.
void AsyncConnectionGet::processValueNotFound(std::string dataKey)
{
  //look through multimap to find 
  std::pair <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange =  allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;

  for(outQueriesIter = eqRange.first; outQueriesIter != eqRange.second; ++outQueriesIter)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::GET )
    {
      //says that this is a get.
      CraqOperationResult* cor  = new CraqOperationResult(0,
                                                          outQueriesIter->second->currentlySearchingFor,
                                                          outQueriesIter->second->tracking_number,
                                                          true,
                                                          CraqOperationResult::GET,
                                                          false); //this is a not_found, means that we add 0 for the id found

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
      
      mOperationResultVector.push_back(cor); //loads this not found result into the results vector.


      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter); //
    }
  }
}





//just see comments for not found stuff
//looks to see if we received a value in the response.
//eg: VALUE000000000000000000000000000000000Z120000000000YY
bool AsyncConnectionGet::checkValue(std::string& response)
{
  bool returner = false;
  size_t valueIndex = response.find(STREAM_CRAQ_VALUE_RESP);
  
  std::string prefixed = "";
  std::string suffixed = "";
  
  if (valueIndex != std::string::npos)
  {
    prefixed = response.substr(0,valueIndex); //prefixed will be everything before the first STORED tag

    suffixed = response.substr(valueIndex); //first index should start with STORED_______

    size_t suffixValueIndex = suffixed.find(STREAM_CRAQ_VALUE_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP,STREAM_CRAQ_VALUE_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP));
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP));
    //Looking until we get to Z
    size_t endValueIndex = suffixed.find("YZ");
    if (endValueIndex != std::string::npos)
    {
      tmpSizeVec.push_back(suffixed.find("YZ") + 2);
    }
    
    size_t smallestNext = smallestIndex(tmpSizeVec);

    std::string valuePhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      valuePhrase = suffixed.substr(suffixValueIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      //      response = prefixed +suffixed;

      std::string dataKey;
      int sID;
      if (parseValueValue(valuePhrase,dataKey,sID)) //sends it
      {
        processValueFound(dataKey,sID);
        response = prefixed + suffixed;
        returner = true;  //set returner here because we know we got a correct response.
      }
      else
      {
        response = response + valuePhrase + suffixed;
        return false;
      }
    }
    else
    {
      //means that the stored value is the last
      valuePhrase = suffixed.substr(suffixValueIndex);
      response = prefixed;

      std::string dataKey;
      int sID;
      if (parseValueValue(valuePhrase,dataKey,sID)) //sends it
      {
        processValueFound(dataKey,sID);
        returner = true;  //set returner here because we know we got a correct response.
      }
      else
      {
        response = response + valuePhrase;
        //        printf("\n\nOffending response: %s\n ",response.c_str() );
        return false;
      }
    }
  }
  return returner;
}


//takes the string associated with the datakey of a value found message and inserts it into operation value found
void AsyncConnectionGet::processValueFound(std::string dataKey, int sID)
{
  //look through multimap to find
  std::pair  <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange = allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;


  for(outQueriesIter = eqRange.first; outQueriesIter != eqRange.second; ++outQueriesIter)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::GET) //we only need to 
    {
      
      CraqOperationResult* cor  = new CraqOperationResult(sID,
                                                          outQueriesIter->second->currentlySearchingFor,
                                                          outQueriesIter->second->tracking_number,
                                                          true,
                                                          CraqOperationResult::GET,
                                                          false); //this is a not_found, means that we add 0 for the id found

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
      mOperationResultVector.push_back(cor);

      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter); //
      
    }
  }
}






//VALUE|CRAQ KEY|SIZE|VALUE
//returns the datakey and id associated with a value found response.
bool AsyncConnectionGet::parseValueValue(std::string response, std::string& dataKey,int& sID)
{
  size_t valueIndex = response.find(STREAM_CRAQ_VALUE_RESP);

    
  if (valueIndex == std::string::npos)
    return false;//means that value isn't actually in the response

  if (valueIndex != 0)
    return false;  //means that the value is in the wrong place.  return false so that can initiate kill sequence.

  
  //********Parse data key
  
  dataKey = response.substr(STREAM_CRAQ_VALUE_RESP_SIZE, CRAQ_DATA_KEY_SIZE);
  
  if ((int)dataKey.size() != CRAQ_DATA_KEY_SIZE)
    return false;  //didn't read enough of the key header

  //*******Parse server id
  
  //next two characters are size


  if (STREAM_CRAQ_VALUE_RESP_SIZE + CRAQ_DATA_KEY_SIZE + STREAM_SIZE_SIZE_TAG_GET_RESPONSE > (int)response.size())
  {
    //    printf("\n\nThis is the response we're trying to substring ERROR:  %s\n\n", response.c_str());
    //    fflush(stdout);
    return false;
  }
    
  std::string tmpSID = response.substr(STREAM_CRAQ_VALUE_RESP_SIZE + CRAQ_DATA_KEY_SIZE + STREAM_SIZE_SIZE_TAG_GET_RESPONSE, CRAQ_SERVER_SIZE); // the +2 is from 

  //the above is calling sig abrt

  
    
  if ((int)tmpSID.size() != CRAQ_SERVER_SIZE)
    return false; //didn't read enough of the key header to find server id

  //parse tmpSID to int
  sID = std::atoi(tmpSID.c_str());

  return true;
}






void AsyncConnectionGet::processStoredValue(std::string dataKey)
{
  //look through multimap to find
  std::pair  <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange = allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;

  for(outQueriesIter = eqRange.first; outQueriesIter != eqRange.second; ++outQueriesIter)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::SET) //we only need to 
    {
      CraqOperationResult* cor  = new CraqOperationResult(outQueriesIter->second->currentlySettingTo,
                                                          outQueriesIter->second->currentlySearchingFor,
                                                          outQueriesIter->second->tracking_number,
                                                          true,
                                                          CraqOperationResult::SET,
                                                          outQueriesIter->second->is_tracking); //this is a not_found, means that we add 0 for the id found

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';
      mOperationResultVector.push_back(cor);

      
      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter); //
      
    }
  }
}


bool AsyncConnectionGet::parseStoredValue(const std::string& response, std::string& dataKey)
{

  size_t storedIndex = response.find(STREAM_CRAQ_STORED_RESP);

  if (storedIndex == std::string::npos)
    return false;//means that there isn't actually a not found tag in this 

  if (storedIndex != 0)
    return false;//means that not found was in the wrong place.  return false so that can initiate kill sequence.

  //the not_found value was upfront.
  dataKey = response.substr(STREAM_CRAQ_STORED_RESP_SIZE, CRAQ_DATA_KEY_SIZE);

  if ((int)dataKey.size() != CRAQ_DATA_KEY_SIZE)
    return false;  //didn't read enough of the key header

  return true;
  
}




void AsyncConnectionGet::set_generic_stored_not_found_error_handler()
{
  boost::asio::streambuf * sBuff = new boost::asio::streambuf;
  //  const boost::regex reg ("(ND\r\n|ERROR\r\n)");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.
  //  const boost::regex reg ("Z\r\n");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.

  const boost::regex reg ("(Z\r\n|ERROR\r\n)");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.

  mHandlerState = true;
  
  //sets read handler
  //  boost::asio::async_read_until((*mSocket),
  //                                (*sBuff),
  //                                reg,
  //                                boost::bind(&AsyncConnectionGet::generic_read_stored_not_found_error_handler,this,_1,_2,sBuff));

  boost::asio::async_read_until((*mSocket),
                                (*sBuff),
                                reg,
                                mStrand->wrap(boost::bind(&AsyncConnectionGet::generic_read_stored_not_found_error_handler,this,_1,_2,sBuff)));

}




void AsyncConnectionGet::generic_read_stored_not_found_error_handler ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff)
{
  static std::string prevReadFlag = "Initial";
  
  mHandlerState = false;
  //  set_generic_stored_not_found_error_handler();
  if (error)
  {

    if (mSocket == NULL)
      return;

    
    std::cout<<"\n\nTHIS UPPER ERROR\n\n";
    
    killSequence();
    return;
  }
  
  std::istream is(sBuff);
  std::string response = "";
  std::string tmpLine;
  
  is >> tmpLine;
  
  
  while (tmpLine.size() != 0)
  {
    response.append(tmpLine);
    tmpLine = "";
    is >> tmpLine;
  }

  //  std::cout<<"\n\n\n\nResponse:    "<<response<<"\n\n\n\n";
  mAllResponseCount += countInstancesOf("VALUE",response);
  response = "";


  bool anything = true;
  //bool anything = processEntireResponse(response); //this will go through everything that we read out.  And sort it by errors, storeds, not_founds, and values.
  
  delete sBuff;

  prevReadFlag = response;

  set_generic_stored_not_found_error_handler();
}


int AsyncConnectionGet::getRespCount()
{
  return mAllResponseCount;
}

//counts the number of instances of toFind in initialString
int AsyncConnectionGet::countInstancesOf(const std::string& needle, const std::string& haystack)
{
  int returner   = 0;
  int indexBegin = 0;
  int sizeNeedle = needle.size();
  
  size_t index = haystack.find(needle,indexBegin);
  while (index != std::string::npos)
  {
    indexBegin = ((int)index) + sizeNeedle;
    index = haystack.find(needle,indexBegin);
    ++returner;
  }

  return returner;
}



// Looks for and removes all instances of complete stored messages
// Processes them as well
//
bool AsyncConnectionGet::checkStored(std::string& response)
{
  bool returner = false;
  size_t storedIndex = response.find(STREAM_CRAQ_STORED_RESP);
  
  std::string prefixed = "";
  std::string suffixed = "";
  
  
  if (storedIndex != std::string::npos)
  {
    prefixed = response.substr(0,storedIndex); //prefixed will be everything before the first STORED tag

    
    suffixed = response.substr(storedIndex); //first index should start with STORED_______

    size_t suffixStoredIndex = suffixed.find(STREAM_CRAQ_STORED_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP,STREAM_CRAQ_STORED_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP));
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP));
   
    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string storedPhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      storedPhrase = suffixed.substr(suffixStoredIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      response = prefixed +suffixed;
    }
    else
    {
      //means that the stored value is the last
      storedPhrase = suffixed.substr(suffixStoredIndex);

      response = prefixed;
    }

    std::string dataKey;
    if (parseStoredValue(storedPhrase, dataKey))
    {
      //      std::cout<<"\n\nDebug: value stored.\n";
      //      std::cout<<"\tdataKey: "<<dataKey<<"\n\n";
      returner = true;      
      processStoredValue(dataKey);
    }
    else
    {
      response = response + storedPhrase;
      return false;
    }
  }
  return returner;
}




//returns the smallest entry in the entered vector.
size_t AsyncConnectionGet::smallestIndex(std::vector<size_t> sizeVec)
{
  std::sort(sizeVec.begin(), sizeVec.end());
  
  return sizeVec[0];
}



bool AsyncConnectionGet::checkError(std::string& response)
{
  bool returner = false;
  size_t errorIndex = response.find(STREAM_CRAQ_ERROR_RESP);
  
  std::string prefixed = "";
  std::string suffixed = "";
  
  
  if (errorIndex != std::string::npos)
  {
    prefixed = response.substr(0,errorIndex); //prefixed will be everything before the first STORED tag

    
    suffixed = response.substr(errorIndex); //first index should start with STORED_______

    size_t suffixErrorIndex = suffixed.find(STREAM_CRAQ_ERROR_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP,STREAM_CRAQ_ERROR_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP));

    
    size_t endValueIndex = suffixed.find("YZ");
    if (endValueIndex != std::string::npos)
    {
      tmpSizeVec.push_back(suffixed.find("YZ") + 2);
    }
    
    
    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string errorPhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      errorPhrase = suffixed.substr(suffixErrorIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      returner = true;
      
      if ((prefixed.size() > 1000) || (suffixed.size() > 1000))
        printf("\n\n******Long prefix/suffix:   %s \n\n\n %s\n\n\n",prefixed.c_str(),suffixed.c_str());
      
      response = prefixed +suffixed;
    }
    else
    {
      //means that the stored value is the last
      errorPhrase = suffixed.substr(suffixErrorIndex);
      returner = true;
      response = prefixed;
    }

  }
  return returner;
}

}




