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
#include "LyShine_precompiled.h"

#include "LyShine.h"

#include "Draw2d.h"

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
CLyShine::CLyShine(ISystem* system)
    : AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityUI())
    , AzFramework::InputTextEventListener(AzFramework::InputTextEventListener::GetPriorityUI())
    , m_system(system)
    , m_draw2d(new CDraw2d)
    , m_uiRenderer(new UiRenderer)
    , m_uiCanvasManager(new UiCanvasManager)
    , m_uiCursorTexture(nullptr)
    , m_uiCursorVisibleCounter(0)
{
    // Reflect the Deprecated Lua buses using the behavior context.
    // This support will be removed at some point
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        EBUS_EVENT_RESULT(behaviorContext, AZ::ComponentApplicationBus, GetBehaviorContext);
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

    UiAnimationSystem::StaticInitialize();

    m_system->GetIRenderer()->AddRenderDebugListener(this);

    AzFramework::InputChannelEventListener::Connect();
    AzFramework::InputTextEventListener::Connect();
    UiCursorBus::Handler::BusConnect();

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
    EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, componentUuidsForMetricsCollection);



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
    AzFramework::InputTextEventListener::Disconnect();
    AzFramework::InputChannelEventListener::Disconnect();

    if (m_system->GetIRenderer())
    {
        m_system->GetIRenderer()->RemoveRenderDebugListener(this);
    }

    UiCanvasComponent::Shutdown();

    // must be done after UiCanvasComponent::Shutdown
    CSprite::Shutdown();

    if (m_uiCursorTexture)
    {
        m_uiCursorTexture->Release();
        m_uiCursorTexture = nullptr;
    }
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
AZ::EntityId CLyShine::CreateCanvas()
{
    return m_uiCanvasManager->CreateCanvas();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::LoadCanvas(const string& assetIdPathname)
{
    return m_uiCanvasManager->LoadCanvas(assetIdPathname.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::CreateCanvasInEditor(UiEntityContext* entityContext)
{
    return m_uiCanvasManager->CreateCanvasInEditor(entityContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::LoadCanvasInEditor(const string& assetIdPathname, const string& sourceAssetPathname, UiEntityContext* entityContext)
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
AZ::EntityId CLyShine::FindLoadedCanvasByPathName(const string& assetIdPathname)
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
ISprite* CLyShine::LoadSprite(const string& pathname)
{
    return CSprite::LoadSprite(pathname);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite* CLyShine::CreateSprite(const string& renderTargetName)
{
    return CSprite::CreateSprite(renderTargetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLyShine::DoesSpriteTextureAssetExist(const AZStd::string& pathname)
{
    return CSprite::DoesSpriteTextureAssetExist(pathname);
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
    FRAME_PROFILER(__FUNCTION__, gEnv->pSystem, PROFILE_UI);

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
    FRAME_PROFILER(__FUNCTION__, gEnv->pSystem, PROFILE_UI);

    if (!gEnv || !gEnv->pRenderer || gEnv->pRenderer->GetRenderType() == ERenderType::eRT_Null)
    {
        // if the renderer is not initialized or it is the null renderer (e.g. running as a server)
        // then do nothing
        return;
    }

    if (m_updatingLoadedCanvases)
    {
        // Don't render if an update is in progress. This can occur if an update triggers the
        // load screen component's UpdateAndRender (ex. when a texture is loaded)
        return;
    }

    // Fix for bug where this function could get called before CRenderer::InitSystemResources has been called.
    // To check if it has been called we see if the default textures have been setup
    int whiteTextureId = gEnv->pRenderer->GetWhiteTextureId();
    if (whiteTextureId == -1 || gEnv->pRenderer->EF_GetTextureByID(whiteTextureId) == nullptr)
    {
        // system resources are not yet initialized, this makes it impossible to render the UI and could
        // cause a crash if we attempt to.
        return;
    }

#ifndef _RELEASE
    GetUiRenderer()->DebugSetRecordingOptionForTextureData(CV_ui_DisplayTextureData);
#endif
   
    GetUiRenderer()->BeginUiFrameRender();
        
    // Render all the canvases loaded in game
    m_uiCanvasManager->RenderLoadedCanvases();

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
void CLyShine::OnDebugDraw()
{
    LyShineDebug::RenderDebug();
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
    if (m_uiCursorTexture)
    {
        m_uiCursorTexture->Release();
        m_uiCursorTexture = nullptr;
    }

    if (cursorImagePath && *cursorImagePath && gEnv && gEnv->pRenderer)
    {
        m_uiCursorTexture = gEnv->pRenderer->EF_LoadTexture(cursorImagePath, FT_DONT_RELEASE | FT_DONT_STREAM);
        if (m_uiCursorTexture)
        {
            m_uiCursorTexture->SetClamp(true);
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
    return AZ::Vector2(systemCursorPositionNormalized.GetX() * static_cast<float>(gEnv->pRenderer->GetOverlayWidth()),
        systemCursorPositionNormalized.GetY() * static_cast<float>(gEnv->pRenderer->GetOverlayHeight()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLyShine::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

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
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

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
void CLyShine::RenderUiCursor()
{
    if (!gEnv || !gEnv->pRenderer || !m_uiCursorTexture || !IsUiCursorVisible())
    {
        return;
    }

    const AzFramework::InputDevice* mouseDevice = AzFramework::InputDeviceRequests::FindInputDevice(AzFramework::InputDeviceMouse::Id);
    if (!mouseDevice || !mouseDevice->IsConnected())
    {
        return;
    }

    const AZ::Vector2 position = GetUiCursorPosition();
    const AZ::Vector2 dimensions(static_cast<float>(m_uiCursorTexture->GetWidth()), static_cast<float>(m_uiCursorTexture->GetHeight()));

    m_draw2d->BeginDraw2d();
    m_draw2d->DrawImage(m_uiCursorTexture->GetTextureID(), position, dimensions);
    m_draw2d->EndDraw2d();
}

#ifndef _RELEASE
////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::DebugReportDrawCalls(IConsoleCmdArgs* cmdArgs)
{
    if (gEnv->pLyShine)
    {
        // We want to use an internal-only non-static function so downcast to CLyShine
        CLyShine* pLyShine = static_cast<CLyShine*>(gEnv->pLyShine);

        // There is an optional parameter which is a name to include in the output filename
        AZStd::string name;
        if (cmdArgs->GetArgCount() > 1)
        {
            name = cmdArgs->GetArg(1);
        }

        // Use the canvas manager to access all the loaded canvases
        pLyShine->m_uiCanvasManager->DebugReportDrawCalls(name);
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

    CLyShine* lyShine = static_cast<CLyShine*>(gEnv->pLyShine);
    AZ_Assert(lyShine, "Attempting to run unit-tests prior to LyShine initialization!");

    TextMarkup::UnitTest(cmdArgs);
    UiTextComponent::UnitTest(lyShine, cmdArgs);
    UiTextComponent::UnitTestLocalization(lyShine, cmdArgs);
    UiTransform2dComponent::UnitTest(lyShine, cmdArgs);
    UiMarkupButtonComponent::UnitTest(lyShine, cmdArgs);
}
#endif

