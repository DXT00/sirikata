/*  Sirikata Network Utilities
 *  ASIOConnectAndHandshake.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "util/Platform.hpp"
#include "network/Asio.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "ASIOReadBuffer.hpp"
#include "TCPStream.hpp"
#include "MultiplexedSocket.hpp"
#include "ASIOConnectAndHandshake.hpp"
namespace Sirikata { namespace Network {
using namespace boost::asio::ip;
void ASIOConnectAndHandshake::checkHeaderContents(bool noDelay, 
                                                  unsigned int whichSocket,
                                                  Array<uint8,TCPStream::MaxWebSocketHeaderSize>* buffer,
                                                  const ErrorCode&error,
                                                  std::size_t bytes_received) {
    MultiplexedSocketPtr connection=mConnection.lock();
    if (connection) {
        char normalMode[]={0x48, 0x54, 0x54, 0x50, 0x2F, 0x31, 0x2E, 0x31, 0x20, 0x31, 0x30, 0x31, 0x20, 0x57, 0x65, 0x62,
                           0x20, 0x53, 0x6F, 0x63, 0x6B, 0x65, 0x74, 0x20, 0x50, 0x72, 0x6F, 0x74, 0x6F, 0x63, 0x6F, 0x6C,
                           0x20, 0x48, 0x61, 0x6E, 0x64, 0x73, 0x68, 0x61, 0x6B, 0x65, 0x0D, 0x0A,'\0'};
        size_t whereHeaderEnds=3;
        for (;whereHeaderEnds<bytes_received;++whereHeaderEnds) {
            if ((*buffer)[whereHeaderEnds]=='\n'&&
                (*buffer)[whereHeaderEnds-1]=='\r'&&
                (*buffer)[whereHeaderEnds-2]=='\n'&&
                (*buffer)[whereHeaderEnds-3]=='\r') {
                break;
            }
        }
        if (!memcmp(buffer->begin(),normalMode,sizeof(normalMode)-1/*not including null*/)) {
            if (mFinishedCheckCount==(int)connection->numSockets()) {
                mFirstReceivedHeader=*buffer;
            }
            if (mFinishedCheckCount>=1) {
                boost::asio::ip::tcp::no_delay option(noDelay);
                connection->getASIOSocketWrapper(whichSocket).getSocket().set_option(option);
                mFinishedCheckCount--;
                if (mFinishedCheckCount==0) {
                    connection->connectedCallback();
                }
                
                MemoryReference mb(buffer->begin()+whereHeaderEnds+1,bytes_received-(whereHeaderEnds+1));
                MakeASIOReadBuffer(connection,whichSocket,mb);
            }else {
                mFinishedCheckCount-=1;
            }
        }else {
                connection->connectionFailedCallback(whichSocket,"Bad header comparison "
                                                     +std::string((char*)buffer->begin(),bytes_received)
                                                     +" does not match "
                                                     +std::string(normalMode));
                mFinishedCheckCount-=connection->numSockets();
                mFinishedCheckCount-=1;
        }
    }
    delete buffer;
}
void ASIOConnectAndHandshake::connectToIPAddress(const ASIOConnectAndHandshakePtr& thus,
                                                 const Address& address,
                                                 bool no_delay,
                                                 unsigned int whichSocket,
                                                 const tcp::resolver::iterator &it,
                                                 const ErrorCode &error) {
    MultiplexedSocketPtr connection=thus->mConnection.lock();
    if (!connection) {
        return;
    }
    if (error) {
        if (it == tcp::resolver::iterator()) {
            //this checks if anyone else has failed
            if (thus->mFinishedCheckCount>=1) {
                //We're the first to fail, decrement until negative
                thus->mFinishedCheckCount-=connection->numSockets();
                thus->mFinishedCheckCount-=1;
                connection->connectionFailedCallback(whichSocket,error);
            }else {
                //keep it negative, indicate one further failure
                thus->mFinishedCheckCount-=1;
            }
        }else {
            tcp::resolver::iterator nextIterator=it;
            ++nextIterator;
            connection->getASIOSocketWrapper(whichSocket).getSocket()
                .async_connect(*it,
                               boost::bind(&ASIOConnectAndHandshake::connectToIPAddress,
                                           thus,
                                           address,
                                           no_delay,
                                           whichSocket,
                                           nextIterator,
                                           boost::asio::placeholders::error));
        }
    } else {
        connection->getASIOSocketWrapper(whichSocket)
            .sendProtocolHeader(connection,
                                address,
                                thus->mHeaderUUID,
                                connection->numSockets());
        Array<uint8,TCPStream::MaxWebSocketHeaderSize> *header=new Array<uint8,TCPStream::MaxWebSocketHeaderSize>;
        ASIOSocketWrapper::CheckCRLF headerCheck(header);
        boost::asio::async_read(connection->getASIOSocketWrapper(whichSocket).getSocket(),
                                boost::asio::buffer(header->begin(),(int)TCPStream::MaxWebSocketHeaderSize>(int)ASIOReadBuffer::sBufferLength?(int)ASIOReadBuffer::sBufferLength:(int)TCPStream::MaxWebSocketHeaderSize),
                                headerCheck,
                                boost::bind(&ASIOConnectAndHandshake::checkHeader,
                                            thus,
                                            no_delay,
                                            whichSocket,
                                            header,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }
}

void ASIOConnectAndHandshake::handleResolve(const ASIOConnectAndHandshakePtr& thus,
                                            const Address&address, 
                                            bool no_delay,
                                            const boost::system::error_code &error,
                                            tcp::resolver::iterator it) {
    MultiplexedSocketPtr connection=thus->mConnection.lock();
    if (!connection) {
        return;
    }
    if (error) {
        connection->connectionFailedCallback(error);
    }else {
        unsigned int numSockets=connection->numSockets();
        for (unsigned int whichSocket=0;whichSocket<numSockets;++whichSocket) {
            connectToIPAddress(thus,
                               address,
                               no_delay,
                               whichSocket,
                               it,
                               boost::asio::error::host_not_found);
        }
    }

}

void ASIOConnectAndHandshake::connect(const ASIOConnectAndHandshakePtr &thus,
                                      const Address&address, 
                                      bool no_delay){
    tcp::resolver::query query(tcp::v4(), address.getHostName(), address.getService());
    thus->mResolver.async_resolve(query,
                                  boost::bind(&ASIOConnectAndHandshake::handleResolve,
                                              thus,
                                              address,
                                              no_delay,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::iterator));

}

ASIOConnectAndHandshake::ASIOConnectAndHandshake(const MultiplexedSocketPtr &connection,
                                                 const UUID&sharedUuid):
    mResolver(connection->getASIOService()),
        mConnection(connection),
        mFinishedCheckCount(connection->numSockets()),
        mHeaderUUID(sharedUuid) {
}


} }
