/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"

#include <AzCore/Component/ComponentApplicationBus.h>

#include <Cry_Camera.h>
#include <ILevelSystem.h>
#include "ViewSystem.h"
#include "PNoise3.h"
#include "DebugCamera.h"
#include <MathConversion.h>

#include <AzCore/Casting/lossy_cast.h>

#define VS_CALL_LISTENERS(func)                                                                                     \
    {                                                                                                               \
        size_t count = m_listeners.size();                                                                          \
        if (count > 0)                                                                                              \
        {                                                                                                           \
            const size_t memSize = count * sizeof(IViewSystemListener*);                                            \
            IViewSystemListener* *pArray = (IViewSystemListener**) alloca(memSize);                                 \
            memcpy(pArray, &*m_listeners.begin(), memSize);                                                         \
            while (count--)                                                                                         \
            {                                                                                                       \
                (*pArray)->func; ++pArray;                                                                          \
            }                                                                                                       \
        }                                                                                                           \
    }

namespace LegacyViewSystem
{

void ToggleDebugCamera([[maybe_unused]] IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE)
    if (!gEnv->IsDedicated())
    {
        DebugCamera* debugCamera = CViewSystem::s_debugCamera;
        if (debugCamera)
        {
            if (!debugCamera->IsEnabled())
            {
                debugCamera->OnEnable();
            }
            else
            {
                debugCamera->OnNextMode();
            }
        }
    }
#endif
}

void ToggleDebugCameraInvertY([[maybe_unused]] IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE)
    if (!gEnv->IsDedicated())
    {
        DebugCamera* debugCamera = CViewSystem::s_debugCamera;
        if (debugCamera)
        {
            debugCamera->OnInvertY();
        }
    }
#endif
}

void DebugCameraMove([[maybe_unused]] IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE)
    if (!gEnv->IsDedicated())
    {
        if (pArgs->GetArgCount() != 4)
        {
            CryLogAlways("debugCameraMove requires 3 args, not %d.", pArgs->GetArgCount() - 1);
            return;
        }

        DebugCamera* debugCamera = CViewSystem::s_debugCamera;
        if (debugCamera && debugCamera->IsFree())
        {
            Vec3::value_type x = azlossy_cast<float>(atof(pArgs->GetArg(1)));
            Vec3::value_type y = azlossy_cast<float>(atof(pArgs->GetArg(2)));
            Vec3::value_type z = azlossy_cast<float>(atof(pArgs->GetArg(3)));
            Vec3 newPos(x, y, z);
            debugCamera->MovePosition(newPos);
        }
    }
#endif
}

DebugCamera* CViewSystem::s_debugCamera = nullptr;

