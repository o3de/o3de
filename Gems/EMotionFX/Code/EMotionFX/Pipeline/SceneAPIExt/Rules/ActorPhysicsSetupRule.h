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
#include <EMotionFX/Source/PhysicsSetup.h>
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
            class ActorPhysicsSetupRule
                : public ExternalToolRule<AZStd::shared_ptr<EMotionFX::PhysicsSetup>>
            {
            public:
                AZ_RTTI(ActorPhysicsSetupRule, "{B18E9412-85DC-442D-9AA3-293B583EC1A6}", AZ::SceneAPI::DataTypes::IRule);
                AZ_CLASS_ALLOCATOR(ActorPhysicsSetupRule, AZ::SystemAllocator, 0)

                ActorPhysicsSetupRule();
                ActorPhysicsSetupRule(const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& data);

                const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& GetData() const override      { return m_data; }
                void SetData(const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& data) override   { m_data = data; }

                static void Reflect(AZ::ReflectContext* context);

            private:
                AZStd::shared_ptr<EMotionFX::PhysicsSetup> m_data;
            };
        } // Rule
    } // Pipeline
} // EMotionFX
