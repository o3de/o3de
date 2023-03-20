/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CommonSampleComponentBase.h>


#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>

#include <Utils/Utils.h>
#include <Utils/ImGuiSidebar.h>
#include <Atom/Utils/ImGuiMaterialDetails.h>
#include <Utils/ImGuiAssetBrowser.h>

#include <Atom/Bootstrap/DefaultWindowBus.h>
#include <Atom/Feature/ImGui/ImGuiUtils.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>

#include <MeshletsAssets.h>
#include <MeshletsRenderObject.h>

namespace AZ
{
    namespace Meshlets
    {
        class MeshletsFeatureProcessor;
    }
}

namespace AtomSampleViewer
{
    class MeshletsExampleComponent final
        : public CommonSampleComponentBase
        , public AZ::Render::Bootstrap::DefaultWindowNotificationBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(MeshletsExampleComponent, "{BFE93321-91A4-4087-BABE-8B475087BBAD}", CommonSampleComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        MeshletsExampleComponent();
        ~MeshletsExampleComponent() override = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AZ::EntityBus::MultiHandler
        void OnEntityDestruction(const AZ::EntityId& entityId) override;

        void ModelChange();

        void CreateGroundPlane();
        void UpdateGroundPlane();
        void RemoveGroundPlane();

        void UseArcBallCameraController();
        void UseNoClipCameraController();
        void RemoveController();

        void SetArcBallControllerParams();
        void ResetCameraController();

        void DefaultWindowCreated() override;

        AZ::Meshlets::MeshletsFeatureProcessor* GetMeshletsFeatureProcessor();

        AZStd::shared_ptr<AZ::RPI::WindowContext> m_windowContext;
        AZ::Render::ImGuiActiveContextScope m_imguiScope;

        enum class CameraControllerType : int32_t 
        {
            ArcBall = 0,
            NoClip,
            Count
        };
        static const uint32_t CameraControllerCount = static_cast<uint32_t>(CameraControllerType::Count);
        static const char* CameraControllerNameTable[CameraControllerCount];
        CameraControllerType m_currentCameraControllerType = CameraControllerType::ArcBall;

        AZ::Render::MeshFeatureProcessorInterface::ModelChangedEvent::Handler m_changedHandler;
        
        static constexpr float ArcballRadiusMinModifier = 0.01f;
        static constexpr float ArcballRadiusMaxModifier = 4.0f;
        static constexpr float ArcballRadiusDefaultModifier = 2.0f;
        
        AZ::RPI::Cullable::LodConfiguration m_lodConfig;

        bool m_enableMaterialOverride = true;

        // If false, only azmaterials generated from ".material" files will be listed.
        // Otherwise, all azmaterials, regardless of its source (e.g ".fbx"), will
        // be shown in the material list.
        bool m_showModelMaterials = false;

        bool m_showGroundPlane = false;

        bool m_cameraControllerDisabled = false;

        AZ::Meshlets::MeshletsFeatureProcessor* m_meshletsFeatureProcessor = nullptr;

        AZ::Data::Instance<AZ::RPI::Material> m_materialOverrideInstance; //< Holds a copy of the material instance being used when m_enableMaterialOverride is true.
        AZ::Render::MeshFeatureProcessorInterface::MeshHandle m_meshHandle;
        AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;

        // This is the data stored for the copied mesh with the new generated meshlets structure.
        AZ::Data::Instance<AZ::RPI::Material> m_meshletsDebugMaterial;
        AZ::Meshlets::MeshletsModel* m_meshetsModel = nullptr;
        AZ::Meshlets::MeshletsRenderObject* m_meshetsRenderObject = nullptr;
        AZ::Data::Asset<AZ::RPI::ModelAsset> m_meshletsModelAsset;
        AZ::Render::MeshFeatureProcessorInterface::MeshHandle m_meshletsMeshHandle;
        AZ::Render::TransformServiceFeatureProcessorInterface::ObjectId m_meshletObjectId;

        AZ::Data::Asset<AZ::RPI::ModelAsset> m_groundPlaneModelAsset;
        AZ::Render::MeshFeatureProcessorInterface::MeshHandle m_groundPlandMeshHandle;
        AZ::Data::Instance<AZ::RPI::Material> m_groundPlaneMaterial;

        ImGuiSidebar m_imguiSidebar;
        AZ::Render::ImGuiMaterialDetails m_imguiMaterialDetails;
        ImGuiAssetBrowser m_materialBrowser;
        ImGuiAssetBrowser m_modelBrowser;
    };
} // namespace AtomSampleViewer
