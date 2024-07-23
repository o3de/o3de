/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(CARBONATED)

#include <AzFramework/API/ApplicationAPI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Time/ITime.h>
#include <CryCommon/ISystem.h>
#include "IConsole.h"

#include "LoadScreenComponent.h"

#if AZ_LOADSCREENCOMPONENT_ENABLED

namespace
{
    // Due to issues with DLLs sometimes there can be different values of gEnv in different DLLs.
    // So we use this preferred method of getting the global environment
    SSystemGlobalEnvironment* GetGlobalEnv()
    {
        if (!GetISystem())
        {
            return nullptr;
        }

        return GetISystem()->GetGlobalEnvironment();
    }

    static const char* const s_gameFixedFpsCvarName = "game_load_screen_sequence_fixed_fps";
    static const char* const s_gameMaxFpsCvarName = "game_load_screen_max_fps";
    static const char* const s_gameMinimumLoadTimeCvarName = "game_load_screen_minimum_time";

    static const char* const s_levelFixedFpsCvarName = "level_load_screen_sequence_fixed_fps";
    static const char* const s_levelMaxFpsCvarName = "level_load_screen_max_fps";
    static const char* const s_levelMinimumLoadTimeCvarName = "level_load_screen_minimum_time";

#if defined(CARBONATED)
    static const char* const s_levelLoadScreenUiCanvasPath = "level_load_screen_uicanvas_path";

    static const char* const s_globalLevelFixedFpsCvarName = "global_level_load_screen_sequence_fixed_fps";
    static const char* const s_globalLevelMaxFpsCvarName = "global_level_load_screen_max_fps";
    static const char* const s_globalLevelMinimumLoadTimeCvarName = "global_level_load_screen_minimum_time";
#endif
}

void LoadScreenComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<LoadScreenComponent, AZ::Component>()
            ->Version(1)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<LoadScreenComponent>(
                "Load screen manager", "Allows management of a load screen")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "Game")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
            ;
        }
    }
}

void LoadScreenComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("LoadScreenService", 0x901b031c));
}

void LoadScreenComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
{
    incompatible.push_back(AZ_CRC("LoadScreenService", 0x901b031c));
}

static float GetAsyncTime()
{
    const AZ::TimeMs timeMs = AZ::GetRealElapsedTimeMs();
    return AZ::TimeMsToSeconds(timeMs);
}

void LoadScreenComponent::Reset()
{
    m_loadScreenState = LoadScreenState::None;

    m_fixedDeltaTimeInSeconds = -1.0f;
    m_maxDeltaTimeInSeconds = -1.0f;
    m_previousCallTimeForUpdateAndRender = CTimeValue();
    m_processingLoadScreen.store(false);

    // Reset CVars so they're not carried over to other levels
    SSystemGlobalEnvironment* pGEnv = GetGlobalEnv();
    if (pGEnv && pGEnv->pConsole)
    {
        if (ICVar* var = pGEnv->pConsole->GetCVar(s_levelFixedFpsCvarName))
        {
            var->Set("");
        }
        if (ICVar* var = pGEnv->pConsole->GetCVar(s_levelMaxFpsCvarName))
        {
            var->Set("");
        }
        if (ICVar* var = pGEnv->pConsole->GetCVar(s_levelMinimumLoadTimeCvarName))
        {
            var->Set("");
        }
    }
}

void LoadScreenComponent::LoadConfigSettings(const char* fixedFpsVarName, const char* maxFpsVarName, const char* minimumLoadTimeVarName)
{
    m_fixedDeltaTimeInSeconds = -1.0f;
    m_maxDeltaTimeInSeconds = -1.0f;
    m_minimumLoadTimeInSeconds = 0.0f;

    SSystemGlobalEnvironment* pGEnv = GetGlobalEnv();
    if (pGEnv && pGEnv->pConsole)
    {
        ICVar* fixedFpsVar = pGEnv->pConsole->GetCVar(fixedFpsVarName);
        if (fixedFpsVar && fixedFpsVar->GetFVal() > 0.0f)
        {
            m_fixedDeltaTimeInSeconds = (1.0f / fixedFpsVar->GetFVal());
        }

        ICVar* maxFpsVar = pGEnv->pConsole->GetCVar(maxFpsVarName);
        if (maxFpsVar && maxFpsVar->GetFVal() > 0.0f)
        {
            m_maxDeltaTimeInSeconds = (1.0f / maxFpsVar->GetFVal());
        }

        if (ICVar* minimumLoadTimeVar = pGEnv->pConsole->GetCVar(minimumLoadTimeVarName))
        {
            // Never allow values below 0 seconds
            m_minimumLoadTimeInSeconds = AZStd::max<float>(minimumLoadTimeVar->GetFVal(), 0.0f);
        }
    }
}

