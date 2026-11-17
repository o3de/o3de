/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/GradientGI/GradientGILogic.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Utils/Utils.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConstants.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientGI/GradientGIComponentController.h>

namespace AZ
{
    namespace Render
    {
        // =====================================================================
        // Reflect
        // =====================================================================

        void GradientGIComponentController::Reflect(ReflectContext* context)
        {
            GradientGIComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<GradientGIComponentController>()->Version(0)->Field(
                    "Configuration", &GradientGIComponentController::m_configuration);
            }

            if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                // Expose the enum values for Script Canvas
                behaviorContext
                    ->Enum<static_cast<uint32_t>(GradientGIUpdateMode::Static)>("GradientGIUpdateMode_Static")
                    ->Enum<static_cast<uint32_t>(GradientGIUpdateMode::Dynamic)>("GradientGIUpdateMode_Dynamic")
                    ->Enum<static_cast<uint32_t>(GradientGITextureMapping::Tiled)>("GradientGITextureMapping_Tiled")
                    ->Enum<static_cast<uint32_t>(GradientGITextureMapping::Stretched)>("GradientGITextureMapping_Stretched")
                    ->Enum<static_cast<uint32_t>(GradientGITextureMapping::Cube)>("GradientGITextureMapping_Cube")
                    ->Enum<static_cast<uint32_t>(GradientGIBlendMode::Multiply)>("GradientGIBlendMode_Multiply")
                    ->Enum<static_cast<uint32_t>(GradientGIBlendMode::Add)>("GradientGIBlendMode_Add")
                    ->Enum<static_cast<uint32_t>(GradientGIBlendMode::Screen)>("GradientGIBlendMode_Screen")
                    ->Enum<static_cast<uint32_t>(GradientGIBlendMode::Overlay)>("GradientGIBlendMode_Overlay")
                    ->Enum<static_cast<uint32_t>(GradientGIBlendMode::Replace)>("GradientGIBlendMode_Replace")
                    ;

