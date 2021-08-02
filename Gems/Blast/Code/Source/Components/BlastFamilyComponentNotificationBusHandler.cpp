/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/BlastFamilyComponentNotificationBusHandler.h>

namespace Blast
{
    BlastFamilyComponentNotificationBusHandler::BlastFamilyComponentNotificationBusHandler()
    {
        m_events.resize(static_cast<int>(Function::Count));
        SetEvent(&BlastFamilyComponentNotificationBusHandler::OnActorCreatedDummy, "On Actor Created");
        SetEvent(&BlastFamilyComponentNotificationBusHandler::OnActorDestroyedDummy, "On Actor Destroyed");
    }

    void BlastFamilyComponentNotificationBusHandler::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<BlastFamilyComponentNotificationBus>("BlastFamilyComponentNotificationBus")
                ->Attribute(AZ::Script::Attributes::Module, "destruction")
                ->Attribute(AZ::Script::Attributes::Category, "Blast")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Handler<BlastFamilyComponentNotificationBusHandler>();
        }
    }

    void BlastFamilyComponentNotificationBusHandler::Disconnect()
    {
        BusDisconnect();
    }

    bool BlastFamilyComponentNotificationBusHandler::Connect(AZ::BehaviorValueParameter* id)
    {
        return AZ::Internal::EBusConnector<BlastFamilyComponentNotificationBusHandler>::Connect(this, id);
    }

    bool BlastFamilyComponentNotificationBusHandler::IsConnected()
    {
        return AZ::Internal::EBusConnector<BlastFamilyComponentNotificationBusHandler>::IsConnected(this);
    }

    bool BlastFamilyComponentNotificationBusHandler::IsConnectedId(AZ::BehaviorValueParameter* id)
    {
        return AZ::Internal::EBusConnector<BlastFamilyComponentNotificationBusHandler>::IsConnectedId(this, id);
    }

    int BlastFamilyComponentNotificationBusHandler::GetFunctionIndex(const char* functionName) const
    {
        if (strcmp(functionName, "On Actor Created") == 0) {
            return static_cast<int>(Function::OnActorCreated);
        }
        if (strcmp(functionName, "On Actor Destroyed") == 0) {
            return static_cast<int>(Function::OnActorDestroyed);
        }
        return -1;
    }

    void BlastFamilyComponentNotificationBusHandler::OnActorCreatedDummy([[maybe_unused]] BlastActorData blastActor)
    {
        // This is never invoked, and only used for type deduction when calling SetEvent
    }

    void BlastFamilyComponentNotificationBusHandler::OnActorDestroyedDummy([[maybe_unused]] BlastActorData blastActor)
    {
        // This is never invoked, and only used for type deduction when calling SetEvent
    }

    void BlastFamilyComponentNotificationBusHandler::OnActorCreated(const BlastActor& blastActor)
    {
        Call(static_cast<int>(Function::OnActorCreated), BlastActorData(blastActor));
    }

    void BlastFamilyComponentNotificationBusHandler::OnActorDestroyed(const BlastActor& blastActor)
    {
        Call(static_cast<int>(Function::OnActorDestroyed), BlastActorData(blastActor));
    }
} // namespace Blast
