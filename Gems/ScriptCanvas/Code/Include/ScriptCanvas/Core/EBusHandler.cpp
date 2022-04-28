/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EBusHandler.h"
#include <AzCore/Script/lua/lua.h>

namespace ScriptCanvas
{
    EBusHandler::EBusHandler(ExecutionStateWeakPtr executionState, AZStd::string_view busName, AZ::BehaviorContext* behaviorContext)
        : Nodeable(executionState)
    {
        InitializeEBusHandling(busName, behaviorContext);
    }

    EBusHandler::EBusHandler(AZStd::string_view busName, AZ::BehaviorContext* behaviorContext)
    {
        InitializeEBusHandling(busName, behaviorContext);
    }

    EBusHandler::~EBusHandler()
    {
        m_handler->Disconnect();
        m_ebus->m_destroyHandler->Invoke(m_handler);
    }

    EBusHandler* EBusHandler::Create(ExecutionStateWeakPtr executionState, AZStd::string_view busName)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Can't create the ebus handler without a behavior context!");
        return aznew EBusHandler(executionState, busName, behaviorContext);
    }

    void EBusHandler::CreateHandler(AZStd::string_view ebusName, AZ::BehaviorContext* behaviorContext)
    {
        const auto& ebusIterator = behaviorContext->m_ebuses.find(ebusName);
        AZ_Assert(ebusIterator != behaviorContext->m_ebuses.end(), "No ebus by name of %s in the behavior context!", ebusName.data());
        
        m_ebus = ebusIterator->second;
        AZ_Assert(m_ebus, "ebus == nullptr in %s", ebusName.data());
        AZ_Assert(m_ebus->m_createHandler, "The ebus %s has no create handler!", ebusName.data());
        AZ_Assert(m_ebus->m_destroyHandler, "The ebus %s has no destroy handler!", ebusName.data());

        AZ_Verify(m_ebus->m_createHandler->InvokeResult(m_handler), "Ebus handler creation failed %s", ebusName.data());
        AZ_Assert(m_handler, "Ebus create handler failed %s", ebusName.data());
    }

    bool EBusHandler::Connect()
    {
        AZ::BehaviorValueParameter noBusId;
        noBusId.m_typeId = AZ::AzTypeInfo<void>::Uuid();
        return ConnectTo(noBusId);
    }

    bool EBusHandler::ConnectTo(AZ::BehaviorValueParameter& busId)
    {
        m_handler->Disconnect();
        const bool isConnected = m_handler->Connect(&busId);
        // \todo flush out this warning with the address
        AZ_Warning("ScriptCanvas", isConnected, "Unable to connect to EBus (%s) ", m_ebus->m_name.data());
        return isConnected;
    }

    void EBusHandler::Disconnect()
    {
        m_handler->Disconnect();
    }

    const AZStd::string& EBusHandler::GetEBusName() const
    {
        return m_ebus->m_name;
    }

    int EBusHandler::GetEventIndex(AZStd::string_view eventName) const
    {
        return m_handler->GetFunctionIndex(eventName.data());
    }

    void EBusHandler::InitializeEBusHandling(AZStd::string_view busName, AZ::BehaviorContext* behaviorContext)
    {
        CreateHandler(busName, behaviorContext);
        InitializeExecutionOuts(m_handler->GetEvents().size());
    }

    bool EBusHandler::IsConnected() const
    {
        return m_handler->IsConnected();
    }

    bool EBusHandler::IsConnectedTo(AZ::BehaviorValueParameter& busId) const
    {
        return m_handler->IsConnectedId(&busId);
    }

    bool EBusHandler::IsActive() const
    {
        return IsConnected();
    }

    void EBusHandler::HandleEvent(int eventIndex)
    {
        m_handler->InstallGenericHook(eventIndex, &OnEventGenericHook, this);
    }

    void EBusHandler::OnDeactivate()
    {
        Disconnect();
    }

    void EBusHandler::OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters)
    {
        AZ_UNUSED(eventName);
        AZ_PROFILE_SCOPE(ScriptCanvas, "EBusEventHandler::OnEvent %s", eventName);
        auto handler = reinterpret_cast<EBusHandler*>(userData);
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(handler->GetExecutionState());
        handler->OnEvent(nullptr, eventIndex, result, numParameters, parameters);
    }

    void EBusHandler::OnEvent(const char* /*eventName*/, const int eventIndex, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters)
    {
        CallOut(eventIndex, result, parameters, numParameters);
    }

    void EBusHandler::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<EBusHandler>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ;

            // \note Do not further expose this to BehaviorContext. This is directly registered to Lua here:
            // void RegisterEBusHandlerAPI(lua_State* lua);
            //
            // \see NodeableOutIntepreted.cpp 
        }
    }
}
