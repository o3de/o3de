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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <LmbrCentral/Rendering/EditorLightComponentBus.h>
#include <LmbrCentral/Rendering/EditorCameraCorrectionBus.h>

#include "LightComponent.h"

#include <IStatObj.h>

namespace LmbrCentral
{
    class EditorLightComponent;

    /**
     * Extends LightConfiguration structure to add editor functionality
     * such as property handlers and visibility filters, as well as
     * reflection for editing.
     */

    class EditorLightConfiguration
        : public LightConfiguration
    {
    public:
        AZ_TYPE_INFO(EditorLightConfiguration, "{1D3B114F-8FB2-47BD-9C21-E089F4F37861}");

        ~EditorLightConfiguration() override {}

        static void Reflect(AZ::ReflectContext* context);

        AZ::Crc32 GetAmbientLightVisibility() const override;
        AZ::Crc32 GetPointLightVisibility() const override;
        AZ::Crc32 GetProjectorLightVisibility() const override;
        AZ::Crc32 GetProbeLightVisibility() const override;
        AZ::Crc32 GetShadowSpecVisibility() const override;
        AZ::Crc32 GetShadowSettingsVisibility() const override;
        AZ::Crc32 GetAreaSettingVisibility() const override;

        AZ::Crc32 MinorPropertyChanged() override;
        AZ::Crc32 MajorPropertyChanged() override;
        AZ::Crc32 OnAnimationSettingChanged() override;
        AZ::Crc32 OnCubemapAssetChanged() override;
        bool CanGenerateCubemap() const override;

        void SetComponent(EditorLightComponent* component);
        EditorLightComponent* m_component;
    };