                behaviorContext->EBus<GradientGIComponentRequestBus>("GradientGIComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    // Single-layer color setters/getters
                    ->Event("SetLowColor", &GradientGIComponentRequestBus::Events::SetLowColor)
                    ->Event("GetLowColor", &GradientGIComponentRequestBus::Events::GetLowColor)
                    ->Event("SetMidColor", &GradientGIComponentRequestBus::Events::SetMidColor)
                    ->Event("GetMidColor", &GradientGIComponentRequestBus::Events::GetMidColor)
                    ->Event("SetHighColor", &GradientGIComponentRequestBus::Events::SetHighColor)
                    ->Event("GetHighColor", &GradientGIComponentRequestBus::Events::GetHighColor)
                    // Set all three gradient layers at once
                    ->Event("SetGradientColors", &GradientGIComponentRequestBus::Events::SetGradientColors)
                    // Exposure
                    ->Event("SetExposure", &GradientGIComponentRequestBus::Events::SetExposure)
                    ->Event("GetExposure", &GradientGIComponentRequestBus::Events::GetExposure)
                    // Resolution
                    ->Event("SetFaceResolution", &GradientGIComponentRequestBus::Events::SetFaceResolution)
                    ->Event("GetFaceResolution", &GradientGIComponentRequestBus::Events::GetFaceResolution)
                    // Update mode (Static / Dynamic)
                    ->Event("SetUpdateMode", &GradientGIComponentRequestBus::Events::SetUpdateMode)
                    ->Event("GetUpdateMode", &GradientGIComponentRequestBus::Events::GetUpdateMode)
                    // Detail texture layer (scripting uses AssetId; the AssetPath route is not
                    // reflected -- a raw path can't resolve to a processed product asset, so only
                    // the AssetId node is exposed to Script Canvas).
                    ->Event("SetDetailTextureAssetId", &GradientGIComponentRequestBus::Events::SetDetailTextureAssetId)
                    ->Event("GetDetailTextureAssetId", &GradientGIComponentRequestBus::Events::GetDetailTextureAssetId)
                    ->Event("SetDetailMapping", &GradientGIComponentRequestBus::Events::SetDetailMapping)
                    ->Event("GetDetailMapping", &GradientGIComponentRequestBus::Events::GetDetailMapping)
                    ->Event("SetDetailBlend", &GradientGIComponentRequestBus::Events::SetDetailBlend)
                    ->Event("GetDetailBlend", &GradientGIComponentRequestBus::Events::GetDetailBlend)
                    ->Event("SetDetailStrength", &GradientGIComponentRequestBus::Events::SetDetailStrength)
                    ->Event("GetDetailStrength", &GradientGIComponentRequestBus::Events::GetDetailStrength)
                    // Specular texture layer (scripting uses AssetId; AssetPath not reflected, see above)
                    ->Event("SetSpecularTextureAssetId", &GradientGIComponentRequestBus::Events::SetSpecularTextureAssetId)
                    ->Event("GetSpecularTextureAssetId", &GradientGIComponentRequestBus::Events::GetSpecularTextureAssetId)
                    ->Event("SetSpecularMapping", &GradientGIComponentRequestBus::Events::SetSpecularMapping)
                    ->Event("GetSpecularMapping", &GradientGIComponentRequestBus::Events::GetSpecularMapping)
                    ->Event("SetSpecularBlend", &GradientGIComponentRequestBus::Events::SetSpecularBlend)
                    ->Event("GetSpecularBlend", &GradientGIComponentRequestBus::Events::GetSpecularBlend)
                    ->Event("SetSpecularStrength", &GradientGIComponentRequestBus::Events::SetSpecularStrength)
                    ->Event("GetSpecularStrength", &GradientGIComponentRequestBus::Events::GetSpecularStrength)
                    // Virtual properties for Script Canvas property nodes
                    ->VirtualProperty("LowColor", "GetLowColor", "SetLowColor")
                    ->VirtualProperty("MidColor", "GetMidColor", "SetMidColor")
                    ->VirtualProperty("HighColor", "GetHighColor", "SetHighColor")
                    ->VirtualProperty("Exposure", "GetExposure", "SetExposure")
                    ->VirtualProperty("FaceResolution", "GetFaceResolution", "SetFaceResolution")
                    ->VirtualProperty("UpdateMode", "GetUpdateMode", "SetUpdateMode")
                    ->VirtualProperty("DetailTextureAssetId", "GetDetailTextureAssetId", "SetDetailTextureAssetId")
                    ->VirtualProperty("DetailMapping", "GetDetailMapping", "SetDetailMapping")
                    ->VirtualProperty("DetailBlend", "GetDetailBlend", "SetDetailBlend")
                    ->VirtualProperty("DetailStrength", "GetDetailStrength", "SetDetailStrength")
                    ->VirtualProperty("SpecularTextureAssetId", "GetSpecularTextureAssetId", "SetSpecularTextureAssetId")
                    ->VirtualProperty("SpecularMapping", "GetSpecularMapping", "SetSpecularMapping")
                    ->VirtualProperty("SpecularBlend", "GetSpecularBlend", "SetSpecularBlend")
                    ->VirtualProperty("SpecularStrength", "GetSpecularStrength", "SetSpecularStrength");
            }
        }

        // =====================================================================
        // Service Descriptors
        // =====================================================================

