/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_ANIMKEY_H
#define CRYINCLUDE_CRYCOMMON_ANIMKEY_H
#pragma once

#include <IConsole.h> // <> required for Interfuscator
#include <ISystem.h>
#include <Cry_Color.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Component/EntityId.h>

enum EAnimKeyFlags
{
    AKEY_SELECTED = 0x01,       //! This key is selected in track view.
    AKEY_SORT_MARKER = 0x02     //! Internal use to locate a key after a sort.
};

//! Interface to animation key.
//! Not real interface though...
//! No virtuals for optimization reason.
struct IKey
{
    float time;
    int flags;

    // compare keys.
    bool operator<(const IKey& key) const { return time < key.time; }
    bool operator==(const IKey& key) const { return time == key.time; }
    bool operator>(const IKey& key) const { return time > key.time; }
    bool operator<=(const IKey& key) const { return time <= key.time; }
    bool operator>=(const IKey& key) const { return time >= key.time; }
    bool operator!=(const IKey& key) const { return time != key.time; }

    IKey()
        : time(0)
        , flags(0) {};

    virtual ~IKey() = default;
};

/** I2DBezierKey used in float tracks.
        Its x component actually represents kinda time-warping curve.
*/
struct I2DBezierKey
    : public IKey
{
    Vec2 value;
};

/** ITcbKey used in all TCB tracks.
*/
struct ITcbKey
    : public IKey
{
    // Values.
    float fval[4];
    // Key controls.
    float tens;         //!< Key tension value.
    float cont;       //!< Key continuity value.
    float bias;       //!< Key bias value.
    float easeto;     //!< Key ease to value.
    float easefrom;   //!< Key ease from value.

    //! Protect from direct instantiation of this class.
    //! Only derived classes can be created,
    ITcbKey()
    {
        fval[0] = 0;
        fval[1] = 0;
        fval[2] = 0;
        fval[3] = 0;
        tens = 0, cont = 0, bias = 0, easeto = 0, easefrom = 0;
    };

    void SetFloat(float val) { fval[0] = val; };
    void SetVec3(const Vec3& val)
    {
        fval[0] = val.x;
        fval[1] = val.y;
        fval[2] = val.z;
    };
    void SetQuat(const Quat& val)
    {
        fval[0] = val.v.x;
        fval[1] = val.v.y;
        fval[2] = val.v.z;
        fval[3] = val.w;
    };

    ILINE void SetValue(float val)       { SetFloat(val); }
    ILINE void SetValue(const Vec3& val) { SetVec3(val); }
    ILINE void SetValue(const Quat& val) { SetQuat(val); }

    float GetFloat() const { return *((float*)fval); };
    Vec3 GetVec3() const
    {
        Vec3 vec;
        vec.x = fval[0];
        vec.y = fval[1];
        vec.z = fval[2];
        return vec;
    };
    Quat GetQuat() const
    {
        Quat quat;
        quat.v.x = fval[0];
        quat.v.y = fval[1];
        quat.v.z = fval[2];
        quat.w = fval[3];
        return quat;
    };
    ILINE void GetValue(float& val) { val = GetFloat(); };
    ILINE void GetValue(Vec3& val)  { val = GetVec3(); };
    ILINE void GetValue(Quat& val)  { val = GetQuat(); };
};

struct IEventKey
    : public IKey
{
    AZStd::string event;
    AZStd::string eventValue;
    AZStd::string animation;
    AZStd::string target;

    union
    {
        float value;
        float duration;
    };
    bool bNoTriggerInScrubbing;

    IEventKey()
    {
        duration = 0;
        bNoTriggerInScrubbing = false;
    }
};

/** ISelectKey used in Camera selection track or Scene node.
*/
struct ISelectKey
    : public IKey
{
    AZStd::string szSelection;      //!< Node name (an existing camera entity name), or empty.
    AZ::EntityId cameraAzEntityId;  //!< Valid EntityId for an existing camera, or invalid.
    float fDuration;                //!< Duration of camera activity in CSelectTrack, not user-defined, calculated for compatibility and UI sliders ranges.
    float fBlendTime;               //!< Time in seconds to a next key where camera parameters are interpolated.
    //!< Initial properties of an existing camera, used to interpolate between camera keys, stored when CSelectTrack is activated.
    float m_FoV;                    //!< Initial FoV of an existing camera; positive value indicates that the key was initialized.
    float m_nearZ;                  //!< Initial near clipping distance of an existing camera, if initialized.
    AZ::Vector3 m_position;         //!< Initial world position of an existing camera, if initialized.
    AZ::Quaternion m_rotation;      //!< Initial world rotation of an existing camera, if initialized.

    ISelectKey()
    {
        Reset();
    }

    //!< @returns True if a valid camera controller EntityId and Name are set, otherwise returns false.
    bool IsValid() const { return cameraAzEntityId.IsValid() && !szSelection.empty(); }

    //!< @returns True if a valid camera controller EntityId is set and camera properties are stored, otherwise returns false.
    bool IsInitialized() const { return IsValid() && m_FoV > 0.0f; } 

    //!< @returns True if a valid camera controller EntityId is set, otherwise returns false and invalidates camera properties.
    bool CheckValid()
    {
        if (IsValid())
        {
            return true;
        }

        Reset();
        return false;
    }

    //!< Invalidates all key camera data.
    void Reset()
    {
        szSelection.clear();
        cameraAzEntityId = AZ::EntityId();
        fDuration = 0;
        fBlendTime = 0;
        ResetCameraProperies();
    }

    //!< Invalidates key camera properties.
    void ResetCameraProperies()
    {
        m_FoV = -1.0f;
        m_nearZ = 0;
        m_position = AZ::Vector3::CreateZero();
        m_rotation = AZ::Quaternion::CreateIdentity();
    }

    //!< Copies stored key camera properties.
    void CopyCameraProperies(const ISelectKey& rhs)
    {
        AZ_Assert(cameraAzEntityId == rhs.cameraAzEntityId, "Invalid camera data.")
        m_FoV = rhs.m_FoV;
        m_nearZ = rhs.m_nearZ;
        m_position = rhs.m_position;
        m_rotation = rhs.m_rotation;
    }
};

/** ISequenceKey used in sequence track.
*/
struct ISequenceKey
    : public IKey
{
    AZStd::string szSelection;          //!@deprecated : use sequenceEntityId to identify sequences
    AZ::EntityId sequenceEntityId;
    float fDuration;
    float fStartTime;
    float fEndTime;
    bool bOverrideTimes;
    bool  bDoNotStop;

    ISequenceKey()
    {
        fDuration = 0;
        fStartTime = 0;
        fEndTime = 0;
        bOverrideTimes = false;
        bDoNotStop = false;
    }
};

/** ISoundKey used in sound track.
*/
struct ISoundKey
    : public IKey
{
    ISoundKey()
        : fDuration(0.0f)
    {
        customColor.x = Col_TrackviewDefault.r;
        customColor.y = Col_TrackviewDefault.g;
        customColor.z = Col_TrackviewDefault.b;
    }

    AZStd::string sStartTrigger;
    AZStd::string sStopTrigger;
    float         fDuration;
    Vec3          customColor;
};

/** ITimeRangeKey used in time ranges animation track.
*/
#define ANIMKEY_TIME_RANGE_END_TIME_UNSET .0f
struct ITimeRangeKey
    : public IKey
{
    float m_duration;       //!< Duration in seconds of this animation.
    float m_startTime;      //!< Start time of this animation (Offset from beginning of animation).
    float m_endTime;        //!< End time of this animation (can be smaller than the duration).
    float m_speed;          //!< Speed multiplier for this key.
    bool  m_bLoop;          //!< True if time is looping

    ITimeRangeKey()
    {
        m_duration = 0.0f;
        m_endTime = ANIMKEY_TIME_RANGE_END_TIME_UNSET;
        m_startTime = 0.0f;
        m_speed = 1.0f;
        m_bLoop = false;
    }

    float GetValidEndTime() const
    {
        float endTime = m_endTime;
        if (endTime == ANIMKEY_TIME_RANGE_END_TIME_UNSET || (!m_bLoop && endTime > m_duration))
        {
            endTime = m_duration;
        }
        return endTime;
    }

    float GetValidSpeed() const
    {
        float speed = m_speed;
        if (speed <= 0.0f)
        {
            speed = 1.0f;
        }
        return speed;
    }

    float GetActualDuration() const
    {
        return (GetValidEndTime() - m_startTime) / GetValidSpeed();
    }

    // Return true if the input time falls in range of the start/end time for this key.
    bool IsInRange(float sequenceTime) const
    {
        return sequenceTime >= time && sequenceTime <= (time + GetActualDuration());
    }
};

/** ICharacterKey used in Character animation track.
*/
struct ICharacterKey
    : public ITimeRangeKey
{
    AZStd::string m_animation;      //!< Name of character animation.
    bool m_bBlendGap;               //!< True if gap to next animation should be blended
    bool m_bInPlace;                // Play animation in place (Do not move root).

    ICharacterKey()
        : ITimeRangeKey()
    {
        m_bLoop = false;
        m_bBlendGap = false;
        m_bInPlace = false;
    }
};

/** IExprKey used in expression animation track.
*/
struct IExprKey
    : public IKey
{
    IExprKey()
    {
        pszName[0] = 0;
        fAmp = 1.0f;
        fBlendIn = 0.5f;
        fHold = 1.0f;
        fBlendOut = 0.5f;
    }
    char pszName[128];  //!< Name of morph-target
    float fAmp;
    float fBlendIn;
    float fHold;
    float fBlendOut;
};

/** IConsoleKey used in Console track, triggers console commands and variables.
*/
struct IConsoleKey
    : public IKey
{
    AZStd::string command;
};

struct ILookAtKey
    : public IKey
{
    AZStd::string szSelection;  //!< Node name.
    float fDuration;
    AZStd::string lookPose;
    float smoothTime;

    ILookAtKey()
    {
        fDuration = 0;
        smoothTime = 0.2f;
    }
};

//! Discrete (non-interpolated) float key.
struct IDiscreteFloatKey
    : public IKey
{
    float m_fValue;

    void SetValue(float fValue)
    {
        m_fValue = fValue;
    }

    IDiscreteFloatKey()
    {
        m_fValue = -1.0f;
    }
};

//! A key for the capture track.
struct ICaptureKey
    : public IKey
{
    friend class AnimSerializer;

    AZStd::string folder;
    AZStd::string prefix;
    float duration;
    float timeStep;
    bool once;

    ICaptureKey()
        : IKey()
        , duration(0.0f)
        , timeStep(0.033f)
        , once(false)
    {
        ICVar* pCaptureFolderCVar = gEnv->pConsole->GetCVar("capture_folder");
        if (pCaptureFolderCVar != NULL  && pCaptureFolderCVar->GetString())
        {
            folder = pCaptureFolderCVar->GetString();
        }
        ICVar* pCaptureFilePrefixCVar = gEnv->pConsole->GetCVar("capture_file_prefix");
        if (pCaptureFilePrefixCVar != NULL  && pCaptureFilePrefixCVar->GetString())
        {
            prefix = pCaptureFilePrefixCVar->GetString();
        }
    }

    ICaptureKey(const ICaptureKey& other)
        : IKey(other)
        , folder(other.folder)
        , prefix(other.prefix)
        , duration(other.duration)
        , timeStep(other.timeStep)
        , once(other.once)
    {
    }
};

//! Boolean key.
struct IBoolKey
    : public IKey
{
    IBoolKey() {};
};

//! Comment Key.
struct ICommentKey
    : public IKey
{
    enum ETextAlign : int
    {
        eTA_Left    =   0,
        eTA_Center  =   BIT(1),
        eTA_Right   =   BIT(2)
    };

    //-----------------------------------------------------------------------------
    //!
    ICommentKey()
        : m_duration(1.f)
        , m_size(1.f)
        , m_align(eTA_Left)
        , m_strFont("default")
        , m_color(1.f, 1.f, 1.f, 1.f)
    {
    }

    //-----------------------------------------------------------------------------
    //!
    ICommentKey(const ICommentKey& other)
        : IKey(other)
        , m_strComment(other.m_strComment)
        , m_strFont(other.m_strFont)
    {
        m_duration = other.m_duration;
        m_color = other.m_color;
        m_size = other.m_size;
        m_align = other.m_align;
    }

    AZStd::string m_strComment;
    float   m_duration;

    AZStd::string m_strFont;
    AZ::Color    m_color;
    float   m_size;
    ETextAlign  m_align;
};

//-----------------------------------------------------------------------------
//!
struct IScreenFaderKey
    : public IKey
{
    //-----------------------------------------------------------------------------
    //!
    enum EFadeType : int
    {
        eFT_FadeIn = 0, eFT_FadeOut = 1
    };
    enum EFadeChangeType : int
    {
        eFCT_Linear = 0, eFCT_Square = 1, eFCT_CubicSquare = 2, eFCT_SquareRoot = 3, eFCT_Sin = 4
    };

    //-----------------------------------------------------------------------------
    //!
    IScreenFaderKey()
        : IKey()
        , m_fadeTime(2.f)
        , m_bUseCurColor(true)
        , m_fadeType(eFT_FadeOut)
        , m_fadeChangeType(eFCT_Linear)
    {
        m_fadeColor = AZ::Color(.0f, .0f, .0f, 1.0f);
    }

    //-----------------------------------------------------------------------------
    //!
    IScreenFaderKey(const IScreenFaderKey& other)
        : IKey(other)
        , m_fadeTime(other.m_fadeTime)
        , m_bUseCurColor(other.m_bUseCurColor)
        , m_fadeType(other.m_fadeType)
        , m_fadeChangeType(other.m_fadeChangeType)
    {
        m_fadeColor = other.m_fadeColor;
        m_strTexture = other.m_strTexture;
    }

    //-----------------------------------------------------------------------------
    //!
    float       m_fadeTime;
    AZ::Color   m_fadeColor;
    AZStd::string m_strTexture;
    bool        m_bUseCurColor;
    EFadeType   m_fadeType;
    EFadeChangeType m_fadeChangeType;
};

/** IStringKey used in string tracks.
 */
struct IStringKey
    : public IKey
{
    AZStd::string m_strValue;

    IStringKey() = default;

    IStringKey(const AZStd::string value)
        : m_strValue(value) 
    {
    }
};

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(IKey, "{680BD51E-C106-4BBF-9A6F-CD551E00519F}");
    AZ_TYPE_INFO_SPECIALIZE(IBoolKey, "{DBF8044F-6E64-403D-807D-F3152F640703}");
    AZ_TYPE_INFO_SPECIALIZE(ICaptureKey, "{93AA8D63-6B1E-4D33-8CC3-C82147BB95CB}");
    AZ_TYPE_INFO_SPECIALIZE(ICharacterKey, "{6D1FB9E2-128C-4B33-84FF-4F696C1F7D53}");
    AZ_TYPE_INFO_SPECIALIZE(ICommentKey, "{99C2234E-A4DD-45D1-90C3-D5AFC54FA47F}");
    AZ_TYPE_INFO_SPECIALIZE(IConsoleKey, "{8C0DCB9B-297D-4AF4-A0D1-F5160E6900E8}");
    AZ_TYPE_INFO_SPECIALIZE(IDiscreteFloatKey, "{469A2B90-E019-4147-A53F-2EB42E179596}");
    AZ_TYPE_INFO_SPECIALIZE(IEventKey, "{F09533AA-9780-494D-9E5C-8CB98266AC5E}");
    AZ_TYPE_INFO_SPECIALIZE(ILookAtKey, "{6F4CED0E-D83A-40E2-B7BF-038D82BC0374}");
    AZ_TYPE_INFO_SPECIALIZE(IScreenFaderKey, "{FA15E27D-603F-4829-925A-E36D75C93964}");
    AZ_TYPE_INFO_SPECIALIZE(ISelectKey, "{FCEADCF5-042E-473B-845F-0778F087B6DC}");
    AZ_TYPE_INFO_SPECIALIZE(ISequenceKey, "{B55294AD-F14E-43AC-B6B5-AC27B377FE00}");
    AZ_TYPE_INFO_SPECIALIZE(ISoundKey, "{452E50CF-B7D0-42D5-A86A-B295682674BB}");
    AZ_TYPE_INFO_SPECIALIZE(ITimeRangeKey, "{17807C95-C7A1-481B-AD94-C54D83928D0B}");
    AZ_TYPE_INFO_SPECIALIZE(IStringKey, "{A35D94C2-776B-4BA7-BBBC-1A1FD4402023}");
}
#endif // CRYINCLUDE_CRYCOMMON_ANIMKEY_H
