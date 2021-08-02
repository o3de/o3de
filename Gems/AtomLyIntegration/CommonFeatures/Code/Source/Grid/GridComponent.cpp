/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Grid/GridComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        GridComponent::GridComponent(const GridComponentConfig& config)
            : BaseClass(config)
        {
        }

        void GridComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<GridComponent, BaseClass>()
                    ->Version(0)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<GridComponent>()->RequestBus("GridComponentRequestBus");

                behaviorContext->ConstantProperty("GridComponentTypeId", BehaviorConstant(Uuid(GridComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    } // namespace Render
} // namespace AZ
