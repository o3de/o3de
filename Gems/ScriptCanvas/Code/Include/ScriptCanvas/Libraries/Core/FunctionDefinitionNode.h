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
                    Initial  = 1
                };

            public:

                SCRIPTCANVAS_NODE(FunctionDefinitionNode);

                FunctionDefinitionNode() = default;
                ~FunctionDefinitionNode() = default;

                ConnectionType GetConnectionType() const { return m_externalConnectionType; }

                // Node...
                bool IsEntryPoint() const override;

                void OnConfigured() override;
                void OnActivate() override;

                void OnInputSignal(const SlotId&) override;

                bool OnValidateNode(ValidationResults& validationResults) override;
                ////

                AZStd::vector<const ScriptCanvas::Slot*> GetEntrySlots() const;

                void OnEndpointConnected(const ScriptCanvas::Endpoint& endpoint) override;

                void OnEndpointDisconnected(const ScriptCanvas::Endpoint& endpoint) override;

                void SignalEntrySlots();

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

            protected:

                void OnDisplayNameChanged() override;

                void ConfigureExternalConnectionType();
                
                ConnectionType m_externalConnectionType = ConnectionType::Unknown;

            private:

                void SetupSlots();

                bool IsValidDisplayName() const;
                AZStd::string GenerateErrorMessage() const;
                
                AZStd::vector<const ScriptCanvas::Slot*> m_entrySlots;
            };
        }
    }
}
