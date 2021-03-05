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

#include <Scripting/EntityReferenceComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        EntityReferenceComponent::EntityReferenceComponent(const EntityReferenceComponentConfig& config)
            : BaseClass(config)
        {

        }

        void EntityReferenceComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EntityReferenceComponent, BaseClass>()
                    ->Version(0)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->EBus<EntityReferenceRequestBus>("EntityReferenceRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "entity")
                    ->Event("GetEntityReferences", &EntityReferenceRequests::GetEntityReferences)
                    ;

                behaviorContext->ConstantProperty("EntityReferenceComponentTypeId", BehaviorConstant(Uuid(EntityReferenceComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ;
            }
        }
    }
}
