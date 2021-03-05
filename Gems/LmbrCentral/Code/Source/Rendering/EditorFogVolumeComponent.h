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

#include <AzCore/base.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include "FogVolumeComponent.h"
#include "FogVolumeRequestsHandler.h"

namespace LmbrCentral
{
    class EditorFogVolumeConfiguration
        : public FogVolumeConfiguration
    {
    public:
        AZ_TYPE_INFO(EditorFogVolumeConfiguration, "{9D431EA0-92F9-4A00-96C6-28B189A6EE56}");

        static void Reflect(AZ::ReflectContext* context);
        AZ::Crc32 PropertyChanged() override;
    };

    /**
     * In-editor fog volume component.
     */
    class EditorFogVolumeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private RenderNodeRequestBus::Handler
        , private FogVolumeComponentRequestsBusHandler
        , private AZ::TransformNotificationBus::Handler
        , private ShapeComponentNotificationsBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    private:
        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:
        AZ_EDITOR_COMPONENT(EditorFogVolumeComponent, "{8CA5AB61-96D8-482F-B07C-DAD72ED73646}");

        ~EditorFogVolumeComponent() override = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        // RenderNodeRequestBus::Handler
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;

        // FogVolumeComponentRequestBus::Handler
        void RefreshFog() override;

        // TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // ShapeComponentNotificationsBus::Handler
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason) override;

        // AzToolsFramework::EditorEvents::Handler
        void OnEditorSpecChange() override;

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:
        FogVolumeConfiguration& GetConfiguration() override
        {
            return m_configuration;
        }

    private:
        EditorFogVolumeConfiguration m_configuration;
        FogVolume m_fogVolume;
    };
}