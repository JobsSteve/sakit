/// @file
/// @author  Boris Mikic
/// @version 1.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
/// 
/// @section DESCRIPTION
/// 
/// Defines a base class for all socket operations.

#ifndef SAKIT_BASE_H
#define SAKIT_BASE_H

#include <hltypes/hltypesUtil.h>
#include <hltypes/hmutex.h>
#include <hltypes/hstream.h>
#include <hltypes/hstring.h>

#include "Host.h"
#include "sakitExport.h"
#include "State.h"

namespace sakit
{
	class PlatformSocket;

	class sakitExport Base
	{
	public:
		virtual ~Base();

		HL_DEFINE_GET(Host, localHost, LocalHost);
		HL_DEFINE_GET(unsigned short, localPort, LocalPort);

		virtual void update(float timeSinceLastFrame = 0.0f) = 0;

	protected:
		PlatformSocket* socket;
		State state;
		hmutex mutexState;
		Host localHost;
		unsigned short localPort;

		Base();

		int _sendDirect(hstream* stream, int count);
		int _receiveDirect(hstream* stream, int maxBytes);
		int _receiveFromDirect(hstream* stream, Host& remoteHost, unsigned short& remotePort);

		void __register();
		void __unregister();

	};

}
#endif
