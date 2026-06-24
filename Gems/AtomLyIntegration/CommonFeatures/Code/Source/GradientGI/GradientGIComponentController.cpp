/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientGI/GradientGIComponentController.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConstants.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/math.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Utils/Utils.h>

namespace
{
    // Normalise a user- or script-supplied asset path to the catalog's product-path convention:
    // unify separators to forward slashes and strip any leading slashes so the result is
    // catalog-relative. This lets a manually typed string path and a script-variable path resolve
    // through the same GetAssetIdByPath lookup regardless of how they were entered.
    AZStd::string NormalizeAssetPath(AZStd::string_view rawPath)
    {
        AZStd::string path(rawPath);
        AZStd::replace(path.begin(), path.end(), '\\', '/');
        const size_t firstReal = path.find_first_not_of('/');
        path.erase(0, (firstReal == AZStd::string::npos) ? path.size() : firstReal);
        return path;
    }
}

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
                serializeContext->Class<GradientGIComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &GradientGIComponentController::m_configuration)
                    ;
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
                    ->Event("SetLowColor",    &GradientGIComponentRequestBus::Events::SetLowColor)
                    ->Event("GetLowColor",    &GradientGIComponentRequestBus::Events::GetLowColor)
                    ->Event("SetMidColor",    &GradientGIComponentRequestBus::Events::SetMidColor)
                    ->Event("GetMidColor",    &GradientGIComponentRequestBus::Events::GetMidColor)
                    ->Event("SetHighColor",   &GradientGIComponentRequestBus::Events::SetHighColor)
                    ->Event("GetHighColor",   &GradientGIComponentRequestBus::Events::GetHighColor)
                    // Set all three gradient layers at once
                    ->Event("SetGradientColors", &GradientGIComponentRequestBus::Events::SetGradientColors)
                    // Exposure
                    ->Event("SetExposure",    &GradientGIComponentRequestBus::Events::SetExposure)
                    ->Event("GetExposure",    &GradientGIComponentRequestBus::Events::GetExposure)
                    // Resolution
                    ->Event("SetFaceResolution", &GradientGIComponentRequestBus::Events::SetFaceResolution)
                    ->Event("GetFaceResolution", &GradientGIComponentRequestBus::Events::GetFaceResolution)
                    // Update mode (Static / Dynamic)
                    ->Event("SetUpdateMode",  &GradientGIComponentRequestBus::Events::SetUpdateMode)
                    ->Event("GetUpdateMode",  &GradientGIComponentRequestBus::Events::GetUpdateMode)
                    // Detail texture layer (scripting uses AssetId / AssetPath, not Asset<>)
                    ->Event("SetDetailTextureAssetId",   &GradientGIComponentRequestBus::Events::SetDetailTextureAssetId)
                    ->Event("GetDetailTextureAssetId",   &GradientGIComponentRequestBus::Events::GetDetailTextureAssetId)
                    ->Event("SetDetailTextureAssetPath", &GradientGIComponentRequestBus::Events::SetDetailTextureAssetPath)
                    ->Event("GetDetailTextureAssetPath", &GradientGIComponentRequestBus::Events::GetDetailTextureAssetPath)
                    ->Event("SetDetailMapping",  &GradientGIComponentRequestBus::Events::SetDetailMapping)
                    ->Event("GetDetailMapping",  &GradientGIComponentRequestBus::Events::GetDetailMapping)
                    ->Event("SetDetailBlend",    &GradientGIComponentRequestBus::Events::SetDetailBlend)
                    ->Event("GetDetailBlend",    &GradientGIComponentRequestBus::Events::GetDetailBlend)
                    ->Event("SetDetailStrength", &GradientGIComponentRequestBus::Events::SetDetailStrength)
                    ->Event("GetDetailStrength", &GradientGIComponentRequestBus::Events::GetDetailStrength)
                    // Specular texture layer (scripting uses AssetId / AssetPath, not Asset<>)
                    ->Event("SetSpecularTextureAssetId",   &GradientGIComponentRequestBus::Events::SetSpecularTextureAssetId)
                    ->Event("GetSpecularTextureAssetId",   &GradientGIComponentRequestBus::Events::GetSpecularTextureAssetId)
                    ->Event("SetSpecularTextureAssetPath", &GradientGIComponentRequestBus::Events::SetSpecularTextureAssetPath)
                    ->Event("GetSpecularTextureAssetPath", &GradientGIComponentRequestBus::Events::GetSpecularTextureAssetPath)
                    ->Event("SetSpecularMapping",  &GradientGIComponentRequestBus::Events::SetSpecularMapping)
                    ->Event("GetSpecularMapping",  &GradientGIComponentRequestBus::Events::GetSpecularMapping)
                    ->Event("SetSpecularBlend",    &GradientGIComponentRequestBus::Events::SetSpecularBlend)
                    ->Event("GetSpecularBlend",    &GradientGIComponentRequestBus::Events::GetSpecularBlend)
                    ->Event("SetSpecularStrength", &GradientGIComponentRequestBus::Events::SetSpecularStrength)
                    ->Event("GetSpecularStrength", &GradientGIComponentRequestBus::Events::GetSpecularStrength)
                    // Virtual properties for Script Canvas property nodes
                    ->VirtualProperty("LowColor",       "GetLowColor",       "SetLowColor")
                    ->VirtualProperty("MidColor",       "GetMidColor",       "SetMidColor")
                    ->VirtualProperty("HighColor",      "GetHighColor",      "SetHighColor")
                    ->VirtualProperty("Exposure",       "GetExposure",       "SetExposure")
                    ->VirtualProperty("FaceResolution", "GetFaceResolution", "SetFaceResolution")
                    ->VirtualProperty("UpdateMode",     "GetUpdateMode",     "SetUpdateMode")
                    ->VirtualProperty("DetailTextureAssetId",   "GetDetailTextureAssetId",   "SetDetailTextureAssetId")
                    ->VirtualProperty("DetailTextureAssetPath", "GetDetailTextureAssetPath", "SetDetailTextureAssetPath")
                    ->VirtualProperty("DetailMapping",  "GetDetailMapping",  "SetDetailMapping")
                    ->VirtualProperty("DetailBlend",    "GetDetailBlend",    "SetDetailBlend")
                    ->VirtualProperty("DetailStrength", "GetDetailStrength", "SetDetailStrength")
                    ->VirtualProperty("SpecularTextureAssetId",   "GetSpecularTextureAssetId",   "SetSpecularTextureAssetId")
                    ->VirtualProperty("SpecularTextureAssetPath", "GetSpecularTextureAssetPath", "SetSpecularTextureAssetPath")
                    ->VirtualProperty("SpecularMapping",  "GetSpecularMapping",  "SetSpecularMapping")
                    ->VirtualProperty("SpecularBlend",    "GetSpecularBlend",    "SetSpecularBlend")
                    ->VirtualProperty("SpecularStrength", "GetSpecularStrength", "SetSpecularStrength")
                    ;
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

            m_featureProcessor =
                RPI::Scene::GetFeatureProcessorForEntity<GradientGIFeatureProcessorInterface>(m_entityId);
            AZ_Error("GradientGIComponentController", m_featureProcessor,
                "Unable to find GradientGIFeatureProcessorInterface on this entity's scene.");

            if (m_featureProcessor)
            {
                // Push all initial configuration into the FP.
                m_featureProcessor->SetUpdateMode(
                    static_cast<GradientGIFeatureProcessorInterface::UpdateMode>(m_configuration.m_updateMode));
                UpdateColors();
                m_featureProcessor->SetExposure(m_configuration.m_exposure);
                m_featureProcessor->SetFaceResolution(m_configuration.m_faceResolution);

                // Texture layers: push params now; the textures load asynchronously.
                m_featureProcessor->SetDetailParams(
                    static_cast<uint8_t>(m_configuration.m_detailMapping),
                    static_cast<uint8_t>(m_configuration.m_detailBlend),
                    m_configuration.m_detailStrength);
                m_featureProcessor->SetSpecularParams(
                    static_cast<uint8_t>(m_configuration.m_specularMapping),
                    static_cast<uint8_t>(m_configuration.m_specularBlend),
                    m_configuration.m_specularStrength);
                LoadDetailTexture();
                LoadSpecularTexture();

                GradientGIComponentRequestBus::Handler::BusConnect(m_entityId);
            }
        }

        void GradientGIComponentController::Deactivate()
        {
            GradientGIComponentRequestBus::Handler::BusDisconnect();
            Data::AssetBus::MultiHandler::BusDisconnect();

            if (m_featureProcessor)
            {
                m_featureProcessor->Reset();
                m_featureProcessor = nullptr;
            }

            m_entityId = EntityId(EntityId::InvalidEntityId);
        }

        void GradientGIComponentController::SetConfiguration(const GradientGIComponentConfig& config)
        {
            m_configuration = config;
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

        void GradientGIComponentController::SetGradientColors(
            const Color& low, const Color& mid, const Color& high)
        {
            m_configuration.m_lowColor  = low;
            m_configuration.m_midColor  = mid;
            m_configuration.m_highColor = high;
            UpdateColors();
        }

        void GradientGIComponentController::SetExposure(float exposure)
        {
            m_configuration.m_exposure = exposure;
            if (m_featureProcessor)
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
            // Script Canvas hands us a Number (float). Round to the nearest whole pixel, then clamp
            // into the supported range before narrowing to the unsigned config field.
            const int rounded = static_cast<int>(AZStd::lround(resolution));
            m_configuration.m_faceResolution = static_cast<uint32_t>(AZStd::clamp(rounded, 4, 256));
            if (m_featureProcessor)
            {
                m_featureProcessor->SetFaceResolution(m_configuration.m_faceResolution);
            }
        }

        float GradientGIComponentController::GetFaceResolution() const
        {
            return static_cast<float>(m_configuration.m_faceResolution);
        }

        void GradientGIComponentController::SetUpdateMode(GradientGIUpdateMode mode)
        {
            m_configuration.m_updateMode = mode;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetUpdateMode(
                    static_cast<GradientGIFeatureProcessorInterface::UpdateMode>(mode));
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
            Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_detailTexture.GetId());
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
            SetDetailTexture(GetAssetFromPath<RPI::StreamingImageAsset>(
                NormalizeAssetPath(path), m_configuration.m_detailTexture.GetAutoLoadBehavior()));
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
            Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_specularTexture.GetId());
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
            SetSpecularTexture(GetAssetFromPath<RPI::StreamingImageAsset>(
                NormalizeAssetPath(path), m_configuration.m_specularTexture.GetAutoLoadBehavior()));
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
            if (m_featureProcessor)
            {
                m_featureProcessor->SetGradientColors(
                    m_configuration.m_lowColor,
                    m_configuration.m_midColor,
                    m_configuration.m_highColor);
            }
        }

        // =====================================================================
        // Detail Texture Loading
        // =====================================================================

        void GradientGIComponentController::LoadDetailTexture()
        {
            // AssetBus is disconnected wholesale in Deactivate (which always precedes a fresh
            // Activate via the editor adapter), so we only connect here.
            auto& asset = m_configuration.m_detailTexture;
            if (asset.GetId().IsValid())
            {
                // If already loaded, BusConnect triggers OnAssetReady immediately.
                Data::AssetBus::MultiHandler::BusConnect(asset.GetId());
                asset.QueueLoad();
            }
            else if (m_featureProcessor)
            {
                m_featureProcessor->SetDetailTexture(asset); // clear the layer
            }
        }

        void GradientGIComponentController::LoadSpecularTexture()
        {
            auto& asset = m_configuration.m_specularTexture;
            if (asset.GetId().IsValid())
            {
                Data::AssetBus::MultiHandler::BusConnect(asset.GetId());
                asset.QueueLoad();
            }
            else if (m_featureProcessor)
            {
                m_featureProcessor->SetSpecularTexture(asset); // clear the layer
            }
        }

        void GradientGIComponentController::PushDetailToFeatureProcessor()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->SetDetailTexture(m_configuration.m_detailTexture);
            }
        }

        void GradientGIComponentController::PushSpecularToFeatureProcessor()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->SetSpecularTexture(m_configuration.m_specularTexture);
            }
        }

        void GradientGIComponentController::PushDetailParams()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->SetDetailParams(
                    static_cast<uint8_t>(m_configuration.m_detailMapping),
                    static_cast<uint8_t>(m_configuration.m_detailBlend),
                    m_configuration.m_detailStrength);
            }
        }

        void GradientGIComponentController::PushSpecularParams()
        {
            if (m_featureProcessor)
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
            const bool isDetail   = (asset.GetId() == m_configuration.m_detailTexture.GetId());
            const bool isSpecular = (asset.GetId() == m_configuration.m_specularTexture.GetId());
            if (!isDetail && !isSpecular)
            {
                return;
            }

            // Defer to the next tick: instantiating a StreamingImage from within an AssetBus
            // callback can deadlock the copy queue (see ImageBasedLightComponentController).
            auto postTick = [this, asset, isDetail, isSpecular]()
            {
                if (isDetail && asset.GetId() == m_configuration.m_detailTexture.GetId())
                {
                    m_configuration.m_detailTexture = asset;
                    PushDetailToFeatureProcessor();
                }
                if (isSpecular && asset.GetId() == m_configuration.m_specularTexture.GetId())
                {
                    m_configuration.m_specularTexture = asset;
                    PushSpecularToFeatureProcessor();
                }
            };
            AZ::TickBus::QueueFunction(AZStd::move(postTick));
        }

    } // namespace Render
} // namespace AZ
