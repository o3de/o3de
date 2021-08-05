/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneComponent.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneComponentConstants.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>

namespace AZ
{
    namespace Render
    {        
        class EditorOcclusionCullingPlaneComponent final
            : public EditorRenderComponentAdapter<OcclusionCullingPlaneComponentController, OcclusionCullingPlaneComponent, OcclusionCullingPlaneComponentConfig>
            , private AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<OcclusionCullingPlaneComponentController, OcclusionCullingPlaneComponent, OcclusionCullingPlaneComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorOcclusionCullingPlaneComponent, EditorOcclusionCullingPlaneComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorOcclusionCullingPlaneComponent();
            EditorOcclusionCullingPlaneComponent(const OcclusionCullingPlaneComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;
            void Deactivate() override;
        };
    } // namespace Render
} // namespace AZ
