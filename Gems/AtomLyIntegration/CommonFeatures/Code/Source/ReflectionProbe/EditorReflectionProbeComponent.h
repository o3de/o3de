/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <ReflectionProbe/ReflectionProbeComponent.h>
#include <ReflectionProbe/ReflectionProbeComponentConstants.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/ReflectionProbe/EditorReflectionProbeBus.h>

namespace AZ
{
    namespace Render
    {        
        class EditorReflectionProbeComponent final
            : public EditorRenderComponentAdapter<ReflectionProbeComponentController, ReflectionProbeComponent, ReflectionProbeComponentConfig>
            , public EditorReflectionProbeBus::Handler
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , private AZ::TickBus::Handler
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<ReflectionProbeComponentController, ReflectionProbeComponent, ReflectionProbeComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorReflectionProbeComponent, EditorReflectionProbeComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorReflectionProbeComponent() = default;
            EditorReflectionProbeComponent(const ReflectionProbeComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;
            void Deactivate() override;

            // AzFramework::EntityDebugDisplayEventBus::Handler overrides
            void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        private:

            // AZ::TickBus overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // validation
            AZ::Outcome<void, AZStd::string> OnUseBakedCubemapValidate(void* newValue, const AZ::Uuid& valueType);

            // change notifications
            AZ::u32 OnUseBakedCubemapChanged();
            AZ::u32 OnAuthoredCubemapChanged();

            // retrieves visibility for baked or authored cubemap controls
            AZ::u32 GetBakedCubemapVisibilitySetting();
            AZ::u32 GetAuthoredCubemapVisibilitySetting();

            // initiate the reflection probe cubemap generation, returns the refresh value for the ChangeNotify UIElement attribute 
            AZ::u32 BakeReflectionProbe() override;

            // save the cubemap data to the output file
            void WriteOutputFile(AZStd::string filePath, uint8_t* const* cubeMapTextureData, RHI::Format cubeMapTextureFormat);

            // EditorComponentSelectionRequestsBus overrides
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
            bool SupportsEditorRayIntersect() override;

            // UI settings
            // the user can select between a baked cubemap or an authored cubemap asset
            bool m_useBakedCubemap = true;
            BakedCubeMapQualityLevel m_bakedCubeMapQualityLevel = BakedCubeMapQualityLevel::Medium;
            AZStd::string m_bakedCubeMapRelativePath;
            Data::Asset<RPI::StreamingImageAsset> m_bakedCubeMapAsset;
            Data::Asset<RPI::StreamingImageAsset> m_authoredCubeMapAsset;

            // flag indicating if a cubemap bake is currently in progress
            AZStd::atomic_bool m_bakeInProgress = false;
        };

    } // namespace Render
} // namespace AZ