//------------------------------------------------------------------------
CViewSystem::CViewSystem(ISystem* pSystem)
    : m_pSystem(pSystem)
    , m_activeViewId(0)
    , m_nextViewIdToAssign(1000)
    , m_preSequenceViewId(0)
    , m_cutsceneViewId(0)
    , m_cutsceneCount(0)
    , m_bOverridenCameraRotation(false)
    , m_bActiveViewFromSequence(false)
    , m_fBlendInPosSpeed(0.0f)
    , m_fBlendInRotSpeed(0.0f)
    , m_bPerformBlendOut(false)
    , m_useDeferredViewSystemUpdate(false)
    , m_bControlsAudioListeners(true)
{
#if !defined(_RELEASE)
    if (!gEnv->IsDedicated())
    {
        if (!s_debugCamera)
        {
            s_debugCamera = new DebugCamera;
        }

        REGISTER_COMMAND("debugCameraToggle", ToggleDebugCamera, VF_DEV_ONLY, "Toggle the debug camera.\n");
        REGISTER_COMMAND("debugCameraInvertY", ToggleDebugCameraInvertY, VF_DEV_ONLY, "Toggle debug camera Y-axis inversion.\n");
        REGISTER_COMMAND("debugCameraMove", DebugCameraMove, VF_DEV_ONLY, "Move the debug camera the specified distance (x y z).\n");
        gEnv->pConsole->CreateKeyBind("ctrl_keyboard_key_punctuation_backslash", "debugCameraToggle");
        gEnv->pConsole->CreateKeyBind("alt_keyboard_key_punctuation_backslash", "debugCameraInvertY");
    }
#endif

    REGISTER_CVAR2("cl_camera_noise", &m_fCameraNoise, -1, 0,
        "Adds hand-held like camera noise to the camera view. \n The higher the value, the higher the noise.\n A value <= 0 disables it.");
    REGISTER_CVAR2("cl_camera_noise_freq", &m_fCameraNoiseFrequency, 2.5326173f, 0,
        "Defines camera noise frequency for the camera view. \n The higher the value, the higher the noise.");

    REGISTER_CVAR2("cl_ViewSystemDebug", &m_nViewSystemDebug, 0, VF_CHEAT,
        "Sets Debug information of the ViewSystem.");

    REGISTER_CVAR2("cl_DefaultNearPlane", &m_fDefaultCameraNearZ, DEFAULT_NEAR, VF_CHEAT,
        "The default camera near plane. ");

    //Register as level system listener
    if (m_pSystem->GetILevelSystem())
    {
        m_pSystem->GetILevelSystem()->AddListener(this);
    }

    Camera::CameraSystemRequestBus::Handler::BusConnect();
}

//------------------------------------------------------------------------
CViewSystem::~CViewSystem()
{
    Camera::CameraSystemRequestBus::Handler::BusDisconnect();

    ClearAllViews();

    IConsole* pConsole = gEnv->pConsole;
    CRY_ASSERT(pConsole);
    pConsole->UnregisterVariable("cl_camera_noise", true);
    pConsole->UnregisterVariable("cl_camera_noise_freq", true);
    pConsole->UnregisterVariable("cl_ViewSystemDebug", true);
    pConsole->UnregisterVariable("cl_DefaultNearPlane", true);

    //Remove as level system listener
    if (m_pSystem->GetILevelSystem())
    {
        m_pSystem->GetILevelSystem()->RemoveListener(this);
    }

#if !defined(_RELEASE)
    if (!gEnv->IsDedicated())
    {
        UNREGISTER_COMMAND("debugCameraToggle");
        UNREGISTER_COMMAND("debugCameraInvertY");
        UNREGISTER_COMMAND("debugCameraMove");

        if (s_debugCamera)
        {
            delete s_debugCamera;
            s_debugCamera = nullptr;
        }
    }
#endif
}

