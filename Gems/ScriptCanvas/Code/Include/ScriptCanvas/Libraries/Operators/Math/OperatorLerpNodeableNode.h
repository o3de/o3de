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

#include <ScriptCanvas/Core/NodeableNodeOverloaded.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        class NodeableNodeOverloadedLerp
            : public NodeableNodeOverloaded
        {
        public:
            AZ_COMPONENT(NodeableNodeOverloadedLerp, "{A4CFB2F2-4045-47ED-AE73-ED60C2072EE4}", NodeableNodeOverloaded);

            static void Reflect(AZ::ReflectContext* reflectContext);

            AZStd::vector<AZStd::unique_ptr<Nodeable>> GetInitializationNodeables() const override;

        protected:
            void ConfigureSlots() override;

            AZ::Crc32 GetDataDynamicGroup() const { return AZ_CRC("LerpDataDynamicGroup", 0x62b5f89e); }
        };

    } 

} 

