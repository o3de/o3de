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