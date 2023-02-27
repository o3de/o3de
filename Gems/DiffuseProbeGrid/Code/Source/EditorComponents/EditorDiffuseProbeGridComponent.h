/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <Components/DiffuseProbeGridComponent.h>
#include <Components/DiffuseProbeGridComponentConstants.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>

namespace AZ
{
    namespace Render
    {        
        class EditorDiffuseProbeGridComponent final
            : public EditorRenderComponentAdapter<DiffuseProbeGridComponentController, DiffuseProbeGridComponent, DiffuseProbeGridComponentConfig>
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , private AZ::TickBus::Handler
            , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
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

            // AZ::TickBus overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // EditorComponentSelectionRequestsBus overrides
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
            bool SupportsEditorRayIntersect() override;

            // EditorEntityInfoNotifications overrides
            void OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible) override;

            // helper functions
            AZStd::string ValidateOrCreateNewTexturePath(const AZStd::string& relativePath, const char* fileSuffix);
            void CheckoutSourceTextureFile(const AZStd::string& fullPath);
            void CheckTextureAssetNotification(const AZStd::string& relativePath, Data::Asset<RPI::StreamingImageAsset>& configurationAsset);
            AZStd::vector<Edit::EnumConstant<DiffuseProbeGridNumRaysPerProbe>> GetNumRaysPerProbeEnumList() const;

            // property change notifications
            AZ::Outcome<void, AZStd::string> OnProbeSpacingValidateX(void* newValue, const AZ::Uuid& valueType);
            AZ::Outcome<void, AZStd::string> OnProbeSpacingValidateY(void* newValue, const AZ::Uuid& valueType);
            AZ::Outcome<void, AZStd::string> OnProbeSpacingValidateZ(void* newValue, const AZ::Uuid& valueType);
            AZ::u32 OnProbeSpacingChanged();
            AZ::u32 OnAmbientMultiplierChanged();
            AZ::u32 OnViewBiasChanged();
            AZ::u32 OnNormalBiasChanged();
            AZ::u32 OnNumRaysPerProbeChanged();
            AZ::Outcome<void, AZStd::string> OnScrollingChangeValidate(void* newValue, const AZ::Uuid& valueType);
            AZ::u32 OnScrollingChanged();
            AZ::u32 OnEdgeBlendIblChanged();
            AZ::u32 OnFrameUpdateCountChanged();
            AZ::u32 OnTransparencyModeChanged();
            AZ::u32 OnEmissiveMultiplierChanged();
            AZ::u32 OnEditorModeChanged();
            AZ::u32 OnRuntimeModeChanged();
            AZ::u32 OnShowVisualizationChanged();
            AZ::u32 OnShowInactiveProbesChanged();
            AZ::u32 OnVisualizationSphereRadiusChanged();
            AZ::Outcome<void, AZStd::string> OnModeChangeValidate(void* newValue, const AZ::Uuid& valueType);

            // Button handler
            AZ::u32 BakeDiffuseProbeGrid();
            AZ::u32 GetBakeDiffuseProbeGridVisibilitySetting();

            // properties
            float m_probeSpacingX = DefaultDiffuseProbeGridSpacing;
            float m_probeSpacingY = DefaultDiffuseProbeGridSpacing;
            float m_probeSpacingZ = DefaultDiffuseProbeGridSpacing;
            float m_ambientMultiplier = DefaultDiffuseProbeGridAmbientMultiplier;
            float m_viewBias = DefaultDiffuseProbeGridViewBias;
            float m_normalBias = DefaultDiffuseProbeGridNormalBias;
            DiffuseProbeGridNumRaysPerProbe m_numRaysPerProbe = DefaultDiffuseProbeGridNumRaysPerProbe;
            bool m_scrolling = false;
            bool m_edgeBlendIbl = true;
            uint32_t m_frameUpdateCount = 1;
            DiffuseProbeGridTransparencyMode m_transparencyMode = DefaultDiffuseProbeGridTransparencyMode;
            float m_emissiveMultiplier = DefaultDiffuseProbeGridEmissiveMultiplier;
            DiffuseProbeGridMode m_editorMode = DiffuseProbeGridMode::RealTime;
            DiffuseProbeGridMode m_runtimeMode = DiffuseProbeGridMode::RealTime;
            bool m_showVisualization = false;
            bool m_showInactiveProbes = false;
            float m_visualizationSphereRadius = DefaultVisualizationSphereRadius;

            // flags
            bool m_editorModeSet = false;
            AZStd::atomic_bool m_bakeInProgress = false;

            // handler for the diffuse probe grid changing the underlying box dimensions
            AZ::Event<bool>::Handler m_boxChangedByGridHandler;
        };
    } // namespace Render
} // namespace AZ
