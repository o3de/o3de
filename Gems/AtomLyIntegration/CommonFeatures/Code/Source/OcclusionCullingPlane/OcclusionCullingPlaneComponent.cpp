/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OcclusionCullingPlane/OcclusionCullingPlaneComponent.h>

namespace AZ
{
    namespace Render
    {
        OcclusionCullingPlaneComponent::OcclusionCullingPlaneComponent(const OcclusionCullingPlaneComponentConfig& config)
            : BaseClass(config)
        {
        }

        void OcclusionCullingPlaneComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<OcclusionCullingPlaneComponent, BaseClass>()
                    ->Version(0)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("OcclusionCullingPlaneComponentTypeId", BehaviorConstant(Uuid(OcclusionCullingPlaneComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    } // namespace Render
} // namespace AZ
