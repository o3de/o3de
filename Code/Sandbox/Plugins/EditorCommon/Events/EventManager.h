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

#ifndef CRYINCLUDE_EDITORCOMMON_EVENTS_EVENTMANAGER_H
#define CRYINCLUDE_EDITORCOMMON_EVENTS_EVENTMANAGER_H
#pragma once


#include "platform.h"

#include "EditorCommonAPI.h"
#include "IEditor.h"

#include "Serialization/IArchive.h"

struct SSystemGlobalEnvironment;

class CEventConnection
{
    friend class CEventManager;
    friend class CScopedEventConnection;

public:
    CEventConnection()
        : m_bConnected(false) {}

    EDITOR_COMMON_API void Disconnect();

    uint GetHandlerId() const { return m_handlerId; }

private:
    CEventConnection(const uint address, const string& eventName, const uint handlerId)
        : m_address(address)
        , m_eventName(eventName)
        , m_handlerId(handlerId)
        , m_bConnected(true) {}

    void Reset()
    {
        m_bConnected = false;
        m_address = 0;
        m_handlerId = 0;
        m_eventName.clear();
    }

    void Move(CEventConnection& other)
    {
        m_bConnected = other.m_bConnected;
        m_address = other.m_address;
        m_handlerId = other.m_handlerId;
        m_eventName.swap(other.m_eventName);
        other.Reset();
    }

    bool m_bConnected;
    uint m_address;
    uint m_handlerId;
    string m_eventName;
};

class CScopedEventConnection
    : public CEventConnection
{
public:
    CScopedEventConnection()
        : CEventConnection() {}

    CScopedEventConnection(CScopedEventConnection&& connection)
    {
        Move(connection);
    }

    CScopedEventConnection(CEventConnection&& connection)
    {
        Move(connection);
    }

    CScopedEventConnection& operator =(CEventConnection&& connection)
    {
        if (&connection != this)
        {
            Disconnect();
            Move(connection);
        }
        return *this;
    }

    ~CScopedEventConnection() { Disconnect(); }

private:
    CScopedEventConnection(const CScopedEventConnection&); // no implementation
    CScopedEventConnection& operator =(const CScopedEventConnection&); // no implementation
};

class EDITOR_COMMON_API CEventManager
{
    friend class CEventConnection;

public:
    CEventManager();

    static CEventManager* GetInstance();

    void Init(SSystemGlobalEnvironment* pEnv);

    // Registers an address and returns its ID. Multiple event handlers can listen to the same address, allowing broadcasts.
    uint GetAddressId(const char* name);

    // Registers a new unique address
    uint GetUniqueAddressId();

public:

    // Sends an event to an address
    //
    // TMessageType must be a serializable struct
    //
    template <class TMessageType>
    void SendEvent(const uint address, const TMessageType& message)
    {
        const string json = SerializeMessageToJSON(Serialization::SStruct(message));
        SendEventRaw(address, TMessageType::GetName(), json);
    }

    template <class TMessageType>
    void SendEvent(const uint address, const TMessageType& message, const DynArray<uint>& excludedHandlers) const
    {
        const string json = SerializeMessageToJSON(Serialization::SStruct(message));
        SendEventRaw(address, TMessageType::GetName(), json, excludedHandlers);
    }

    // For sending a raw JSON message.
    void SendEventRaw(const uint address, const char* eventName, const char* message) const;
    void SendEventRaw(const uint address, const char* eventName, const char* message, const DynArray<uint>& excludedHandlers) const;

    // Tests if a call to SendEvent would actually send a message (there is someone listening to this message)
    bool CanDeliverRaw(const uint address, const char* eventName) const;
    template<class TMessageType>
    bool CanDeliver(const uint address) const
    {
        const char* pEventName = TMessageType::GetName();
        return CanDeliverRaw(address, pEventName);
    }

    // This should be the most common way to add an event callback
    //
    // This example will install an event handler for OnEvent(const SMessageType &message) that is sent to a specific address:
    // CEventManager::GetInstance()->AddEventCallback(componentId, this, &CEventHandler::OnEvent);
    //
    // TMessageType must be a serializable struct
    //
    // Returns a CEventConnection. The callback is removed when this object is destroyed.
    //
    template <class TClassType, class TMessageType>
    CEventConnection AddEventCallback(const uint address, TClassType* pThis, void (TClassType::* pMethod)(const TMessageType&))
    {
        return AddEventCallback<TMessageType>(address, std::bind(pMethod, pThis, std::placeholders::_1));
    }

    // Same as above, but you can pass in any function object that takes TMessageType& as an argument directly.
    //
    template <class TMessageType>
    CEventConnection AddEventCallback(const uint address, std::function<void (const TMessageType&)> callback)
    {
        return AddEventCallbackRaw(address, TMessageType::GetName(), [=](const string& json)
            {
                TMessageType message;
                DeserializeFromJSON(Serialization::SStruct(message), json);
                callback(message);
            });
    }

    // This can be used if raw parsing of JSON is preferred.
    typedef std::function<void (const char*)> TEventHandlerFunc;
    CEventConnection AddEventCallbackRaw(const uint componentId, const char* eventName, TEventHandlerFunc callback);

private:
    void SendEventImplementation(const uint address, const string& eventName, const string& message, const DynArray<uint>& excludedHandlers) const;

    string SerializeMessageToJSON(const Serialization::SStruct& ref) const;
    void DeserializeFromJSON(const Serialization::SStruct& ref, const string& json);

    uint m_nextAddress;
    uint m_nextHandlerId;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    std::map<string, uint, stl::less_stricmp<string> > m_nameToAddressMap;
    std::map<std::pair<uint, string>, std::vector<std::pair<uint, TEventHandlerFunc> > > m_messageRoutingMap;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    static CEventManager* ms_pEventManager;
};

#endif // CRYINCLUDE_EDITORCOMMON_EVENTS_EVENTMANAGER_H
