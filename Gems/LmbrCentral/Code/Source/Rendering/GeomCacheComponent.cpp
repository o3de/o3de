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

#include "LmbrCentral_precompiled.h"

#include "GeomCacheComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include "MathConversion.h"

#include <LmbrCentral/Rendering/MeshComponentBus.h>

namespace LmbrCentral
{
    //BehaviorContext GeometryCacheComponentNotificationBus Forwarder
    class BehaviorGeometryCacheComponentNotificationBusHandler
        : public GeometryCacheComponentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorGeometryCacheComponentNotificationBusHandler, "{8E0B4617-DD82-47D8-AA2F-3DF3E6677B4B}", AZ::SystemAllocator,
            OnPlaybackStart,
            OnPlaybackPause,
            OnPlaybackStop,
            OnStandinChanged);

        void OnPlaybackStart() override
        {
            Call(FN_OnPlaybackStart);
        }

        void OnPlaybackPause() override
        {
            Call(FN_OnPlaybackPause);
        }

        void OnPlaybackStop() override
        {
            Call(FN_OnPlaybackStop);
        }

        void OnStandinChanged(StandinType standinType) override
        {
            Call(FN_OnStandinChanged, standinType);
        }
    };

    void GeometryCacheCommon::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GeometryCacheCommon>()
                ->Version(1)
                ->Field("Visible", &GeometryCacheCommon::m_visible)
                ->Field("MinSpec", &GeometryCacheCommon::m_minSpec)
                ->Field("GeomCacheAsset", &GeometryCacheCommon::m_geomCacheAsset)
                ->Field("MaterialOverrideAsset", &GeometryCacheCommon::m_materialOverrideAsset)
                ->Field("Loop", &GeometryCacheCommon::m_loop)
                ->Field("PlayOnStart", &GeometryCacheCommon::m_playOnStart)
                ->Field("StartTime", &GeometryCacheCommon::m_startTime)
                ->Field("StreamInDistance", &GeometryCacheCommon::m_streamInDistance)
                ->Field("FirstFrameStandin", &GeometryCacheCommon::m_firstFrameStandin)
                ->Field("LastFrameStandin", &GeometryCacheCommon::m_lastFrameStandin)
                ->Field("Standin", &GeometryCacheCommon::m_standin)
                ->Field("StandinDistance", &GeometryCacheCommon::m_standinDistance)
                ->Field("MaxViewDistance", &GeometryCacheCommon::m_maxViewDistance)
                ->Field("ViewDistanceMultiplier", &GeometryCacheCommon::m_viewDistanceMultiplier)
                ->Field("LODDistanceRatio", &GeometryCacheCommon::m_lodDistanceRatio)
                ->Field("CastShadows", &GeometryCacheCommon::m_castShadows)
                ->Field("UseVisArea", &GeometryCacheCommon::m_useVisAreas)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<GeometryCacheComponentRequestBus>("GeometryCacheComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("Play", &GeometryCacheComponentRequestBus::Events::Play)
                ->Event("Pause", &GeometryCacheComponentRequestBus::Events::Pause)
                ->Event("Stop", &GeometryCacheComponentRequestBus::Events::Stop)

                ->Event("GetTimeRemaining", &GeometryCacheComponentRequestBus::Events::GetTimeRemaining)

                ->Event("SetVisible", &GeometryCacheComponentRequestBus::Events::SetVisible)
                ->Event("GetVisible", &GeometryCacheComponentRequestBus::Events::GetVisible)
                ->VirtualProperty("Visible", "GetVisible", "SetVisible")

                ->Event("SetLoop", &GeometryCacheComponentRequestBus::Events::SetLoop)
                ->Event("GetLoop", &GeometryCacheComponentRequestBus::Events::GetLoop)
                ->VirtualProperty("Loop", "GetLoop", "SetLoop")

                ->Event("SetStartTime", &GeometryCacheComponentRequestBus::Events::SetStartTime)
                ->Event("GetStartTime", &GeometryCacheComponentRequestBus::Events::GetStartTime)
                ->VirtualProperty("StartTime", "GetStartTime", "SetStartTime")

                ->Event("SetFirstFrameStandIn", &GeometryCacheComponentRequestBus::Events::SetFirstFrameStandIn)
                ->Event("GetFirstFrameStandIn", &GeometryCacheComponentRequestBus::Events::GetFirstFrameStandIn)
                ->VirtualProperty("FirstFrameStandIn", "GetFirstFrameStandIn", "SetFirstFrameStandIn")

                ->Event("SetLastFrameStandIn", &GeometryCacheComponentRequestBus::Events::SetLastFrameStandIn)
                ->Event("GetLastFrameStandIn", &GeometryCacheComponentRequestBus::Events::GetLastFrameStandIn)
                ->VirtualProperty("LastFrameStandIn", "GetLastFrameStandIn", "SetLastFrameStandIn")

                ->Event("SetStandIn", &GeometryCacheComponentRequestBus::Events::SetStandIn)
                ->Event("GetStandIn", &GeometryCacheComponentRequestBus::Events::GetStandIn)
                ->VirtualProperty("StandIn", "GetStandIn", "SetStandIn")

                ->Event("SetStandInDistance", &GeometryCacheComponentRequestBus::Events::SetStandInDistance)
                ->Event("GetStandInDistance", &GeometryCacheComponentRequestBus::Events::GetStandInDistance)
                ->VirtualProperty("StandInDistance", "GetStandInDistance", "SetStandInDistance")

                ->Event("SetStreamInDistance", &GeometryCacheComponentRequestBus::Events::SetStreamInDistance)
                ->Event("GetStreamInDistance", &GeometryCacheComponentRequestBus::Events::GetStreamInDistance)
                ->VirtualProperty("StreamInDistance", "GetStreamInDistance", "SetStreamInDistance")

                ;

            behaviorContext->EBus<GeometryCacheComponentNotificationBus>("GeometryCacheComponentNotificationBus")
                ->Handler<BehaviorGeometryCacheComponentNotificationBusHandler>()
                ;

            behaviorContext->Class<GeometryCacheComponent>()
                ->RequestBus("GeometryCacheComponentRequestBus")
                ->NotificationBus("GeometryCacheComponentNotificationBus")
                ;
        }
    }

    GeometryCacheCommon::~GeometryCacheCommon()
    {
        if (m_geomCacheRenderNode)
        {
            delete m_geomCacheRenderNode;
            m_geomCacheRenderNode = nullptr;
        }
    }

    void GeometryCacheCommon::Init(const AZ::EntityId& entityId)
    {
        m_entityId = entityId;

        if (gEnv && gEnv->p3DEngine && !m_geomCacheRenderNode)
        {
            m_geomCacheRenderNode = static_cast<IGeomCacheRenderNode*>(gEnv->p3DEngine->CreateRenderNode(eERType_GeomCache));
        }
    }

    void GeometryCacheCommon::Activate()
    {    
        m_isRegisteredWithRenderer = false;

        m_currentTime = m_startTime; //We default to m_startTime here instead of 0.0 so that animations actually start at m_startTime
        
        //If there is no set asset use the defaultgeomcache asset that is expected to exist in EngineAssets
        if (!m_geomCacheAsset.GetId().IsValid())
        {
            AZ::Data::AssetId defaultGeomCacheAssetId;
            const char* defaultGeomCacheAssetName = "engineassets/geomcaches/defaultgeomcache.cax";
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(defaultGeomCacheAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, defaultGeomCacheAssetName, AZ::Data::s_invalidAssetType, false);

            if (defaultGeomCacheAssetId.IsValid())
            {
                m_geomCacheAsset.Create(defaultGeomCacheAssetId);
            }
            else
            {
                AZ_Warning("GeometryCache", false, "Default Geometry Cache was not found");
            }
        }
        
        CreateGeomCache();

        //Hide all referenced entities by default
        //If a standin *should* be shown that will be determined on the first tick
        HideAllStandins();
        m_currentStandinType = StandinType::None;
        ShowCurrentStandin();   

        GeometryCacheComponentRequestBus::Handler::BusConnect(m_entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        AZ::TickBus::Handler::BusConnect();
        MaterialOwnerRequestBus::Handler::BusConnect(m_entityId);

        //Get initial transform and update the render node with it
        AZ::Transform world = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(world, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        OnTransformChanged(AZ::Transform::CreateIdentity(), world);

        m_playing = false;
        if (m_playOnStart)
        {
            Play();
        }
    }

    void GeometryCacheCommon::Deactivate()
    {
        HideAllStandins();

        GeometryCacheComponentRequestBus::Handler::BusDisconnect(m_entityId);
        AZ::TransformNotificationBus::Handler::BusDisconnect(m_entityId);
        AZ::TickBus::Handler::BusDisconnect();
        MaterialOwnerRequestBus::Handler::BusDisconnect(m_entityId);

        DestroyGeomCache();
    }

    void GeometryCacheCommon::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldPos = world.GetTranslation();

        if (m_geomCacheRenderNode)
        {
            Matrix34 cryMat = AZTransformToLYTransform(world);
            m_geomCacheRenderNode->SetMatrix(cryMat);

            //Re-register to update position
            RegisterRenderNode();
        }
    }

    void GeometryCacheCommon::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_geomCacheRenderNode && m_playing)
        {
            float playbackTime = m_currentTime;

            m_currentTime += deltaTime;
            playbackTime = m_currentTime;

            m_geomCacheRenderNode->SetPlaybackTime(playbackTime);
        }

        //Don't care about handling stand-in visibility if the whole
        //GeomCache shouldn't be visible
        if (m_hidden)
        {
            return;
        }

        bool standinInUse = false;

        //Distance based stand-in
        //This has the highest priority
        const Vec3 cameraPos = gEnv->p3DEngine->GetRenderingCamera().GetPosition();
        float distance = (LYVec3ToAZVec3(cameraPos) - m_currentWorldPos).GetLength();
        if (distance > m_standinDistance)
        {
            standinInUse = true;

            if (m_standin.IsValid() && m_currentStandinType != StandinType::Distance)
            {
                HideCurrentStandin();
                m_currentStandinType = StandinType::Distance;
                ShowCurrentStandin();
            }
        }
        else
        {
            //Stand-in for first frame
            if (m_currentTime == m_startTime)
            {
                standinInUse = true;

                if (m_firstFrameStandin.IsValid() && m_currentStandinType != StandinType::FirstFrame)
                {
                    HideCurrentStandin();
                    m_currentStandinType = StandinType::FirstFrame;
                    ShowCurrentStandin();
                }
            }
            //Stand-in for last frame
            else if (!m_loop && m_geomCache && m_currentTime >= m_geomCache->GetDuration())
            {
                standinInUse = true;

                if (m_lastFrameStandin.IsValid() && m_currentStandinType != StandinType::LastFrame)
                {
                    HideCurrentStandin();
                    m_currentStandinType = StandinType::LastFrame;
                    ShowCurrentStandin();
                }
            }
        }

        //If none of the other stand-ins are being rendered, remove any existing ones
        //and go back to just rendering the geom cache
        if (!standinInUse && m_currentStandinType != StandinType::None)
        {
            HideCurrentStandin();
            m_currentStandinType = StandinType::None;
            ShowCurrentStandin();
        }
    }

    void GeometryCacheCommon::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_geomCacheAsset)
        {
            m_geomCacheAsset = asset;
            m_geomCache = m_geomCacheAsset.Get()->m_geomCache;

            if (m_geomCache != nullptr)
            {
                m_geomCacheRenderNode->SetGeomCache(m_geomCache);

                //Allow this geom cache to clean up after we've properly set it on the render node
                m_geomCache->SetProcessedByRenderNode(true);

                if (m_materialOverride)
                {
                    m_geomCacheRenderNode->SetMaterial(m_materialOverride);
                }

                if (gEnv && gEnv->p3DEngine)
                {
                    gEnv->p3DEngine->RegisterEntity(m_geomCacheRenderNode);
                    m_isRegisteredWithRenderer = true;
                }

                //Apply latest transform to the render node
                AZ::Transform world = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(world, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

                OnTransformChanged(AZ::Transform::CreateIdentity(), world);
            }
        }
    }

    void GeometryCacheCommon::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_geomCacheAsset)
        {
            // We need to make sure m_geomCacheAsset is assigned with the new asset.
            // Since the new asset passed in has zero refCount. The assigning will clear the refCount of the old m_geomCacheAsset
            // and set the refCount of the new asset to 1, otherwise it will be unloaded.
            m_geomCacheAsset = asset;
            m_geomCache = m_geomCacheAsset.Get()->m_geomCache;
        }
    }

    void GeometryCacheCommon::SetMaterial(_smart_ptr<IMaterial> material)
    {
        m_materialOverride = material;
        
        if (material)
        {
            m_materialOverrideAsset.SetAssetPath(material->GetName());
        }
        else
        {
            m_materialOverrideAsset.SetAssetPath("");
        }

        DestroyGeomCache();
        CreateGeomCache();
    }

    _smart_ptr<IMaterial> GeometryCacheCommon::GetMaterial()
    {
        if (m_materialOverride)
        {
            return m_materialOverride;
        }

        if (m_geomCache == nullptr)
        {
            return nullptr;
        }

        return m_geomCache->GetMaterial();
    }

    void GeometryCacheCommon::Play()
    {
        if (!m_playing)
        {
            m_playing = true;
            GeometryCacheComponentNotificationBus::Broadcast(&GeometryCacheComponentNotificationBus::Events::OnPlaybackStart);
        }
    }

    void GeometryCacheCommon::Pause()
    {
        if (m_playing)
        {
            m_playing = false;
            GeometryCacheComponentNotificationBus::Broadcast(&GeometryCacheComponentNotificationBus::Events::OnPlaybackPause);
        }
    }

    void GeometryCacheCommon::Stop()
    {
        if (m_playing)
        {
            m_playing = false;
            m_currentTime = 0;
            if (m_geomCacheRenderNode)
            {
                m_geomCacheRenderNode->StopStreaming();
            }
            GeometryCacheComponentNotificationBus::Broadcast(&GeometryCacheComponentNotificationBus::Events::OnPlaybackStop);
        }
    }

    float GeometryCacheCommon::GetTimeRemaining()
    {
        if (m_playing)
        {
            return m_geomCache->GetDuration() - m_currentTime;
        }
        else
        {
            return -1.0f;
        }
    }

    void GeometryCacheCommon::SetGeomCacheAsset(const AZ::Data::AssetId& id)
    {
        DestroyGeomCache();
        m_geomCacheAsset.Create(id);
        CreateGeomCache();
    }

    void GeometryCacheCommon::SetVisible(bool visible)
    {
        m_visible = visible;
        OnRenderOptionsChanged();
    }

    void GeometryCacheCommon::SetLoop(bool loop)
    {
        m_loop = loop;
        OnLoopChanged();
    }

    void GeometryCacheCommon::SetStartTime(float startTime)
    {
        m_startTime = startTime;
        OnStartTimeChanged();
    }

    void GeometryCacheCommon::SetFirstFrameStandIn(AZ::EntityId entityId)
    {
        if (!entityId.IsValid())
        {
            return;
        }

        if (m_currentStandinType == StandinType::FirstFrame)
        {
            HideCurrentStandin();
        }

        m_firstFrameStandin = entityId;

        if (m_currentStandinType == StandinType::FirstFrame)
        {
            ShowCurrentStandin();
        }
    }

    void GeometryCacheCommon::SetLastFrameStandIn(AZ::EntityId entityId)
    {
        if (!entityId.IsValid())
        {
            return;
        }

        if (m_currentStandinType == StandinType::LastFrame)
        {
            HideCurrentStandin();
        }

        m_lastFrameStandin = entityId;

        if (m_currentStandinType == StandinType::LastFrame)
        {
            ShowCurrentStandin();
        }
    }

    void GeometryCacheCommon::SetStandIn(AZ::EntityId entityId)
    {
        if (!entityId.IsValid())
        {
            return;
        }

        if (m_currentStandinType == StandinType::Distance)
        {
            HideCurrentStandin();
        }

        m_standin = entityId;

        if (m_currentStandinType == StandinType::Distance)
        {
            ShowCurrentStandin();
        }
    }

    void GeometryCacheCommon::SetStandInDistance(float standInDistance)
    {
        m_standinDistance = standInDistance;
    }

    void GeometryCacheCommon::SetStreamInDistance(float streamInDistance)
    {
        m_streamInDistance = streamInDistance;
        OnStreamInDistanceChanged();
    }

    void GeometryCacheCommon::OnRenderOptionsChanged()
    {
        AZ::u32 rendFlags = 0;

        //We want to hide the GeomCache if we're rendering a stand-in
        if (m_renderingStandin)
        {
            rendFlags |= ERF_HIDDEN;
        }
        //But if the GeomCache is marked as invisible (or the min spec is too high), neither 
        //the GeomCache or any stand-ins should render
        const int configSpec = gEnv->pSystem->GetConfigSpec(true);

        m_hidden = !m_visible || (static_cast<AZ::u32>(configSpec) < static_cast<AZ::u32>(m_minSpec));
        if (m_hidden)
        {
            rendFlags |= ERF_HIDDEN;
            HideAllStandins();
        }

        if (!m_useVisAreas)
        {
            rendFlags |= ERF_OUTDOORONLY;
        }

        if (m_castShadows)
        {
            rendFlags |= ERF_HAS_CASTSHADOWMAPS;
            rendFlags |= ERF_CASTSHADOWMAPS;
        }

        rendFlags |= ERF_COMPONENT_ENTITY;
        m_geomCacheRenderNode->SetRndFlags(rendFlags);

        //Re-register to update flags
        RegisterRenderNode();
    }

    void GeometryCacheCommon::OnGeomCacheAssetChanged()
    {
        DestroyGeomCache();
        CreateGeomCache();
    }

    void GeometryCacheCommon::OnMaterialOverrideChanged()
    {
        LoadMaterialOverride();
        if (m_materialOverride)
        {
            m_geomCacheRenderNode->SetMaterial(m_materialOverride);
        }

        //On Activate this will be false and that's expected.
        //However when the asset actually loads, the material should be applied.
        //This is just for whenever the material override *changes*
        //but not necessarily for material application on startup.
        RegisterRenderNode();
    }

    void GeometryCacheCommon::OnStartTimeChanged()
    {
        if (m_geomCache)
        {
            if (m_startTime > m_geomCache->GetDuration())
            {
                m_startTime = m_geomCache->GetDuration();
            }
        }

        //If the start time has changed, restart the animation
        m_currentTime = m_startTime;
    }

    void GeometryCacheCommon::OnLoopChanged()
    {
        m_geomCacheRenderNode->SetLooping(m_loop);
    }

    void GeometryCacheCommon::OnMaxViewDistanceChanged()
    {
        m_geomCacheRenderNode->SetBaseMaxViewDistance(m_maxViewDistance);

        RegisterRenderNode();
    }

    void GeometryCacheCommon::OnViewDistanceMultiplierChanged()
    {
        m_geomCacheRenderNode->SetViewDistanceMultiplier(m_viewDistanceMultiplier);

        RegisterRenderNode();
    }

    void GeometryCacheCommon::OnLODDistanceRatioChanged()
    {
        m_geomCacheRenderNode->SetLodRatio(m_lodDistanceRatio);

        RegisterRenderNode();
    }

    void GeometryCacheCommon::OnStreamInDistanceChanged()
    {
        m_geomCacheRenderNode->SetStreamInDistance(m_streamInDistance);
    }

    void GeometryCacheCommon::LoadMaterialOverride()
    {
        const AZStd::string& materialOverridePath = m_materialOverrideAsset.GetAssetPath();
        if (!materialOverridePath.empty())
        {
            m_materialOverride = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialOverridePath.c_str());

            AZ_Warning("GeomCacheComponent", m_materialOverride != gEnv->p3DEngine->GetMaterialManager()->GetDefaultMaterial(),
                "Failed to load override material \"%s\".",
                materialOverridePath.c_str());
        }
        else
        {
            m_materialOverride = nullptr;
        }
    }

    void GeometryCacheCommon::RegisterRenderNode()
    {
        if(m_isRegisteredWithRenderer && gEnv && gEnv->p3DEngine)
        {
            gEnv->p3DEngine->RegisterEntity(m_geomCacheRenderNode);
        }
    }

    void GeometryCacheCommon::CreateGeomCache()
    {
        if (gEnv && gEnv->p3DEngine && !m_geomCacheRenderNode)
        {
            m_geomCacheRenderNode = static_cast<IGeomCacheRenderNode*>(gEnv->p3DEngine->CreateRenderNode(eERType_GeomCache));
        }

        if (m_geomCacheAsset.GetId().IsValid())
        {
            if (!AZ::Data::AssetBus::Handler::BusIsConnected())
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_geomCacheAsset.GetId());
            }

            m_geomCacheAsset.QueueLoad();
        }

        //Apply starting params to the render node
        ApplyAllRenderNodeParams();
    }

    void GeometryCacheCommon::DestroyGeomCache()
    {
        if (m_isRegisteredWithRenderer && m_geomCacheRenderNode)
        {
            m_geomCacheRenderNode->StopStreaming();
            m_isRegisteredWithRenderer = false;

            gEnv->p3DEngine->FreeRenderNodeState(m_geomCacheRenderNode);
        }

        m_geomCache = nullptr;
        m_materialOverride = nullptr;

        m_geomCacheAsset.Release();

        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void GeometryCacheCommon::ApplyAllRenderNodeParams()
    {
        OnRenderOptionsChanged();
        OnMaterialOverrideChanged();
        OnLoopChanged();
        OnMaxViewDistanceChanged();
        OnViewDistanceMultiplierChanged();
        OnLODDistanceRatioChanged();
        OnStreamInDistanceChanged();
    }

    void GeometryCacheCommon::ShowCurrentStandin()
    {
        switch (m_currentStandinType)
        {
        case StandinType::FirstFrame:
        {
            MeshComponentRequestBus::Event(m_firstFrameStandin, &MeshComponentRequestBus::Events::SetVisibility, true);
            break;
        }
        case StandinType::LastFrame:
        {
            MeshComponentRequestBus::Event(m_lastFrameStandin, &MeshComponentRequestBus::Events::SetVisibility, true);
            break;
        }
        case StandinType::Distance:
        {
            MeshComponentRequestBus::Event(m_standin, &MeshComponentRequestBus::Events::SetVisibility, true);
            break;
        }
        case StandinType::None:
        {
            //Show the geom cache by removing the ERF_HIDDEN flag
            m_renderingStandin = false;
            OnRenderOptionsChanged();
            break;
        }
        }

        GeometryCacheComponentNotificationBus::Broadcast(&GeometryCacheComponentNotificationBus::Events::OnStandinChanged, m_currentStandinType);
    }

    void GeometryCacheCommon::HideCurrentStandin()
    {
        switch (m_currentStandinType)
        {
        case StandinType::FirstFrame:
        {
            MeshComponentRequestBus::Event(m_firstFrameStandin, &MeshComponentRequestBus::Events::SetVisibility, false);
            break;
        }
        case StandinType::LastFrame:
        {
            MeshComponentRequestBus::Event(m_lastFrameStandin, &MeshComponentRequestBus::Events::SetVisibility, false);
            break;
        }
        case StandinType::Distance:
        {
            MeshComponentRequestBus::Event(m_standin, &MeshComponentRequestBus::Events::SetVisibility, false);
            break;
        }
        case StandinType::None:
        {
            //Hide the geom cache by adding the ERF_HIDDEN flag
            m_renderingStandin = true;
            OnRenderOptionsChanged();
            break;
        }
        }
    }

    void GeometryCacheCommon::HideAllStandins()
    {
        if (m_firstFrameStandin.IsValid())
        {
            MeshComponentRequestBus::Event(m_firstFrameStandin, &MeshComponentRequestBus::Events::SetVisibility, false);
        }
        if (m_lastFrameStandin.IsValid())
        {
            MeshComponentRequestBus::Event(m_lastFrameStandin, &MeshComponentRequestBus::Events::SetVisibility, false);
        }
        if (m_standin.IsValid())
        {
            MeshComponentRequestBus::Event(m_standin, &MeshComponentRequestBus::Events::SetVisibility, false);
        }
    }

    void GeometryCacheComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("GeomCacheService", 0x3d2bc48c));
    }

    void GeometryCacheComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void GeometryCacheComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GeometryCacheComponent, AZ::Component>()
                ->Version(1)
                ->Field("Common", &GeometryCacheComponent::m_common)
                ;
        }

        GeometryCacheCommon::Reflect(context);
    }

    void GeometryCacheComponent::Init()
    {
        m_common.Init(GetEntityId());
    }

    void GeometryCacheComponent::Activate()
    {
        m_common.Activate();
    }

    void GeometryCacheComponent::Deactivate()
    {
        m_common.Deactivate();
    }
} //namespace LmbrCentral
