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

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/EntityId.h>

struct ILightSource;
struct IRenderNode;
class CDLight;

namespace LmbrCentral
{
    class LightConfiguration;
    class LensFlareConfiguration;

    /*!
     * Renderer specific implementation of a Renderer light
     */
    class LightInstance final
        : private AZ::TransformNotificationBus::Handler
    {
    public:

        AZ_TYPE_INFO(LightInstance, "{844D6585-6613-4E0D-BBA7-C37073B84F5F}");

        LightInstance();
        ~LightInstance();

        void SetEntity(AZ::EntityId id);

        void CreateRenderLight(const LightConfiguration& configuration);
        void UpdateRenderLight(const LightConfiguration& configuration);

        void CreateRenderLight(const LensFlareConfiguration& configuration);
        void UpdateRenderLight(const LensFlareConfiguration& configuration);

        void DestroyRenderLight();

        IRenderNode* GetRenderNode();

        bool IsOn() const;

        bool TurnOn();
        bool TurnOff();

        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus::Handler interface implementation
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////

    private:
        template <typename ConfigurationType, typename ConfigToLightParamsFunc>
        void CreateRenderLightInternal(const ConfigurationType& configuration, ConfigToLightParamsFunc configToLightParams);

        AZ::EntityId m_entityId;
        ILightSource* m_renderLight;
    };
} // namespace LmbrCentral
