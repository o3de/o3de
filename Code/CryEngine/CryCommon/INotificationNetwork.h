/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_INOTIFICATIONNETWORK_H
#define CRYINCLUDE_CRYCOMMON_INOTIFICATIONNETWORK_H
#pragma once


// Constants

#define NN_CHANNEL_NAME_LENGTH_MAX  16

struct INotificationNetworkClient;

// User Interfaces

struct INotificationNetworkListener
{
    // <interfuscator:shuffle>
    virtual ~INotificationNetworkListener(){}
    // Called upon receiving data from the Channel the Listener is binded to.
    virtual void OnNotificationNetworkReceive(const void* pBuffer, size_t length) = 0;
    // </interfuscator:shuffle>
};

struct INotificationNetworkConnectionCallback
{
    // <interfuscator:shuffle>
    virtual ~INotificationNetworkConnectionCallback(){}
    virtual void OnConnect(INotificationNetworkClient* pClient, bool bSucceeded) = 0;
    virtual void OnDisconnected(INotificationNetworkClient* pClient) = 0;
    // </interfuscator:shuffle>
};

// Interfaces

struct INotificationNetworkClient
{
    // <interfuscator:shuffle>
    virtual ~INotificationNetworkClient(){}
    virtual void Release() = 0;

    // Binds a Listener to the given Notification Channel.
    // Each Listener can be binded only to one Channel, calling the method
    // again with an already added Listener and a different Channel will rebind it.
    // The Channel name cannot exceed NN_CHANNEL_NAME_LENGTH_MAX chars.
    virtual bool ListenerBind(const char* channelName, INotificationNetworkListener* pListener) = 0;

    // If it exist, removes the given Listener form the Notification Network.
    virtual bool ListenerRemove(INotificationNetworkListener* pListener) = 0;

    // Sends arbitrary data to the Notification Network the Client is connected to.
    virtual bool Send(const char* channelName, const void* pBuffer, size_t length) = 0;

    // Checks if the current client is connected.
    // Returns true if it is connected, false otherwise.
    virtual bool IsConnected() = 0;

    // Checks if the connection attempt failed.
    // Returns true if it failed to connect by any reason (such as timeout).
    virtual bool IsFailedToConnect() const = 0;

    //  Start the connection request for this particular client.
    // Parameters:
    //  address -   Is the host name or ipv4 (for now) address string to which
    //                      we want to connect.
    //  port        -   Is the TCP port to which we want to connect.
    //  Remarks:    Port 9432 is being used by the live preview already.
    virtual bool Connect(const char* address, uint16 port) = 0;

    // Tries to register a callback listener object.
    // A callback listener object will receive events from the client element,
    // such as connection result information.
    // Parameters:
    //  - pConnectionCallback - Is a pointer to an object implementing interface
    //  INotificationNetworkConnectionCallback which will be called when
    //  the events happen, such as connection, disconnection and failed attempt
    //  to connect.
    // Return Value:
    //  - It will return true if registered the callback object successfully.
    //  - It will return false when there the callback object is already
    // registered.
    virtual bool RegisterCallbackListener(INotificationNetworkConnectionCallback* pConnectionCallback) = 0;

    // Tries to unregister a callback listener object.
    // A callback listener object will receive events from the client element,
    // such as connection result information.
    // Parameters:
    //  - pConnectionCallback - Is a pointer to an object implementing interface
    //  INotificationNetworkConnectionCallback which will be called when
    //  the events happen, such as connection, disconnection and failed attempt
    //  to connect and that we want to unregister.
    // Return Value:
    //  - It will return true if unregistered the callback object successfully.
    //  - It will return false when no object matching the one requested is found
    //  int the object.
    virtual bool UnregisterCallbackListener(INotificationNetworkConnectionCallback* pConnectionCallback) = 0;
    // </interfuscator:shuffle>
};

struct INotificationNetwork
{
    // <interfuscator:shuffle>
    virtual ~INotificationNetwork(){}

    virtual void Release() = 0;

    // Creates a disconnected client.
    virtual INotificationNetworkClient* CreateClient() = 0;

    // Attempts to connect to the Notification Network at the given address,
    // returns a Client interface if communication is possible.
    virtual INotificationNetworkClient* Connect(const char* address, uint16 port) = 0;

    // Returns the Connection count of the given Channel. If NULL is passed
    // instead of a valid Channel name the total count of all Connections is
    // returned.
    virtual size_t GetConnectionCount(const char* channelName = NULL) = 0;

    // Has to be called from the main thread to process received notifications.
    virtual void Update() = 0;

    // Binds a Listener to the given Notification Channel.
    // Each Listener can be binded only to one Channel, calling the method
    // again with an already added Listener and a different Channel will rebind it.
    // The Channel name cannot exceed NN_CHANNEL_NAME_LENGTH_MAX chars.
    virtual bool ListenerBind(const char* channelName, INotificationNetworkListener* pListener) = 0;

    // If it exist, removes the given Listener form the Notification Network.
    virtual bool ListenerRemove(INotificationNetworkListener* pListener) = 0;

    // Sends arbitrary data to all the Connections listening to the given Channel.
    virtual uint32 Send(const char* channel, const void* pBuffer, size_t length) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_INOTIFICATIONNETWORK_H
