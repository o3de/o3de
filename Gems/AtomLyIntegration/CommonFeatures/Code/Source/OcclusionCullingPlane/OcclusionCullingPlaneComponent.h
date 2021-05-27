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
