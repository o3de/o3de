/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <CryCommon/BaseTypes.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/Bootstrap/BootstrapNotificationBus.h>

#ifndef _RELEASE
#include <AzCore/std/containers/unordered_set.h>
#endif

class ITexture;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UI render interface
//
class UiRenderer
    : public AZ::Render::Bootstrap::NotificationBus::Handler
{
public: // types

    // Cached shader data
    struct UiShaderData
    {
        AZ::RHI::ShaderInputImageIndex m_imageInputIndex;
        AZ::RHI::ShaderInputConstantIndex m_viewProjInputIndex;
        AZ::RHI::ShaderInputConstantIndex m_isClampInputIndex;

        AZ::RPI::ShaderVariantId m_shaderVariantTextureLinear;
        AZ::RPI::ShaderVariantId m_shaderVariantTextureSrgb;
        AZ::RPI::ShaderVariantId m_shaderVariantAlphaTestMask;
        AZ::RPI::ShaderVariantId m_shaderVariantGradientMask;
    };

    // Base state
    struct BaseState
    {
        BaseState()
        {
            ResetToDefault();
        }

        void ResetToDefault()
        {
            // Enable blend/color write
            m_blendStateEnabled = true;
            m_blendStateWriteMask = 0xF;

            // Disable stencil
            m_stencilState = AZ::RHI::StencilState();
            m_stencilState.m_enable = 0;

            m_useAlphaTest = false;
            m_modulateAlpha = false;
        }

        uint32_t m_blendStateEnabled = true;
        uint32_t m_blendStateWriteMask = 0xF;
        AZ::RHI::StencilState m_stencilState;
        bool m_useAlphaTest = false;
        bool m_modulateAlpha = false;
        bool m_srgbWrite = true;
    };

public: // member functions

    //! Constructor, constructed by the LyShine class
    UiRenderer(AZ::RPI::ViewportContextPtr viewportContext = nullptr);
    ~UiRenderer();

    //! Returns whether RPI has loaded all its assets and is ready to render
    bool IsReady();

    //! Start the rendering of the frame for LyShine
    void BeginUiFrameRender();

    //! End the rendering of the frame for LyShine
    void EndUiFrameRender();

    //! Start the rendering of a UI canvas
    void BeginCanvasRender();

    //! End the rendering of a UI canvas
    void EndCanvasRender();

    //! Return the dynamic draw context associated with this UI renderer
    AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> GetDynamicDrawContext();

    AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> CreateDynamicDrawContextForRTT(const AZStd::string& rttName);

    //! Return the shader data for the ui shader
    const UiShaderData& GetUiShaderData();

    //! Return the current orthographic view matrix
    AZ::Matrix4x4 GetModelViewProjectionMatrix();

    //! Return the curent viewport size
    AZ::Vector2 GetViewportSize();

    //! Get the current base state
    BaseState GetBaseState();

    //! Set the base state
    void SetBaseState(BaseState state);

    //! Get the shader variant based on current render properties
    AZ::RPI::ShaderVariantId GetCurrentShaderVariant();

    //! Get the current stencil test reference value
    uint32 GetStencilRef();

    //! Set the stencil test reference value
    void SetStencilRef(uint32);

    //! Increment the current stencil reference value
    void IncrementStencilRef();

    //! Decrement the current stencil reference value
    void DecrementStencilRef();

    //! Return the viewport context set by the user, or the default if not set
    AZStd::shared_ptr<AZ::RPI::ViewportContext> GetViewportContext();

#ifndef _RELEASE
    //! Setup to record debug texture data before rendering
    void DebugSetRecordingOptionForTextureData(int recordingOption);

    //! Display debug texture data after rendering
    void DebugDisplayTextureData(int recordingOption);
#endif

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiRenderer);

    // AZ::Render::Bootstrap::Notification
    void OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene) override;
    // ~AZ::Render::Bootstrap::Notification

    //! Create a scene for the user defined viewportContext
    AZ::RPI::ScenePtr CreateScene(AZStd::shared_ptr<AZ::RPI::ViewportContext> viewportContext);

    //! Create a dynamic draw context for this renderer
    AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> CreateDynamicDrawContext(
        AZ::Data::Instance<AZ::RPI::Shader> uiShader);

    //! Bind the global white texture for all the texture units we use
    void BindNullTexture();

    //! Store shader data for later use
    void CacheShaderData(const AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext>& dynamicDraw);

protected: // attributes

    static constexpr char LogName[] = "UiRenderer";

    BaseState m_baseState;
    uint32 m_stencilRef = 0;

    UiShaderData m_uiShaderData;
    AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> m_dynamicDraw;
    bool m_isRPIReady = false;

    // Set by user when viewport context is not the main/default viewport
    AZStd::shared_ptr<AZ::RPI::ViewportContext> m_viewportContext;

    AZ::RPI::ScenePtr m_ownedScene;
    AZ::RPI::Scene* m_scene = nullptr;

#ifndef _RELEASE
    int m_debugTextureDataRecordLevel = 0;
    AZStd::unordered_set<ITexture*> m_texturesUsedInFrame; // LYSHINE_ATOM_TODO - convert to RPI::Image
#endif
};
