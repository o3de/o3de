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
