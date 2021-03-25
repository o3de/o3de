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

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include "RenderToTexture/RenderToTextureBus.h"
#include "RenderToTextureBase.h"

namespace RenderToTexture
{
    class RenderToTextureComponent
        : public AZ::Component
        , public RenderToTextureBase
        , protected AZ::TickBus::Handler
        , public RenderToTextureRequestBus::Handler
    {
    public:
        ~RenderToTextureComponent() override = default;
        AZ_COMPONENT(RenderToTextureComponent, "{7687B16D-5F73-4ECA-BFF4-39CF0ECEE0D0}", RenderToTextureBase);

        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {};
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        ////////////////////////////////////////////////////////////////////////
        // RenderToTextureRequestBus interface implementation
        int GetTextureResourceId() const override;
        void SetAlphaMode(AzRTT::AlphaMode mode) override;
        void SetCamera(const AZ::EntityId& id) override;
        void SetEnabled(bool enabled) override;
        void SetMaxFPS(double fps) override;
        void SetWriteSRGBEnabled(bool enabled) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler 
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // RenderToTextureComponent
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        ////////////////////////////////////////////////////////////////////////

        RenderToTextureConfig m_config;
    private:
        bool m_configDirty = false;
    };
}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