//------------------------------------------------------------------------
void CViewSystem::Update(float frameTime)
{
    if (gEnv->IsDedicated())
    {
        return;
    }

    if (s_debugCamera)
    {
        s_debugCamera->Update();
    }

    CView* const pActiveView = static_cast<CView*>(GetActiveView());

    TViewMap::const_iterator Iter(m_views.begin());
    TViewMap::const_iterator const IterEnd(m_views.end());

    for (; Iter != IterEnd; ++Iter)
    {
        IView* const pView = Iter->second;

        bool const bIsActive = (pView == pActiveView);

        pView->Update(frameTime, bIsActive);

        if (bIsActive)
        {
            CCamera& rCamera = pView->GetCamera();
            if (const SViewParams* currentParams = pView->GetCurrentParams())
            {
                SViewParams copyCurrentParams = *currentParams;
                rCamera.SetJustActivated(copyCurrentParams.justActivated);

                copyCurrentParams.justActivated = false;
                pView->SetCurrentParams(copyCurrentParams);
            }

            if (m_bOverridenCameraRotation)
            {
                // When camera rotation is overridden.
                Vec3 pos = rCamera.GetMatrix().GetTranslation();
                Matrix34 camTM(m_overridenCameraRotation);
                camTM.SetTranslation(pos);
                rCamera.SetMatrix(camTM);
            }
            else
            {
                // Normal setting of the camera

                if (m_fCameraNoise > 0)
                {
                    Matrix33 m = Matrix33(rCamera.GetMatrix());
                    m.OrthonormalizeFast();
                    Ang3 aAng1 = Ang3::GetAnglesXYZ(m);
                    //Ang3 aAng2 = RAD2DEG(aAng1);

                    Matrix34 camTM = rCamera.GetMatrix();
                    Vec3 pos = camTM.GetTranslation();
                    camTM.SetIdentity();

                    const float fScale = 0.1f;
                    CPNoise3* pNoise = m_pSystem->GetNoiseGen();
                    float fRes = pNoise->Noise1D(gEnv->pTimer->GetCurrTime() * m_fCameraNoiseFrequency);
                    aAng1.x += fRes * m_fCameraNoise * fScale;
                    pos.z -= fRes * m_fCameraNoise * fScale;
                    fRes = pNoise->Noise1D(17 + gEnv->pTimer->GetCurrTime() * m_fCameraNoiseFrequency);
                    aAng1.y -= fRes * m_fCameraNoise * fScale;

                    //aAng1.z+=fRes*0.025f; // left / right movement should be much less visible

                    camTM.SetRotationXYZ(aAng1);
                    camTM.SetTranslation(pos);
                    rCamera.SetMatrix(camTM);
                }
            }

            AZ_ErrorOnce("CryLegacy", false, "CryLegacy view system no longer available (CViewSystem::Update)");
        }
    }

    if (s_debugCamera)
    {
        s_debugCamera->PostUpdate();
    }

    // Display debug info on screen
    if (m_nViewSystemDebug)
    {
        DebugDraw();
    }
}

//------------------------------------------------------------------------
IView* CViewSystem::CreateView()
{
    CView* newView = new CView(m_pSystem);

    if (newView)
    {
        AddView(newView);
    }

    return newView;
}

unsigned int CViewSystem::AddView(IView* pView)
{
    assert(pView);

    m_views.insert(TViewMap::value_type(m_nextViewIdToAssign, pView));
    return m_nextViewIdToAssign++;
}

void CViewSystem::RemoveView(IView* pView)
{
    RemoveViewById(GetViewId(pView));
}

void CViewSystem::RemoveView(unsigned int viewId)
{
    RemoveViewById(viewId);
}

void CViewSystem::RemoveViewById(unsigned int viewId)
{
    TViewMap::iterator iter = m_views.find(viewId);

    if (iter != m_views.end())
    {
        if (viewId == m_activeViewId)
        {
            m_activeViewId = 0;
        }
        if (viewId == m_preSequenceViewId)
        {
            m_preSequenceViewId = 0;
        }
        SAFE_RELEASE(iter->second);
        m_views.erase(iter);
    }
}

//------------------------------------------------------------------------
void CViewSystem::SetActiveView(IView* pView)
{
    if (pView != NULL)
    {
        IView* const pPrevView = GetView(m_activeViewId);

        if (pPrevView != pView)
        {
            if (pPrevView != NULL)
            {
                pPrevView->SetActive(false);
            }

            pView->SetActive(true);
            m_activeViewId = GetViewId(pView);
        }
    }
    else
    {
        m_activeViewId = ~0u;
    }

    m_bActiveViewFromSequence = false;
}

//------------------------------------------------------------------------
void CViewSystem::SetActiveView(unsigned int viewId)
{
    IView* const pPrevView = GetView(m_activeViewId);

    if (pPrevView != NULL)
    {
        pPrevView->SetActive(false);
    }

    IView* const pView = GetView(viewId);

    if (pView != NULL)
    {
        pView->SetActive(true);
        m_activeViewId = viewId;
        m_bActiveViewFromSequence = false;
    }
}

//------------------------------------------------------------------------
IView* CViewSystem::GetView(unsigned int viewId)
{
    TViewMap::iterator it = m_views.find(viewId);

    if (it != m_views.end())
    {
        return it->second;
    }

    return NULL;
}

