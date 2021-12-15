/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCursorBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Input/Events/InputTextEventListener.h>

#include <Atom/Bootstrap/BootstrapNotificationBus.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Reflect/Image/Image.h>

#include "LyShinePassDataBus.h"

#if !defined(_RELEASE)
#define LYSHINE_INTERNAL_UNIT_TEST
#endif

#if defined(_RELEASE) && defined(LYSHINE_INTERNAL_UNIT_TEST)
#error "Internal unit test enabled on release build! Please disable."
#endif

class CDraw2d;
class UiRenderer;
class UiCanvasManager;
struct IConsoleCmdArgs;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! CLyShine is the full implementation of the ILyShine interface
class CLyShine
    : public ILyShine
    , public UiCursorBus::Handler
    , public AzFramework::InputChannelEventListener
    , public AzFramework::InputTextEventListener
    , public AZ::TickBus::Handler
    , public AZ::RPI::ViewportContextNotificationBus::Handler
    , protected AZ::Render::Bootstrap::NotificationBus::Handler
    , protected LyShinePassDataRequestBus::Handler
{
public:

    //! Create the LyShine object, the given system pointer is stored internally
    CLyShine(ISystem* system);

    // ILyShine

    ~CLyShine() override;

    void Release() override;

    IDraw2d* GetDraw2d() override;

    AZ::EntityId CreateCanvas() override;
    AZ::EntityId LoadCanvas(const AZStd::string& assetIdPathname) override;
    AZ::EntityId CreateCanvasInEditor(UiEntityContext* entityContext) override;
    AZ::EntityId LoadCanvasInEditor(const AZStd::string& assetIdPathname, const AZStd::string& sourceAssetPathname, UiEntityContext* entityContext) override;
    AZ::EntityId ReloadCanvasFromXml(const AZStd::string& xmlString, UiEntityContext* entityContext) override;
    AZ::EntityId FindCanvasById(LyShine::CanvasId id) override;
    AZ::EntityId FindLoadedCanvasByPathName(const AZStd::string& assetIdPathname) override;

    void ReleaseCanvas(AZ::EntityId canvas, bool forEditor) override;
    void ReleaseCanvasDeferred(AZ::EntityId canvas) override;

    ISprite* LoadSprite(const AZStd::string& pathname) override;
    ISprite* CreateSprite(const AZStd::string& renderTargetName) override;
    bool DoesSpriteTextureAssetExist(const AZStd::string& pathname) override;

    void PostInit() override;

    void SetViewportSize(AZ::Vector2 viewportSize) override;
    void Update(float deltaTimeInSeconds) override;
    void Render() override;
    void ExecuteQueuedEvents() override;

    void Reset() override;
    void OnLevelUnload() override;
    void OnLoadScreenUnloaded() override;

    // ~ILyShine

    // UiCursorInterface
    void IncrementVisibleCounter() override;
    void DecrementVisibleCounter() override;
    bool IsUiCursorVisible() override;
    void SetUiCursor(const char* cursorImagePath) override;
    AZ::Vector2 GetUiCursorPosition() override;
    // ~UiCursorInterface

    // InputChannelEventListener
    bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
    // ~InputChannelEventListener

    // InputTextEventListener
    bool OnInputTextEventFiltered(const AZStd::string& textUTF8) override;
    // ~InputTextEventListener

    // TickEvents
    void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    int GetTickOrder() override;
    // ~TickEvents

    // AZ::RPI::ViewportContextNotificationBus::Handler overrides...
    void OnRenderTick() override;

    // AZ::Render::Bootstrap::NotificationBus
    void OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene) override;
    // ~AZ::Render::Bootstrap::NotificationBus

    // LyShinePassDataRequestBus
    LyShine::AttachmentImagesAndDependencies GetRenderTargets() override;
    // ~LyShinePassDataRequestBus

    // Get the UIRenderer for the game (which is owned by CLyShine). This is not exposed outside the gem.
    UiRenderer* GetUiRenderer();

    // Get/set the UIRenderer for the Editor (which is owned by CLyShine). This is not exposed outside the gem.
    UiRenderer* GetUiRendererForEditor();
    void SetUiRendererForEditor(AZStd::shared_ptr<UiRenderer> uiRenderer);

public: // static member functions

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    static void RunUnitTests(IConsoleCmdArgs* cmdArgs);
#endif

private: // member functions

    AZ_DISABLE_COPY_MOVE(CLyShine);

    void LoadUiCursor();
    void RenderUiCursor();

private:  // static member functions

#ifndef _RELEASE
    static void DebugReportDrawCalls(IConsoleCmdArgs* cmdArgs);
#endif

private: // data

    std::unique_ptr<CDraw2d> m_draw2d;  // using a pointer rather than an instance to avoid including Draw2d.h
    std::unique_ptr<UiRenderer> m_uiRenderer;  // using a pointer rather than an instance to avoid including UiRenderer.h
    AZStd::shared_ptr<UiRenderer> m_uiRendererForEditor;

    std::unique_ptr<UiCanvasManager> m_uiCanvasManager;

    AZStd::string m_cursorImagePathToLoad;
    AZ::Data::Instance<AZ::RPI::Image> m_uiCursorTexture;
    int m_uiCursorVisibleCounter;

    bool m_updatingLoadedCanvases = false;  // guard against nested updates

    // Console variables
#ifndef _RELEASE
    DeclareStaticConstIntCVar(CV_ui_DisplayTextureData, 0);
    DeclareStaticConstIntCVar(CV_ui_DisplayCanvasData, 0);
    DeclareStaticConstIntCVar(CV_ui_DisplayDrawCallData, 0);
    DeclareStaticConstIntCVar(CV_ui_DisplayElemBounds, 0);
    DeclareStaticConstIntCVar(CV_ui_DisplayElemBoundsCanvasIndex, -1);
#endif

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    DeclareStaticConstIntCVar(CV_ui_RunUnitTestsOnStartup, 0);
#endif
};
