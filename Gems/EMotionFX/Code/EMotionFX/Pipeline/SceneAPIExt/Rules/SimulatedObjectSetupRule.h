/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPIExt/Rules/ExternalToolRule.h>


namespace AZ
{
    class ReflectContext;
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IGroup;
        }
    }
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class SimulatedObjectSetupRule
                : public ExternalToolRule<AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>>
            {
            public:
                AZ_RTTI(SimulatedObjectSetupRule, "{7A69FC94-874B-4EF7-8F15-89781953CD3C}", AZ::SceneAPI::DataTypes::IRule);
                AZ_CLASS_ALLOCATOR(SimulatedObjectSetupRule, AZ::SystemAllocator)

                SimulatedObjectSetupRule();
                SimulatedObjectSetupRule(const AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>& data);

                const AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>& GetData() const override      { return m_data; }
                void SetData(const AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>& data) override   { m_data = data; }

                static void Reflect(AZ::ReflectContext* context);

            private:
                AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup> m_data;
            };
        } // Rule
    } // Pipeline
} // EMotionFX
