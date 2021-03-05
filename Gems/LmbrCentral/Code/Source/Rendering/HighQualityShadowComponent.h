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
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Vector3.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/HighQualityShadowComponentBus.h>

namespace LmbrCentral
{
    /*!
     * Stores configuration settings for per-entity shadows
     */
    class HighQualityShadowConfig
    {
    public:
        AZ_TYPE_INFO(HighQualityShadowConfig, "{3B3CD21A-E61B-401A-8F54-B76FB6278B11}");

        HighQualityShadowConfig();
        virtual ~HighQualityShadowConfig() {}

        static void Reflect(AZ::ReflectContext* context);

        bool m_enabled;
        float m_constBias;
        float m_slopeBias;
        float m_jitter;
        AZ::Vector3 m_bboxScale;
        int m_shadowMapSize;

        // Property event-handlers implemented in editor component.
        virtual void EditorRefresh() { }
    };

    /*!
     * Provides a entity-specific shadow map to achieving higher quality shadows on a specific entity. 
     * Has performance and memory impact so use sparingly. 
     * Corresponds to legacy PerEntityShadows feature.
     */
    class HighQualityShadowComponent
        : public AZ::Component
        , public HighQualityShadowComponentRequestBus::Handler
        , public MeshComponentNotificationBus::Handler
    {
    public:
        friend class EditorHighQualityShadowComponent;

        AZ_COMPONENT(HighQualityShadowComponent, "{B692F9D9-4850-4D6E-9A32-760901455E40}");
        
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // HighQualityShadowComponentRequestBus interface implementation
        void SetEnabled(bool enabled) override;
        bool GetEnabled() override;
        ///////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus interface implementation
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;
        ///////////////////////////////////

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("HighQualityShadowService", 0x43dea981));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("HighQualityShadowService", 0x43dea981));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            // HighQualityShadowComponent is only applicable to Entities that cast and/or receive shadows.
            required.push_back(AZ_CRC("MeshService", 0x71d8a455));
        }

        static void Reflect(AZ::ReflectContext* context);

    protected:

        HighQualityShadowConfig m_config;
    };

    namespace HighQualityShadowComponentUtils
    {
        void ApplyShadowSettings(AZ::EntityId entityId, const HighQualityShadowConfig& config);
        void RemoveShadow(AZ::EntityId entityId);
    }

} // namespace LmbrCentral