void LoadScreenComponent::Init()
{
    Reset();
}

void LoadScreenComponent::Activate()
{
    CrySystemEventBus::Handler::BusConnect();
    LoadScreenBus::Handler::BusConnect(GetEntityId());
    AZ::AssetLoadNotification::AssetLoadNotificatorBus::Handler::BusConnect();
}

void LoadScreenComponent::Deactivate()
{
    AZ::AssetLoadNotification::AssetLoadNotificatorBus::Handler::BusDisconnect();
    LoadScreenBus::Handler::BusDisconnect(GetEntityId());
    CrySystemEventBus::Handler::BusDisconnect();
}

void LoadScreenComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
{
    SSystemGlobalEnvironment* pGEnv = system.GetGlobalEnvironment();

    // Can't use macros here because we have to use our pointer.
    if (pGEnv && pGEnv->pConsole)
    {
        pGEnv->pConsole->Register("ly_EnableLoadingThread", &m_loadingThreadEnabled, 0, VF_NULL,
            "EXPERIMENTAL. Enable fully threaded loading where the LoadingScreen is drawn on a thread that isn't loading data.");
    }

    if (pGEnv && !pGEnv->IsEditor())
    {
        // If not running from the editor, then run GameStart
        GameStart();
    }
}

void LoadScreenComponent::OnCrySystemShutdown(ISystem&)
{
}

void LoadScreenComponent::UpdateAndRender()
{
    if (m_loadScreenState == LoadScreenState::Showing)
    {
        AZ_Assert(GetCurrentThreadId() == GetGlobalEnv()->mMainThreadId, "UpdateAndRender should only be called from the main thread");

        // Throttling.
        if (!m_previousCallTimeForUpdateAndRender.GetValue())
        {
            // This is the first call to UpdateAndRender().
            m_previousCallTimeForUpdateAndRender = GetAsyncTime();
        }

        const CTimeValue callTimeForUpdateAndRender = GetAsyncTime();
        const float deltaTimeInSeconds = fabs((callTimeForUpdateAndRender - m_previousCallTimeForUpdateAndRender).GetSeconds());

        // Early-out: We DON'T need to execute UpdateAndRender() at a higher frequency than 30 FPS.
        const bool shouldThrottle = m_maxDeltaTimeInSeconds > 0.0f && deltaTimeInSeconds < m_maxDeltaTimeInSeconds;

        if (!shouldThrottle)
        {
            bool expectedValue = false;
            if (m_processingLoadScreen.compare_exchange_strong(expectedValue, true))
            {
                m_previousCallTimeForUpdateAndRender = callTimeForUpdateAndRender;

                const float updateDeltaTime = (m_fixedDeltaTimeInSeconds == -1.0f) ? deltaTimeInSeconds : m_fixedDeltaTimeInSeconds;

                LoadScreenUpdateNotificationBus::Broadcast(&LoadScreenUpdateNotificationBus::Events::UpdateAndRender, updateDeltaTime);

                // Some platforms (iOS, OSX, AppleTV) require system events to be pumped in order to update the screen
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);

                m_processingLoadScreen.store(false);
            }
        }
    }
}

void LoadScreenComponent::WaitForAssetUpdate()
{
    UpdateAndRender();
}

void LoadScreenComponent::GameStart()
{
    if (m_loadScreenState == LoadScreenState::None)
    {
        LoadConfigSettings(s_gameFixedFpsCvarName, s_gameMaxFpsCvarName, s_gameMinimumLoadTimeCvarName);

        const bool usingLoadingThread = IsLoadingThreadEnabled();

        AZ::EBusLogicalResult<bool, AZStd::logical_or<bool>> anyHandled(false);
        EBUS_EVENT_RESULT(anyHandled, LoadScreenNotificationBus, NotifyGameLoadStart, usingLoadingThread);

        if (anyHandled.value)
        {
            if (usingLoadingThread)
            {
                m_loadScreenState = LoadScreenState::ShowingMultiThreaded;

                AZ_ErrorOnce(nullptr, false, "StartLoadtimePlayback for loading thread is not supported as unused for the current game");
                //FIXME: GetGlobalEnv()->pRenderer->StartLoadtimePlayback(this);
            }
            else
            {
                m_loadScreenState = LoadScreenState::Showing;

                // Kick-start the first frame.
                UpdateAndRender();
            }

            m_lastStartTime = GetAsyncTime();
        }
    }
}

