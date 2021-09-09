/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : View System interfaces.

#pragma once

#include <Cry_Camera.h>
#include <AzCore/Component/EntityId.h>

//
#define VIEWID_NORMAL 0
#define VIEWID_FOLLOWHEAD 1
#define VIEWID_VEHICLE 2
#define VIEWID_RAGDOLL 3

//Forward declaration of AZ::Entity
namespace AZ {
    class Entity;
}

enum EMotionBlurType
{
    eMBT_None = 0,
    eMBT_Accumulation = 1,
    eMBT_Velocity = 2
};

struct SViewParams
{
    SViewParams()
        : position(ZERO)
        , rotation(IDENTITY)
        , localRotationLast(IDENTITY)
        , nearplane(0.0f)
        , farplane(0.0f)
        , fov(0.0f)
        , viewID(0)
        , groundOnly(false)
        , shakingRatio(0.0f)
        , currentShakeQuat(IDENTITY)
        , currentShakeShift(ZERO)
        , targetPos(ZERO)
        , frameTime(0.0f)
        , angleVel(0.0f)
        , vel(0.0f)
        , dist(0.0f)
        , blend(true)
        , blendPosSpeed(5.0f)
        , blendRotSpeed(10.0f)
        , blendFOVSpeed(5.0f)
        , blendPosOffset(ZERO)
        , blendRotOffset(IDENTITY)
        , blendFOVOffset(0)
        , justActivated(false)
        , viewIDLast(0)
        , positionLast(ZERO)
        , rotationLast(IDENTITY)
        , FOVLast(0)
    {
    }

    void SetViewID(uint8 id, bool shouldBlend = true)
    {
        viewID = id;
        if (!shouldBlend)
        {
            viewIDLast = id;
        }
    }

    void UpdateBlending(float curFrameTime)
    {
        //if necessary blend the view
        if (blend)
        {
            if (viewIDLast != viewID)
            {
                blendPosOffset = positionLast - position;
                blendRotOffset = (rotationLast / rotation).GetNormalized();
                blendFOVOffset = FOVLast - fov;
            }
            else
            {
                blendPosOffset -= blendPosOffset * min(1.0f, blendPosSpeed * curFrameTime);
                blendRotOffset = Quat::CreateSlerp(blendRotOffset, IDENTITY, min(1.0f, curFrameTime * blendRotSpeed));
                blendFOVOffset -= blendFOVOffset * min(1.0f, blendFOVSpeed * curFrameTime);
            }

            position += blendPosOffset;
            rotation *= blendRotOffset;
            fov += blendFOVOffset;
        }
        else
        {
            blendPosOffset.zero();
            blendRotOffset.SetIdentity();
            blendFOVOffset = 0.0f;
        }

        viewIDLast = viewID;
    }

    void BlendFrom(const SViewParams& params)
    {
        positionLast = params.position;
        rotationLast = params.rotation;
        FOVLast = params.fov;
        localRotationLast = params.localRotationLast;
        blend = true;
        viewIDLast = 0xff;
    }

    void SaveLast()
    {
        if (viewIDLast != 0xff)
        {
            positionLast = position;
            rotationLast = rotation;
            FOVLast = fov;
        }
        else
        {
            viewIDLast = 0xfe;
        }
    }

    void ResetBlending()
    {
        blendPosOffset.zero();
        blendRotOffset.SetIdentity();
    }

    const Vec3& GetPositionLast() { return positionLast; }
    const Quat& GetRotationLast() { return rotationLast; }

    //
    Vec3 position;//view position
    Quat rotation;//view orientation
    Quat localRotationLast;

    float nearplane;//custom near clipping plane, 0 means use engine defaults
    float farplane;//custom far clipping plane, 0 means use engine defaults
    float   fov;

    uint8 viewID;

    //view shake status
    bool  groundOnly;
    float shakingRatio;//whats the ammount of shake, from 0.0 to 1.0
    Quat currentShakeQuat;//what the current angular shake
    Vec3 currentShakeShift;//what is the current translational shake

    // For damping camera movement.
    Vec3 targetPos;     // Where the target was.
    float frameTime;    // current dt.
    float angleVel;     // previous rate of change of angle.
    float vel;          // previous rate of change of dist between target and camera.
    float dist;         // previous dist of cam from target

    //blending
    bool    blend;
    float   blendPosSpeed;
    float   blendRotSpeed;
    float   blendFOVSpeed;
    Vec3    blendPosOffset;
    Quat    blendRotOffset;
    float   blendFOVOffset;
    bool    justActivated;

private:
    uint8 viewIDLast;
    Vec3 positionLast;//last view position
    Quat rotationLast;//last view orientation
    float FOVLast;
};

