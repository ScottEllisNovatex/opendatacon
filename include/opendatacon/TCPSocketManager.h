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
 * TCPSocketManager.h
 *
 *  Created on: 2018-01-20
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */

//TODO: probably move the code into a cpp in ODC library

//Helper class for managing access to a TCP socket, either as server or client
//Usage:
//	-- Construct
//	-- Call Open()
//	-- Wait for a connection (state callback)
//	-- Data will continuously be read from socket if available and passed to the read callback, unitl the socket is closed
//	-- Optionally Write() to the socket. Data will be buffered if the connection isn't open.
//	-- If the socket closes for any reason you'll get a state callback
//	-- Call Close() to intentionally close the socket 8-)

#ifndef TCPSOCKETMANAGER
#define TCPSOCKETMANAGER

#include <opendatacon/asio.h>
#include <string>
#include <functional>


namespace odc
{

typedef asio::basic_streambuf<std::allocator<char>> buf_t;

//buffer to track a data container
//T must be a container with a data(), size() and get_allocator() members
template <typename T>
class shared_const_buffer: public asio::const_buffer
{
public:
	shared_const_buffer(std::shared_ptr<T> pCon):
		asio::const_buffer(pCon->data(),pCon->size()*sizeof(pCon->get_allocator())),
		con(pCon)
	{}
	//Implement the ConstBufferSequence requirements
	//(so I don't have to put a single one in a container)
	typedef const shared_const_buffer* const_iterator;
	const_iterator begin() const
	{
		return this;
	}
	const_iterator end() const
	{
		return this + 1;
	}

private:
	std::shared_ptr<T> con;
};

template <typename Q>
class TCPSocketManager
{
public:
	TCPSocketManager
		(std::shared_ptr<odc::asio_service> apIOS,        //pointer to an asio io_service
		bool aisServer,                                   //Whether to act as a server or client
		const std::string& aEndPoint,                     //IP addr or hostname (to connect to if client, or bind to if server)
		const std::string& aPort,                         //Port to connect to if client, or listen on if server
		const std::function<void(buf_t&)>& aReadCallback, //Handler for data read off socket
		const std::function<void(bool)>& aStateCallback,  //Handler for communicating the connection state of the socket
		const size_t abuffer_limit                        //
		      = std::numeric_limits<size_t>::max(),       //maximum number of writes to buffer
		bool aauto_reopen = false,                        //Keeps the socket open (retry on error), unless you explicitly Close() it
		uint16_t aretry_time_ms = 0):                     //You can specify a fixed retry time if auto_open is enabled, zero means exponential backoff
		isConnected(false),
		manuallyClosed(true),
		pIOS(apIOS),
		isServer(aisServer),
		ReadCallback(aReadCallback),
		StateCallback(aStateCallback),
		pSock(pIOS->make_tcp_socket()),
		pReadStrand(pIOS->make_strand()),
		pWriteStrand(pIOS->make_strand()),
		pSockStrand(pIOS->make_strand()),
		pRetryTimer(pIOS->make_steady_timer()),
		buffer_limit(abuffer_limit),
		auto_reopen(aauto_reopen),
		retry_time_ms(aretry_time_ms),
		ramp_time_ms(0),
		EndpointIterator(pIOS->make_tcp_resolver()->resolve(aEndPoint,aPort)),
		pAcceptor(nullptr)
	{}

	void Open()
	{
		pSockStrand->post([this]()
			{
				manuallyClosed = false;
				if(isConnected)
					return;
				if(isServer)
				{
				      pAcceptor = pIOS->make_tcp_acceptor(EndpointIterator);
				      pAcceptor->async_accept(*pSock,[this](asio::error_code err_code)
						{
							ConnectCompletionHandler(err_code);
							pAcceptor.reset();
						});
				}
				else
				{
				      pSock->async_connect(*EndpointIterator,[this](asio::error_code err_code)
						{
							ConnectCompletionHandler(err_code);
						});
				}
			});
	}
	void Close()
	{
		pSockStrand->post([this]()
			{
				manuallyClosed = true;
				ramp_time_ms = 0;
				pRetryTimer->cancel();
				pAcceptor.reset();
				AutoClose();
			});
	}