    /*!
     * In-editor light component.
     * Handles previewing and activating lights in the editor.
     */
    class EditorLightComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private EditorLightComponentRequestBus::Handler
        , private RenderNodeRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private EditorCameraCorrectionRequestBus::Handler
    {
    private:
        using Base = AzToolsFramework::Components::EditorComponentBase;
    public:
        
        friend EditorLightConfiguration;
        //old guid "{33BB1CD4-6A33-46AA-87ED-8BBB40D94B0D}" before splitting editor light component
        AZ_COMPONENT(EditorLightComponent, "{7C18B273-5BA3-4E0F-857D-1F30BD6B0733}", AzToolsFramework::Components::EditorComponentBase); 
 
        EditorLightComponent();
        ~EditorLightComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorVisibilityNotificationBus interface implementation
        void OnEntityVisibilityChanged(bool visibility) override;
        //////////////////////////////////////////////////////////////////////////

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        //////////////////////////////////////////////////////////////////////////
        // EditorLightComponentRequestBus::Handler interface implementation
        void SetCubemap(const AZStd::string& cubemap) override;
        void SetProjectorTexture(const AZStd::string& projectorTexture) override;

        AZ::u32 GetCubemapResolution() override;
        void SetCubemapResolution(AZ::u32 resolution) override;
        bool UseCustomizedCubemap() const override;
        void RefreshLight() override;
        const LightConfiguration& GetConfiguration() const override;
        AZ::Crc32 OnCubemapAssetChanged();
        AZ::Crc32 OnCustomizedCubemapChanged();

        // EditorCameraCorrectionRequestBus interface implementation
        // Light is aligned along the the x-axis so add a transform correction to align along the y-axis.
        AZ::Matrix3x3 GetTransformCorrection() override { return AZ::Matrix3x3::CreateRotationZ(-AZ::Constants::HalfPi); }

        ///////////////////////////////////////////
        // EditorLightComponentRequestBus Modifiers
        void SetVisible(bool isVisible) override;
        bool GetVisible() override;

        void SetOnInitially(bool onInitially) override;
        bool GetOnInitially() override;

        void SetColor(const AZ::Color& newColor) override;
        const AZ::Color GetColor() override;

        void SetDiffuseMultiplier(float newMultiplier) override;
        float GetDiffuseMultiplier() override;

        void SetSpecularMultiplier(float newMultiplier) override;
        float GetSpecularMultiplier() override;

        void SetAmbient(bool isAmbient) override;
        bool GetAmbient() override;

        void SetPointMaxDistance(float newMaxDistance) override;
        float GetPointMaxDistance() override;

        void SetPointAttenuationBulbSize(float newAttenuationBulbSize) override;
        float GetPointAttenuationBulbSize() override;

        void SetAreaMaxDistance(float newMaxDistance) override;
        float GetAreaMaxDistance() override;

        void SetAreaWidth(float newWidth) override;
        float GetAreaWidth() override;

        void SetAreaHeight(float newHeight) override;
        float GetAreaHeight() override;

        void SetAreaFOV(float newFOV) override;
        float GetAreaFOV() override;

        void SetProjectorMaxDistance(float newMaxDistance) override;
        float GetProjectorMaxDistance() override;

        void SetProjectorAttenuationBulbSize(float newAttenuationBulbSize) override;
        float GetProjectorAttenuationBulbSize() override;

        void SetProjectorFOV(float newFOV) override;
        float GetProjectorFOV() override;

        void SetProjectorNearPlane(float newNearPlane) override;
        float GetProjectorNearPlane() override;

        // Environment Probe Settings
        void SetProbeAreaDimensions(const AZ::Vector3& newDimensions) override;
        const AZ::Vector3 GetProbeAreaDimensions() override;

        void SetProbeSortPriority(AZ::u32 newPriority) override;
        AZ::u32 GetProbeSortPriority() override;

        void SetProbeBoxProjected(bool isProbeBoxProjected) override;
        bool GetProbeBoxProjected() override;

        void SetProbeBoxHeight(float newHeight) override;
        float GetProbeBoxHeight() override;

        void SetProbeBoxLength(float newLength) override;
        float GetProbeBoxLength() override;

        void SetProbeBoxWidth(float newWidth) override;
        float GetProbeBoxWidth() override;

        void SetProbeAttenuationFalloff(float newAttenuationFalloff) override;
        float GetProbeAttenuationFalloff() override;

        void SetProbeFade(float fade) override;
        float GetProbeFade() override;

        void SetIndoorOnly(bool indoorOnly) override;
        bool GetIndoorOnly() override;

        void SetCastShadowSpec(AZ::u32 castShadowSpec) override;
        AZ::u32 GetCastShadowSpec() override;

        void SetViewDistanceMultiplier(float viewDistanceMultiplier) override;
        float GetViewDistanceMultiplier() override;

        void SetProbeArea(const AZ::Vector3& probeArea) override;
        AZ::Vector3 GetProbeArea() override;

        void SetVolumetricFogOnly(bool volumetricFogOnly) override;
        bool GetVolumetricFogOnly() override;

        void SetVolumetricFog(bool volumetricFog) override;
        bool GetVolumetricFog() override;

        void SetAttenuationFalloffMax(float attenFalloffMax);
        float GetAttenuationFalloffMax();

        void SetUseVisAreas(bool useVisAreas) override;
        bool GetUseVisAreas() override;

        void SetAffectsThisAreaOnly(bool affectsThisAreaOnly) override;
        bool GetAffectsThisAreaOnly() override;

        void SetBoxHeight(float boxHeight) override;
        float GetBoxHeight() override;

        void SetBoxWidth(float boxWidth) override;
        float GetBoxWidth() override;

        void SetBoxLength(float boxLength) override;
        float GetBoxLength() override;

        void SetBoxProjected(bool boxProjected) override;
        bool GetBoxProjected() override;

        void SetShadowBias(float shadowBias) override;
        float GetShadowBias() override;

        void SetShadowSlopeBias(float shadowSlopeBias) override;
        float GetShadowSlopeBias() override;

        void SetShadowResScale(float shadowResScale) override;
        float GetShadowResScale() override;

        void SetShadowUpdateMinRadius(float shadowUpdateMinRadius) override;
        float GetShadowUpdateMinRadius() override;

        void SetShadowUpdateRatio(float shadowUpdateRatio) override;
        float GetShadowUpdateRatio() override;
        //////////////////////////////////////////////////////////////////////////

        // Animation parameters
        void SetAnimIndex(AZ::u32 animIndex) override;
        AZ::u32 GetAnimIndex() override;

        void SetAnimSpeed(float animSpeed) override;
        float GetAnimSpeed() override;

        void SetAnimPhase(float animPhase) override;
        float GetAnimPhase() override;

        //////////////////////////////////////////////////////////////////////////
        // RenderNodeRequestBus::Handler interface implementation
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus::Handler interface implementation
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //////////////////////////////////////////////////////////////////////////
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            LightComponent::GetProvidedServices(provided);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            LightComponent::GetRequiredServices(required);
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            LightComponent::GetDependentServices(dependent);
            dependent.push_back(AZ_CRC("EditorVisibilityService", 0x90888caf));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            LightComponent::GetIncompatibleServices(incompatible);
        }

