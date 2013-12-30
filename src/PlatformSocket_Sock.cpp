/// @file
/// @author  Boris Mikic
/// @version 1.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#if defined(_WIN32) && !defined(_WINRT)
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#define _NO_WIN_H
#include <hltypes/hplatform.h>

#ifdef _WIN32
#define _WIN32_WINNT 0x602
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

extern int h_errno;

#define SOCKET unsigned int
#define ioctlsocket ioctl
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SD_SEND SHUT_WR
#define SD_BOTH SHUT_RDWR

#define socketret_t ssize_t
#define opt_t int
#define optlen_t socklen_t
#define CONNECT_WOULD_BLOCK EINPROGRESS
#define EWOULDBLOCK EAGAIN
#endif

#include <hltypes/hlog.h>
#include <hltypes/hstream.h>
#include <hltypes/hstring.h>

#include "Ip.h"
#include "PlatformSocket.h"
#include "sakit.h"

namespace sakit
{
	static timeval interval = {5, 0};

	void PlatformSocket::platformInit()
	{
#if defined(_WIN32) && !defined(_WINRT)
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0)
		{
			hlog::error(sakit::logTag, "Error: " + hstr(result));
		}
#endif
	}

	void PlatformSocket::platformDestroy()
	{
#if defined(_WIN32) && !defined(_WINRT)
		int result = WSACleanup();
		if (result != 0)
		{
			hlog::error(sakit::logTag, "Error: " + hstr(result));
		}
#endif
	}

	PlatformSocket::PlatformSocket()
	{
		this->connected = false;
		this->sock = -1;
		this->info = NULL;
		memset(this->buffer, 0, sizeof(BUFFER_SIZE));
	}

	bool PlatformSocket::connect(Ip host, unsigned int port)
	{
		int result = 0;
		// create host info
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
#ifndef _ANDROID
		hints.ai_family = AF_INET;
#else
		hints.ai_family = PF_INET;
#endif
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_IP;
		result = getaddrinfo(host.getAddress().split("/", 1).first().c_str(), hstr(port).c_str(), &hints, &this->info);
		if (result != 0)
		{
			hlog::error(sakit::logTag, hstr::from_unicode(gai_strerrorW(result)));
			this->disconnect();
			return false;
		}
		// create socket
		this->sock = socket(this->info->ai_family, this->info->ai_socktype, this->info->ai_protocol);
		if (this->sock == SOCKET_ERROR)
		{
			hlog::debug(sakit::logTag, "socket() error!");
			this->_printLastError();
			this->disconnect();
			return false;
		}
		// open socket
		if (::connect(this->sock, this->info->ai_addr, this->info->ai_addrlen) == SOCKET_ERROR)
		{
			hlog::debug(sakit::logTag, "connect() error!");
			this->_printLastError();
			this->disconnect();
			return false;
		}
		this->connected = true;
		return true;
	}

	bool PlatformSocket::disconnect()
	{
		if (this->info != NULL)
		{
			freeaddrinfo(this->info);
			this->info = NULL;
		}
		if (this->sock != -1)
		{
			closesocket(this->sock);
			this->sock = -1;
		}
		bool previouslyConnected = this->connected;
		this->connected = false;
		return previouslyConnected;
	}

	bool PlatformSocket::receive(hsbase* stream, hmutex& mutex, int& maxBytes)
	{
		bool success = false;
		interval.tv_sec = 5;
		interval.tv_usec = 0;
		memset(&this->readSet, 0, sizeof(this->readSet));
		unsigned long received = 0;
#ifdef _WIN32
		u_long* read = (u_long*)&received;
#else
		uint32_t* read = (uint32_t*)&received;
#endif
		int result = 0;
		while (true)
		{
			/*
			// select socket
			FD_ZERO(&this->readSet);
			FD_SET(this->sock, &this->readSet);
			int result = select(0, &this->readSet, NULL, NULL, &interval);
			if (result == 0)
			{
				hlog::error(sakit::logTag, "Socket select timeout!");
				break;
			}
			if (result == SOCKET_ERROR)
			{
				hlog::debug(sakit::logTag, "select() error!");
				this->_printLastError();
				break;
			}
			//*/
			// control socket IO
			result = ioctlsocket(this->sock, FIONREAD, read);
			if (result == SOCKET_ERROR)
			{
				hlog::debug(sakit::logTag, "ioctlsocket() error!");
				this->_printLastError();
				break;
			}
			if (received == 0)
			{
				success = true;
				break;
			}
			received = recv(this->sock, this->buffer, hmin(maxBytes, BUFFER_SIZE), 0);
			if (received == SOCKET_ERROR)
			{
				hlog::debug(sakit::logTag, "recv() error!");
				this->_printLastError();
				break;
			}
			success = true;
			mutex.lock();
			stream->write_raw(this->buffer, received);
			mutex.unlock();
			maxBytes -= received;
			if (maxBytes == 0)
			{
				break;
			}
		}
		return success;
	}

	bool PlatformSocket::send(hsbase* stream)
	{
		int size = stream->size();
		int position = stream->position();
		unsigned char* data = new unsigned char[size];
		stream->rewind();
		stream->read_raw(data, size);
		stream->seek(position, hsbase::START);
		int sent;
		const char* buffer = (const char*)data;
		while (size > 0)
		{
			sent = ::send(this->sock, buffer, size, 0);
			buffer += sent;
			size -= sent;
		}
		delete [] data;
		return (size == 0);
	}

}
#endif