	template <typename T>
	void Write(T&& aContainer)
	{
		//shared_const_buffer is a ref counted wraper that will delete the data in good time
		auto buf = shared_const_buffer<T>(std::make_shared<T>(std::move(aContainer)));

		pSockStrand->post([this,buf]()
			{
				if(!isConnected)
				{
				      pWriteStrand->post([this,buf]()
						{
							writebufs.push_back(buf);
							if(writebufs.size() > buffer_limit)
								writebufs.erase(writebufs.begin());
						});
				      return;
				}

				asio::async_write(*pSock,buf,asio::transfer_all(),pWriteStrand->wrap([this,buf](asio::error_code err_code, std::size_t n)
					{
						if(err_code)
						{
						      writebufs.push_back(buf);
						      if(writebufs.size() > buffer_limit)
								writebufs.erase(writebufs.begin());
						      AutoClose();
						      AutoOpen();
						      return;
						}
					}));
			});
	}

	~TCPSocketManager()
	{
		Close();
	}
	bool IsConnected() { return isConnected;  }

private:
	bool isConnected;
	bool manuallyClosed;
	std::shared_ptr<odc::asio_service> pIOS;
	const bool isServer;
	const std::function<void(buf_t&)> ReadCallback;
	const std::function<void(bool)> StateCallback;

	buf_t readbuf;
	std::vector<shared_const_buffer<Q>> writebufs;
	std::unique_ptr<asio::ip::tcp::socket> pSock;

	//Strand to sync access to read buffer
	std::unique_ptr<asio::io_service::strand> pReadStrand;
	//Strand to sync access to write buffers
	std::unique_ptr<asio::io_service::strand> pWriteStrand;
	//Strand to sync access to socket state
	std::unique_ptr<asio::io_service::strand> pSockStrand;

	//for timing open-retries
	std::unique_ptr<asio::steady_timer> pRetryTimer;

	size_t buffer_limit;

	//Auto open funtionality - see constructor for description
	bool auto_reopen;
	uint16_t retry_time_ms;
	uint16_t ramp_time_ms; //keep track of exponential backoff

	//Host/IP and Port are resolved into these:
	asio::ip::tcp::resolver::iterator EndpointIterator;
	std::unique_ptr<asio::ip::tcp::acceptor> pAcceptor;

	void ConnectCompletionHandler(asio::error_code err_code)
	{
		if(err_code)
		{
			//TODO: implement logging
			AutoOpen();
			return;
		}

		pSockStrand->post([this]()
			{
				isConnected = true;
				StateCallback(isConnected);
				ramp_time_ms = 0;
				//if there's anything in the buffer sequence, write it
				pWriteStrand->post([this]()
					{
						if(writebufs.size() > 0)
						{
						      auto n = asio::write(*pSock,writebufs,asio::transfer_all());
						      if(n == 0)
						      {
						            AutoClose();
						            AutoOpen();
						            return;
							}
						      writebufs.clear();
						}
					});
				Read();
			});
	}
	void Read()
	{
		asio::async_read(*pSock, readbuf, asio::transfer_at_least(1), pReadStrand->wrap([this](asio::error_code err_code, std::size_t n)
			{
				if(err_code)
				{
				      AutoClose();
				      AutoOpen();
				}
				else
				{
				      ReadCallback(readbuf);
				      Read();
				}
			}));
	}
	void AutoOpen()
	{
		pSockStrand->post([this]()
			{
				if(!auto_reopen || manuallyClosed)
					return;

				if(retry_time_ms != 0)
				{
				      ramp_time_ms = retry_time_ms;
				}

				if(ramp_time_ms == 0)
				{
				      ramp_time_ms = 125;
				      Open();
				}
				else
				{
				      pRetryTimer->expires_from_now(std::chrono::milliseconds(ramp_time_ms));
				      pRetryTimer->async_wait([this](asio::error_code err_code)
						{
							if (err_code != asio::error::operation_aborted)
							{
							      ramp_time_ms *= 2;
							      Open();
							}
						});
				}
			});
	}
	void AutoClose()
	{
		pSockStrand->post([this]()
			{
				if(!isConnected)
				{
				      return;
				}
				pSock->close();
				isConnected = false;
				StateCallback(isConnected);
			});
	}
};

} //namespace odc

#endif // TCPSOCKETMANAGER