        static AZ::ExportedComponent ExportLightComponent(AZ::Component* thisComponent, const AZ::PlatformTagSet& /*platformTags*/);

        static void Reflect(AZ::ReflectContext* context);
        //////////////////////////////////////////////////////////////////////////

    protected:
        void SetLightType(EditorLightConfiguration::LightType lightType)
        {
            m_configuration.m_lightType = lightType;
        }

        AZ::Uuid GetCubemapId()
        {
            return m_configuration.m_cubemapId;
        }

        virtual const char* GetLightTypeText() const;

    private:

        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);

        //! Handles rendering of the preview cubemap by creating a simple cubemapped sphere.
        class CubemapPreview : public IRenderNode
        {
        public:
            CubemapPreview();
            void Setup(const char* textureName);
            void UpdateTexture(const char* textureName);
            void SetTransform(const Matrix34& transform);

        private:
            //////////////////////////////////////////////////////////////////////////
            // IRenderNode interface implementation
            void Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo) override;
            EERType GetRenderNodeType() override;
            const char* GetName() const override;
            const char* GetEntityClassName() const override;
            Vec3 GetPos(bool bWorldOnly = true) const override;
            const AABB GetBBox() const override;
            void SetBBox([[maybe_unused]] const AABB& WSBBox) override {}
            void OffsetPosition([[maybe_unused]] const Vec3& delta) override {}
            void SetMaterial(_smart_ptr<IMaterial> pMat) override {}
            _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = nullptr) override;
            _smart_ptr<IMaterial> GetMaterialOverride() override;
            IStatObj* GetEntityStatObj(unsigned int nPartId = 0, unsigned int nSubPartId = 0, Matrix34A* pMatrix = nullptr, bool bReturnOnlyVisible = false);
            float GetMaxViewDist() override;
            void GetMemoryUsage(class ICrySizer* pSizer) const override;
            //////////////////////////////////////////////////////////////////////////
        private:
            Matrix34 m_renderTransform;
            _smart_ptr<IStatObj> m_statObj;
        };

        //if an cubemap asset is added or changed
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;

        void OnEditorSpecChange() override;

        //! Returns whether the light is an environment probe
        bool IsProbe() const;

        //! Returns whether the light has a cubemap assigned
        bool HasCubemap() const;

        //! Triggers regeneration of the environment probe's cubemap (the current cubemap output will be baked into the new cubemap as well).
        void GenerateCubemap();

        //! Removes the associated cubemap, returning the environment probe to its default state
        void ClearCubemap();

        void OnViewCubemapChanged();

        //! Returns true if it's an environment probe and not using customized cubemap
        bool CanGenerateCubemap() const;

        //! Returns the name to use for the Generate Cubemap button.
        const char* GetGenerateCubemapButtonName() const;
        
        const char* GetCubemapAssetName() const;

        void DrawProjectionGizmo(AzFramework::DebugDisplayRequests& dc, const float radius) const;
        void DrawPlaneGizmo(AzFramework::DebugDisplayRequests& dc, const float depth) const;

        EditorLightConfiguration m_configuration;
        bool m_viewCubemap;
        bool m_useCustomizedCubemap;
        bool m_cubemapRegen;
        bool m_cubemapClear;

        CubemapPreview m_cubemapPreview;

        LightInstance m_light;

        //Statics that can be used by every light
        static IEditor*             m_editor;
        static IMaterialManager*    m_materialManager;
        static const char* BUTTON_GENERATE;
        static const char* BUTTON_ADDBOUNCE;
    };
} // namespace LmbrCentral

