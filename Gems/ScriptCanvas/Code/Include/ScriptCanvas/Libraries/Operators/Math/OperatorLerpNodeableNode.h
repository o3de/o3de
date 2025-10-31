/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            AZ::Crc32 GetDataDynamicGroup() const { return AZ_CRC_CE("LerpDataDynamicGroup"); }
        };

    }

}

