/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <OcclusionCullingPlane/OcclusionCullingPlaneComponentController.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        class OcclusionCullingPlaneComponent final
            : public AzFramework::Components::ComponentAdapter<OcclusionCullingPlaneComponentController, OcclusionCullingPlaneComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<OcclusionCullingPlaneComponentController, OcclusionCullingPlaneComponentConfig>;
            AZ_COMPONENT(AZ::Render::OcclusionCullingPlaneComponent, OcclusionCullingPlaneComponentTypeId, BaseClass);

            OcclusionCullingPlaneComponent() = default;
            OcclusionCullingPlaneComponent(const OcclusionCullingPlaneComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