struct IAnimSequence;
struct SCameraParams;

struct IView
{
    virtual ~IView() {}
    struct SShakeParams
    {
        Ang3 shakeAngle;
        Vec3 shakeShift;
        float sustainDuration;
        float fadeInDuration;
        float fadeOutDuration;
        float frequency;
        float randomness;
        int shakeID;
        bool bFlipVec;
        bool bUpdateOnly;
        bool bGroundOnly;
        bool bPermanent; // if true, sustainDuration is ignored
        bool isSmooth;

        SShakeParams()
            : shakeAngle(0, 0, 0)
            , shakeShift(0, 0, 0)
            , sustainDuration(0)
            , fadeInDuration(0)
            , fadeOutDuration(2.f)
            , frequency(0)
            , randomness(0)
            , shakeID(0)
            , bFlipVec(true)
            , bUpdateOnly(false)
            , bGroundOnly(false)
            , bPermanent(false)
            , isSmooth(false)
        {
        }
    };

    virtual void Release() = 0;
    virtual void Update(float frameTime, bool isActive) = 0;
    virtual void LinkTo(AZ::Entity* follow) = 0;
    virtual void Unlink() = 0;
    virtual AZ::EntityId GetLinkedId() = 0;
    virtual CCamera& GetCamera() = 0;
    virtual const CCamera& GetCamera() const = 0;

    virtual void PostSerialize() = 0;
    virtual void SetCurrentParams(SViewParams& params) = 0;
    virtual const SViewParams* GetCurrentParams() = 0;
    virtual void SetViewShake(Ang3 shakeAngle, Vec3 shakeShift, float duration, float frequency, float randomness, int shakeID, bool bFlipVec = true, bool bUpdateOnly = false, bool bGroundOnly = false) = 0;
    virtual void SetViewShakeEx(const SShakeParams& params) = 0;
    virtual void StopShake(int shakeID) = 0;
    virtual void ResetShaking() = 0;
    virtual void ResetBlending() = 0;
    virtual void SetFrameAdditiveCameraAngles(const Ang3& addFrameAngles) = 0;
    virtual void SetScale(const float scale) = 0;
    virtual void SetZoomedScale(const float scale) = 0;
    virtual void SetActive(const bool bActive) = 0;
};

struct IViewSystemListener
{
    virtual ~IViewSystemListener() {}
    virtual bool OnBeginCutScene(IAnimSequence* pSeq, bool bResetFX) = 0;
    virtual bool OnEndCutScene(IAnimSequence* pSeq) = 0;
    virtual bool OnCameraChange(const SCameraParams& cameraParams) = 0;
};

struct IViewSystem
{
    virtual ~IViewSystem() {}
    virtual void Release() = 0;
    virtual void Update(float frameTime) = 0;
    virtual IView* CreateView() = 0;
    virtual unsigned int AddView(IView* pView) = 0;
    virtual void RemoveView(IView* pView) = 0;
    virtual void RemoveView(unsigned int viewId) = 0;

    virtual void SetActiveView(IView* pView) = 0;
    virtual void SetActiveView(unsigned int viewId) = 0;

    //utility functions
    virtual IView* GetView(unsigned int viewId) = 0;
    virtual IView* GetActiveView() = 0;

    virtual unsigned int GetViewId(IView* pView) = 0;
    virtual unsigned int GetActiveViewId() = 0;

    virtual IView* GetViewByEntityId(const AZ::EntityId& id, bool forceCreate = false) = 0;

    virtual bool AddListener(IViewSystemListener* pListener) = 0;
    virtual bool RemoveListener(IViewSystemListener* pListener) = 0;

    virtual void PostSerialize() = 0;

    // Get default distance to near clipping plane.
    virtual float GetDefaultZNear() = 0;

    virtual void SetBlendParams(float fBlendPosSpeed, float fBlendRotSpeed, bool performBlendOut) = 0;

    // Used by time demo playback.
    virtual void SetOverrideCameraRotation(bool bOverride, Quat rotation) = 0;

    virtual bool IsPlayingCutScene() const = 0;

    virtual void SetDeferredViewSystemUpdate(bool const bDeferred) = 0;
    virtual bool UseDeferredViewSystemUpdate() const = 0;
    virtual void SetControlAudioListeners(bool const bActive) = 0;
    virtual void ForceUpdate(float elapsed) = 0;
};
