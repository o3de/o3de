/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LyShine.h"

#include "UiCanvasComponent.h"
#include "UiCanvasManager.h"
#include "LyShineDebug.h"
#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiImageComponent.h"
#include "UiTextComponent.h"
#include "UiButtonComponent.h"
#include "UiCheckboxComponent.h"
#include "UiSliderComponent.h"
#include "UiTextInputComponent.h"
#include "UiScrollBarComponent.h"
#include "UiScrollBoxComponent.h"
#include "UiFaderComponent.h"
#include "UiFlipbookAnimationComponent.h"
#include "UiLayoutFitterComponent.h"
#include "UiMarkupButtonComponent.h"
#include "UiMaskComponent.h"
#include "UiLayoutColumnComponent.h"
#include "UiLayoutRowComponent.h"
#include "UiLayoutGridComponent.h"
#include "UiParticleEmitterComponent.h"
#include "UiRadioButtonComponent.h"
#include "UiRadioButtonGroupComponent.h"
#include "UiTooltipComponent.h"
#include "UiTooltipDisplayComponent.h"
#include "UiDynamicLayoutComponent.h"
#include "UiDynamicScrollBoxComponent.h"
#include "UiDropdownComponent.h"
#include "UiDropdownOptionComponent.h"
#include "Script/UiCanvasNotificationLuaBus.h"
#include "Script/UiCanvasLuaBus.h"
#include "Script/UiElementLuaBus.h"
#include "Sprite.h"
#include "UiSerialize.h"
#include "UiRenderer.h"
#include "Draw2d.h"

#include <LyShine/Bus/UiCursorBus.h>
#include <LyShine/Bus/UiDraggableBus.h>
#include <LyShine/Bus/UiDropTargetBus.h>

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
#include "TextMarkup.h"
#endif

#include <AzCore/Math/Crc.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

#include <Atom/RHI/RHIUtils.h>

#include "Animation/UiAnimationSystem.h"
#include "World/UiCanvasAssetRefComponent.h"
#include "World/UiCanvasProxyRefComponent.h"
#include "World/UiCanvasOnMeshComponent.h"

//! \brief Simple utility class for LyShine functionality in Lua.
//!
//! Functionality unrelated to UI, such as showing the mouse cursor, should
//! eventually be moved into other modules (for example, mouse cursor functionality
//! should be moved to input, which matches more closely how FG modules are
//! organized).
class LyShineLua
{
public:
    static void ShowMouseCursor(bool visible)
    {
        static bool sShowCursor = false;

        if (visible)
        {
            if (!sShowCursor)
            {
                sShowCursor = true;
                UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);
            }
        }
        else
        {
            if (sShowCursor)
            {
                sShowCursor = false;
                UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
            }
        }
    }
};

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(LyShineLua, "{2570D3B3-2D18-4DB1-A0DE-E017A2F491D1}");
}

