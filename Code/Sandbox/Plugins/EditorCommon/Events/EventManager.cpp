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

#include "EditorCommon_precompiled.h"
#include "EventManager.h"
#include "Serialization/JSONIArchive.h"
#include "Serialization/JSONOArchive.h"

CEventManager* CEventManager::ms_pEventManager;

CEventManager::CEventManager()
    : m_nextAddress(0)
    , m_nextHandlerId(0)
{
    if (ms_pEventManager)
    {
        CryFatalError("There should be only one event manager instance");
    }

    ms_pEventManager = this;
}

void CEventManager::Init([[maybe_unused]] SSystemGlobalEnvironment* env)
{
}

CEventManager* CEventManager::GetInstance()
{
    return ms_pEventManager;
}

uint CEventManager::GetAddressId(const char* name)
{
    auto findIter = m_nameToAddressMap.find(name);
    if (findIter != m_nameToAddressMap.end())
    {
        return findIter->second;
    }

    uint newId = m_nextAddress++;
    m_nameToAddressMap[name] = newId;
    return newId;
}

uint CEventManager::GetUniqueAddressId()
{
    return m_nextAddress++;
}

void CEventManager::SendEventRaw(const uint address, const char* eventName, const char* message) const
{
    SendEventImplementation(address, eventName, message, DynArray<uint>());
}

void CEventManager::SendEventRaw(const uint address, const char* eventName, const char* message, const DynArray<uint>& excludedHandlers) const
{
    SendEventImplementation(address, eventName, message, excludedHandlers);
}

void CEventManager::SendEventImplementation(const uint address, const string& eventName, const string& message, const DynArray<uint>& excludedHandlers) const
{
    auto handlerFindIter = m_messageRoutingMap.find(std::make_pair(address, eventName));
    if (handlerFindIter != m_messageRoutingMap.end())
    {
        const std::vector<std::pair<uint, TEventHandlerFunc> >& handlers = handlerFindIter->second;

        for (uint i = 0; i < handlers.size(); ++i)
        {
            if (!stl::find(excludedHandlers, handlers[i].first))
            {
                handlers[i].second(message);
            }
        }
    }
}

bool CEventManager::CanDeliverRaw(const uint address, const char* eventName) const
{
    auto handlerFindIter = m_messageRoutingMap.find(std::make_pair(address, eventName));
    if (handlerFindIter != m_messageRoutingMap.end())
    {
        const std::vector<std::pair<uint, TEventHandlerFunc> >& handlers = handlerFindIter->second;
        return !handlers.empty();
    }

    return false;
}

CEventConnection CEventManager::AddEventCallbackRaw(const uint address, const char* eventName, TEventHandlerFunc callback)
{
    const uint handlerId = m_nextHandlerId++;
    m_messageRoutingMap[std::make_pair(address, eventName)].push_back(std::make_pair(handlerId, callback));
    return CEventConnection(address, eventName, handlerId);
}

string CEventManager::SerializeMessageToJSON(const Serialization::SStruct& ref) const
{
    Serialization::JSONOArchive oArchive;
    oArchive(ref);
    return oArchive.buffer();
}

void CEventManager::DeserializeFromJSON(const Serialization::SStruct& ref, const string& json)
{
    Serialization::JSONIArchive iArchive;
    if (iArchive.open(json.data(), json.size()))
    {
        iArchive(ref);
    }
}

void CEventConnection::Disconnect()
{
    if (m_bConnected)
    {
        auto& messageMap = CEventManager::GetInstance()->m_messageRoutingMap;
        auto findIter = messageMap.find(std::make_pair(m_address, m_eventName));

        if (findIter != messageMap.end())
        {
            stl::find_and_erase_if(findIter->second, [=](const std::pair<uint, CEventManager::TEventHandlerFunc>& h)
                {
                    return h.first == m_handlerId;
                });
        }

        if (findIter->second.empty())
        {
            messageMap.erase(findIter);
        }

        Reset();
    }
}
