/*	opendatacon
 *
 *	Copyright (c) 2014:
 *
 *		DCrip3fJguWgVCLrZFfA7sIGgvx1Ou3fHfCxnrz4svAi
 *		yxeOtDhDCXf1Z4ApgXvX5ahqQmzRfJ2DoX8S05SqHA==
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */
/*
 * TCPSocketManager.cpp
 *
 *  Created on: 2018-01-20
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */


#include <opendatacon/TCPSocketManager.h>

namespace odc
{


TCPSocketManager::TCPSocketManager
	(std::shared_ptr<odc::asio_service> apIOS,
	const bool aisServer,
	const std::string& aEndPoint,
	const std::string& aPort,
	const std::function<void(buf_t&)>& aReadCallback,
	const std::function<void(bool)>& aStateCallback,
	const size_t abuffer_limit,
	const bool aauto_reopen,
	const uint16_t aretry_time_ms,
	const std::function<void(const std::string&,const std::string&)>& aLogCallback,
	const bool useKeepalives,
	const unsigned int KeepAliveTimeout_s,
	const unsigned int KeepAliveRetry_s,
	const unsigned int KeepAliveFailcount):
	handler_tracker(std::make_shared<char>()),
	isConnected(false),
	pending_connections(0),
	pending_write(false),
	pending_read(false),
	manuallyClosed(true),
	pIOS(apIOS),
	isServer(aisServer),
	ReadCallback(aReadCallback),
	StateCallback(aStateCallback),
	LogCallback(aLogCallback),
	Keepalives(useKeepalives,KeepAliveTimeout_s,KeepAliveRetry_s,KeepAliveFailcount),
	queue_writebufs(&writebufs1),
	dispatch_writebufs(&writebufs2),
	pSock(nullptr),
	pSockStrand(pIOS->make_strand()),
	pRetryTimer(pIOS->make_steady_timer()),
	buffer_limit(abuffer_limit),
	auto_reopen(aauto_reopen),
	retry_time_ms(aretry_time_ms),
	ramp_time_ms(0),
	host_name(aEndPoint),
	service_name(aPort),
	EndpointIterator(),
	resolve_time(msSinceEpoch()),
	pAcceptor(nullptr)
{}


void TCPSocketManager::Open()
{
	pSockStrand->post([this,h{handler_tracker}](){Open(h);});
}


void TCPSocketManager::Close()
{
	auto tracker = handler_tracker;
	pSockStrand->post([this,tracker]()
		{
			LogCallback("debug","Connection manual close.");
			manuallyClosed = true;
			ramp_time_ms = 0;
			pRetryTimer->cancel();
			pAcceptor.reset();
			AutoClose(tracker);
		});
}


TCPSocketManager::~TCPSocketManager()
{
	//There may be some outstanding handlers if we're destroyed right after Close()
	std::weak_ptr<void> tracker = handler_tracker;
	handler_tracker.reset();

	while(!tracker.expired() && !pIOS->stopped())
		pIOS->poll_one();

	LogCallback("debug","Write total "+std::to_string(write_count)+" bytes");
}

//----------------Public^^^
//----------------Private

void TCPSocketManager::Write(shared_const_buffer buf)
{
	auto tracker = handler_tracker;
	pSockStrand->post([this,tracker,buf]()
		{
			if(!isConnected || manuallyClosed || pending_write)
			{
			      queue_writebufs->push_back(buf);
			      if(queue_writebufs->size() > buffer_limit)
			      {
			            LogCallback("warning","Write queue full: dropping "+std::to_string(queue_writebufs->begin()->size())+" bytes.");
			            queue_writebufs->erase(queue_writebufs->begin());
				}
			      return;
			}

			pending_write = true;
			auto pWriteSock = pSock;

			std::string remote_addr_str;
			asio::error_code addr_err;
			auto remote_end = pWriteSock->remote_endpoint(addr_err);
			if(!addr_err)
				remote_addr_str = remote_end.address().to_string()+":"+std::to_string(remote_end.port());
			else
				remote_addr_str = "'"+addr_err.message()+"'";

			asio::async_write(*pWriteSock,buf,asio::transfer_all(),pSockStrand->wrap([this,tracker,buf,pWriteSock,remote_addr_str](asio::error_code err_code, std::size_t n)
				{
					write_count += n;
					pending_write = false;
					if(err_code)
					{
					      LogCallback("debug","Async write to "+remote_addr_str+" ("+std::to_string(n)+" of "+std::to_string(buf.size())+" bytes) error: "+err_code.message());
					      if(buf.size() > n)
					      {
					            LogCallback("debug","Buffering "+std::to_string(buf.size()-n)+" bytes");
					            queue_writebufs->push_back(buf);
					            queue_writebufs->back() += n;
					            if(queue_writebufs->size() > buffer_limit)
					            {
					                  LogCallback("warning","Write queue full: dropping "+std::to_string(queue_writebufs->begin()->size())+" bytes.");
					                  queue_writebufs->erase(queue_writebufs->begin());
							}
						}
					}
					CheckLastWrite(pWriteSock,remote_addr_str,tracker);
				}));
		});
}

void TCPSocketManager::Open(std::shared_ptr<void> tracker)
{
	manuallyClosed = false;
	if(isConnected || pending_connections || pending_read)
		return;

	if(!EndPointResolved(tracker))
		return;

	asio::ip::tcp::resolver::iterator end;
	for(auto endpoint_it = EndpointIterator; endpoint_it != end; endpoint_it++)
	{
		auto addr_str = endpoint_it->endpoint().address().to_string()+":"+std::to_string(endpoint_it->endpoint().port());
		std::shared_ptr<asio::ip::tcp::socket> pCandidateSock = pIOS->make_tcp_socket();

		if(isServer)
			ServerOpen(pCandidateSock, endpoint_it, addr_str, tracker);
		else
			ClientOpen(pCandidateSock, endpoint_it, addr_str, tracker);
	}
}


bool TCPSocketManager::EndPointResolved(std::shared_ptr<void> tracker)
{
	asio::ip::tcp::resolver::iterator end;

	//resolve between connections or every 15mins
	if(EndpointIterator == end || (msSinceEpoch()-resolve_time) > 900000)
	{
		//need to resolve endpoint(s)
		std::shared_ptr<asio::ip::tcp::resolver> res = pIOS->make_tcp_resolver();
		res->async_resolve(host_name,service_name,pSockStrand->wrap(
			[this,res,tracker](asio::error_code err_code, asio::ip::tcp::resolver::iterator endpoint_it)
			{
				if(manuallyClosed)
					return;
				if(err_code)
				{
				      LogCallback("error","Resolution error: "+err_code.message());
				      AutoOpen(tracker);
				      return;
				}
				LogCallback("debug","Resolved endpoint(s).");
				EndpointIterator = endpoint_it;
				resolve_time = msSinceEpoch();
				AutoOpen(tracker);
			}));
		return false;
	}
	return true;
}


void TCPSocketManager::ServerOpen(std::shared_ptr<asio::ip::tcp::socket> pCandidateSock, asio::ip::tcp::resolver::iterator endpoint_it, std::string addr_str, std::shared_ptr<void> tracker)
{
	try
	{
		pAcceptor = pIOS->make_tcp_acceptor(endpoint_it);
		LogCallback("info","Listening on "+addr_str);
	}
	catch(std::exception& e)
	{
		LogCallback("error","Failed to start server on "+addr_str+". Exception: "+e.what());
		if(pending_connections == 0)
			AutoOpen(tracker);
		return;
	}
	pending_connections++;
	pAcceptor->async_accept(*pCandidateSock,pSockStrand->wrap([this,pCandidateSock,addr_str,tracker](asio::error_code err_code)
		{
			pending_connections--;

			std::string remote_addr_str;
			asio::error_code addr_err;
			auto remote_end = pCandidateSock->remote_endpoint(addr_err);
			if(!addr_err)
				remote_addr_str = remote_end.address().to_string()+":"+std::to_string(remote_end.port());
			else
				remote_addr_str = "'"+addr_err.message()+"'";

			if(!err_code)
				LogCallback("info","Connection accepted on "+addr_str+" from "+remote_addr_str);

			ConnectCompletionHandler(tracker,err_code,pCandidateSock,addr_str,remote_addr_str);
			pAcceptor.reset();
		}));
}


void TCPSocketManager::ClientOpen(std::shared_ptr<asio::ip::tcp::socket> pCandidateSock, asio::ip::tcp::resolver::iterator endpoint_it, std::string addr_str, std::shared_ptr<void> tracker)
{
	LogCallback("info","Connecting to "+addr_str);
	pending_connections++;
	pCandidateSock->async_connect(*endpoint_it,pSockStrand->wrap([this,pCandidateSock,addr_str,tracker](asio::error_code err_code)
		{
			pending_connections--;

			std::string local_addr_str;
			asio::error_code addr_err;
			auto local_end = pCandidateSock->local_endpoint(addr_err);
			if(!addr_err)
				local_addr_str = local_end.address().to_string()+":"+std::to_string(local_end.port());
			else
				local_addr_str = "'"+addr_err.message()+"'";

			if(!err_code)
				LogCallback("info","Connection established to "+addr_str+" from "+local_addr_str);

			ConnectCompletionHandler(tracker,err_code,pCandidateSock,local_addr_str,addr_str);
		}));
}


void TCPSocketManager::CheckLastWrite(std::shared_ptr<asio::ip::tcp::socket> pWriteSock, std::string remote_addr_str, std::shared_ptr<void> tracker)
{
	//if the connection is still the active one, keep writing
	if(pWriteSock == pSock && isConnected && !manuallyClosed)
		WriteBuffer(pWriteSock,remote_addr_str,tracker);
	else
	{ //this socket is finished with - we're responsible for shutting down send
		LogCallback("debug","Finished sending to "+remote_addr_str+" - shutting socket send");
		asio::error_code err;
		pWriteSock->shutdown(asio::ip::tcp::socket::shutdown_send,err);
		if(err)
			LogCallback("debug","Send shutdown failed from write handler: "+err.message());
		AutoOpen(tracker);
	}
}


void TCPSocketManager::WriteBuffer(std::shared_ptr<asio::ip::tcp::socket> pWriteSock, std::string remote_addr_str, std::shared_ptr<void> tracker)
{
	//if there's anything in the buffer sequence, write it
	if((queue_writebufs->size() || dispatch_writebufs->size()) && !pending_write)
	{
		if(dispatch_writebufs->size() == 0)
		{
			//swap buffers
			auto temp = queue_writebufs;
			queue_writebufs = dispatch_writebufs;
			dispatch_writebufs = temp;
		}

		pending_write = true;
		asio::async_write(*pWriteSock,*dispatch_writebufs,asio::transfer_all(),pSockStrand->wrap([this,pWriteSock,remote_addr_str,tracker](asio::error_code err_code, std::size_t n)
			{
				pending_write = false;
				write_count += n;

				if(err_code)
				{
				      size_t consumed = 0;
				      while(consumed < n)
				      {
				            if(dispatch_writebufs->begin()->size() > (n-consumed))
				            {
				                  (*(dispatch_writebufs->begin())) += (n-consumed);
				                  consumed = n;
				                  break;
						}
				            consumed += dispatch_writebufs->begin()->size();
				            dispatch_writebufs->erase(dispatch_writebufs->begin());
					}

				      size_t left = 0;
				      for(auto& b : *dispatch_writebufs)
						left += b.size();

				      LogCallback("debug","Buffer async write to "+remote_addr_str+" ("+std::to_string(n)+" of "+std::to_string(left+consumed)+" bytes) error: "+err_code.message());
				      LogCallback("debug","Leaving "+std::to_string(left)+" bytes in buffer");
				}
				else //wrote everything, connection still healthy
					dispatch_writebufs->clear();

				CheckLastWrite(pWriteSock,remote_addr_str,tracker);
			}));
	}
}


void TCPSocketManager::ConnectCompletionHandler(std::shared_ptr<void> tracker, asio::error_code err_code, std::shared_ptr<asio::ip::tcp::socket> pCandidateSock, std::string addr_str, std::string remote_addr_str)
{
	if(err_code)
	{
		LogCallback("error","Connection error from "+addr_str+" to "+remote_addr_str+" : "+err_code.message());
		if(pending_connections == 0)
			AutoOpen(tracker);
		return;
	}

	if(isConnected)
	{
		LogCallback("warning","Dropping redundant connection from "+addr_str+" to "+remote_addr_str);
		return;
	}

	pSock = pCandidateSock;
	SetTCPKeepalives(*pSock,Keepalives.enabled,Keepalives.idle_timeout_s,Keepalives.retry_interval_s,Keepalives.fail_count);
	isConnected = true;
	StateCallback(isConnected);
	ramp_time_ms = 0;
	Read(remote_addr_str,tracker);

	if(manuallyClosed)
	{ //must have been manually closed after we'd already initiated a connection
		//still close the socket nicely and read anything off the socket
		//but don't write anything
		LogCallback("debug","Closing unneeded connection on "+addr_str+" from "+remote_addr_str);
		AutoClose(tracker);
		return;
	}

	WriteBuffer(pSock,remote_addr_str,tracker);
}


void TCPSocketManager::Read(std::string remote_addr_str, std::shared_ptr<void> tracker)
{
	pending_read = true;
	asio::async_read(*pSock, readbuf, asio::transfer_at_least(1), pSockStrand->wrap([this,tracker,remote_addr_str](asio::error_code err_code, std::size_t n)
		{
			pending_read = false;
			if(n)
				ReadCallback(readbuf);
			if(err_code)
			{
			      auto level = (err_code == asio::error::eof) ? "info" : "warning";
			      LogCallback(level,"Async read from "+remote_addr_str+" ("+std::to_string(n)+" bytes) : "+err_code.message());
			      pSock->shutdown(asio::ip::tcp::socket::shutdown_receive,err_code);
			      if(err_code)
					LogCallback("debug","Read shutdown failed: "+err_code.message());
			      AutoClose(tracker);
			      AutoOpen(tracker);
			}
			else
				Read(remote_addr_str,tracker);
		}));
}


void TCPSocketManager::AutoOpen(std::shared_ptr<void> tracker)
{
	pSockStrand->post([this,tracker]()
		{
			if(!auto_reopen || manuallyClosed || isConnected || pending_connections || pending_read)
				return;

			if(retry_time_ms != 0)
			{
			      ramp_time_ms = retry_time_ms;
			}

			if(ramp_time_ms == 0)
			{
			      ramp_time_ms = 125;
			      LogCallback("debug","Trying to open socket...");
			      Open(tracker);
			}
			else
			{
			      pRetryTimer->expires_from_now(std::chrono::milliseconds(ramp_time_ms));
			      pRetryTimer->async_wait(pSockStrand->wrap([this,tracker](asio::error_code err_code)
					{
						if(manuallyClosed || err_code)
							return;
						ramp_time_ms *= 2;
						LogCallback("debug","Re-trying to open socket...");
						Open(tracker);
					}));
			}
		});
}


void TCPSocketManager::AutoClose(std::shared_ptr<void> tracker)
{
	pSockStrand->post([this,tracker]()
		{
			if(!isConnected)
			{
			      return;
			}
			LogCallback("debug","Connection auto close.");
			if(!pending_write) //if there's a pending write - the write handler will shutdown sending
			{
			      LogCallback("debug","Not sending - shutting socket send from auto-close");
			      asio::error_code err;
			      pSock->shutdown(asio::ip::tcp::socket::shutdown_send,err);
			      if(err)
					LogCallback("debug","Send shutdown failed from auto-close: "+err.message());
			}
			else if(!pending_read) //only time there's no pending read is if other side sent fin
			{
			      asio::error_code err;
			      pSock->cancel(err);
			      if(err)
					LogCallback("debug","Cancel pending write failed from auto-close: "+err.message());
			}
			isConnected = false;
			StateCallback(isConnected);
			//Force endpoint(s) resolution between connections
			EndpointIterator = asio::ip::tcp::resolver::iterator();
		});
}

} // namespace odc

