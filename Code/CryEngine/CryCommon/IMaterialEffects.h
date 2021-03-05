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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Interface to the Material Effects System


#ifndef CRYINCLUDE_CRYCOMMON_IMATERIALEFFECTS_H
#define CRYINCLUDE_CRYCOMMON_IMATERIALEFFECTS_H
#pragma once

#if !defined(_RELEASE)
    #define MATERIAL_EFFECTS_DEBUG
#endif


#include "CryFixedArray.h"

struct IRenderNode;
struct ISurfaceType;

//////////////////////////////////////////////////////////////////////////
enum EMFXPlayFlags
{
    eMFXPF_Disable_Delay = BIT(0),
    eMFXPF_Audio         = BIT(1),
    eMFXPF_Decal         = BIT(2),
    eMFXPF_Particles     = BIT(3),
    eMFXPF_Deprecated0   = BIT(4), // formerly eMFXPF_Flowgraph
    eMFXPF_ForceFeedback = BIT(5),
    eMFXPF_All           = (eMFXPF_Audio | eMFXPF_Decal | eMFXPF_Particles | eMFXPF_Deprecated0 | eMFXPF_ForceFeedback),
};

#define MFX_INVALID_ANGLE (gf_PI2 + 1)

//////////////////////////////////////////////////////////////////////////
struct SMFXAudioEffectRtpc
{
    SMFXAudioEffectRtpc()
    {
        rtpcName = "";
        rtpcValue = 0.0f;
    }
    const char* rtpcName;
    float       rtpcValue;
};

//////////////////////////////////////////////////////////////////////////
struct SMFXRunTimeEffectParams
{
    static const int MAX_AUDIO_RTPCS = 4;

    SMFXRunTimeEffectParams()
        : playSoundFP(false)
        , playflags(eMFXPF_All)
        , fLastTime(0.0f)
        , srcSurfaceId(0)
        , trgSurfaceId(0)
        , srcRenderNode(0)
        , trgRenderNode(0)
        , partID(0)
        , pos(ZERO)
        , decalPos(ZERO)
        , normal(0.0f, 0.0f, 1.0f)
        , angle(MFX_INVALID_ANGLE)
        , scale(1.0f)
        , audioComponentOffset(ZERO)
        , numAudioRtpcs(0)
        , fDecalPlacementTestMaxSize(1000.f)
    {
        dir[0].Set(0.0f, 0.0f, -1.0f);
        dir[1].Set(0.0f, 0.0f, 1.0f);
    }

    bool AddAudioRtpc(const char* name, float val)
    {
        if (numAudioRtpcs < MAX_AUDIO_RTPCS)
        {
            audioRtpcs[numAudioRtpcs].rtpcName  = name;
            audioRtpcs[numAudioRtpcs].rtpcValue = val;
            ++numAudioRtpcs;
            return true;
        }
        return false;
    }

    void ResetAudioRtpcs()
    {
        numAudioRtpcs = 0;
    }

public:
    uint16 playSoundFP;   // Sets 1p/3p audio switch
    uint16 playflags;     // See EMFXPlayFlags
    float  fLastTime;         // Last time this effect was played
    float  fDecalPlacementTestMaxSize;

    int          srcSurfaceId;
    int          trgSurfaceId;
    IRenderNode* srcRenderNode;
    IRenderNode* trgRenderNode;
    int          partID;

    Vec3  pos;
    Vec3  decalPos;
    Vec3  dir[2];
    Vec3  normal;
    float angle;
    float scale;

    // audio related
    Vec3     audioComponentOffset;        // in case of audio component, uses this offset

    SMFXAudioEffectRtpc audioRtpcs[MAX_AUDIO_RTPCS];
    uint32              numAudioRtpcs;
};

struct SMFXBreakageParams
{
    enum EBreakageRequestFlags
    {
        eBRF_Matrix                         = BIT(0),
        eBRF_HitPos                         = BIT(1),
        eBRF_HitImpulse                 = BIT(2),
        eBRF_Velocity           = BIT(3),
        eBRF_ExplosionImpulse       = BIT(4),
        eBRF_Mass               = BIT(5),
        eBFR_Entity             = BIT(6),
    };

