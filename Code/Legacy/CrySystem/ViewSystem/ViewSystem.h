/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

    typedef std::map<unsigned int, IView*> TViewMap;
    typedef std::vector<unsigned int> TViewIdVector;

public:

    //IViewSystem
    virtual IView* CreateView();
    virtual unsigned int AddView(IView* pView) override;
    virtual void RemoveView(IView* pView);
    virtual void RemoveView(unsigned int viewId);

    virtual void SetActiveView(IView* pView);
    virtual void SetActiveView(unsigned int viewId);

    //CameraSystemRequestBus
    AZ::EntityId GetActiveCamera() override { return m_activeViewId ? GetActiveView()->GetLinkedId() : AZ::EntityId(); }

    //utility functions
    virtual IView* GetView(unsigned int viewId);
    virtual IView* GetActiveView();

    virtual unsigned int GetViewId(IView* pView);
    virtual unsigned int GetActiveViewId();

    virtual void Serialize(TSerialize ser);
    virtual void PostSerialize();

    virtual IView* GetViewByEntityId(const AZ::EntityId& id, bool forceCreate);

    virtual float GetDefaultZNear() { return m_fDefaultCameraNearZ; };
    virtual void SetBlendParams(float fBlendPosSpeed, float fBlendRotSpeed, bool performBlendOut) { m_fBlendInPosSpeed = fBlendPosSpeed; m_fBlendInRotSpeed = fBlendRotSpeed; m_bPerformBlendOut = performBlendOut; };
    virtual void SetOverrideCameraRotation(bool bOverride, Quat rotation);
    virtual bool IsPlayingCutScene() const
    {
        return m_cutsceneCount > 0;
    }
    virtual void UpdateSoundListeners();

    virtual void SetDeferredViewSystemUpdate(bool const bDeferred){ m_useDeferredViewSystemUpdate = bDeferred; }
    virtual bool UseDeferredViewSystemUpdate() const { return m_useDeferredViewSystemUpdate; }
    virtual void SetControlAudioListeners(bool const bActive);
    //~IViewSystem

    //IMovieUser
    virtual void SetActiveCamera(const SCameraParams& Params);
    virtual void BeginCutScene(IAnimSequence* pSeq, unsigned long dwFlags, bool bResetFX);
    virtual void EndCutScene(IAnimSequence* pSeq, unsigned long dwFlags);
    virtual void SendGlobalEvent(const char* pszEvent);
    //~IMovieUser

    // ILevelSystemListener
    virtual void OnLevelNotFound([[maybe_unused]] const char* levelName) {};
    virtual void OnLoadingStart([[maybe_unused]] const char* levelName);
    virtual void OnLoadingComplete([[maybe_unused]] const char* levelName){};
    virtual void OnLoadingError([[maybe_unused]] const char* levelName, [[maybe_unused]] const char* error){};
    virtual void OnLoadingProgress([[maybe_unused]] const char* levelName, [[maybe_unused]] int progressAmount){};
    virtual void OnUnloadComplete([[maybe_unused]] const char* levelName);
    //~ILevelSystemListener

    CViewSystem(ISystem* pSystem);
    ~CViewSystem();

    void Release() override { delete this; };
    void Update(float frameTime) override;

    virtual void ForceUpdate(float elapsed) { Update(elapsed); }

    //void RegisterViewClass(const char *name, IView *(*func)());

    bool AddListener(IViewSystemListener* pListener)
    {
        return stl::push_back_unique(m_listeners, pListener);
    }

    bool RemoveListener(IViewSystemListener* pListener)
    {
        return stl::find_and_erase(m_listeners, pListener);
    }

    void GetMemoryUsage(ICrySizer* s) const;

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
