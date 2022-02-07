/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : View System interfaces.

#pragma once

#include "View.h"
#include "IMovieSystem.h"
#include <ILevelSystem.h>
#include <AzFramework/Components/CameraBus.h>

namespace LegacyViewSystem
{

class DebugCamera;

class CViewSystem
    : public IViewSystem
    , public IMovieUser
    , public ILevelSystemListener
    , public Camera::CameraSystemRequestBus::Handler
{
private:

    using TViewMap = std::map<unsigned int, IView*>;
    using TViewIdVector = std::vector<unsigned int>;

public:

    //IViewSystem
    IView* CreateView() override;
    unsigned int AddView(IView* pView) override;
    void RemoveView(IView* pView) override;
    void RemoveView(unsigned int viewId) override;

    void SetActiveView(IView* pView) override;
    void SetActiveView(unsigned int viewId) override;

    //CameraSystemRequestBus
    AZ::EntityId GetActiveCamera() override { return m_activeViewId ? GetActiveView()->GetLinkedId() : AZ::EntityId(); }

    //utility functions
    IView* GetView(unsigned int viewId) override;
    IView* GetActiveView() override;

    unsigned int GetViewId(IView* pView) override;
    unsigned int GetActiveViewId() override;

    void PostSerialize() override;

    IView* GetViewByEntityId(const AZ::EntityId& id, bool forceCreate) override;

    float GetDefaultZNear() override { return m_fDefaultCameraNearZ; };
    void SetBlendParams(float fBlendPosSpeed, float fBlendRotSpeed, bool performBlendOut) override { m_fBlendInPosSpeed = fBlendPosSpeed; m_fBlendInRotSpeed = fBlendRotSpeed; m_bPerformBlendOut = performBlendOut; };
    void SetOverrideCameraRotation(bool bOverride, Quat rotation) override;
    bool IsPlayingCutScene() const override
    {
        return m_cutsceneCount > 0;
    }
    void SetDeferredViewSystemUpdate(bool const bDeferred) override{ m_useDeferredViewSystemUpdate = bDeferred; }
    bool UseDeferredViewSystemUpdate() const override { return m_useDeferredViewSystemUpdate; }
    void SetControlAudioListeners(bool const bActive) override;
    //~IViewSystem

    //IMovieUser
    void SetActiveCamera(const SCameraParams& Params) override;
    void BeginCutScene(IAnimSequence* pSeq, unsigned long dwFlags, bool bResetFX) override;
    void EndCutScene(IAnimSequence* pSeq, unsigned long dwFlags) override;
    void SendGlobalEvent(const char* pszEvent) override;
    //~IMovieUser

    // ILevelSystemListener
    void OnLevelNotFound([[maybe_unused]] const char* levelName) override {};
    void OnLoadingStart([[maybe_unused]] const char* levelName) override;
    void OnLoadingComplete([[maybe_unused]] const char* levelName) override{};
    void OnLoadingError([[maybe_unused]] const char* levelName, [[maybe_unused]] const char* error) override{};
    void OnLoadingProgress([[maybe_unused]] const char* levelName, [[maybe_unused]] int progressAmount) override{};
    void OnUnloadComplete([[maybe_unused]] const char* levelName) override;
    //~ILevelSystemListener

    CViewSystem(ISystem* pSystem);
    ~CViewSystem();

    void Release() override { delete this; };
    void Update(float frameTime) override;

    void ForceUpdate(float elapsed) override { Update(elapsed); }

    //void RegisterViewClass(const char *name, IView *(*func)());

    bool AddListener(IViewSystemListener* pListener) override
    {
        return stl::push_back_unique(m_listeners, pListener);
    }

    bool RemoveListener(IViewSystemListener* pListener) override
    {
        return stl::find_and_erase(m_listeners, pListener);
    }

    void ClearAllViews();

private:

    void RemoveViewById(unsigned int viewId);
    void ClearCutsceneViews();
    void DebugDraw();

    ISystem* m_pSystem;

    //TViewClassMap m_viewClasses;
    TViewMap m_views;

    // Listeners
    std::vector<IViewSystemListener*> m_listeners;

    unsigned int m_activeViewId;
    unsigned int m_nextViewIdToAssign;  // next id which will be assigned
    unsigned int m_preSequenceViewId; // viewId before a movie cam dropped in

    unsigned int m_cutsceneViewId;
    unsigned int m_cutsceneCount;

    bool m_bActiveViewFromSequence;

    bool m_bOverridenCameraRotation;
    Quat m_overridenCameraRotation;
    float m_fCameraNoise;
    float m_fCameraNoiseFrequency;

    float m_fDefaultCameraNearZ;
    float m_fBlendInPosSpeed;
    float m_fBlendInRotSpeed;
    bool m_bPerformBlendOut;
    int m_nViewSystemDebug;

    bool m_useDeferredViewSystemUpdate;
    bool m_bControlsAudioListeners;

public:
    static DebugCamera* s_debugCamera;
};

} // namespace LegacyViewSystem
