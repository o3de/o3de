/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"

#include <Cry_Camera.h>
#include <HMDBus.h>
#include "View.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <Random.h>
#include <MathConversion.h>

namespace LegacyViewSystem
{

static ICVar* pCamShakeMult = 0;
static ICVar* pHmdReferencePoint = 0;

//------------------------------------------------------------------------
CView::CView(ISystem* pSystem)
    : m_pSystem(pSystem)
    , m_linkedTo(0)
    , m_frameAdditiveAngles(0.0f, 0.0f, 0.0f)
    , m_scale(1.0f)
    , m_zoomedScale(1.0f)
{
    if (!pCamShakeMult)
    {
        pCamShakeMult = gEnv->pConsole->GetCVar("c_shakeMult");
    }
    if (!pHmdReferencePoint)
    {
        pHmdReferencePoint = gEnv->pConsole->GetCVar("hmd_reference_point");
    }
}

//------------------------------------------------------------------------
CView::~CView()
{
}

//-----------------------------------------------------------------------
void CView::Release()
{
    delete this;
}

//------------------------------------------------------------------------
void CView::Update([[maybe_unused]] float frameTime, [[maybe_unused]] bool isActive)
{
    AZ_ErrorOnce("CryLegacy", false, "CryLegacy view system no longer available (CView::Update)");
}

//-----------------------------------------------------------------------
void CView::ApplyFrameAdditiveAngles(Quat& cameraOrientation)
{
    if ((m_frameAdditiveAngles.x != 0.f) || (m_frameAdditiveAngles.y != 0.f) || (m_frameAdditiveAngles.z != 0.f))
    {
        Ang3 cameraAngles(cameraOrientation);
        cameraAngles += m_frameAdditiveAngles;

        cameraOrientation.SetRotationXYZ(cameraAngles);

        m_frameAdditiveAngles.Set(0.0f, 0.0f, 0.0f);
    }
}

//------------------------------------------------------------------------
void CView::SetViewShake(Ang3 shakeAngle, Vec3 shakeShift, float duration, float frequency, float randomness, int shakeID, bool bFlipVec, bool bUpdateOnly, bool bGroundOnly)
{
    SShakeParams params;
    params.shakeAngle = shakeAngle;
    params.shakeShift = shakeShift;
    params.frequency = frequency;
    params.randomness = randomness;
    params.shakeID = shakeID;
    params.bFlipVec = bFlipVec;
    params.bUpdateOnly = bUpdateOnly;
    params.bGroundOnly = bGroundOnly;
    params.fadeInDuration = 0;                  //
    params.fadeOutDuration = duration;  // originally it was faded out from start. that is why the values are set this way here, to preserve compatibility.
    params.sustainDuration = 0;             //

    SetViewShakeEx(params);
}


//------------------------------------------------------------------------
void CView::SetViewShakeEx(const SShakeParams& params)
{
    float shakeMult = GetScale();
    if (shakeMult < 0.001f)
    {
        return;
    }

    int shakes = static_cast<int>(m_shakes.size());
    SShake* pSetShake(NULL);

    for (int i = 0; i < shakes; ++i)
    {
        SShake* pShake = &m_shakes[i];
        if (pShake->ID == params.shakeID)
        {
            pSetShake = pShake;
            break;
        }
    }

    if (!pSetShake)
    {
        m_shakes.push_back(SShake(params.shakeID));
        pSetShake = &m_shakes.back();
    }

    if (pSetShake)
    {
        // this can be set dynamically
        pSetShake->frequency = max(0.00001f, params.frequency);

        // the following are set on a 'new' shake as well
        if (params.bUpdateOnly == false)
        {
            pSetShake->amount = params.shakeAngle * shakeMult;
            pSetShake->amountVector = params.shakeShift * shakeMult;
            pSetShake->randomness = params.randomness;
            pSetShake->doFlip = params.bFlipVec;
            pSetShake->groundOnly = params.bGroundOnly;
            pSetShake->isSmooth = params.isSmooth;
            pSetShake->permanent = params.bPermanent;
            pSetShake->fadeInDuration = params.fadeInDuration;
            pSetShake->sustainDuration = params.sustainDuration;
            pSetShake->fadeOutDuration = params.fadeOutDuration;
            pSetShake->timeDone = 0;
            pSetShake->updating = true;
            pSetShake->interrupted = false;
            pSetShake->goalShake = Quat(ZERO);
            pSetShake->goalShakeSpeed = Quat(ZERO);
            pSetShake->goalShakeVector = Vec3(ZERO);
            pSetShake->goalShakeVectorSpeed = Vec3(ZERO);
            pSetShake->nextShake = 0.0f;
        }
    }
}

//------------------------------------------------------------------------
void CView::SetScale(const float scale)
{
    CRY_ASSERT_MESSAGE(scale == 1.0f || m_scale == 1.0f, "Attempting to CView::SetScale but has already been set!");
    m_scale = scale;
}

void CView::SetZoomedScale(const float scale)
{
    CRY_ASSERT_MESSAGE(scale == 1.0f || m_zoomedScale == 1.0f, "Attempting to CView::SetZoomedScale but has already been set!");
    m_zoomedScale = scale;
}

//------------------------------------------------------------------------
const float CView::GetScale()
{
    float shakeMult(pCamShakeMult->GetFVal());
    return m_scale * shakeMult * m_zoomedScale;
}

//------------------------------------------------------------------------
void CView::ProcessShaking(float frameTime)
{
    m_viewParams.currentShakeQuat.SetIdentity();
    m_viewParams.currentShakeShift.zero();
    m_viewParams.shakingRatio = 0;
    m_viewParams.groundOnly = false;

    int shakes = static_cast<int>(m_shakes.size());
    for (int i = 0; i < shakes; ++i)
    {
        ProcessShake(&m_shakes[i], frameTime);
    }
}

//------------------------------------------------------------------------
void CView::ProcessShake(SShake* pShake, float frameTime)
{
    if (!pShake->updating)
    {
        return;
    }

    pShake->timeDone += frameTime;

    if (pShake->isSmooth)
    {
        ProcessShakeSmooth(pShake, frameTime);
    }
    else
    {
        ProcessShakeNormal(pShake, frameTime);
    }
}

//------------------------------------------------------------------------
void CView::ProcessShakeNormal(SShake* pShake, float frameTime)
{
    float endSustain = pShake->fadeInDuration + pShake->sustainDuration;
    float totalDuration = endSustain + pShake->fadeOutDuration;

    bool finalDamping = (!pShake->permanent && pShake->timeDone > totalDuration) || (pShake->interrupted && pShake->ratio < 0.05f);

    if (finalDamping)
    {
        ProcessShakeNormal_FinalDamping(pShake, frameTime);
    }
    else
    {
        ProcessShakeNormal_CalcRatio(pShake, frameTime, endSustain);
        ProcessShakeNormal_DoShaking(pShake, frameTime);

        //for the global shaking ratio keep the biggest
        if (pShake->groundOnly)
        {
            m_viewParams.groundOnly = true;
        }
        m_viewParams.shakingRatio = max(m_viewParams.shakingRatio, pShake->ratio);
        m_viewParams.currentShakeQuat *= pShake->shakeQuat;
        m_viewParams.currentShakeShift += pShake->shakeVector;
    }
}

//////////////////////////////////////////////////////////////////////////
void CView::ProcessShakeSmooth(SShake* pShake, float frameTime)
{
    assert(pShake->timeDone >= 0);

    float endTimeFadeIn = pShake->fadeInDuration;
    float endTimeSustain = pShake->sustainDuration + endTimeFadeIn;
    float totalTime = endTimeSustain + pShake->fadeOutDuration;

    if (pShake->interrupted && endTimeFadeIn <= pShake->timeDone && pShake->timeDone < endTimeSustain)
    {
        pShake->timeDone = endTimeSustain;
    }

    float damping = 1.f;
    if (pShake->timeDone < endTimeFadeIn)
    {
        damping = pShake->timeDone / endTimeFadeIn;
    }
    else if (endTimeSustain < pShake->timeDone && pShake->timeDone < totalTime)
    {
        damping = (totalTime - pShake->timeDone) / (totalTime - endTimeSustain);
    }
    else if (totalTime <= pShake->timeDone)
    {
        pShake->shakeQuat.SetIdentity();
        pShake->shakeVector.zero();
        pShake->ratio = 0.0f;
        pShake->nextShake = 0.0f;
        pShake->flip = false;
        pShake->updating = false;
        return;
    }

    ProcessShakeSmooth_DoShaking(pShake, frameTime);

    if (pShake->groundOnly)
    {
        m_viewParams.groundOnly = true;
    }
    pShake->ratio = (3.f - 2.f * damping) * damping * damping; // smooth ration change
    m_viewParams.shakingRatio = max(m_viewParams.shakingRatio, pShake->ratio);
    m_viewParams.currentShakeQuat *= Quat::CreateSlerp(IDENTITY, pShake->shakeQuat, pShake->ratio);
    m_viewParams.currentShakeShift += Vec3::CreateLerp(ZERO, pShake->shakeVector, pShake->ratio);
}

//////////////////////////////////////////////////////////////////////////
void CView::GetRandomQuat(Quat& quat, SShake* pShake)
{
    quat.SetRotationXYZ(pShake->amount);
    float randomAmt(pShake->randomness);
    float len(fabs(pShake->amount.x) + fabs(pShake->amount.y) + fabs(pShake->amount.z));
    len /= 3.f;
    float r = len * randomAmt;
    quat *= Quat::CreateRotationXYZ(Ang3(cry_random(-r, r), cry_random(-r, r), cry_random(-r, r)));
}

//////////////////////////////////////////////////////////////////////////
void CView::GetRandomVector(Vec3& vec, SShake* pShake)
{
    vec = pShake->amountVector;
    float randomAmt(pShake->randomness);
    float len = fabs(pShake->amountVector.x) + fabs(pShake->amountVector.y) + fabs(pShake->amountVector.z);
    len /= 3.f;
    float r = len * randomAmt;
    vec += Vec3(cry_random(-r, r), cry_random(-r, r), cry_random(-r, r));
}

//////////////////////////////////////////////////////////////////////////
void CView::CubeInterpolateQuat(float t, SShake* pShake)
{
    Quat p0 = pShake->startShake;
    Quat p1 = pShake->goalShake;
    Quat v0 = pShake->startShakeSpeed * 0.5f;
    Quat v1 = pShake->goalShakeSpeed * 0.5f;

    pShake->shakeQuat = (((p0 *  2.f + p1 * -2.f + v0 + v1) * t
        + (p0 * -3.f + p1 *  3.f + v0 * -2.f - v1)) * t
        + (v0)) * t
        + p0;

    pShake->shakeQuat.Normalize();
}

//////////////////////////////////////////////////////////////////////////
void CView::CubeInterpolateVector(float t, SShake* pShake)
{
    Vec3 p0 = pShake->startShakeVector;
    Vec3 p1 = pShake->goalShakeVector;
    Vec3 v0 = pShake->startShakeVectorSpeed * 0.8f;
    Vec3 v1 = pShake->goalShakeVectorSpeed * 0.8f;

    pShake->shakeVector = (((p0 *  2.f + p1 * -2.f + v0 + v1) * t
        + (p0 * -3.f + p1 *  3.f + v0 * -2.f - v1)) * t
        + (v0)) * t
        + p0;
}

//////////////////////////////////////////////////////////////////////////
void CView::ProcessShakeSmooth_DoShaking(SShake* pShake, float frameTime)
{
    if (pShake->nextShake <= 0.0f)
    {
        pShake->nextShake = pShake->frequency;

        pShake->startShake              = pShake->goalShake;
        pShake->startShakeSpeed         = pShake->goalShakeSpeed;
        pShake->startShakeVector        = pShake->goalShakeVector;
        pShake->startShakeVectorSpeed   = pShake->goalShakeVectorSpeed;

        GetRandomQuat(pShake->goalShake, pShake);
        GetRandomQuat(pShake->goalShakeSpeed, pShake);
        GetRandomVector(pShake->goalShakeVector, pShake);
        GetRandomVector(pShake->goalShakeVectorSpeed, pShake);

        if (pShake->flip)
        {
            pShake->goalShake.Invert();
            pShake->goalShakeSpeed.Invert();
            pShake->goalShakeVector = -pShake->goalShakeVector;
            pShake->goalShakeVectorSpeed = -pShake->goalShakeVectorSpeed;
        }

        if (pShake->doFlip)
        {
            pShake->flip = !pShake->flip;
        }
    }

    pShake->nextShake -= frameTime;

    float t = (pShake->frequency - pShake->nextShake) / pShake->frequency;
    CubeInterpolateQuat(t, pShake);
    CubeInterpolateVector(t, pShake);
}

//////////////////////////////////////////////////////////////////////////
void CView::ProcessShakeNormal_FinalDamping(SShake* pShake, float frameTime)
{
    pShake->shakeQuat = Quat::CreateSlerp(pShake->shakeQuat, IDENTITY, frameTime * 5.0f);
    m_viewParams.currentShakeQuat *= pShake->shakeQuat;

    pShake->shakeVector = Vec3::CreateLerp(pShake->shakeVector, ZERO, frameTime * 5.0f);
    m_viewParams.currentShakeShift += pShake->shakeVector;

    float svlen2(pShake->shakeVector.len2());
    bool quatIsIdentity(Quat::IsEquivalent(IDENTITY, pShake->shakeQuat, 0.0001f));

    if (quatIsIdentity && svlen2 < 0.01f)
    {
        pShake->shakeQuat.SetIdentity();
        pShake->shakeVector.zero();

        pShake->ratio = 0.0f;
        pShake->nextShake = 0.0f;
        pShake->flip = false;

        pShake->updating = false;
    }
}


// "ratio" is the amplitude of the shaking
void CView::ProcessShakeNormal_CalcRatio(SShake* pShake, float frameTime, float endSustain)
{
    const float FADEOUT_TIME_WHEN_INTERRUPTED = 0.5f;

    if (pShake->interrupted)
    {
        pShake->ratio = max(0.f, pShake->ratio - (frameTime / FADEOUT_TIME_WHEN_INTERRUPTED));    // fadeout after interrupted
    }
    else
        if (pShake->timeDone >= endSustain && pShake->fadeOutDuration > 0)
        {
            float timeFading = pShake->timeDone - endSustain;
            pShake->ratio = clamp_tpl(1.f - timeFading / pShake->fadeOutDuration, 0.f, 1.f);   // fadeOut
        }
        else
            if (pShake->timeDone >= pShake->fadeInDuration)
            {
                pShake->ratio = 1.f; // sustain
            }
            else
            {
                pShake->ratio = min(1.f, pShake->timeDone / pShake->fadeInDuration);   // fadeIn
            }

    if (pShake->permanent && pShake->timeDone >= pShake->fadeInDuration && !pShake->interrupted)
    {
        pShake->ratio = 1.f; // permanent standing
    }
}

//////////////////////////////////////////////////////////////////////////
void CView::ProcessShakeNormal_DoShaking(SShake* pShake, float frameTime)
{
    float t;
    if (pShake->nextShake <= 0.0f)
    {
        //angular
        pShake->goalShake.SetRotationXYZ(pShake->amount);
        if (pShake->flip)
        {
            pShake->goalShake.Invert();
        }

        //translational
        pShake->goalShakeVector = pShake->amountVector;
        if (pShake->flip)
        {
            pShake->goalShakeVector = -pShake->goalShakeVector;
        }

        if (pShake->doFlip)
        {
            pShake->flip = !pShake->flip;
        }

        //randomize it a little
        float randomAmt(pShake->randomness);
        float len(fabs(pShake->amount.x) + fabs(pShake->amount.y) + fabs(pShake->amount.z));
        len /= 3.0f;
        float r = len * randomAmt;
        pShake->goalShake *= Quat::CreateRotationXYZ(Ang3(cry_random(-r, r), cry_random(-r, r), cry_random(-r, r)));

        //translational randomization
        len = fabs(pShake->amountVector.x) + fabs(pShake->amountVector.y) + fabs(pShake->amountVector.z);
        len /= 3.0f;
        r = len * randomAmt;
        pShake->goalShakeVector += Vec3(cry_random(-r, r), cry_random(-r, r), cry_random(-r, r));

        //damp & bounce it in a non linear fashion
        t = 1.0f - (pShake->ratio * pShake->ratio);
        pShake->goalShake = Quat::CreateSlerp(pShake->goalShake, IDENTITY, t);
        pShake->goalShakeVector = Vec3::CreateLerp(pShake->goalShakeVector, ZERO, t);

        pShake->nextShake = pShake->frequency;
    }

    pShake->nextShake = max(0.0f, pShake->nextShake - frameTime);

    t = min(1.0f, frameTime * (1.0f / pShake->frequency));
    pShake->shakeQuat = Quat::CreateSlerp(pShake->shakeQuat, pShake->goalShake, t);
    pShake->shakeQuat.Normalize();
    pShake->shakeVector = Vec3::CreateLerp(pShake->shakeVector, pShake->goalShakeVector, t);
}


//------------------------------------------------------------------------
void CView::StopShake(int shakeID)
{
    uint32 num = static_cast<int>(m_shakes.size());
    for (uint32 i = 0; i < num; ++i)
    {
        if (m_shakes[i].ID == shakeID && m_shakes[i].updating)
        {
            m_shakes[i].interrupted = true;
        }
    }
}


//------------------------------------------------------------------------
void CView::ResetShaking()
{
    // disable shakes
    std::vector<SShake>::iterator iter = m_shakes.begin();
    std::vector<SShake>::iterator iterEnd = m_shakes.end();
    while (iter != iterEnd)
    {
        SShake& shake = *iter;
        shake.updating = false;
        shake.timeDone = 0;
        ++iter;
    }
}

//------------------------------------------------------------------------
void CView::LinkTo(AZ::Entity* follow)
{
    CRY_ASSERT(follow);
    m_azEntity = follow;
    m_linkedTo = follow->GetId();
    m_viewParams.targetPos = Vec3();// This should be quickly overwritten by the camera's acutal position from its matrix
}

//------------------------------------------------------------------------
void CView::Unlink()
{
    m_azEntity = nullptr;
    m_linkedTo.SetInvalid();
    m_viewParams.targetPos = Vec3();
}

//------------------------------------------------------------------------
void CView::SetFrameAdditiveCameraAngles(const Ang3& addFrameAngles)
{
    m_frameAdditiveAngles = addFrameAngles;
}

void CView::PostSerialize()
{
}

//////////////////////////////////////////////////////////////////////////
void CView::SetActive([[maybe_unused]] bool const bActive)
{
}

} // namespace LegacyViewSystem