        void GradientGIComponentController::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GradientGIService"));
        }

        void GradientGIComponentController::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("GradientGIService"));
            // Gradient GI and a Global Skylight (IBL) both drive the scene's IBL slots.
            // Reject both on the same entity so they cannot fight over ownership; cross-entity
            // coexistence is handled cooperatively in the feature processor (yield / reclaim).
            incompatible.push_back(AZ_CRC_CE("ImageBasedLightService"));
        }

        // =====================================================================
        // Lifecycle
        // =====================================================================

        GradientGIComponentController::GradientGIComponentController(const GradientGIComponentConfig& config)
            : m_configuration(config)
        {
        }

        void GradientGIComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<GradientGIFeatureProcessorInterface>(m_entityId);
            AZ_Error(
                "GradientGIComponentController",
                m_featureProcessor,
                "Unable to find GradientGIFeatureProcessorInterface on this entity's scene.");

            if (m_featureProcessor)
            {
                GradientGIComponentRequestBus::Handler::BusConnect(m_entityId);
                m_featureProcessor->ConnectToOwnerChangedEvent(m_ownerChangedHandler);
                m_featureProcessor->Enable(m_entityId);
            }
        }

        void GradientGIComponentController::Deactivate()
        {
            m_ownerChangedHandler.Disconnect();
            AZ::TickBus::Handler::BusDisconnect();
            GradientGIComponentRequestBus::Handler::BusDisconnect();
            Data::AssetBus::MultiHandler::BusDisconnect();

            m_detailTextureNeedsUpdate = false;
            m_specularTextureNeedsUpdate = false;

            if (m_featureProcessor)
            {
                // Scoped to this entity: if another GradientGI component has already taken
                // ownership (the editor-only copy re-activating as a spawned copy tears down),
                // this call is ignored and the live one keeps lighting the scene.
                m_featureProcessor->Disable(m_entityId);
                m_featureProcessor = nullptr;
            }

            m_entityId = EntityId(EntityId::InvalidEntityId);
        }

        void GradientGIComponentController::SetConfiguration(const GradientGIComponentConfig& config)
        {
            AZ::TickBus::Handler::BusDisconnect();
            Data::AssetBus::MultiHandler::BusDisconnect();

            m_detailTextureNeedsUpdate = false;
            m_specularTextureNeedsUpdate = false;

            m_configuration = config;

            if (IsActiveOwner())
            {
                ApplyConfiguration();
            }
            else if (m_featureProcessor)
            {
                LoadDetailTexture();
                LoadSpecularTexture();
            }
        }

        const GradientGIComponentConfig& GradientGIComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        // =====================================================================
        // Bus Handlers
        // =====================================================================

        void GradientGIComponentController::SetLowColor(const Color& color)
        {
            m_configuration.m_lowColor = color;
            UpdateColors();
        }

        Color GradientGIComponentController::GetLowColor() const
        {
            return m_configuration.m_lowColor;
        }

        void GradientGIComponentController::SetMidColor(const Color& color)
        {
            m_configuration.m_midColor = color;
            UpdateColors();
        }

        Color GradientGIComponentController::GetMidColor() const
        {
            return m_configuration.m_midColor;
        }

        void GradientGIComponentController::SetHighColor(const Color& color)
        {
            m_configuration.m_highColor = color;
            UpdateColors();
        }

        Color GradientGIComponentController::GetHighColor() const
        {
            return m_configuration.m_highColor;
        }

        void GradientGIComponentController::SetGradientColors(const Color& low, const Color& mid, const Color& high)
        {
            m_configuration.m_lowColor = low;
            m_configuration.m_midColor = mid;
            m_configuration.m_highColor = high;
            UpdateColors();
        }

        void GradientGIComponentController::SetExposure(float exposure)
        {
            m_configuration.m_exposure = exposure;

            if (IsActiveOwner())
            {
                m_featureProcessor->SetExposure(exposure);
            }
        }

        float GradientGIComponentController::GetExposure() const
        {
            return m_configuration.m_exposure;
        }

        void GradientGIComponentController::SetFaceResolution(float resolution)
        {
            // Script Canvas hands us a Number (float). Round to the nearest whole pixel and clamp
            // into the supported range before storing into the unsigned config field.
            m_configuration.m_faceResolution = GradientGI::RoundAndClampFaceResolution(resolution);
            SnapResolutionForCurrentMode();

            if (IsActiveOwner())
            {
                m_featureProcessor->SetFaceResolution(m_configuration.m_faceResolution);
            }
        }

        void GradientGIComponentController::SnapResolutionForCurrentMode()
        {
            // CPU/Static renders only power-of-two cubemap faces (engine streaming-image upload
            // limit -- a per-face >64KB non-power-of-two subresource samples black). Snap the stored
            // value so the slider and the scripting getter agree with what is actually rendered, with
            // no hidden clip. GPU/Dynamic composites any size on the GPU and is left untouched.
            if (m_configuration.m_updateMode == GradientGIUpdateMode::Static ||
                (IsActiveOwner() && m_featureProcessor->GetUpdateMode() == GradientGIFeatureProcessorInterface::UpdateMode::Static))
            {
                m_configuration.m_faceResolution = GradientGI::SnapCpuFaceResolutionToPow2(m_configuration.m_faceResolution);
            }
        }

        float GradientGIComponentController::GetFaceResolution() const
        {
            return static_cast<float>(m_configuration.m_faceResolution);
        }

        void GradientGIComponentController::SetUpdateMode(GradientGIUpdateMode mode)
        {
            m_configuration.m_updateMode = mode;

            if (IsActiveOwner())
            {
                m_featureProcessor->SetUpdateMode(static_cast<GradientGIFeatureProcessorInterface::UpdateMode>(mode));
            }

            SnapResolutionForCurrentMode();

            if (IsActiveOwner())
            {
                m_featureProcessor->SetFaceResolution(m_configuration.m_faceResolution);
            }
        }

        GradientGIUpdateMode GradientGIComponentController::GetUpdateMode() const
        {
            return m_configuration.m_updateMode;
        }

        // =====================================================================
        // Bus Handlers -- Detail Texture Layer
        // =====================================================================

        void GradientGIComponentController::SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture)
        {
            m_detailTextureNeedsUpdate = false;

            if (m_configuration.m_detailTexture.GetId() != m_configuration.m_specularTexture.GetId())
            {
                Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_detailTexture.GetId());
            }

            m_configuration.m_detailTexture = texture;
            LoadDetailTexture();
        }

        Data::Asset<RPI::StreamingImageAsset> GradientGIComponentController::GetDetailTexture() const
        {
            return m_configuration.m_detailTexture;
        }

        void GradientGIComponentController::SetDetailTextureAssetId(const Data::AssetId& id)
        {
            SetDetailTexture(GetAssetFromId<RPI::StreamingImageAsset>(
                id, m_configuration.m_detailTexture.GetAutoLoadBehavior()));
        }

        Data::AssetId GradientGIComponentController::GetDetailTextureAssetId() const
        {
            return m_configuration.m_detailTexture.GetId();
        }

        void GradientGIComponentController::SetDetailTextureAssetPath(const AZStd::string& path)
        {
            SetDetailTexture(
                GetAssetFromPath<RPI::StreamingImageAsset>(
                    GradientGI::NormalizeAssetPath(path), m_configuration.m_detailTexture.GetAutoLoadBehavior()));
        }

        AZStd::string GradientGIComponentController::GetDetailTextureAssetPath() const
        {
            AZStd::string path;
            Data::AssetCatalogRequestBus::BroadcastResult(
                path, &Data::AssetCatalogRequests::GetAssetPathById, m_configuration.m_detailTexture.GetId());
            return path;
        }

        void GradientGIComponentController::SetDetailMapping(GradientGITextureMapping mapping)
        {
            m_configuration.m_detailMapping = mapping;
            PushDetailParams();
        }

        GradientGITextureMapping GradientGIComponentController::GetDetailMapping() const
        {
            return m_configuration.m_detailMapping;
        }

        void GradientGIComponentController::SetDetailBlend(GradientGIBlendMode blend)
        {
            m_configuration.m_detailBlend = blend;
            PushDetailParams();
        }

        GradientGIBlendMode GradientGIComponentController::GetDetailBlend() const
        {
            return m_configuration.m_detailBlend;
        }

        void GradientGIComponentController::SetDetailStrength(float strength)
        {
            m_configuration.m_detailStrength = strength;
            PushDetailParams();
        }

        float GradientGIComponentController::GetDetailStrength() const
        {
            return m_configuration.m_detailStrength;
        }

        // =====================================================================
        // Bus Handlers -- Specular Texture Layer
        // =====================================================================

        void GradientGIComponentController::SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture)
        {
            m_specularTextureNeedsUpdate = false;

            if (m_configuration.m_specularTexture.GetId() != m_configuration.m_detailTexture.GetId())
            {
                Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_specularTexture.GetId());
            }

            m_configuration.m_specularTexture = texture;
            LoadSpecularTexture();
        }

        Data::Asset<RPI::StreamingImageAsset> GradientGIComponentController::GetSpecularTexture() const
        {
            return m_configuration.m_specularTexture;
        }

        void GradientGIComponentController::SetSpecularTextureAssetId(const Data::AssetId& id)
        {
            SetSpecularTexture(GetAssetFromId<RPI::StreamingImageAsset>(
                id, m_configuration.m_specularTexture.GetAutoLoadBehavior()));
        }

        Data::AssetId GradientGIComponentController::GetSpecularTextureAssetId() const
        {
            return m_configuration.m_specularTexture.GetId();
        }

        void GradientGIComponentController::SetSpecularTextureAssetPath(const AZStd::string& path)
        {
            SetSpecularTexture(
                GetAssetFromPath<RPI::StreamingImageAsset>(
                    GradientGI::NormalizeAssetPath(path), m_configuration.m_specularTexture.GetAutoLoadBehavior()));
        }

        AZStd::string GradientGIComponentController::GetSpecularTextureAssetPath() const
        {
            AZStd::string path;
            Data::AssetCatalogRequestBus::BroadcastResult(
                path, &Data::AssetCatalogRequests::GetAssetPathById, m_configuration.m_specularTexture.GetId());
            return path;
        }

        void GradientGIComponentController::SetSpecularMapping(GradientGITextureMapping mapping)
        {
            m_configuration.m_specularMapping = mapping;
            PushSpecularParams();
        }

        GradientGITextureMapping GradientGIComponentController::GetSpecularMapping() const
        {
            return m_configuration.m_specularMapping;
        }

        void GradientGIComponentController::SetSpecularBlend(GradientGIBlendMode blend)
        {
            m_configuration.m_specularBlend = blend;
            PushSpecularParams();
        }

        GradientGIBlendMode GradientGIComponentController::GetSpecularBlend() const
        {
            return m_configuration.m_specularBlend;
        }

        void GradientGIComponentController::SetSpecularStrength(float strength)
        {
            m_configuration.m_specularStrength = strength;
            PushSpecularParams();
        }

        float GradientGIComponentController::GetSpecularStrength() const
        {
            return m_configuration.m_specularStrength;
        }

        // =====================================================================
        // Helpers
        // =====================================================================

        void GradientGIComponentController::UpdateColors()
        {
            if (IsActiveOwner())
            {
                m_featureProcessor->SetGradientColors(
                    m_configuration.m_lowColor,
                    m_configuration.m_midColor,
                    m_configuration.m_highColor);
            }
        }

        bool GradientGIComponentController::IsActiveOwner() const
        {
            return m_featureProcessor &&
                m_entityId.IsValid() &&
                m_featureProcessor->GetCurrentOwner() == m_entityId;
        }

        void GradientGIComponentController::OnOwnerChanged(EntityId owner)
        {
            if (owner == m_entityId)
            {
                ApplyConfiguration();
            }
        }

        void GradientGIComponentController::ApplyConfiguration()
        {
            m_featureProcessor->SetUpdateMode(
                static_cast<GradientGIFeatureProcessorInterface::UpdateMode>(m_configuration.m_updateMode));
            UpdateColors();
            m_featureProcessor->SetExposure(m_configuration.m_exposure);

            SnapResolutionForCurrentMode();
            m_featureProcessor->SetFaceResolution(m_configuration.m_faceResolution);

            PushDetailParams();
            PushSpecularParams();

            LoadDetailTexture();
            LoadSpecularTexture();

            // Replace the previous owner's bindings on tick, even when our textures are still loading.
            m_detailTextureNeedsUpdate = true;
            m_specularTextureNeedsUpdate = true;
            AZ::TickBus::Handler::BusConnect();
        }

        // =====================================================================
        // Detail Texture Loading
        // =====================================================================

        void GradientGIComponentController::LoadDetailTexture()
        {
            m_detailTextureNeedsUpdate = false;

            auto& asset = m_configuration.m_detailTexture;
            if (asset.GetId().IsValid())
            {
                // If already loaded, BusConnect triggers OnAssetReady immediately.
                Data::AssetBus::MultiHandler::BusConnect(asset.GetId());
                asset.QueueLoad();

                if (asset.IsReady())
                {
                    OnAssetReady(asset);
                }
            }
            else if (IsActiveOwner())
            {
                m_detailTextureNeedsUpdate = true;
                AZ::TickBus::Handler::BusConnect();
            }
        }

        void GradientGIComponentController::LoadSpecularTexture()
        {
            m_specularTextureNeedsUpdate = false;

            auto& asset = m_configuration.m_specularTexture;
            if (asset.GetId().IsValid())
            {
                Data::AssetBus::MultiHandler::BusConnect(asset.GetId());
                asset.QueueLoad();

                if (asset.IsReady())
                {
                    OnAssetReady(asset);
                }
            }
            else if (IsActiveOwner())
            {
                m_specularTextureNeedsUpdate = true;
                AZ::TickBus::Handler::BusConnect();
            }
        }

        void GradientGIComponentController::PushDetailToFeatureProcessor()
        {
            if (IsActiveOwner())
            {
                if (m_configuration.m_detailTexture.IsReady())
                {
                    m_featureProcessor->SetDetailTexture(m_configuration.m_detailTexture);
                }
                else
                {
                    m_featureProcessor->SetDetailTexture({});
                }
            }
        }

        void GradientGIComponentController::PushSpecularToFeatureProcessor()
        {
            if (IsActiveOwner())
            {
                if (m_configuration.m_specularTexture.IsReady())
                {
                    m_featureProcessor->SetSpecularTexture(m_configuration.m_specularTexture);
                }
                else
                {
                    m_featureProcessor->SetSpecularTexture({});
                }
            }
        }

        void GradientGIComponentController::PushDetailParams()
        {
            if (IsActiveOwner())
            {
                m_featureProcessor->SetDetailParams(
                    static_cast<uint8_t>(m_configuration.m_detailMapping),
                    static_cast<uint8_t>(m_configuration.m_detailBlend),
                    m_configuration.m_detailStrength);
            }
        }

        void GradientGIComponentController::PushSpecularParams()
        {
            if (IsActiveOwner())
            {
                m_featureProcessor->SetSpecularParams(
                    static_cast<uint8_t>(m_configuration.m_specularMapping),
                    static_cast<uint8_t>(m_configuration.m_specularBlend),
                    m_configuration.m_specularStrength);
            }
        }

        void GradientGIComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            OnAssetReloaded(asset);
        }

        void GradientGIComponentController::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            OnAssetReloaded(asset);
        }

        void GradientGIComponentController::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            const bool isDetail = (asset.GetId() == m_configuration.m_detailTexture.GetId());
            const bool isSpecular = (asset.GetId() == m_configuration.m_specularTexture.GetId());
            if (!isDetail && !isSpecular)
            {
                return;
            }

            if (isDetail)
            {
                m_configuration.m_detailTexture = asset;
                m_detailTextureNeedsUpdate = true;
            }

            if (isSpecular)
            {
                m_configuration.m_specularTexture = asset;
                m_specularTextureNeedsUpdate = true;
            }

            // Defer GPU work outside AssetBus, with a handler that deactivation can cancel.
            AZ::TickBus::Handler::BusConnect();
        }

        void GradientGIComponentController::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
        {
            AZ::TickBus::Handler::BusDisconnect();

            if (m_detailTextureNeedsUpdate)
            {
                m_detailTextureNeedsUpdate = false;
                PushDetailToFeatureProcessor();
            }

            if (m_specularTextureNeedsUpdate)
            {
                m_specularTextureNeedsUpdate = false;
                PushSpecularToFeatureProcessor();
            }
        }

    } // namespace Render
} // namespace AZ
