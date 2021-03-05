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
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Vector3.h>

#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <IEntityRenderState.h>

#include "FogVolumeCommon.h"
#include "FogVolumeRequestsHandler.h"

namespace LmbrCentral
{
    /*!
    * In-game Fog Volume component.
    */
    class FogVolumeComponent
        : public AZ::Component
        , public RenderNodeRequestBus::Handler
        , private FogVolumeComponentRequestsBusHandler
        , private AZ::TransformNotificationBus::Handler
        , private ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(FogVolumeComponent, "{C01B9E8F-C015-46AC-9065-79445CE1408A}");
        ~FogVolumeComponent() override = default;

        template<typename T>
        void SetConfiguration(T&& configuration)
        {
            m_configuration = AZStd::forward<T>(configuration);
        }

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        // RenderNodeRequestBus::Handler interface implementation
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;
        static const float s_renderNodeRequestBusOrder;

        // TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // ShapeComponentNotificationsBus::Handler
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason) override;

        // FogVolumeComponentRequestBus::Handler
        void RefreshFog() override;

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void ExposeRequestsBusInBehaviorContext(AZ::BehaviorContext* behaviorContext, const char* name);

    protected:
        FogVolumeConfiguration& GetConfiguration() override
        {
            return m_configuration;
        }

    private:
        FogVolumeConfiguration m_configuration;
        FogVolume m_fogVolume;
    };
}