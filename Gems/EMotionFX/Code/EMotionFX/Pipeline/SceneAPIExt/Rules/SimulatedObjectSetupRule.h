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
                AZ_CLASS_ALLOCATOR(SimulatedObjectSetupRule, AZ::SystemAllocator, 0)

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