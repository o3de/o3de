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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include "HighQualityShadowComponent.h"

namespace LmbrCentral
{
    /**
     * Extends HighQualityShadowConfig structure for editor functionality.
     */
    class EditorHighQualityShadowConfig
        : public HighQualityShadowConfig
    {
    public:
        AZ_TYPE_INFO(EditorHighQualityShadowConfig, "{4A7D67C6-E689-427A-B126-A71A4BF8A2C7}");

        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_entityId; // Editor-only, not reflected.

        void EditorRefresh() override;
    };

    /*!
     * In-editor high quality shadow component.
     */
    class EditorHighQualityShadowComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public EditorHighQualityShadowComponentRequestBus::Handler
        , public MeshComponentNotificationBus::Handler
    {
    public:

        AZ_COMPONENT(EditorHighQualityShadowComponent, "{9C86E09D-0727-476E-A4A1-25989CDBF9C6}", AzToolsFramework::Components::EditorComponentBase);

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus interface implementation
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;
        ///////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorHighQualityShadowComponentRequestBus interface implementation
        void RefreshProperties() override;
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

        EditorHighQualityShadowConfig m_config;
    };
} // namespace LmbrCentral
