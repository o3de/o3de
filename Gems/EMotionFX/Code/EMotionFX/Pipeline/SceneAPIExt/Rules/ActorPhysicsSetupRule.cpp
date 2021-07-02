/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPIExt/Rules/ActorPhysicsSetupRule.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            ActorPhysicsSetupRule::ActorPhysicsSetupRule()
                : ExternalToolRule<AZStd::shared_ptr<EMotionFX::PhysicsSetup>>()
            {
            }


            ActorPhysicsSetupRule::ActorPhysicsSetupRule(const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& data)
                : ActorPhysicsSetupRule()
            {
                m_data = data;
            }


            void ActorPhysicsSetupRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ActorPhysicsSetupRule>()
                        ->Version(1)
                        ->Field("data", &ActorPhysicsSetupRule::m_data)
                        ;
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX
