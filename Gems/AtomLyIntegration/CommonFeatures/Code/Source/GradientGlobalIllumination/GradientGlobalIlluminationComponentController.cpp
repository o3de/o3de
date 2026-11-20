/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/GradientGlobalIllumination/GradientGlobalIlluminationLogic.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Utils/Utils.h>
#include <AtomLyIntegration/CommonFeatures/GradientGlobalIllumination/GradientGlobalIlluminationComponentConstants.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientGlobalIllumination/GradientGlobalIlluminationComponentController.h>

namespace AZ::Render
{
    void GradientGlobalIlluminationComponentController::Reflect(ReflectContext* context)
    {
        GradientGlobalIlluminationComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<GradientGlobalIlluminationComponentController>()->Version(1)->Field(
                "Configuration",
                &GradientGlobalIlluminationComponentController::m_configuration);
        }

        if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Enum<static_cast<uint32_t>(GradientGlobalIlluminationUpdateMode::Static)>("GradientGlobalIlluminationUpdateMode_Static")
                ->Enum<static_cast<uint32_t>(GradientGlobalIlluminationUpdateMode::Dynamic)>("GradientGlobalIlluminationUpdateMode_Dynamic")
                ->Enum<static_cast<uint32_t>(GradientGlobalIlluminationTextureMapping::Tiled)>("GradientGlobalIlluminationTextureMapping_Tiled")
                ->Enum<static_cast<uint32_t>(GradientGlobalIlluminationTextureMapping::Stretched)>("GradientGlobalIlluminationTextureMapping_Stretched")
                ->Enum<static_cast<uint32_t>(GradientGlobalIlluminationTextureMapping::Cube)>("GradientGlobalIlluminationTextureMapping_Cube")
                ->Enum<static_cast<uint32_t>(GradientGlobalIlluminationBlendMode::Multiply)>("GradientGlobalIlluminationBlendMode_Multiply")
                ->Enum<static_cast<uint32_t>(GradientGlobalIlluminationBlendMode::Add)>("GradientGlobalIlluminationBlendMode_Add")
                ->Enum<static_cast<uint32_t>(GradientGlobalIlluminationBlendMode::Screen)>("GradientGlobalIlluminationBlendMode_Screen")
                ->Enum<static_cast<uint32_t>(GradientGlobalIlluminationBlendMode::Overlay)>("GradientGlobalIlluminationBlendMode_Overlay")
                ->Enum<static_cast<uint32_t>(GradientGlobalIlluminationBlendMode::Replace)>("GradientGlobalIlluminationBlendMode_Replace");

