/// @file
/// @version 1.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <stdlib.h>

#include <hltypes/hstream.h>
#include <hltypes/hthread.h>

#include "PlatformSocket.h"
#include "sakit.h"
#include "SocketDelegate.h"
#include "BroadcasterThread.h"

namespace sakit
{
	BroadcasterThread::BroadcasterThread(PlatformSocket* socket) : WorkerThread(socket), remotePort(0)
	{
		this->stream = new hstream();
	}

	BroadcasterThread::~BroadcasterThread()
	{
		delete this->stream;
	}

	void BroadcasterThread::_updateProcess()
	{
		bool result = this->socket->broadcast(this->adapters, this->remotePort, this->stream, (int)this->stream->size());
		hmutex::ScopeLock lock(&this->resultMutex);
		this->result = (result ? State::Finished : State::Failed);
		this->stream->clear();
	}

}
