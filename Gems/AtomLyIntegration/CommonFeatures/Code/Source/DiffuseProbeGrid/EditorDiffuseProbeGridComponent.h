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
#include <DiffuseProbeGrid/DiffuseProbeGridComponent.h>
#include <DiffuseProbeGrid/DiffuseProbeGridComponentConstants.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>

namespace AZ
{
    namespace Render
    {        
        class EditorDiffuseProbeGridComponent final
            : public EditorRenderComponentAdapter<DiffuseProbeGridComponentController, DiffuseProbeGridComponent, DiffuseProbeGridComponentConfig>
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<DiffuseProbeGridComponentController, DiffuseProbeGridComponent, DiffuseProbeGridComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorDiffuseProbeGridComponent, EditorDiffuseProbeGridComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorDiffuseProbeGridComponent();
            EditorDiffuseProbeGridComponent(const DiffuseProbeGridComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;
            void Deactivate() override;

        private:
            // EditorComponentSelectionRequestsBus overrides
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
            bool SupportsEditorRayIntersect() override;

            // property change notifications
            AZ::Outcome<void, AZStd::string> OnProbeSpacingValidateX(void* newValue, const AZ::Uuid& valueType);
            AZ::Outcome<void, AZStd::string> OnProbeSpacingValidateY(void* newValue, const AZ::Uuid& valueType);
            AZ::Outcome<void, AZStd::string> OnProbeSpacingValidateZ(void* newValue, const AZ::Uuid& valueType);
            AZ::u32 OnProbeSpacingChanged();
            AZ::u32 OnAmbientMultiplierChanged();
            AZ::u32 OnViewBiasChanged();
            AZ::u32 OnNormalBiasChanged();

            // properties
            float m_probeSpacingX = DefaultDiffuseProbeGridSpacing;
            float m_probeSpacingY = DefaultDiffuseProbeGridSpacing;
            float m_probeSpacingZ = DefaultDiffuseProbeGridSpacing;
            float m_ambientMultiplier = DefaultDiffuseProbeGridAmbientMultiplier;
            float m_viewBias = DefaultDiffuseProbeGridViewBias;
            float m_normalBias = DefaultDiffuseProbeGridNormalBias;
        };
    } // namespace Render
} // namespace AZ
