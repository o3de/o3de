/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : View System interfaces.

# pragma once

#include "IViewSystem.h"
#include <Cry_Camera.h>

class CGameObject;

namespace LegacyViewSystem
{

class CView
    : public IView
{
public:

    CView(ISystem* pSystem);
    virtual ~CView();

    //shaking
    struct SShake
    {
        bool updating;
        bool flip;
        bool doFlip;
        bool groundOnly;
        bool permanent;
        bool interrupted; // when forcefully stopped
        bool isSmooth;

        int ID;

        float nextShake;
        float timeDone;
        float sustainDuration;
        float fadeInDuration;
        float fadeOutDuration;

        float frequency;
        float ratio;

        float randomness;

        Quat startShake;
        Quat startShakeSpeed;
        Vec3 startShakeVector;
        Vec3 startShakeVectorSpeed;

        Quat goalShake;
        Quat goalShakeSpeed;
        Vec3 goalShakeVector;
        Vec3 goalShakeVectorSpeed;

        Ang3 amount;
        Vec3 amountVector;

        Quat shakeQuat;
        Vec3 shakeVector;

        SShake(int shakeID)
        {
            memset(this, 0, sizeof(SShake));

            startShake.SetIdentity();
            startShakeSpeed.SetIdentity();
            goalShake.SetIdentity();
            shakeQuat.SetIdentity();

            randomness = 0.5f;

            ID = shakeID;
        }

        void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const { /*nothing*/}
    };


    // IView
    virtual void Release();
    virtual void Update(float frameTime, bool isActive);
    virtual void ProcessShaking(float frameTime);
    virtual void ProcessShake(SShake* pShake, float frameTime);
    virtual void ResetShaking();
    virtual void ResetBlending() { m_viewParams.ResetBlending(); }
    virtual void LinkTo(AZ::Entity* follow);
    virtual void Unlink();
    virtual AZ::EntityId GetLinkedId() {return m_linkedTo; };
    virtual void SetCurrentParams(SViewParams& params) { m_viewParams = params; };
    virtual const SViewParams* GetCurrentParams() {return &m_viewParams; }
    virtual void SetViewShake(Ang3 shakeAngle, Vec3 shakeShift, float duration, float frequency, float randomness, int shakeID, bool bFlipVec = true, bool bUpdateOnly = false, bool bGroundOnly = false);
    virtual void SetViewShakeEx(const SShakeParams& params);
    virtual void StopShake(int shakeID);
    virtual void SetFrameAdditiveCameraAngles(const Ang3& addFrameAngles);
    virtual void SetScale(const float scale);
    virtual void SetZoomedScale(const float scale);
    virtual void SetActive(const bool bActive);
    // ~IView

    void Serialize(TSerialize ser) override;
    void PostSerialize() override;
    CCamera& GetCamera() override { return m_camera; }
    const CCamera& GetCamera() const override { return m_camera; }
    void UpdateAudioListener(const Matrix34& rMatrix) override;

    void GetMemoryUsage(ICrySizer* s) const;

protected:

    void ProcessShakeNormal(SShake* pShake, float frameTime);
    void ProcessShakeNormal_FinalDamping(SShake* pShake, float frameTime);
    void ProcessShakeNormal_CalcRatio(SShake* pShake, float frameTime, float endSustain);
    void ProcessShakeNormal_DoShaking(SShake* pShake, float frameTime);

    void ProcessShakeSmooth(SShake* pShake, float frameTime);
    void ProcessShakeSmooth_DoShaking(SShake* pShake, float frameTime);

    void ApplyFrameAdditiveAngles(Quat& cameraOrientation);

    const float GetScale();

private:

    void GetRandomQuat(Quat& quat, SShake* pShake);
    void GetRandomVector(Vec3& vec3, SShake* pShake);
    void CubeInterpolateQuat(float t, SShake* pShake);
    void CubeInterpolateVector(float t, SShake* pShake);

protected:

    bool m_active;
    AZ::EntityId m_linkedTo;
    AZ::Entity* m_azEntity = nullptr;

    SViewParams m_viewParams;
    CCamera m_camera;

    ISystem* m_pSystem;

    std::vector<SShake> m_shakes;

    Ang3     m_frameAdditiveAngles; // Used mainly for cinematics, where the game can slightly override camera orientation

    float m_scale;
    float m_zoomedScale;
};

} // namespace LegacyViewSystem
