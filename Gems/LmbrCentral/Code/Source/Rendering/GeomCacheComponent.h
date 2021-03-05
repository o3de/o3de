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

#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>

#include <AzCore/Slice/SliceAsset.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>

#include <LmbrCentral/Rendering/GeomCacheComponentBus.h>

#include <LmbrCentral/Scripting/SpawnerComponentBus.h>

#include <IEntityRenderState.h>

namespace LmbrCentral
{
    enum class StandinType : AZ::u32
    {
        None,
        FirstFrame,
        LastFrame,
        Distance
    };

    /**
     * A Common structure for the GeometryCache and EditorGeometryCache.
     *
     * This is where most of the GeometryCache logic lives and it's made
     * available to the GeometryCacheComponent and the EditorGeometryCacheComponent.
     */
    class GeometryCacheCommon
        : public AZ::TransformNotificationBus::Handler
        , public AZ::Data::AssetBus::Handler
        , public AZ::TickBus::Handler
        , public MaterialOwnerRequestBus::Handler
        , public GeometryCacheComponentRequestBus::Handler
    {
        friend class EditorGeometryCacheCommon; //So that we can reflect private members

    public:
        AZ_TYPE_INFO(GeometryCacheCommon, "{4534C4C4-50CC-4256-83F0-85B0274A5E26}");
        AZ_CLASS_ALLOCATOR(GeometryCacheCommon, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        virtual ~GeometryCacheCommon();

        //Startup helpers
        void Init(const AZ::EntityId& entityId);
        virtual void Activate();
        virtual void Deactivate();

        // AZ::TransformNotificationBus interface implementation
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // MaterialOwnerRequestBus
        void SetMaterial(_smart_ptr<IMaterial> material) override;
        _smart_ptr<IMaterial> GetMaterial() override;

        // GeometryCacheComponentRequestBus
        void Play() override;
        void Pause() override;
        void Stop() override;

        float GetTimeRemaining() override;

        StandinType GetCurrentStandinType() override { return m_currentStandinType; }

        void SetGeomCacheAsset(const AZ::Data::AssetId& id);
        AZ::Data::Asset<AZ::Data::AssetData> GetGeomCacheAsset() { return m_geomCacheAsset; }

        void SetVisible(bool visible) override;
        bool GetVisible() override { return m_visible; }

        void SetLoop(bool loop) override;
        bool GetLoop() override { return m_loop; }

        void SetStartTime(float startTime) override;
        float GetStartTime() override { return m_startTime; }

        void SetFirstFrameStandIn(AZ::EntityId entityId) override;
        AZ::EntityId GetFirstFrameStandIn() override { return m_firstFrameStandin; }

        void SetLastFrameStandIn(AZ::EntityId entityId) override;
        AZ::EntityId GetLastFrameStandIn() override { return m_lastFrameStandin; }

        void SetStandIn(AZ::EntityId entityId) override;
        AZ::EntityId GetStandIn() override { return m_standin; }

        void SetStandInDistance(float standInDistance) override;
        float GetStandInDistance() override { return m_standinDistance; }

        void SetStreamInDistance(float streamInDistance) override;
        float GetStreamInDistance() override { return m_streamInDistance; }

        IGeomCacheRenderNode* GetGeomCacheRenderNode() override { return m_geomCacheRenderNode; }

        void ClearGeomCacheRenderNode() { m_geomCacheRenderNode = nullptr; }

    protected:
        //Reflected Members
        bool m_visible = true;
        bool m_loop = false;
        bool m_playOnStart = false;
        bool m_castShadows = true;
        bool m_useVisAreas = true;
        float m_startTime = 0.0f;
        EngineSpec m_minSpec = EngineSpec::Low;
        float m_standinDistance = 100.0f;
        float m_streamInDistance = 150.0f;
        float m_maxViewDistance = 8000.0f;
        float m_viewDistanceMultiplier = 1.0f;
        AZ::u32 m_lodDistanceRatio = 100;
        AZ::EntityId m_firstFrameStandin;
        AZ::EntityId m_lastFrameStandin;
        AZ::EntityId m_standin;
        AZ::Data::Asset<GeomCacheAsset> m_geomCacheAsset;
        AzFramework::SimpleAssetReference<MaterialAsset> m_materialOverrideAsset;

        //Unreflected members
        bool m_hidden; //< This is different from visible because this can also be controlled by the min spec being too high.
        bool m_isRegisteredWithRenderer = false; //< We keep track of this because if some params are change we want to re-register but only if already registered.
        bool m_renderingStandin = false;
        bool m_playing;
        float m_currentTime;
        StandinType m_currentStandinType;
        _smart_ptr<IMaterial> m_materialOverride;
        IGeomCacheRenderNode* m_geomCacheRenderNode = nullptr;
        _smart_ptr<IGeomCache> m_geomCache;
        AZ::EntityId m_entityId;
        AZ::Vector3 m_currentWorldPos;
        AZStd::vector<AZ::EntityId> m_currentStandinEntities;

        //Helper methods
        void OnRenderOptionsChanged();
        void OnGeomCacheAssetChanged();
        void OnMaterialOverrideChanged();
        void OnStartTimeChanged();
        virtual void OnPlayOnStartChanged() {} //Only matters at edit time
        void OnLoopChanged();
        void OnMaxViewDistanceChanged();
        void OnViewDistanceMultiplierChanged();
        void OnLODDistanceRatioChanged();
        void OnStreamInDistanceChanged();
        virtual void OnFirstFrameStandinChanged() {}
        virtual void OnLastFrameStandinChanged() {}
        virtual void OnStandinChanged() {}

        void LoadMaterialOverride();

        void RegisterRenderNode();

        void CreateGeomCache();
        void DestroyGeomCache();

        void ApplyAllRenderNodeParams();

        void ShowCurrentStandin();
        void HideCurrentStandin();
        void HideAllStandins();

        AZ::EntityId m_prevFirstFrameStandin;
        AZ::EntityId m_prevLastFrameStandin;
        AZ::EntityId m_prevStandin;
    };

    /**
     * A Component for handling Alembic geometry cache animations. 
     *
     * Most of the logic lives in GeometryCacheCommon.
     */
    class GeometryCacheComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(GeometryCacheComponent, "{B2974790-5A3B-4641-868F-6148C67830EE}", AZ::Component);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);

        GeometryCacheComponent() {};
        explicit GeometryCacheComponent(GeometryCacheCommon* common)
        {
            m_common = *common;
            m_common.ClearGeomCacheRenderNode(); //We don't want to copy the render node from the given common structure
        }
        ~GeometryCacheComponent() = default;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        //Reflected members
        GeometryCacheCommon m_common;
    };
} //namespace LmbrCentral