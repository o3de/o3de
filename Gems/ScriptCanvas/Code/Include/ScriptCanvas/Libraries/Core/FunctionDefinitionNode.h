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
            private:
                enum NodeVersion
                {
                    Initial  = 1,
                    RemoveDefaultDisplayGroup,
                };

            public:

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
