/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Reroute.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Core/Attributes.h>

namespace ScriptCanvas::Nodes::Core
{
    namespace
    {
        constexpr AZ::Crc32 RerouteDataGroup = AZ_CRC_CE("RerouteData");
    }

    Reroute::Reroute()
    {
        SetNodeStyle(".reroute");
        SetNodeName("Reroute");
        SetNodeToolTip("Routes a data or execution connection without affecting translation.");
    }

    void Reroute::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Enum<Mode>()
                ->Value("Data", Mode::Data)
                ->Value("Execution", Mode::Execution);

            serializeContext->Class<Reroute, Node>()
                ->Version(0)
                ->Field("Mode", &Reroute::m_mode);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Reroute>("Reroute", "Routes a connection without affecting execution or generated code")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Utilities")
                    ->Attribute(AZ::Edit::Attributes::CategoryStyle, ".utility")
                    ->Attribute(Attributes::Node::TitlePaletteOverride, "LogicNodeTitlePalette");
            }
        }
    }

    void Reroute::ConfigureMode(Mode mode)
    {
        if (m_mode == mode && GetSlots().size() == 2)
        {
            return;
        }

        AZStd::vector<SlotId> slotIds;
        slotIds.reserve(GetSlots().size());
        for (const Slot& slot : GetSlots())
        {
            slotIds.push_back(slot.GetId());
        }

        for (const SlotId& slotId : slotIds)
        {
            // Keep an already displayed GraphCanvas node synchronized when the connection menu
            // changes a palette-created reroute from its default data mode to execution mode.
            RemoveSlot(slotId, true);
        }

        m_mode = mode;
        ConfigureSlotsForMode();
    }

    Reroute::Mode Reroute::GetMode() const
    {
        return m_mode;
    }

    AZ::Outcome<DependencyReport, void> Reroute::GetDependencies() const
    {
        return AZ::Success(DependencyReport{});
    }

    bool Reroute::IsConnectionTransparentForTranslation() const
    {
        return true;
    }

    bool Reroute::IsNoOp() const
    {
        return true;
    }

    ConstSlotsOutcome Reroute::GetSlotsInExecutionThreadByTypeImpl(
        const Slot&, CombinedSlotType targetSlotType, const Slot*) const
    {
        return AZ::Success(GetSlotsByType(targetSlotType));
    }

    void Reroute::OnInit()
    {
        ConfigureSlotsForMode();
    }

    void Reroute::ConfigureSlotsForMode()
    {
        if (m_mode == Mode::Execution)
        {
            ExecutionSlotConfiguration input("In", ConnectionType::Input);
            input.m_isNameHidden = true;
            AddSlot(input);

            ExecutionSlotConfiguration output("Out", ConnectionType::Output);
            output.m_isNameHidden = true;
            AddSlot(output);
            return;
        }

        DynamicDataSlotConfiguration input;
        input.m_name = "In";
        input.m_dynamicDataType = DynamicDataType::Any;
        input.m_dynamicGroup = RerouteDataGroup;
        input.m_canHaveInputField = false;
        input.m_isNameHidden = true;
        input.SetConnectionType(ConnectionType::Input);
        AddSlot(input);

        DynamicDataSlotConfiguration output;
        output.m_name = "Out";
        output.m_dynamicDataType = DynamicDataType::Any;
        output.m_dynamicGroup = RerouteDataGroup;
        output.m_isNameHidden = true;
        output.SetConnectionType(ConnectionType::Output);
        AddSlot(output);
    }
} // namespace ScriptCanvas::Nodes::Core
