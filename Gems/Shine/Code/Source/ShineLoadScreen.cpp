/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShineLoadScreen.h"

#if AZ_LOADSCREENCOMPONENT_ENABLED

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <Shine/Bus/UiCanvasBus.h>
#include <Shine/Animation/IUiAnimation.h>

namespace Shine
{
    void ShineLoadScreenComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ShineLoadScreenComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void ShineLoadScreenComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.emplace_back(AZ_CRC_CE("ShineLoadScreenService"));
    }

    void ShineLoadScreenComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.emplace_back(AZ_CRC_CE("ShineLoadScreenService"));
    }

    void ShineLoadScreenComponent::Init()
    {
    }

    void ShineLoadScreenComponent::Activate()
    {
        LoadScreenNotificationBus::Handler::BusConnect();
    }

    void ShineLoadScreenComponent::Deactivate()
    {
        LoadScreenUpdateNotificationBus::Handler::BusDisconnect();
        LoadScreenNotificationBus::Handler::BusDisconnect();
    }

    bool ShineLoadScreenComponent::NotifyGameLoadStart(bool usingLoadingThread)
    {
        // Shine does not support the loading thread yet.
        if (usingLoadingThread)
        {
            return false;
        }
        //TODO: gEnv->pRenderer is always null, fix the logic below
        AZ_ErrorOnce(AZ::Debug::Trace::GetDefaultSystemWindow(), false, "NotifyGameLoadStart needs to be removed/ported to use Atom");
        return false;
#if 0
        if (!gEnv || gEnv->pRenderer || !AZ::Interface<IShine>::Get())
        {
            return false;
        }

        AZ_Assert(!m_isPlaying, "ShineLoadScreen was somehow started before the engine loaded.");
        if (m_isPlaying)
        {
            return false;
        }

        if (!m_gameCanvasEntityId.IsValid())
        {
            // Load canvas.
            m_gameCanvasEntityId = loadFromCfg("game_load_screen_uicanvas_path", "game_load_screen_sequence_to_auto_play");
        }

        m_isPlaying = m_gameCanvasEntityId.IsValid();

        if (m_isPlaying)
        {
            LoadScreenUpdateNotificationBus::Handler::BusConnect();
        }

        return m_isPlaying;
#endif
    }

    bool ShineLoadScreenComponent::NotifyLevelLoadStart(bool usingLoadingThread)
    {
        // Shine does not support the loading thread yet.
        if (usingLoadingThread)
        {
            return false;
        }

        AZ_ErrorOnce(AZ::Debug::Trace::GetDefaultSystemWindow(), false, "NotifyLevelLoadStart needs to be removed/ported to use Atom");
        return false;
        //TODO: gEnv->pRenderer is always null, fix the logic below
#if 0
        if (!gEnv || gEnv->pRenderer || !AZ::Interface<IShine>::Get())
        {
            return false;
        }

        AZ_Assert(!m_isPlaying, "ShineLoadScreen was not stopped before another level load started.");
        AZ_Assert(!m_gameCanvasEntityId.IsValid(), "ShineLoadScreen game load canvas was not unloaded before a level load started.");
        if (m_isPlaying || m_gameCanvasEntityId.IsValid())
        {
            return false;
        }

        if (!m_levelCanvasEntityId.IsValid())
        {
            // Load canvas.
            m_levelCanvasEntityId = loadFromCfg("level_load_screen_uicanvas_path", "level_load_screen_sequence_to_auto_play");
        }

        m_isPlaying = m_levelCanvasEntityId.IsValid();

        if (m_isPlaying)
        {
            LoadScreenUpdateNotificationBus::Handler::BusConnect();
        }

        return m_isPlaying;
#endif
    }

    void ShineLoadScreenComponent::NotifyLoadEnd()
    {
        Reset();
    }

    void ShineLoadScreenComponent::UpdateAndRender([[maybe_unused]] float deltaTimeInSeconds)
    {
        AZ_Assert(m_isPlaying, "ShineLoadScreenComponent should not be connected to LoadScreenUpdateNotificationBus while not playing");
        AZ_ErrorOnce(AZ::Debug::Trace::GetDefaultSystemWindow(), m_isPlaying && AZ::Interface<IShine>::Get(), "UpdateAndRender needs to be removed/ported to use Atom");

        //TODO: gEnv->pRenderer is always null, fix the logic below
#if 0
        if (m_isPlaying && gEnv && AZ::Interface<IShine>::Get() && gEnv->pRenderer)
        {
            AZ_Assert(GetCurrentThreadId() == gEnv->mMainThreadId, "UpdateAndRender should only be called from the main thread");

            // update the animation system
            AZ::Interface<IShine>::Get()->Update(deltaTimeInSeconds);

            // Render.
            gEnv->pRenderer->SetViewport(0, 0, gEnv->pRenderer->GetOverlayWidth(), gEnv->pRenderer->GetOverlayHeight());

            gEnv->pRenderer->BeginFrame();
            AZ::Interface<IShine>::Get()->Render();
            gEnv->pRenderer->EndFrame();
        }
#endif
    }

    void ShineLoadScreenComponent::LoadThreadUpdate([[maybe_unused]] float deltaTimeInSeconds)
    {
        AZ_Assert(false, "Shine does not support running on the loading thread yet.")
    }

    void ShineLoadScreenComponent::LoadThreadRender()
    {
        AZ_Assert(false, "Shine does not support running on the loading thread yet.")
    }

    void ShineLoadScreenComponent::Reset()
    {
        LoadScreenUpdateNotificationBus::Handler::BusDisconnect();

        m_isPlaying = false;

        if (AZ::Interface<IShine>::Get())
        {
            AZ::Entity* canvasEntity = nullptr;

            // Release the game canvas.
            if (m_gameCanvasEntityId.IsValid())
            {
                AZ::ComponentApplicationBus::BroadcastResult(
                    canvasEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_gameCanvasEntityId);
                if (canvasEntity)
                {
                    AZ::Interface<IShine>::Get()->ReleaseCanvas(m_gameCanvasEntityId, false);
                    AZ::Interface<IShine>::Get()->OnLoadScreenUnloaded();
                }
            }

            // Release the level canvas.
            if (m_levelCanvasEntityId.IsValid())
            {
                AZ::ComponentApplicationBus::BroadcastResult(
                    canvasEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_levelCanvasEntityId);
                if (canvasEntity)
                {
                    AZ::Interface<IShine>::Get()->ReleaseCanvas(m_levelCanvasEntityId, false);
                    AZ::Interface<IShine>::Get()->OnLoadScreenUnloaded();
                }
            }
        }

        m_gameCanvasEntityId.SetInvalid();
        m_levelCanvasEntityId.SetInvalid();

        // Reset CVars so they're not carried over to other levels
        auto ClearCvar = [](const char* cvarName)
        {
            if (gEnv && gEnv->pConsole)
            {
                if (ICVar* var = gEnv->pConsole->GetCVar(cvarName))
                {
                    var->Set("");
                }
            }
        };

        ClearCvar("level_load_screen_uicanvas_path");
        ClearCvar("level_load_screen_sequence_to_auto_play");
    }

    AZ::EntityId ShineLoadScreenComponent::loadFromCfg(const char* pathVarName, const char* autoPlayVarName)
    {
        ICVar* pathVar = gEnv->pConsole->GetCVar(pathVarName);
        AZStd::string path = pathVar ? pathVar->GetString() : "";
        if (path.empty())
        {
            // No canvas specified.
            Reset();
            return AZ::EntityId();
        }

        AZ::EntityId canvasId = AZ::Interface<IShine>::Get()->LoadCanvas(path);
        AZ_Warning("LoadScreenComponent", canvasId.IsValid(), "Can't load canvas: %s", path.c_str());
        if (!canvasId.IsValid())
        {
            // Error loading canvas.
            Reset();
            return AZ::EntityId();
        }

        UiCanvasBus::Event(canvasId, &UiCanvasBus::Events::SetKeepLoadedOnLevelUnload, true);

        // Set the load screen draw order so it renders in front of other canvases that may load during the level load
        UiCanvasBus::Event(canvasId, &UiCanvasBus::Events::SetDrawOrder, std::numeric_limits<int>::max());

        ICVar* autoPlayVar = gEnv->pConsole->GetCVar(autoPlayVarName);
        AZStd::string sequence = autoPlayVar ? autoPlayVar->GetString() : "";
        if (sequence.empty())
        {
            // Nothing to auto-play.
            return canvasId;
        }

        IUiAnimationSystem* animSystem = nullptr;
        UiCanvasBus::EventResult(animSystem, canvasId, &UiCanvasBus::Events::GetAnimationSystem);
        if (!animSystem)
        {
            // Nothing can be auto-played.
            return canvasId;
        }

        animSystem->PlaySequence(sequence.c_str(), nullptr, false, false);

        return canvasId;
    }

} // namespace Shine

#endif // AZ_LOADSCREENCOMPONENT_ENABLED

