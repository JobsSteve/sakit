/// @file
/// @author  Boris Mikic
/// @version 1.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#include <hltypes/hmutex.h>

#include "PlatformSocket.h"
#include "sakit.h"
#include "TcpServer.h"
#include "TcpServerDelegate.h"
#include "TcpServerThread.h"
#include "TcpSocket.h"

namespace sakit
{
	extern harray<Base*> connections;
	extern hmutex connectionsMutex;

	TcpServer::TcpServer(TcpServerDelegate* tcpServerDelegate, TcpSocketDelegate* acceptedDelegate) : Server(dynamic_cast<ServerDelegate*>(tcpServerDelegate))
	{
		this->serverThread = this->tcpServerThread = new TcpServerThread(this->socket, this->acceptedDelegate);
		this->tcpServerDelegate = tcpServerDelegate;
		this->acceptedDelegate = acceptedDelegate;
		this->socket->setConnectionLess(false);
		this->__register();
	}

	TcpServer::~TcpServer()
	{
		this->__unregister();
		foreach (TcpSocket*, it, this->sockets)
		{
			delete (*it);
		}
	}
	
	harray<TcpSocket*> TcpServer::getSockets()
	{
		this->_updateSockets();
		return this->sockets;
	}

	void TcpServer::update(float timeSinceLastFrame)
	{
		foreach (TcpSocket*, it, this->sockets)
		{
			(*it)->update(timeSinceLastFrame);
		}
		this->_updateSockets();
		this->tcpServerThread->mutex.lock();
		if (this->tcpServerThread->sockets.size() > 0)
		{
			harray<TcpSocket*> sockets = this->tcpServerThread->sockets;
			this->tcpServerThread->sockets.clear();
			this->tcpServerThread->mutex.unlock();
			this->sockets += sockets;
			foreach (TcpSocket*, it, sockets)
			{
				this->tcpServerDelegate->onAccepted(this, (*it));
			}
		}
		else
		{
			this->tcpServerThread->mutex.unlock();
		}
		Server::update(timeSinceLastFrame);
	}

	TcpSocket* TcpServer::accept(float timeout)
	{
		TcpSocket* tcpSocket = NULL;
		this->tcpServerThread->mutex.lock();
		State state = this->tcpServerThread->_state;
		this->tcpServerThread->mutex.unlock();
		if (this->_canStart(state))
		{
			tcpSocket = new TcpSocket(this->acceptedDelegate);
			connectionsMutex.lock();
			connections -= tcpSocket;
			connectionsMutex.unlock();
			float retryTimeout = sakit::getRetryTimeout() * 1000.0f;
			timeout *= 1000.0f;
			while (true)
			{
				if (!this->socket->listen())
				{
					delete tcpSocket;
					tcpSocket = NULL;
					break;
				}
				if (this->socket->accept(tcpSocket))
				{
					this->sockets += tcpSocket;
					break;
				}
				timeout -= retryTimeout;
				if (timeout <= 0.0f)
				{
					delete tcpSocket;
					tcpSocket = NULL;
					break;
				}
				hthread::sleep(retryTimeout);
			}
		}
		return tcpSocket;
	}

	void TcpServer::_updateSockets()
	{
		harray<TcpSocket*> sockets = this->sockets;
		this->sockets.clear();
		foreach (TcpSocket*, it, sockets)
		{
			if ((*it)->isConnected())
			{
				this->sockets += (*it);
			}
			else
			{
				delete (*it);
			}
		}
	}
	
}
