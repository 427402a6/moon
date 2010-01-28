/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * messaging.h: client side messaging
 *
 * Copyright 2009 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */


#ifndef __MOON_MESSAGING_H__
#define __MOON_MESSAGING_H__

#include "dependencyobject.h"
#include "pal.h"

/* @Namespace=None,ManagedEvents=Manual */
class LocalMessageReceiver : public DependencyObject {
public:
	/* @GenerateCBinding,GeneratePInvoke */
	LocalMessageReceiver (const char *receiverName,
			      ReceiverNameScope namescope);

	/* @GenerateCBinding,GeneratePInvoke */
	const char *GetReceiverName () { return receiverName; }

	/* @GenerateCBinding,GeneratePInvoke */
	ReceiverNameScope GetReceiverNameScope () { return namescope; }

	/* @GenerateCBinding,GeneratePInvoke */
	void SetAllowedSenderDomains (char **allowedSenderDomains, int count);

	/* @GenerateCBinding,GeneratePInvoke */
	void ListenWithError (MoonError *error);

	/* @GenerateCBinding,GeneratePInvoke */
	void DisposeWithError (MoonError *error);

	/* @DelegateType=EventHandler<MessageReceivedEventArgs> */
	const static int MessageReceivedEvent;

protected:
	virtual ~LocalMessageReceiver();

private:
	static char* MessageReceivedHandler (const char *msg, gpointer data);
	char* MessageReceived (const char *msg);

	char *receiverName;
	char *receiverDomain;

	char **allowedSenderDomains;
	int allowedSenderDomainsCount;

	ReceiverNameScope namescope;
	MoonMessageListener *listener;
};

/* @Namespace=None,ManagedEvents=Manual */
class LocalMessageSender : public DependencyObject {
public:
	/* @GenerateCBinding,GeneratePInvoke */
	LocalMessageSender (const char *receiverName, const char *receiverDomain);

	/* @GenerateCBinding,GeneratePInvoke */
	void SendAsyncWithError (const char *msg, gpointer managedUserState, MoonError *error);

	/* @DelegateType=EventHandler<SendCompletedEventArgs> */
	const static int SendCompletedEvent;

protected:
	virtual ~LocalMessageSender ();

private:
	static void MessageSentHandler (const char *message, const char *response, gpointer managedUserState, gpointer data);
	void MessageSent (const char *message, const char *msg, gpointer managedUserState);

	MoonMessageSender *sender;

	char *receiverName;
	char *receiverDomain;
};

#endif /* __MOON_MESSAGING_H__ */