            behaviorContext->EBus<GradientGlobalIlluminationComponentRequestBus>("GradientGlobalIlluminationComponentRequestBus", "GradientGIComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Event("SetLowColor", &GradientGlobalIlluminationComponentRequestBus::Events::SetLowColor)
                ->Event("GetLowColor", &GradientGlobalIlluminationComponentRequestBus::Events::GetLowColor)
                ->Event("SetMidColor", &GradientGlobalIlluminationComponentRequestBus::Events::SetMidColor)
                ->Event("GetMidColor", &GradientGlobalIlluminationComponentRequestBus::Events::GetMidColor)
                ->Event("SetHighColor", &GradientGlobalIlluminationComponentRequestBus::Events::SetHighColor)
                ->Event("GetHighColor", &GradientGlobalIlluminationComponentRequestBus::Events::GetHighColor)
                ->Event("SetGradientColors", &GradientGlobalIlluminationComponentRequestBus::Events::SetGradientColors)
                ->Event("SetExposure", &GradientGlobalIlluminationComponentRequestBus::Events::SetExposure)
                ->Event("GetExposure", &GradientGlobalIlluminationComponentRequestBus::Events::GetExposure)
                ->Event("SetFaceResolution", &GradientGlobalIlluminationComponentRequestBus::Events::SetFaceResolution)
                ->Event("GetFaceResolution", &GradientGlobalIlluminationComponentRequestBus::Events::GetFaceResolution)
                ->Event("SetUpdateMode", &GradientGlobalIlluminationComponentRequestBus::Events::SetUpdateMode)
                ->Event("GetUpdateMode", &GradientGlobalIlluminationComponentRequestBus::Events::GetUpdateMode)
                // Script Canvas exposes AssetId accessors. Asset and path accessors remain C++ only.
                ->Event("SetDetailTextureAssetId", &GradientGlobalIlluminationComponentRequestBus::Events::SetDetailTextureAssetId)
                ->Event("GetDetailTextureAssetId", &GradientGlobalIlluminationComponentRequestBus::Events::GetDetailTextureAssetId)
                ->Event("SetDetailMapping", &GradientGlobalIlluminationComponentRequestBus::Events::SetDetailMapping)
                ->Event("GetDetailMapping", &GradientGlobalIlluminationComponentRequestBus::Events::GetDetailMapping)
                ->Event("SetDetailBlend", &GradientGlobalIlluminationComponentRequestBus::Events::SetDetailBlend)
                ->Event("GetDetailBlend", &GradientGlobalIlluminationComponentRequestBus::Events::GetDetailBlend)
                ->Event("SetDetailStrength", &GradientGlobalIlluminationComponentRequestBus::Events::SetDetailStrength)
                ->Event("GetDetailStrength", &GradientGlobalIlluminationComponentRequestBus::Events::GetDetailStrength)
                ->Event("SetSpecularTextureAssetId", &GradientGlobalIlluminationComponentRequestBus::Events::SetSpecularTextureAssetId)
                ->Event("GetSpecularTextureAssetId", &GradientGlobalIlluminationComponentRequestBus::Events::GetSpecularTextureAssetId)
                ->Event("SetSpecularMapping", &GradientGlobalIlluminationComponentRequestBus::Events::SetSpecularMapping)
                ->Event("GetSpecularMapping", &GradientGlobalIlluminationComponentRequestBus::Events::GetSpecularMapping)
                ->Event("SetSpecularBlend", &GradientGlobalIlluminationComponentRequestBus::Events::SetSpecularBlend)
                ->Event("GetSpecularBlend", &GradientGlobalIlluminationComponentRequestBus::Events::GetSpecularBlend)
                ->Event("SetSpecularStrength", &GradientGlobalIlluminationComponentRequestBus::Events::SetSpecularStrength)
                ->Event("GetSpecularStrength", &GradientGlobalIlluminationComponentRequestBus::Events::GetSpecularStrength)
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

    void GradientGlobalIlluminationComponentController::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GradientGlobalIlluminationService"));
    }