void LoadScreenComponent::LevelStart()
{
    if (m_loadScreenState == LoadScreenState::None)
    {
#if defined(CARBONATED)
        SSystemGlobalEnvironment* pGEnv = GetGlobalEnv();
        if (pGEnv && pGEnv->pConsole)
        {
            ICVar* levelPathVar = pGEnv->pConsole->GetCVar(s_levelLoadScreenUiCanvasPath);
            AZStd::string levelPath = levelPathVar ? levelPathVar->GetString() : "";

            if (levelPath != "")
            {
                LoadConfigSettings(s_levelFixedFpsCvarName, s_levelMaxFpsCvarName, s_levelMinimumLoadTimeCvarName);
            }
            else
            {
                LoadConfigSettings(s_globalLevelFixedFpsCvarName, s_globalLevelMaxFpsCvarName, s_globalLevelMinimumLoadTimeCvarName);
            }
        }

#else
        LoadConfigSettings(s_levelFixedFpsCvarName, s_levelMaxFpsCvarName, s_levelMinimumLoadTimeCvarName);
#endif

        const bool usingLoadingThread = IsLoadingThreadEnabled();

        AZ::EBusLogicalResult<bool, AZStd::logical_or<bool>> anyHandled(false);
        EBUS_EVENT_RESULT(anyHandled, LoadScreenNotificationBus, NotifyLevelLoadStart, usingLoadingThread);

        if (anyHandled.value)
        {
            if (usingLoadingThread)
            {
                m_loadScreenState = LoadScreenState::ShowingMultiThreaded;

                AZ_ErrorOnce(nullptr, false, "StartLoadtimePlayback for loading thread is not supported as unused for the current game");
                //FIXME: GetGlobalEnv()->pRenderer->StartLoadtimePlayback(this);
            }
            else
            {
                m_loadScreenState = LoadScreenState::Showing;

                // Kick-start the first frame.
                UpdateAndRender();
            }

            m_lastStartTime = GetAsyncTime();
        }
    }
}

void LoadScreenComponent::Pause()
{
    if (m_loadScreenState == LoadScreenState::Showing)
    {
        m_loadScreenState = LoadScreenState::Paused;
    }
    else if (m_loadScreenState == LoadScreenState::ShowingMultiThreaded)
    {
        m_loadScreenState = LoadScreenState::PausedMultithreaded;
    }
}

void LoadScreenComponent::Resume()
{
    if (m_loadScreenState == LoadScreenState::Paused)
    {
        m_loadScreenState = LoadScreenState::Showing;
    }
    else if (m_loadScreenState == LoadScreenState::PausedMultithreaded)
    {
        m_loadScreenState = LoadScreenState::ShowingMultiThreaded;
    }
}

void LoadScreenComponent::Stop()
{
    // If we were actually in a load screen, check if we need to wait longer.
    if (m_loadScreenState != LoadScreenState::None && m_minimumLoadTimeInSeconds > 0.0f)
    {
        CTimeValue currentTime = GetAsyncTime();
        float timeSinceStart = currentTime.GetDifferenceInSeconds(m_lastStartTime);

        while (timeSinceStart < m_minimumLoadTimeInSeconds)
        {
            // Simple loop that makes sure the loading screens update but also doesn't consume the whole core.

            if (m_loadScreenState == LoadScreenState::Showing)
            {
                EBUS_EVENT(LoadScreenBus, UpdateAndRender);
            }

            CrySleep(0);

            currentTime = GetAsyncTime();
            timeSinceStart = currentTime.GetDifferenceInSeconds(m_lastStartTime);
        }
    }

    if (m_loadScreenState == LoadScreenState::ShowingMultiThreaded)
    {
        // This will block until the other thread completes.
        AZ_ErrorOnce(nullptr, false, "StopLoadtimePlayback for loading thread is not supported as unused for the current game");
        // FIXME: GetGlobalEnv()->pRenderer->StopLoadtimePlayback();
    }

    if (m_loadScreenState != LoadScreenState::None)
    {
        EBUS_EVENT(LoadScreenNotificationBus, NotifyLoadEnd);
    }

    Reset();

    m_loadScreenState = LoadScreenState::None;
}

bool LoadScreenComponent::IsPlaying()
{
    return m_loadScreenState != LoadScreenState::None;
}

void LoadScreenComponent::LoadtimeUpdate(float deltaTime)
{
    if (m_loadScreenState == LoadScreenState::ShowingMultiThreaded)
    {
        EBUS_EVENT(LoadScreenUpdateNotificationBus, LoadThreadUpdate, deltaTime);
    }
}

void LoadScreenComponent::LoadtimeRender()
{
    if (m_loadScreenState == LoadScreenState::ShowingMultiThreaded)
    {
        EBUS_EVENT(LoadScreenUpdateNotificationBus, LoadThreadRender);
    }
}

#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
#endif // CARBONATED