//------------------------------------------------------------------------
IView* CViewSystem::GetActiveView()
{
    return GetView(m_activeViewId);
}

//------------------------------------------------------------------------
unsigned int CViewSystem::GetViewId(IView* pView)
{
    for (TViewMap::iterator it = m_views.begin(); it != m_views.end(); ++it)
    {
        IView* tView = it->second;

        if (tView == pView)
        {
            return it->first;
        }
    }

    return 0;
}

//------------------------------------------------------------------------
unsigned int CViewSystem::GetActiveViewId()
{
    // cutscene can override the games id of the active view
    if (m_cutsceneCount && m_cutsceneViewId)
    {
        return m_cutsceneViewId;
    }
    return m_activeViewId;
}

//------------------------------------------------------------------------
IView* CViewSystem::GetViewByEntityId(const AZ::EntityId& id, bool forceCreate)
{
    for (TViewMap::iterator it = m_views.begin(); it != m_views.end(); ++it)
    {
        IView* tView = it->second;

        if (tView && tView->GetLinkedId() == id)
        {
            return tView;
        }
    }

    if (forceCreate)
    {
        // Component Camera
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
        if (entity)
        {
            if (IView* pNew = CreateView())
            {
                pNew->LinkTo(entity);
                return pNew;
            }
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------
void CViewSystem::SetActiveCamera(const SCameraParams& params)
{
    IView* pView = NULL;

    if (params.cameraEntityId.IsValid())
    {
        pView = GetViewByEntityId(params.cameraEntityId, true);
        if (pView)
        {
            SViewParams viewParams = *pView->GetCurrentParams();
            viewParams.fov = params.fov;
            viewParams.nearplane = params.nearZ;

            if (m_bActiveViewFromSequence == false && m_preSequenceViewId == 0)
            {
                m_preSequenceViewId = m_activeViewId;
                IView* pPrevView = GetView(m_activeViewId);
                if (pPrevView && m_fBlendInPosSpeed > 0.0f && m_fBlendInRotSpeed > 0.0f)
                {
                    viewParams.blendPosSpeed = m_fBlendInPosSpeed;
                    viewParams.blendRotSpeed = m_fBlendInRotSpeed;
                    viewParams.BlendFrom(*pPrevView->GetCurrentParams());
                }
            }

            if (m_activeViewId != GetViewId(pView) && params.justActivated)
            {
                viewParams.justActivated = true;
            }

            pView->SetCurrentParams(viewParams);
            // make this one the active view
            SetActiveView(pView);
            m_bActiveViewFromSequence = true;
        }
    }
    else
    {
        if (m_preSequenceViewId != 0)
        {
            // Restore m_preSequenceViewId view

            IView* pActiveView = GetView(m_activeViewId);
            IView* pNewView = GetView(m_preSequenceViewId);
            if (pActiveView && pNewView && m_bPerformBlendOut)
            {
                SViewParams activeViewParams = *pActiveView->GetCurrentParams();
                SViewParams newViewParams = *pNewView->GetCurrentParams();
                newViewParams.BlendFrom(activeViewParams);
                newViewParams.blendPosSpeed = activeViewParams.blendPosSpeed;
                newViewParams.blendRotSpeed = activeViewParams.blendRotSpeed;

                if (m_activeViewId != m_preSequenceViewId && params.justActivated)
                {
                    newViewParams.justActivated = true;
                }

                pNewView->SetCurrentParams(newViewParams);
                SetActiveView(m_preSequenceViewId);
            }
            else if (pActiveView && m_activeViewId != m_preSequenceViewId && params.justActivated)
            {
                SViewParams activeViewParams = *pActiveView->GetCurrentParams();
                activeViewParams.justActivated = true;

                if (pNewView)
                {
                    pNewView->SetCurrentParams(activeViewParams);
                    SetActiveView(m_preSequenceViewId);
                }
            }

            m_preSequenceViewId = 0;
            m_bActiveViewFromSequence = false;
        }
    }
    m_cutsceneViewId = GetViewId(pView);

    VS_CALL_LISTENERS(OnCameraChange(params));
}

//------------------------------------------------------------------------
void CViewSystem::BeginCutScene(IAnimSequence* pSeq, [[maybe_unused]] unsigned long dwFlags, bool bResetFX)
{
    m_cutsceneCount++;

    VS_CALL_LISTENERS(OnBeginCutScene(pSeq, bResetFX));
}

//------------------------------------------------------------------------
void CViewSystem::EndCutScene(IAnimSequence* pSeq, [[maybe_unused]] unsigned long dwFlags)
{
    m_cutsceneCount -= (m_cutsceneCount > 0);

    ClearCutsceneViews();

    VS_CALL_LISTENERS(OnEndCutScene(pSeq));
}

void CViewSystem::SendGlobalEvent([[maybe_unused]] const char* pszEvent)
{
    // TODO: broadcast to script system
}

//////////////////////////////////////////////////////////////////////////
void CViewSystem::SetOverrideCameraRotation(bool bOverride, Quat rotation)
{
    m_bOverridenCameraRotation = bOverride;
    m_overridenCameraRotation = rotation;
}

//////////////////////////////////////////////////////////////////
void CViewSystem::OnLoadingStart([[maybe_unused]] const char* levelName)
{
    //If the level is being restarted (IsSerializingFile() == 1)
    //views should not be cleared, because the main view (player one) won't be recreated in this case
    //Views will only be cleared when loading a new map, or loading a saved game (IsSerizlizingFile() == 2)
    bool shouldClearViews = gEnv->pSystem ? (gEnv->pSystem->IsSerializingFile() != 1) : false;

    if (shouldClearViews)
    {
        ClearAllViews();
    }
}

/////////////////////////////////////////////////////////////////////
void CViewSystem::OnUnloadComplete([[maybe_unused]] const char* levelName)
{
    bool shouldClearViews = gEnv->pSystem ? (gEnv->pSystem->IsSerializingFile() != 1) : false;

    if (shouldClearViews)
    {
        ClearAllViews();
    }

    assert(m_listeners.empty());
    stl::free_container(m_listeners);
}

/////////////////////////////////////////////////////////////////////
void CViewSystem::ClearCutsceneViews()
{
    //First switch to previous camera if available
    //In practice, the camera should be already restored before reaching this point, but just in case.
    if (m_preSequenceViewId != 0)
    {
        SCameraParams camParams;
        camParams.cameraEntityId.SetInvalid();  //Setting to invalid will try to switch to previous camera
        camParams.fov = 60.0f;
        camParams.nearZ = DEFAULT_NEAR;
        camParams.justActivated = true;
        SetActiveCamera(camParams);
    }
}

///////////////////////////////////////////
void CViewSystem::ClearAllViews()
{
    TViewMap::iterator end = m_views.end();
    for (TViewMap::iterator it = m_views.begin(); it != end; ++it)
    {
        SAFE_RELEASE(it->second);
    }
    stl::free_container(m_views);
    m_preSequenceViewId = 0;
    m_activeViewId = 0;
}

////////////////////////////////////////////////////////////////////
void CViewSystem::DebugDraw()
{
}

//////////////////////////////////////////////////////////////////////////
void CViewSystem::PostSerialize()
{
    TViewMap::iterator iter = m_views.begin();
    TViewMap::iterator iterEnd = m_views.end();
    while (iter != iterEnd)
    {
        iter->second->PostSerialize();
        ++iter;
    }
}

///////////////////////////////////////////////////////////////////////////
void CViewSystem::SetControlAudioListeners(bool bActive)
{
    m_bControlsAudioListeners = bActive;

    TViewMap::const_iterator Iter(m_views.begin());
    TViewMap::const_iterator const IterEnd(m_views.end());

    for (; Iter != IterEnd; ++Iter)
    {
        Iter->second->SetActive(bActive);
    }
}

} // namespace LegacyViewSystem