    void GradientGlobalIlluminationComponentController::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GradientGlobalIlluminationService"));
        // These components share the scene IBL slots and cannot coexist on one entity.
        incompatible.push_back(AZ_CRC_CE("ImageBasedLightService"));
    }

    GradientGlobalIlluminationComponentController::GradientGlobalIlluminationComponentController(const GradientGlobalIlluminationComponentConfig& config)
        : m_configuration(config)
    {
    }

    void GradientGlobalIlluminationComponentController::Activate(EntityId entityId)
    {
        m_entityId = entityId;

        m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<GradientGlobalIlluminationFeatureProcessorInterface>(m_entityId);
        AZ_Error(
            "GradientGlobalIlluminationComponentController",
            m_featureProcessor,
            "Unable to find GradientGlobalIlluminationFeatureProcessorInterface on this entity's scene.");

        if (m_featureProcessor)
        {
            GradientGlobalIlluminationComponentRequestBus::Handler::BusConnect(m_entityId);
            m_featureProcessor->ConnectToOwnerChangedEvent(m_ownerChangedHandler);
            m_featureProcessor->Enable(m_entityId);
        }
    }

    void GradientGlobalIlluminationComponentController::Deactivate()
    {
        m_ownerChangedHandler.Disconnect();
        AZ::TickBus::Handler::BusDisconnect();
        GradientGlobalIlluminationComponentRequestBus::Handler::BusDisconnect();
        Data::AssetBus::MultiHandler::BusDisconnect();

        m_detailTextureNeedsUpdate = false;
        m_specularTextureNeedsUpdate = false;

        if (m_featureProcessor)
        {
            m_featureProcessor->Disable(m_entityId);
            m_featureProcessor = nullptr;
        }

        m_entityId = EntityId(EntityId::InvalidEntityId);
    }

    void GradientGlobalIlluminationComponentController::SetConfiguration(const GradientGlobalIlluminationComponentConfig& config)
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

    const GradientGlobalIlluminationComponentConfig& GradientGlobalIlluminationComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void GradientGlobalIlluminationComponentController::SetLowColor(const Color& color)
    {
        m_configuration.m_lowColor = color;
        UpdateColors();
    }

    Color GradientGlobalIlluminationComponentController::GetLowColor() const
    {
        return m_configuration.m_lowColor;
    }

    void GradientGlobalIlluminationComponentController::SetMidColor(const Color& color)
    {
        m_configuration.m_midColor = color;
        UpdateColors();
    }

    Color GradientGlobalIlluminationComponentController::GetMidColor() const
    {
        return m_configuration.m_midColor;
    }

    void GradientGlobalIlluminationComponentController::SetHighColor(const Color& color)
    {
        m_configuration.m_highColor = color;
        UpdateColors();
    }

    Color GradientGlobalIlluminationComponentController::GetHighColor() const
    {
        return m_configuration.m_highColor;
    }

    void GradientGlobalIlluminationComponentController::SetGradientColors(const Color& low, const Color& mid, const Color& high)
    {
        m_configuration.m_lowColor = low;
        m_configuration.m_midColor = mid;
        m_configuration.m_highColor = high;
        UpdateColors();
    }

    void GradientGlobalIlluminationComponentController::SetExposure(float exposure)
    {
        m_configuration.m_exposure = exposure;

        if (IsActiveOwner())
        {
            m_featureProcessor->SetExposure(exposure);
        }
    }

    float GradientGlobalIlluminationComponentController::GetExposure() const
    {
        return m_configuration.m_exposure;
    }

    void GradientGlobalIlluminationComponentController::SetFaceResolution(float resolution)
    {
        m_configuration.m_faceResolution = GradientGlobalIllumination::RoundAndClampFaceResolution(resolution);
        SnapResolutionForCurrentMode();

        if (IsActiveOwner())
        {
            m_featureProcessor->SetFaceResolution(m_configuration.m_faceResolution);
        }
    }

    void GradientGlobalIlluminationComponentController::SnapResolutionForCurrentMode()
    {
        // Store the snapped size so the inspector and scripting getter agree with CPU output.
        if (m_configuration.m_updateMode == GradientGlobalIlluminationUpdateMode::Static ||
            (IsActiveOwner() && m_featureProcessor->GetUpdateMode() == GradientGlobalIlluminationFeatureProcessorInterface::UpdateMode::Static))
        {
            m_configuration.m_faceResolution = GradientGlobalIllumination::SnapCpuFaceResolutionToPow2(m_configuration.m_faceResolution);
        }
    }

    float GradientGlobalIlluminationComponentController::GetFaceResolution() const
    {
        return static_cast<float>(m_configuration.m_faceResolution);
    }

    void GradientGlobalIlluminationComponentController::SetUpdateMode(GradientGlobalIlluminationUpdateMode mode)
    {
        using UpdateMode = GradientGlobalIlluminationFeatureProcessorInterface::UpdateMode;
        const UpdateMode requested = GradientGlobalIllumination::ResolveUpdateMode(static_cast<UpdateMode>(mode), true);
        m_configuration.m_updateMode = static_cast<GradientGlobalIlluminationUpdateMode>(requested);

        if (IsActiveOwner())
        {
            m_featureProcessor->SetUpdateMode(requested);
        }

        SnapResolutionForCurrentMode();

        if (IsActiveOwner())
        {
            m_featureProcessor->SetFaceResolution(m_configuration.m_faceResolution);
        }
    }

    GradientGlobalIlluminationUpdateMode GradientGlobalIlluminationComponentController::GetUpdateMode() const
    {
        return m_configuration.m_updateMode;
    }

    void GradientGlobalIlluminationComponentController::SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture)
    {
        m_detailTextureNeedsUpdate = false;

        if (m_configuration.m_detailTexture.GetId() != m_configuration.m_specularTexture.GetId())
        {
            Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_detailTexture.GetId());
        }

        m_configuration.m_detailTexture = texture;
        LoadDetailTexture();
    }

    Data::Asset<RPI::StreamingImageAsset> GradientGlobalIlluminationComponentController::GetDetailTexture() const
    {
        return m_configuration.m_detailTexture;
    }

    void GradientGlobalIlluminationComponentController::SetDetailTextureAssetId(const Data::AssetId& id)
    {
        SetDetailTexture(GetAssetFromId<RPI::StreamingImageAsset>(id, m_configuration.m_detailTexture.GetAutoLoadBehavior()));
    }

    Data::AssetId GradientGlobalIlluminationComponentController::GetDetailTextureAssetId() const
    {
        return m_configuration.m_detailTexture.GetId();
    }

    void GradientGlobalIlluminationComponentController::SetDetailTextureAssetPath(const AZStd::string& path)
    {
        SetDetailTexture(
            GetAssetFromPath<RPI::StreamingImageAsset>(
                GradientGlobalIllumination::NormalizeAssetPath(path),
                m_configuration.m_detailTexture.GetAutoLoadBehavior()));
    }

    AZStd::string GradientGlobalIlluminationComponentController::GetDetailTextureAssetPath() const
    {
        AZStd::string path;
        Data::AssetCatalogRequestBus::BroadcastResult(
            path,
            &Data::AssetCatalogRequests::GetAssetPathById,
            m_configuration.m_detailTexture.GetId());
        return path;
    }

    void GradientGlobalIlluminationComponentController::SetDetailMapping(GradientGlobalIlluminationTextureMapping mapping)
    {
        m_configuration.m_detailMapping = mapping;
        PushDetailParams();
    }

    GradientGlobalIlluminationTextureMapping GradientGlobalIlluminationComponentController::GetDetailMapping() const
    {
        return m_configuration.m_detailMapping;
    }

    void GradientGlobalIlluminationComponentController::SetDetailBlend(GradientGlobalIlluminationBlendMode blend)
    {
        m_configuration.m_detailBlend = blend;
        PushDetailParams();
    }

    GradientGlobalIlluminationBlendMode GradientGlobalIlluminationComponentController::GetDetailBlend() const
    {
        return m_configuration.m_detailBlend;
    }

    void GradientGlobalIlluminationComponentController::SetDetailStrength(float strength)
    {
        m_configuration.m_detailStrength = strength;
        PushDetailParams();
    }

    float GradientGlobalIlluminationComponentController::GetDetailStrength() const
    {
        return m_configuration.m_detailStrength;
    }

    void GradientGlobalIlluminationComponentController::SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture)
    {
        m_specularTextureNeedsUpdate = false;

        if (m_configuration.m_specularTexture.GetId() != m_configuration.m_detailTexture.GetId())
        {
            Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_specularTexture.GetId());
        }

        m_configuration.m_specularTexture = texture;
        LoadSpecularTexture();
    }

    Data::Asset<RPI::StreamingImageAsset> GradientGlobalIlluminationComponentController::GetSpecularTexture() const
    {
        return m_configuration.m_specularTexture;
    }

    void GradientGlobalIlluminationComponentController::SetSpecularTextureAssetId(const Data::AssetId& id)
    {
        SetSpecularTexture(GetAssetFromId<RPI::StreamingImageAsset>(id, m_configuration.m_specularTexture.GetAutoLoadBehavior()));
    }

    Data::AssetId GradientGlobalIlluminationComponentController::GetSpecularTextureAssetId() const
    {
        return m_configuration.m_specularTexture.GetId();
    }

    void GradientGlobalIlluminationComponentController::SetSpecularTextureAssetPath(const AZStd::string& path)
    {
        SetSpecularTexture(
            GetAssetFromPath<RPI::StreamingImageAsset>(
                GradientGlobalIllumination::NormalizeAssetPath(path),
                m_configuration.m_specularTexture.GetAutoLoadBehavior()));
    }

    AZStd::string GradientGlobalIlluminationComponentController::GetSpecularTextureAssetPath() const
    {
        AZStd::string path;
        Data::AssetCatalogRequestBus::BroadcastResult(
            path,
            &Data::AssetCatalogRequests::GetAssetPathById,
            m_configuration.m_specularTexture.GetId());
        return path;
    }

    void GradientGlobalIlluminationComponentController::SetSpecularMapping(GradientGlobalIlluminationTextureMapping mapping)
    {
        m_configuration.m_specularMapping = mapping;
        PushSpecularParams();
    }

    GradientGlobalIlluminationTextureMapping GradientGlobalIlluminationComponentController::GetSpecularMapping() const
    {
        return m_configuration.m_specularMapping;
    }

    void GradientGlobalIlluminationComponentController::SetSpecularBlend(GradientGlobalIlluminationBlendMode blend)
    {
        m_configuration.m_specularBlend = blend;
        PushSpecularParams();
    }

    GradientGlobalIlluminationBlendMode GradientGlobalIlluminationComponentController::GetSpecularBlend() const
    {
        return m_configuration.m_specularBlend;
    }

    void GradientGlobalIlluminationComponentController::SetSpecularStrength(float strength)
    {
        m_configuration.m_specularStrength = strength;
        PushSpecularParams();
    }

    float GradientGlobalIlluminationComponentController::GetSpecularStrength() const
    {
        return m_configuration.m_specularStrength;
    }

    void GradientGlobalIlluminationComponentController::UpdateColors()
    {
        if (IsActiveOwner())
        {
            m_featureProcessor->SetGradientColors(m_configuration.m_lowColor, m_configuration.m_midColor, m_configuration.m_highColor);
        }
    }

    bool GradientGlobalIlluminationComponentController::IsActiveOwner() const
    {
        return m_featureProcessor && m_entityId.IsValid() && m_featureProcessor->GetCurrentOwner() == m_entityId;
    }

    void GradientGlobalIlluminationComponentController::OnOwnerChanged(EntityId owner)
    {
        if (owner == m_entityId)
        {
            ApplyConfiguration();
        }
    }

    void GradientGlobalIlluminationComponentController::ApplyConfiguration()
    {
        SetUpdateMode(m_configuration.m_updateMode);
        UpdateColors();
        m_featureProcessor->SetExposure(m_configuration.m_exposure);

        PushDetailParams();
        PushSpecularParams();

        LoadDetailTexture();
        LoadSpecularTexture();

        // Clear the preceding owner's bindings on tick, including when the new textures are not ready.
        m_detailTextureNeedsUpdate = true;
        m_specularTextureNeedsUpdate = true;
        AZ::TickBus::Handler::BusConnect();
    }

    void GradientGlobalIlluminationComponentController::LoadDetailTexture()
    {
        m_detailTextureNeedsUpdate = false;

        auto& asset = m_configuration.m_detailTexture;
        if (asset.GetId().IsValid())
        {
            // Connecting a ready asset can notify synchronously. An existing shared subscription may not notify again.
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

    void GradientGlobalIlluminationComponentController::LoadSpecularTexture()
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

    void GradientGlobalIlluminationComponentController::PushDetailToFeatureProcessor()
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

    void GradientGlobalIlluminationComponentController::PushSpecularToFeatureProcessor()
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

    void GradientGlobalIlluminationComponentController::PushDetailParams()
    {
        if (IsActiveOwner())
        {
            m_featureProcessor->SetDetailParams(
                static_cast<uint8_t>(m_configuration.m_detailMapping),
                static_cast<uint8_t>(m_configuration.m_detailBlend),
                m_configuration.m_detailStrength);
        }
    }

    void GradientGlobalIlluminationComponentController::PushSpecularParams()
    {
        if (IsActiveOwner())
        {
            m_featureProcessor->SetSpecularParams(
                static_cast<uint8_t>(m_configuration.m_specularMapping),
                static_cast<uint8_t>(m_configuration.m_specularBlend),
                m_configuration.m_specularStrength);
        }
    }

    void GradientGlobalIlluminationComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
    {
        OnAssetReloaded(asset);
    }

    void GradientGlobalIlluminationComponentController::OnAssetError(Data::Asset<Data::AssetData> asset)
    {
        OnAssetReloaded(asset);
    }

    void GradientGlobalIlluminationComponentController::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
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

        // Create images after AssetBus dispatch to avoid its mutex blocking the copy queue. Deactivate cancels this work.
        AZ::TickBus::Handler::BusConnect();
    }

    void GradientGlobalIlluminationComponentController::OnTick(float, AZ::ScriptTimePoint)
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
} // namespace AZ::Render
