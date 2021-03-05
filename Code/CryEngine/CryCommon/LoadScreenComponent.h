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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/parallel/atomic.h>

#include <CrySystemBus.h>
#include <LoadScreenBus.h>
#include <IRenderer.h>

#if AZ_LOADSCREENCOMPONENT_ENABLED

/*
* This component is responsible for managing the load screen.
*/
class LoadScreenComponent
    : public AZ::Component
    , public CrySystemEventBus::Handler
    , public LoadScreenBus::Handler
    , public ILoadtimeCallback
{
public:
    AZ_COMPONENT(LoadScreenComponent, "{97CDBD6C-C621-4427-87C8-10E1B8F947FF}");

    LoadScreenComponent() = default;
    ~LoadScreenComponent() = default;

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component interface implementation
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    //////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    // CrySystemEvents
    void OnCrySystemInitialized(ISystem&, const SSystemInitParams& params) override;
    void OnCrySystemShutdown(ISystem&) override;
    ////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // LoadScreenBus interface implementation
    void UpdateAndRender() override;
    void GameStart() override;
    void LevelStart() override;
    void Pause() override;
    void Resume() override;
    void Stop() override;
    bool IsPlaying() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // ILoadtimeCallback interface implementation
    void LoadtimeUpdate(float deltaTime) override;
    void LoadtimeRender() override;
    //////////////////////////////////////////////////////////////////////////

    inline bool IsLoadingThreadEnabled() const
    {
        return m_loadingThreadEnabled != 0;
    }

protected:
    static void Reflect(AZ::ReflectContext* context);
    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

private:
    void Reset();
    void LoadConfigSettings(const char* fixedFpsVarName, const char* maxFpsVarName, const char* minimumLoadTimeVarName);

    //////////////////////////////////////////////////////////////////////////
    enum class LoadScreenState
    {
        None,
        Showing,
        ShowingMultiThreaded,
        Paused,
        PausedMultithreaded,
    };
    LoadScreenState m_loadScreenState{ LoadScreenState::None };

    float m_fixedDeltaTimeInSeconds{ -1.0f };
    float m_maxDeltaTimeInSeconds{ -1.0f };
    float m_minimumLoadTimeInSeconds{ 0.0f };

    CTimeValue m_lastStartTime;
    CTimeValue m_previousCallTimeForUpdateAndRender;
    AZStd::atomic_bool m_processingLoadScreen{ false };

    int32_t m_loadingThreadEnabled{ 0 };
    //////////////////////////////////////////////////////////////////////////
};

#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