// This is the memory allocation for the static data member used for the debug console variables
#ifndef _RELEASE
AllocateConstIntCVar(CLyShine, CV_ui_DisplayTextureData);
AllocateConstIntCVar(CLyShine, CV_ui_DisplayCanvasData);
AllocateConstIntCVar(CLyShine, CV_ui_DisplayDrawCallData);
AllocateConstIntCVar(CLyShine, CV_ui_DisplayElemBounds);
AllocateConstIntCVar(CLyShine, CV_ui_DisplayElemBoundsCanvasIndex);
#endif

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
AllocateConstIntCVar(CLyShine, CV_ui_RunUnitTestsOnStartup);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
CLyShine::CLyShine()
    : AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityUI())
    , AzFramework::InputTextEventListener(AzFramework::InputTextEventListener::GetPriorityUI())
    , m_draw2d(new CDraw2d)
    , m_uiRenderer(new UiRenderer)
    , m_uiCanvasManager(new UiCanvasManager)
    , m_uiCursorVisibleCounter(0)
{
    // Reflect the Deprecated Lua buses using the behavior context.
    // This support will be removed at some point
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
        if (behaviorContext)
        {
            behaviorContext->Class<LyShineLua>("LyShineLua")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("ShowMouseCursor", &LyShineLua::ShowMouseCursor)
            ;

            UiCanvasNotificationLuaProxy::Reflect(behaviorContext);
            UiCanvasLuaProxy::Reflect(behaviorContext);
            UiElementLuaProxy::Reflect(behaviorContext);
        }
    }

    CSprite::Initialize();
    LyShineDebug::Initialize();
    UiElementComponent::Initialize();
    UiCanvasComponent::Initialize();

    AzFramework::InputChannelEventListener::Connect();
    AzFramework::InputTextEventListener::Connect();
    UiCursorBus::Handler::BusConnect();
    AZ::TickBus::Handler::BusConnect();
    AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(
        AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContextName());
    AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();

    // These are internal Amazon components, so register them so that we can send back their names to our metrics collection
    // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
    // This is internal Amazon code, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
    AZStd::vector<AZ::Uuid> componentUuidsForMetricsCollection
    {
        azrtti_typeid<UiCanvasAssetRefComponent>(),
        azrtti_typeid<UiCanvasProxyRefComponent>(),
        azrtti_typeid<UiCanvasOnMeshComponent>(),
        azrtti_typeid<UiCanvasComponent>(),
        azrtti_typeid<UiElementComponent>(),
        azrtti_typeid<UiTransform2dComponent>(),
        azrtti_typeid<UiImageComponent>(),
        azrtti_typeid<UiTextComponent>(),
        azrtti_typeid<UiButtonComponent>(),
        azrtti_typeid<UiCheckboxComponent>(),
        azrtti_typeid<UiSliderComponent>(),
        azrtti_typeid<UiTextInputComponent>(),
        azrtti_typeid<UiScrollBarComponent>(),
        azrtti_typeid<UiScrollBoxComponent>(),
        azrtti_typeid<UiFaderComponent>(),
        azrtti_typeid<UiFlipbookAnimationComponent>(),
        azrtti_typeid<UiMarkupButtonComponent>(),
        azrtti_typeid<UiMaskComponent>(),
        azrtti_typeid<UiLayoutColumnComponent>(),
        azrtti_typeid<UiLayoutRowComponent>(),
        azrtti_typeid<UiLayoutGridComponent>(),
        azrtti_typeid<UiRadioButtonComponent>(),
        azrtti_typeid<UiRadioButtonGroupComponent>(),
        azrtti_typeid<UiDropdownComponent>(),
        azrtti_typeid<UiDropdownOptionComponent>(),
        azrtti_typeid<UiLayoutFitterComponent>(),
        azrtti_typeid<UiParticleEmitterComponent>(),
    };
    AzFramework::MetricsPlainTextNameRegistrationBus::Broadcast(
        &AzFramework::MetricsPlainTextNameRegistrationBus::Events::RegisterForNameSending, componentUuidsForMetricsCollection);



#ifndef _RELEASE
    // Define a debug console variable that controls display of some debug info on UI texture usage
    DefineConstIntCVar3("ui_DisplayTextureData", CV_ui_DisplayTextureData, 0, VF_CHEAT,
        "0=off, 1=display info for all textures used in the frame");

    // Define a debug console variable that controls display of some debug info for all canvases
    DefineConstIntCVar3("ui_DisplayCanvasData", CV_ui_DisplayCanvasData, 0, VF_CHEAT,
        "0=off, 1=display info for all loaded UI canvases, 2=display info for all enabled UI canvases");

    // Define a debug console variable that controls display of some debug info on UI draw calls
    DefineConstIntCVar3("ui_DisplayDrawCallData", CV_ui_DisplayDrawCallData, 0, VF_CHEAT,
        "0=off, 1=display draw call info for all loaded and enabled UI canvases");
    
    // Define a debug console variable that controls display of all element bounds when in game
    DefineConstIntCVar3("ui_DisplayElemBounds", CV_ui_DisplayElemBounds, 0, VF_CHEAT,
        "0=off, 1=display the UI element bounding boxes");

    // Define a debug console variable that filters the display of all element bounds when in game by canvas index
    DefineConstIntCVar3("ui_DisplayElemBoundsCanvasIndex", CV_ui_DisplayElemBoundsCanvasIndex, -1, VF_CHEAT,
        "-1=no filter, 0-N=only for elements from this canvas index (see 'ui_displayCanvasData 2' for index)");

    // Define a console command that outputs a report to a file about the draw calls for all enabled canvases
    REGISTER_COMMAND("ui_ReportDrawCalls", &DebugReportDrawCalls, VF_NULL, "");
