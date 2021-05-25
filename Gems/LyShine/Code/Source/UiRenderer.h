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

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/Bootstrap/BootstrapNotificationBus.h>

#ifndef _RELEASE
#include <AzCore/std/containers/unordered_set.h>
#endif

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

        AZ::RPI::ShaderVariantId m_shaderVariantDefault;
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

    //! Return the shader data for the ui shader
    const UiShaderData& GetUiShaderData();

    //! Return the current orthographic view matrix
    AZ::Matrix4x4 GetModelViewProjectionMatrix();

    //! Return the curent viewport size
    AZ::Vector2 GetViewportSize();

    //! Get the current base state
    int GetBaseState();

    //! Set the base state
    void SetBaseState(int state);

    //! Get the current stencil test reference value
    uint32 GetStencilRef();

    //! Set the stencil test reference value
    void SetStencilRef(uint32);

    //! Increment the current stencil reference value
    void IncrementStencilRef();

    //! Decrement the current stencil reference value
    void DecrementStencilRef();

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
    void CreateDynamicDrawContext(AZ::RPI::ScenePtr scene, AZ::Data::Instance<AZ::RPI::Shader>);

    //! Return the viewport context set by the user, or the default if not set
    AZStd::shared_ptr<AZ::RPI::ViewportContext> GetViewportContext();

    //! Bind the global white texture for all the texture units we use
    void BindNullTexture();

    //! Store shader data for later use
    void CacheShaderData(const AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext>& dynamicDraw);

protected: // attributes

    static constexpr char LogName[] = "UiRenderer";

    int m_baseState;
    uint32 m_stencilRef;

    UiShaderData m_uiShaderData;
    AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> m_dynamicDraw;
    bool m_isRPIReady = false;

    // Set by user when viewport context is not the main/default viewport
    AZStd::shared_ptr<AZ::RPI::ViewportContext> m_viewportContext;

#ifndef _RELEASE
    int m_debugTextureDataRecordLevel = 0;
    AZStd::unordered_set<ITexture*> m_texturesUsedInFrame; // LYSHINE_ATOM_TODO - convert to RPI::Image
#endif
};
