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
/// Defines a basic socket class.

#ifndef SAKIT_SOCKET_H
#define SAKIT_SOCKET_H

#include <hltypes/hltypesUtil.h>
#include <hltypes/hmutex.h>
#include <hltypes/hstream.h>
#include <hltypes/hstring.h>
#include <hltypes/hthread.h>

#include "SocketBase.h"
#include "sakitExport.h"

namespace sakit
{
	class ReceiverThread;
	class SenderThread;
	class SocketDelegate;
	class SocketThread;

	class sakitExport Socket : public SocketBase
	{
	public:
		~Socket();

		bool isSending();
		bool isReceiving();

		void update(float timeSinceLastFrame);

		int send(hstream* stream, int count = INT_MAX);
		int send(chstr data);
		bool sendAsync(hstream* stream, int count = INT_MAX);
		bool sendAsync(chstr data);
		bool stopReceiveAsync();

	protected:
		SocketDelegate* socketDelegate;
		SenderThread* sender;
		ReceiverThread* receiver;

		Socket(SocketDelegate* socketDelegate);

		virtual bool _sendAsync(hstream* stream, int count) = 0;
		virtual bool _sendAsync(chstr data);

		void _updateSending();
		void _updateReceiving();

		bool _checkSendStatus(State senderState);
		bool _checkStartReceiveStatus(State receiverState);
		bool _checkStopReceiveStatus(State receiverState);

	};

}
#endif
