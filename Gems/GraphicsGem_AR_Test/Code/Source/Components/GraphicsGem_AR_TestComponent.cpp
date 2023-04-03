/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/GraphicsGem_AR_TestComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace GraphicsGem_AR_Test
{
    GraphicsGem_AR_TestComponent::GraphicsGem_AR_TestComponent(const GraphicsGem_AR_TestComponentConfig& config)
        : BaseClass(config)
    {
    }

    void GraphicsGem_AR_TestComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphicsGem_AR_TestComponent, BaseClass>()
                ->Version(0)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("GraphicsGem_AR_TestComponentTypeId", BehaviorConstant(AZ::Uuid(GraphicsGem_AR_TestComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
        }
    }
}