    SMFXBreakageParams()
        : m_flags(0)
        , m_worldTM(IDENTITY)
        , m_vHitPos(ZERO)
        , m_vHitImpulse(IDENTITY)
        , m_vVelocity(ZERO)
        , m_fExplosionImpulse(1.0f)
        , m_fMass(0.0f)
    {
    }


    // Matrix
    void SetMatrix(const Matrix34& worldTM)
    {
        m_worldTM = worldTM;
        SetFlag(eBRF_Matrix);
    }

    const Matrix34& GetMatrix() const
    {
        return m_worldTM;
    }

    // HitPos
    void SetHitPos(const Vec3& vHitPos)
    {
        m_vHitPos = vHitPos;
        SetFlag(eBRF_HitPos);
    }

    const Vec3& GetHitPos() const
    {
        return m_vHitPos;
    }

    // HitImpulse
    void SetHitImpulse(const Vec3& vHitImpulse)
    {
        m_vHitImpulse = vHitImpulse;
        SetFlag(eBRF_HitImpulse);
    }

    const Vec3& GetHitImpulse() const
    {
        return m_vHitImpulse;
    }

    // Velocity
    void SetVelocity(const Vec3& vVelocity)
    {
        m_vVelocity = vVelocity;
        SetFlag(eBRF_Velocity);
    }

    const Vec3& GetVelocity() const
    {
        return m_vVelocity;
    }

    // Explosion Impulse
    void SetExplosionImpulse(float fExplosionImpulse)
    {
        m_fExplosionImpulse = fExplosionImpulse;
        SetFlag(eBRF_ExplosionImpulse);
    }

    float GetExplosionImpulse() const
    {
        return m_fExplosionImpulse;
    }

    // Mass
    void SetMass(float fMass)
    {
        m_fMass = fMass;
        SetFlag(eBRF_Mass);
    }

    float GetMass() const
    {
        return m_fMass;
    }

    // Checking for flags
    bool CheckFlag(EBreakageRequestFlags flag) const
    {
        return (m_flags & flag) != 0;
    }

protected:
    void SetFlag(EBreakageRequestFlags flag)
    {
        m_flags |= flag;
    }

    void ClearFlag(EBreakageRequestFlags flag)
    {
        m_flags &= ~flag;
    }

    uint32   m_flags;
    Matrix34 m_worldTM;
    Vec3     m_vHitPos;
    Vec3     m_vHitImpulse;
    Vec3     m_vVelocity;
    float    m_fExplosionImpulse;
    float    m_fMass;
};

class IMFXParticleParams
{
public:
    IMFXParticleParams()
        : name(NULL)
        , userdata(NULL)
        , scale(1.0f)
    {
    }

    const char* name;
    const char* userdata;
    float scale;
};

class SMFXParticleListNode
{
public:
    static SMFXParticleListNode* Create();
    void Destroy();
    static void FreePool();

    IMFXParticleParams m_particleParams;
    SMFXParticleListNode* pNext;

private:
    SMFXParticleListNode()
    {
        pNext = NULL;
    }
    ~SMFXParticleListNode() {}
};

class IMFXAudioParams
{
    const static uint MAX_SWITCH_DATA_ELEMENTS = 4;

public:

    struct SSwitchData
    {
        SSwitchData()
            : switchName(NULL)
            , switchStateName(NULL)
        {
        }

        const char* switchName;
        const char* switchStateName;
    };

    IMFXAudioParams()
        : triggerName(NULL)
    {
    }
    const char* triggerName;

    CryFixedArray<SSwitchData, MAX_SWITCH_DATA_ELEMENTS> triggerSwitches;
};

class SMFXAudioListNode
{
public:
    static SMFXAudioListNode* Create();
    void Destroy();
    static void FreePool();

    IMFXAudioParams   m_audioParams;
    SMFXAudioListNode* pNext;

private:
    SMFXAudioListNode()
        : pNext(NULL)
    {
    }

    ~SMFXAudioListNode()
    {
    }
};

class IMFXDecalParams
{
public:
    IMFXDecalParams()
    {
        filename = 0;
        material = 0;
        minscale = 1.f;
        maxscale = 1.f;
        rotation = -1.f;
        lifetime = 10.0f;
        assemble = false;
        forceedge = false;
    }
    const char* filename;
    const char* material;
    float minscale;
    float maxscale;
    float rotation;
    float lifetime;
    bool assemble;
    bool forceedge;
};

class SMFXDecalListNode
{
public:
    static SMFXDecalListNode* Create();
    void Destroy();
    static void FreePool();

    IMFXDecalParams m_decalParams;
    SMFXDecalListNode* pNext;

private:
    SMFXDecalListNode()
    {
        pNext = 0;
    }
    ~SMFXDecalListNode() {}
};

class IMFXForceFeedbackParams
{
public:
    IMFXForceFeedbackParams()
        : forceFeedbackEventName (NULL)
        , intensityFallOffMinDistanceSqr(0.0f)
        , intensityFallOffMaxDistanceSqr(0.0f)
    {
    }

    const char* forceFeedbackEventName;
    float intensityFallOffMinDistanceSqr;
    float intensityFallOffMaxDistanceSqr;
};

class SMFXForceFeedbackListNode
{
public:
    static SMFXForceFeedbackListNode* Create();
    void Destroy();
    static void FreePool();

    IMFXForceFeedbackParams m_forceFeedbackParams;
    SMFXForceFeedbackListNode* pNext;

private:
    SMFXForceFeedbackListNode()
        : pNext(NULL)
    {
    }
    ~SMFXForceFeedbackListNode() {}
};

struct SMFXResourceList;
typedef _smart_ptr<SMFXResourceList> SMFXResourceListPtr;

struct SMFXResourceList
{
public:
    SMFXParticleListNode* m_particleList;
    SMFXAudioListNode* m_audioList;
    SMFXDecalListNode* m_decalList;
    SMFXForceFeedbackListNode* m_forceFeedbackList;

    void AddRef() { ++m_refs; }
    void Release()
    {
        if (--m_refs <= 0)
        {
            Destroy();
        }
    }

    static SMFXResourceListPtr Create();
    static void FreePool();

private:
    int m_refs;

    virtual void Destroy();

    SMFXResourceList()
        : m_refs(0)
    {
        m_particleList = 0;
        m_audioList = 0;
        m_decalList = 0;
        m_forceFeedbackList = 0;
    }
    virtual ~SMFXResourceList()
    {
        while (m_particleList != 0)
        {
            SMFXParticleListNode* next = m_particleList->pNext;
            m_particleList->Destroy();
            m_particleList = next;
        }
        while (m_audioList != 0)
        {
            SMFXAudioListNode* next = m_audioList->pNext;
            m_audioList->Destroy();
            m_audioList = next;
        }
        while (m_decalList != 0)
        {
            SMFXDecalListNode* next = m_decalList->pNext;
            m_decalList->Destroy();
            m_decalList = next;
        }
        while (m_forceFeedbackList != 0)
        {
            SMFXForceFeedbackListNode* next = m_forceFeedbackList->pNext;
            m_forceFeedbackList->Destroy();
            m_forceFeedbackList = next;
        }
    }
};

typedef uint16 TMFXEffectId;
static const TMFXEffectId InvalidEffectId = 0;

struct SMFXCustomParamValue
{
    float fValue;
};

//////////////////////////////////////////////////////////////////////////
struct IMaterialEffects
{
    // <interfuscator:shuffle>
    virtual ~IMaterialEffects(){}
    virtual void LoadFXLibraries() = 0;
    virtual void Reset(bool bCleanup) = 0;
    virtual void ClearDelayedEffects() = 0;
    virtual TMFXEffectId GetEffectIdByName(const char* libName, const char* effectName) = 0;
    virtual TMFXEffectId GetEffectId(int surfaceIndex1, int surfaceIndex2) = 0;
    virtual TMFXEffectId GetEffectId(const char* customName, int surfaceIndex2) = 0;
    virtual SMFXResourceListPtr GetResources(TMFXEffectId effectId) const = 0;
    virtual void PreLoadAssets() = 0;
    virtual bool ExecuteEffect(TMFXEffectId effectId, SMFXRunTimeEffectParams& runtimeParams) = 0;
    virtual int GetDefaultSurfaceIndex() = 0;
    virtual int GetDefaultCanopyIndex() = 0;

    virtual bool PlayBreakageEffect(ISurfaceType* pSurfaceType, const char* breakageType, const SMFXBreakageParams& mfxBreakageParams) = 0;

    virtual void SetCustomParameter(TMFXEffectId effectId, const char* customParameter, const SMFXCustomParamValue& customParameterValue) = 0;

    virtual void CompleteInit() = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IMATERIALEFFECTS_H
