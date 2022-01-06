/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationIds.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/FocusOnEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h>

namespace ScriptCanvas
{
    // Latent Data Connection Event
    // An event that is generated when data is passed outside
    // of a particular execution scope, and will not be available
    // for use when the node executes
    class ScopedDataConnectionEvent
        : public ValidationEvent
        , public HighlightEntityEffect
        , public FocusOnEntityEffect
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopedDataConnectionEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(ScopedDataConnectionEvent, "{4C77B468-1405-4997-9A0E-A399E7464906}", HighlightEntityEffect, ValidationEvent, FocusOnEntityEffect);
        
        ScopedDataConnectionEvent(const AZ::EntityId& connectionId)
            : ValidationEvent(ValidationSeverity::Warning)
            , m_connectionId(connectionId)
        {
            SetDescription("Data Connection crosses across execution boundaries, and will not provide data.");
        }

        ScopedDataConnectionEvent
            ( const AZ::EntityId& connectionId
            , const Node& targetNode
            , const Slot& targetSlot
            , const Node& sourceNode
            , const Slot& sourceSlot)
            : ValidationEvent(ValidationSeverity::Warning)
            , m_connectionId(connectionId)
        {
            SetDescription(AZStd::string::format
                ( "There is an invalid data connection %s.%s --> %s.%s, the data is not"
                " in the execution path between nodes. Either route execution %s --> %s,"
                " or store the data in a variable if it is needed."
                , sourceNode.GetNodeName().data()
                , sourceSlot.GetName().data()
                , targetNode.GetNodeName().data()
                , targetSlot.GetName().data()
                , sourceNode.GetNodeName().data()
                , targetNode.GetNodeName().data()));
        }

        bool CanAutoFix() const override
        {
            return false;
        }
        
        AZStd::string GetIdentifier() const override
        {
            return DataValidationIds::ScopedDataConnectionId;
        }
        
        AZ::Crc32 GetIdCrc() const override
        {
            return DataValidationIds::ScopedDataConnectionCrc;
        }
        
        const AZ::EntityId& GetConnectionId() const
        {
            return m_connectionId;
        }

        AZStd::string_view GetTooltip() const override
        {
            return "Out of Scope Data Connection";
        }

        // HighlightEntityEffect
        AZ::EntityId GetHighlightTarget() const override
        {
            return m_connectionId;
        }
        ////

        // FocusOnEntityEffect
        AZ::EntityId GetFocusTarget() const override
        {
            return m_connectionId;
        }
        ////
        
    private:
    
        AZ::EntityId m_connectionId;
    };
}
