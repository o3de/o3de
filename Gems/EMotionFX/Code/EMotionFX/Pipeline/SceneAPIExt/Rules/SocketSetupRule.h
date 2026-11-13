/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <EMotionFX/Source/SocketSetup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPIExt/Rules/ExternalToolRule.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class SocketSetupRule
                : public ExternalToolRule<AZStd::shared_ptr<EMotionFX::SocketSetup>>
            {
            public:
                AZ_RTTI(SocketSetupRule, "{91F7D773-9B32-4321-9346-947A73CD1D41}", AZ::SceneAPI::DataTypes::IRule);
                AZ_CLASS_ALLOCATOR(SocketSetupRule, AZ::SystemAllocator)

                SocketSetupRule();
                SocketSetupRule(const AZStd::shared_ptr<EMotionFX::SocketSetup>& data);

                const AZStd::shared_ptr<EMotionFX::SocketSetup>& GetData() const override      { return m_data; }
                void SetData(const AZStd::shared_ptr<EMotionFX::SocketSetup>& data) override   { m_data = data; }

                static void Reflect(AZ::ReflectContext* context);

            private:
                AZStd::shared_ptr<EMotionFX::SocketSetup> m_data;
            };
        } // Rule
    } // Pipeline
} // EMotionFX
