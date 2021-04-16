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

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Components/TransformComponent.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>
/*
#include <Integration/ActorComponentBus.h>
#include <Integration/Components/ActorComponent.h>

#include <EMotionFX/Source/ActorInstance.h>
*/
// Hair specific
//#include <Code/src/TressFX/TressFXAsset.h>
//#include <TressFX/TressFXSettings.h>

#include <Components/HairComponent.h>
//#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

#pragma optimize("", off)

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            HairComponent::HairComponent(const HairComponentConfig& config)
                : BaseClass(config)
            {
            }

            HairComponent::~HairComponent()
            {
            }

            void HairComponent::Reflect(AZ::ReflectContext* context)
            {
                BaseClass::Reflect(context);

                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<HairComponent, BaseClass>();
                }

                if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
                {
                    behaviorContext->Class<HairComponent>()->RequestBus("HairRequestsBus");

                    behaviorContext->ConstantProperty("HairComponentTypeId", BehaviorConstant(Uuid(Hair::HairComponentTypeId)))
                        ->Attribute(AZ::Script::Attributes::Module, "render")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
                }
            }

            void HairComponent::Activate()
            {
                m_controller.SetEntity(GetEntity());
                BaseClass::Activate();
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ

#pragma optimize("", on)