#endif

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    DefineConstIntCVar3("ui_RunUnitTestsOnStartup", CV_ui_RunUnitTestsOnStartup, 0, VF_CHEAT,
        "0=off, 1=run LyShine unit tests on startup");

    REGISTER_COMMAND("ui_RunUnitTests", &RunUnitTests, VF_NULL, "");
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CLyShine::~CLyShine()
{
    UiCursorBus::Handler::BusDisconnect();
    AZ::TickBus::Handler::BusDisconnect();
    AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
    AzFramework::InputTextEventListener::Disconnect();
    AzFramework::InputChannelEventListener::Disconnect();
    AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();
    LyShinePassDataRequestBus::Handler::BusDisconnect();

    UiCanvasComponent::Shutdown();

    // must be done after UiCanvasComponent::Shutdown
    CSprite::Shutdown();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::Release()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d* CLyShine::GetDraw2d()
{
    return m_draw2d.get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRenderer* CLyShine::GetUiRenderer()
{
    return m_uiRenderer.get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRenderer* CLyShine::GetUiRendererForEditor()
{
    return m_uiRendererForEditor.get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::SetUiRendererForEditor(AZStd::shared_ptr<UiRenderer> uiRenderer)
{
    m_uiRendererForEditor = uiRenderer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::CreateCanvas()
{
    return m_uiCanvasManager->CreateCanvas();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::LoadCanvas(const AZStd::string& assetIdPathname)
{
    return m_uiCanvasManager->LoadCanvas(assetIdPathname.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::CreateCanvasInEditor(UiEntityContext* entityContext)
{
    return m_uiCanvasManager->CreateCanvasInEditor(entityContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::LoadCanvasInEditor(const AZStd::string& assetIdPathname, const AZStd::string& sourceAssetPathname, UiEntityContext* entityContext)
{
    return m_uiCanvasManager->LoadCanvasInEditor(assetIdPathname, sourceAssetPathname, entityContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::ReloadCanvasFromXml(const AZStd::string& xmlString, UiEntityContext* entityContext)
{
    return m_uiCanvasManager->ReloadCanvasFromXml(xmlString, entityContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::FindCanvasById(LyShine::CanvasId id)
{
    return m_uiCanvasManager->FindCanvasById(id);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::FindLoadedCanvasByPathName(const AZStd::string& assetIdPathname)
{
    return m_uiCanvasManager->FindLoadedCanvasByPathName(assetIdPathname.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::ReleaseCanvas(AZ::EntityId canvas, bool forEditor)
{
    m_uiCanvasManager->ReleaseCanvas(canvas, forEditor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::ReleaseCanvasDeferred(AZ::EntityId canvas)
{
    m_uiCanvasManager->ReleaseCanvasDeferred(canvas);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite* CLyShine::LoadSprite(const AZStd::string& pathname)
{
    return CSprite::LoadSprite(pathname);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite* CLyShine::CreateSprite(const AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>& attachmentImageAsset)
{
    return CSprite::CreateSprite(attachmentImageAsset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLyShine::DoesSpriteTextureAssetExist(const AZStd::string& pathname)
{
    return CSprite::DoesSpriteTextureAssetExist(pathname);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Data::Instance<AZ::RPI::Image> CLyShine::LoadTexture(const AZStd::string& pathname)
{
    return CDraw2d::LoadTexture(pathname);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::PostInit()
{
#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    if (CV_ui_RunUnitTestsOnStartup)
    {
        RunUnitTests(nullptr);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::SetViewportSize(AZ::Vector2 viewportSize)
{
    // Pass the viewport size to UiCanvasComponents
    m_uiCanvasManager->SetTargetSizeForLoadedCanvases(viewportSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::Update(float deltaTimeInSeconds)
{
    if (!m_uiRenderer->IsReady())
    {
        return;
    }

    // Tell the UI system the size of the viewport we are rendering to - this drives the
    // canvas size for full screen UI canvases. It needs to be set before either lyShine->Update or
    // lyShine->Render are called. It must match the viewport size that the input system is using.
    SetViewportSize(m_uiRenderer->GetViewportSize());

    // Guard against nested updates. This can occur if a canvas update below triggers the load screen component's
    // UpdateAndRender (ex. when a texture is loaded)
    if (!m_updatingLoadedCanvases)
    {
        m_updatingLoadedCanvases = true;

        // Update all the canvases loaded in game
        m_uiCanvasManager->UpdateLoadedCanvases(deltaTimeInSeconds);

        // Execute events that have been queued during the canvas update
        ExecuteQueuedEvents();

        m_updatingLoadedCanvases = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::Render()
{
    if (AZ::RHI::IsNullRHI())
    {
        return;
    }

    if (m_updatingLoadedCanvases)
    {
        // Don't render if an update is in progress. This can occur if an update triggers the
        // load screen component's UpdateAndRender (ex. when a texture is loaded)
        return;
    }

    if (!m_uiRenderer->IsReady())
    {
        return;
    }

#ifndef _RELEASE
    GetUiRenderer()->DebugSetRecordingOptionForTextureData(CV_ui_DisplayTextureData);
#endif
   
    GetUiRenderer()->BeginUiFrameRender();
        
    // Render all the canvases loaded in game
    m_uiCanvasManager->RenderLoadedCanvases();

    // Set sort key for draw2d layer to ensure it renders in front of the canvases
    static const int64_t topLayerKey = 0x1000000;
    m_draw2d->SetSortKey(topLayerKey);
    m_draw2d->RenderDeferredPrimitives();

    // Don't render the UI cursor when in edit mode. For example during UI Preview mode a script could turn on the
    // cursor. But it would draw in the wrong place. It is better to just rely on the regular editor cursor in preview
    // since, in game, the game cursor could be turned on and off any any point, so each UI canvas is not necessarily
    // going to turn it on.
    if (!(gEnv->IsEditor() && gEnv->IsEditing()))
    {
        RenderUiCursor();
    }

    GetUiRenderer()->EndUiFrameRender();

#ifndef _RELEASE
    if (CV_ui_DisplayElemBounds)
    {
        m_uiCanvasManager->DebugDisplayElemBounds(CV_ui_DisplayElemBoundsCanvasIndex);
    }

    if (CV_ui_DisplayTextureData)
    {
        GetUiRenderer()->DebugDisplayTextureData(CV_ui_DisplayTextureData);
    }
    else if (CV_ui_DisplayCanvasData)
    {
        m_uiCanvasManager->DebugDisplayCanvasData(CV_ui_DisplayCanvasData);
    }
    else if (CV_ui_DisplayDrawCallData)
    {
        m_uiCanvasManager->DebugDisplayDrawCallData();
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::ExecuteQueuedEvents()
{
    // Execute events that have been queued during the canvas update
    UiFaderNotificationBus::ExecuteQueuedEvents();
    UiAnimationNotificationBus::ExecuteQueuedEvents();

    // Execute events that have been queued during the input event handler
    UiDraggableNotificationBus::ExecuteQueuedEvents();  // draggable must be done before drop target
    UiDropTargetNotificationBus::ExecuteQueuedEvents();
    UiCanvasNotificationBus::ExecuteQueuedEvents();
    UiButtonNotificationBus::ExecuteQueuedEvents();
    UiInteractableNotificationBus::ExecuteQueuedEvents();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::Reset()
{
    // This is called when the game is terminated.

    // Reset the debug module - this should be called before DestroyLoadedCanvases since it
    // tracks the loaded debug canvas
    LyShineDebug::Reset();

    // Delete all the canvases in m_canvases map that are not open in the editor.
    m_uiCanvasManager->DestroyLoadedCanvases(false);

    // Ensure that the UI Cursor is hidden.
    LyShineLua::ShowMouseCursor(false);
    m_uiCursorVisibleCounter = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::OnLevelUnload()
{
    // This is called when a level is unloaded or a new level is initialized

    // Reset the debug module - this should be called before DestroyLoadedCanvases since it
    // tracks the loaded debug canvas
    LyShineDebug::Reset();

    // Delete all the canvases in m_canvases map that a not loaded in the editor and are not
    // marked to be kept between levels.
    m_uiCanvasManager->DestroyLoadedCanvases(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::OnLoadScreenUnloaded()
{
    m_uiCanvasManager->OnLoadScreenUnloaded();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::IncrementVisibleCounter()
{
    ++m_uiCursorVisibleCounter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::DecrementVisibleCounter()
{
    --m_uiCursorVisibleCounter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLyShine::IsUiCursorVisible()
{
    return m_uiCursorVisibleCounter > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::SetUiCursor(const char* cursorImagePath)
{
    m_uiCursorTexture.reset();
    m_cursorImagePathToLoad.clear();

    if (cursorImagePath && *cursorImagePath)
    {
        m_cursorImagePathToLoad = cursorImagePath;
        // The cursor image can only be loaded after the RPI has been initialized.
        // Note: this check could be avoided if LyShineSystemComponent included the RPISystem
        // as a required service. However, LyShineSystempComponent is currently activated for
        // tools as well as game and RPIService is not available with all tools such as AP. An
        // enhancement would be to break LyShineSystemComponent into a  game only component
        if (m_uiRenderer->IsReady())
        {
            LoadUiCursor();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 CLyShine::GetUiCursorPosition()
{
    AZ::Vector2 systemCursorPositionNormalized;
    AzFramework::InputSystemCursorRequestBus::EventResult(systemCursorPositionNormalized,
        AzFramework::InputDeviceMouse::Id,
        &AzFramework::InputSystemCursorRequests::GetSystemCursorPositionNormalized);

    AZ::Vector2 viewportSize = m_uiRenderer->GetViewportSize();

    return AZ::Vector2(systemCursorPositionNormalized.GetX() * viewportSize.GetX(),
        systemCursorPositionNormalized.GetY() * viewportSize.GetY());
}

void CLyShine::SetUiCursorPosition(const AZ::Vector2& positionNormalized)
{
    AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequests::SetSystemCursorPositionNormalized, positionNormalized);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLyShine::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
{
    // disable UI inputs when console is open except for a primary release
    // if we ignore the primary release when there is an active interactable then it will miss its release
    // which leaves it in a bad state. E.g. a drag operation will be left in flight and not properly
    // terminated.
    if (gEnv->pConsole->GetStatus())
    {
        bool isPrimaryRelease = false;
        if (inputChannel.GetInputChannelId() == AzFramework::InputDeviceMouse::Button::Left ||
            inputChannel.GetInputChannelId() == AzFramework::InputDeviceTouch::Touch::Index0)
        {
            if (inputChannel.IsStateEnded())
            {
                isPrimaryRelease = true;
            }
        }

        if (!isPrimaryRelease)
        {
            return false;
        }
    }

    bool result = m_uiCanvasManager->HandleInputEventForLoadedCanvases(inputChannel);
    if (result)
    {
        // Execute events that have been queued during the input event handler
        ExecuteQueuedEvents();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLyShine::OnInputTextEventFiltered(const AZStd::string& textUTF8)
{
    if (gEnv->pConsole->GetStatus()) // disable UI inputs when console is open
    {
        return false;
    }

    bool result = m_uiCanvasManager->HandleTextEventForLoadedCanvases(textUTF8);
    if (result)
    {
        // Execute events that have been queued during the input event handler
        ExecuteQueuedEvents();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
{
    // Update the loaded UI canvases
    Update(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int CLyShine::GetTickOrder()
{
    return AZ::TICK_PRE_RENDER;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::OnRenderTick()
{
    // Recreate dirty render graphs and send primitive data to the dynamic draw context
    Render();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene)
{
    // Load cursor if its path was set before RPI was initialized
    LoadUiCursor();

    LyShinePassDataRequestBus::Handler::BusConnect(bootstrapScene->GetId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::AttachmentImagesAndDependencies CLyShine::GetRenderTargets()
{
    LyShine::AttachmentImagesAndDependencies attachmentImagesAndDependencies;
    m_uiCanvasManager->GetRenderTargets(attachmentImagesAndDependencies);
    return attachmentImagesAndDependencies;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::LoadUiCursor()
{
    if (!m_cursorImagePathToLoad.empty())
    {
        m_uiCursorTexture = CDraw2d::LoadTexture(m_cursorImagePathToLoad);
        m_cursorImagePathToLoad.clear();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::RenderUiCursor()
{
    if (!m_uiCursorTexture || !IsUiCursorVisible())
    {
        return;
    }

    const AzFramework::InputDevice* mouseDevice = AzFramework::InputDeviceRequests::FindInputDevice(AzFramework::InputDeviceMouse::Id);
    if (!mouseDevice || !mouseDevice->IsConnected())
    {
        return;
    }

    const AZ::Vector2 position = GetUiCursorPosition();
    AZ::RHI::Size cursorSize = m_uiCursorTexture->GetDescriptor().m_size;
    const AZ::Vector2 dimensions(aznumeric_cast<float>(cursorSize.m_width), aznumeric_cast<float>(cursorSize.m_height));

    IDraw2d::ImageOptions imageOptions;
    imageOptions.m_clamp = true;
    const float opacity = 1.0f;
    const float rotation = 0.0f;
    const AZ::Vector2* pivotPoint = nullptr;
    const AZ::Vector2* minMaxTexCoords = nullptr;
    m_draw2d->DrawImage(m_uiCursorTexture, position, dimensions, opacity, rotation, pivotPoint, minMaxTexCoords, &imageOptions);
}

#ifndef _RELEASE
////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::DebugReportDrawCalls(IConsoleCmdArgs* cmdArgs)
{
    if (AZ::Interface<ILyShine>::Get())
    {
        // We want to use an internal-only non-static function so downcast to CLyShine
        CLyShine* lyShine = static_cast<CLyShine*>(AZ::Interface<ILyShine>::Get());

        // There is an optional parameter which is a name to include in the output filename
        AZStd::string name;
        if (cmdArgs->GetArgCount() > 1)
        {
            name = cmdArgs->GetArg(1);
        }

        // Use the canvas manager to access all the loaded canvases
        lyShine->m_uiCanvasManager->DebugReportDrawCalls(name);
    }
}
#endif

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::RunUnitTests(IConsoleCmdArgs* cmdArgs)
{
    // Tests are only valid from the launcher or from game mode (within the editor)
    const bool inEditorEnvironment = gEnv && gEnv->IsEditor() && gEnv->IsEditing();
    if (inEditorEnvironment)
    {
        AZ_Warning("LyShine", false,
            "Unit-tests: skipping! Editor environment detected. Run tests "
            "within editor via game mode (using ui_RunUnitTests) or use "
            "the standalone launcher instead.");
        
        return;
    }

    CLyShine* lyShine = static_cast<CLyShine*>(AZ::Interface<ILyShine>::Get());
    AZ_Assert(lyShine, "Attempting to run unit-tests prior to LyShine initialization!");

    TextMarkup::UnitTest(cmdArgs);
    UiTextComponent::UnitTest(lyShine, cmdArgs);
    UiTextComponent::UnitTestLocalization(lyShine, cmdArgs);
    UiTransform2dComponent::UnitTest(lyShine, cmdArgs);
    UiMarkupButtonComponent::UnitTest(lyShine, cmdArgs);
}
#endif

