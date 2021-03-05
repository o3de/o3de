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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include "GeomCacheComponent.h"

namespace LmbrCentral
{
    /**
     * Editor extensions for the GeometryCacheCommon
     *
     * Some parameters of the GeometryCache we only care
     * about being able to edit on a bus at edit time. Mostly
     * for legacy conversion. Also this class handles the 
     * PlayOnStart parameter differently.
     */
    class EditorGeometryCacheCommon
        : public GeometryCacheCommon
        , public EditorGeometryCacheComponentRequestBus::Handler
    {
    public:
        AZ_TYPE_INFO_LEGACY(EditorGeometryCacheCommon, "{ACE31D8E-F7BC-48B9-950E-AE191E50A80F}", GeometryCacheCommon);
        AZ_CLASS_ALLOCATOR(EditorGeometryCacheCommon, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);
        
        void Activate() override;
        void Deactivate() override;

        void SetMinSpec(EngineSpec minSpec) override;
        EngineSpec GetMinSpec() override { return m_minSpec; }

        void SetPlayOnStart(bool playOnStart) override;
        bool GetPlayOnStart() override { return m_playOnStart; }

        void SetMaxViewDistance(float maxViewDistance) override;
        float GetMaxViewDistance() override { return m_maxViewDistance; }

        void SetViewDistanceMultiplier(float viewDistanceMultiplier) override;
        float GetViewDistanceMultiplier() override { return m_viewDistanceMultiplier; }

        void SetLODDistanceRatio(AZ::u32 lodDistanceRatio) override;
        AZ::u32 GetLODDistanceRatio() override { return m_lodDistanceRatio; }

        void SetCastShadows(bool castShadows) override;
        bool GetCastShadows() override { return m_castShadows; }

        void SetUseVisAreas(bool useVisAreas) override;
        bool GetUseVisAreas() override { return m_useVisAreas; }
        void SetMaterial(_smart_ptr<IMaterial> material) override;
    private:

        //We want the playOnStart param to reflect playing immediately in the editor
        void OnPlayOnStartChanged() override 
        {
            m_playing = m_playOnStart; 
            m_currentTime = m_startTime; 

            //As in GeomCacheCommon::Activate we reset m_currentTime to m_startTime so
            //that animations begin at the requested time.
        }

        void OnFirstFrameStandinChanged() override;
        void OnLastFrameStandinChanged() override;
        void OnStandinChanged() override;

        void HandleStandinChanged(const AZ::EntityId& prevStandinId, const AZ::EntityId& newStandinId, bool evictPrevStandinTransform); //Handles the common logic for OnStandinXChanged events

        AZ::EntityId m_prevFirstFrameStandin;
        AZ::EntityId m_prevLastFrameStandin;
        AZ::EntityId m_prevStandin;

    };

    /**
     * The Edit-time implementation of the GeometryCache component
     */
    class EditorGeometryCacheComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AzFramework::EntityDebugDisplayEventBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        friend class GeomCacheConverter; 

        AZ_COMPONENT(EditorGeometryCacheComponent, "{045C0C58-C13E-49B0-A471-D4AC5D3FC6BD}", AzToolsFramework::Components::EditorComponentBase);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);
        
        ~EditorGeometryCacheComponent() = default;

        // EditorComponentBase
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // AZ::TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        //Reflected members
        EditorGeometryCacheCommon m_common;

        //Unreflected members
        AZ::Transform m_currentWorldTransform;
    };

} //namespace LmbrCentral