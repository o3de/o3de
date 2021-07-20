/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodelingBus.h>

#include "Nodeling.h"

#include <Include/ScriptCanvas/Libraries/Core/FunctionDefinitionNode.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class FunctionDefinitionNode
                : public Internal::Nodeling
            {
            public:
                enum NodeVersion
                {
                    Initial = 1,
                    RemoveDefaultDisplayGroup,
                };

                SCRIPTCANVAS_NODE(FunctionDefinitionNode);

                FunctionDefinitionNode() = default;

                ~FunctionDefinitionNode() = default;

                bool IsExecutionEntry() const;

                bool IsExecutionExit() const;
                
                // Node...
                
                void OnConfigured() override;

                bool OnValidateNode(ValidationResults& validationResults) override;
                ////

                const Slot* GetEntrySlot() const;

                const Slot* GetExitSlot() const;

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                void MarkExecutionExit();

            protected:

                void OnDisplayNameChanged() override;

                bool CanDeleteSlot(const SlotId& slotId) const override;

                void SetupSlots();

                bool IsValidDisplayName() const;

                AZStd::string GenerateErrorMessage() const;

                SlotId CreateDataSlot(AZStd::string_view name, AZStd::string_view toolTip, ConnectionType connectionType);

                static constexpr AZ::Crc32 GetAddNodelingInputDataSlot() { return AZ_CRC_CE("AddNodelingInputDataSlot"); }
                static constexpr AZ::Crc32 GetAddNodelingOutputDataSlot() { return AZ_CRC_CE("AddNodelingOutputDataSlot"); }
                
                AZStd::string GetDataDisplayGroup() const { return "DataDisplayGroup"; }
                
                SlotId HandleExtension(AZ::Crc32 extensionId) override;

                void ConfigureVisualExtensions() override;

                void OnInit() override;

                void OnSetup() override;

            private:
                bool m_isExecutionEntry = true;
                AZStd::vector<const ScriptCanvas::Slot*> m_entrySlots;
                AZStd::vector<const ScriptCanvas::Slot*> m_dataSlots;

                bool m_configureVisualExtensions = false;

            };
        }
    }
}
