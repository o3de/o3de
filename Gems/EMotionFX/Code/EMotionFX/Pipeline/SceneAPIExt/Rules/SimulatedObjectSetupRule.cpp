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
#include <SceneAPIExt/Rules/SimulatedObjectSetupRule.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            SimulatedObjectSetupRule::SimulatedObjectSetupRule()
                : ExternalToolRule<AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>>()
            {
            }


            SimulatedObjectSetupRule::SimulatedObjectSetupRule(const AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>& data)
                : SimulatedObjectSetupRule()
            {
                m_data = data;
            }


            void SimulatedObjectSetupRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SimulatedObjectSetupRule>()
                        ->Version(1)
                        ->Field("data", &SimulatedObjectSetupRule::m_data)
                        ;
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX
