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
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include "RenderToTextureBase.h"
#include "IRenderer.h"

namespace RenderToTexture
{
    class EditorRenderToTextureConfig
        : public RenderToTextureConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorRenderToTextureConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(EditorRenderToTextureConfig, "{DE6728FF-F100-442B-A8B3-E0DE876EAA11}", RenderToTextureConfig);

        static void Reflect(AZ::ReflectContext* reflection);
    };

    class EditorRenderToTextureComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public IRenderDebugListener
        , protected AZ::TickBus::Handler
        , protected RenderToTextureBase
    {
    public:
        ~EditorRenderToTextureComponent() override = default;
        AZ_EDITOR_COMPONENT(EditorRenderToTextureComponent, "{851ED863-2D59-4512-8E34-4FFE8156BBC0}");
        static void Reflect(AZ::ReflectContext* reflection);

        void Activate() override;
        void Deactivate() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //////////////////////////////////////////////////////////////////////////
        // IRenderDebugListener
        void OnDebugDraw() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler 
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorRenderToTextureComponent
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        void ConfigurationChanged();
        ////////////////////////////////////////////////////////////////////////

        EditorRenderToTextureConfig m_config;

    private:
        void UpdateRenderTarget(const AZStd::string& textureName, int width, int height);
        void ReleaseCurrentRenderTarget();

        //! update in editor mode as well as game mode 
        bool m_updateInEditor = true;

        //! track if registered to receive debug draw notifications
        bool m_bRenderDebugDrawRegistered = false;
    };
}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED