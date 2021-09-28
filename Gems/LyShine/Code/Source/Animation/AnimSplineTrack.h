/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <LyShine/Animation/IUiAnimation.h>
//#include "TCBSpline.h"
#include "2DSpline.h"

#define MIN_TIME_PRECISION 0.01f
#define MIN_VALUE_RANGE 1.0f        // prevents fill sliders from being inoperable on the first key frame
/*!
        Templated class that used as a base for all Tcb spline tracks.
 */
template <class ValueType>
class TUiAnimSplineTrack
    : public IUiAnimTrack
{
public:
    AZ_CLASS_ALLOCATOR(TUiAnimSplineTrack, AZ::SystemAllocator, 0)
    AZ_RTTI((TUiAnimSplineTrack, "{A78AAC62-84D0-4E2E-958E-564F51A140D2}", Vec2), IUiAnimTrack);

    TUiAnimSplineTrack()
        : m_refCount(0)
    {
        AllocSpline();
        m_flags = 0;
        m_bCustomColorSet = false;
        m_fMinKeyValue = 0.0f;
        m_fMaxKeyValue = 0.0f;
    }
    ~TUiAnimSplineTrack()
    {
        m_spline.reset();
    }

    //////////////////////////////////////////////////////////////////////////
    void add_ref() override;
    void release() override;
    //////////////////////////////////////////////////////////////////////////

    int GetSubTrackCount() const override { return 0; };
    IUiAnimTrack* GetSubTrack([[maybe_unused]] int nIndex) const override { return 0; };
    AZStd::string GetSubTrackName([[maybe_unused]] int nIndex) const override { return AZStd::string(); };
    void SetSubTrackName([[maybe_unused]] int nIndex, [[maybe_unused]] const char* name) override { assert(0); }

    const CUiAnimParamType& GetParameterType() const override { return m_nParamType; };
    void SetParameterType(CUiAnimParamType type) override { m_nParamType = type; };

    const UiAnimParamData& GetParamData() const override { return m_componentParamData; }
    void SetParamData(const UiAnimParamData& param) override { m_componentParamData = param; }

    void GetKeyValueRange(float& fMin, float& fMax) const override { fMin = m_fMinKeyValue; fMax = m_fMaxKeyValue; };
    void SetKeyValueRange(float fMin, float fMax) override{ m_fMinKeyValue = fMin; m_fMaxKeyValue = fMax; };

    ISplineInterpolator* GetSpline() const override { return m_spline.get(); };

    bool IsKeySelected(int key) const override
    {
        if (GetSpline() && GetSpline()->IsKeySelectedAtAnyDimension(key))
        {
            return true;
        }
        return false;
    }

    void SelectKey(int key, bool select) override
    {
        if (GetSpline())
        {
            GetSpline()->SelectKeyAllDimensions(key, select);
        }
    }

    int GetNumKeys() const override
    {
        return m_spline->num_keys();
    }

    void SetNumKeys(int numKeys) override
    {
        m_spline->resize(numKeys);
    }

    bool HasKeys() const override
    {
        return GetNumKeys() != 0;
    }

    void RemoveKey(int num) override
    {
        if (m_spline && m_spline->num_keys() > num)
        {
            m_spline->erase(num);
        }
        else
        {
            assert(0);
        }
    }

    void GetKey(int index, IKey* key) const override
    {
        assert(index >= 0 && index < GetNumKeys());
        assert(key != 0);
        typename Spline::key_type& k = m_spline->key(index);
        ITcbKey* tcbkey = (ITcbKey*)key;
        tcbkey->time = k.time;
        tcbkey->flags = k.flags;

        tcbkey->tens = k.tens;
        tcbkey->cont = k.cont;
        tcbkey->bias = k.bias;
        tcbkey->easeto = k.easeto;
        tcbkey->easefrom = k.easefrom;

        tcbkey->SetValue(k.value);
    }

    void SetKey(int index, IKey* key) override
    {
        assert(index >= 0 && index < GetNumKeys());
        assert(key != 0);
        typename Spline::key_type& k = m_spline->key(index);
        ITcbKey* tcbkey = (ITcbKey*)key;
        k.time = tcbkey->time;
        k.flags = tcbkey->flags;
        k.tens = tcbkey->tens;
        k.cont = tcbkey->cont;
        k.bias = tcbkey->bias;
        k.easeto = tcbkey->easeto;
        k.easefrom = tcbkey->easefrom;
        tcbkey->GetValue(k.value);
        Invalidate();
    }

    float GetKeyTime(int index) const override
    {
        assert(index >= 0 && index < GetNumKeys());
        return m_spline->time(index);
    }
    void SetKeyTime(int index, float time) override
    {
        assert(index >= 0 && index < GetNumKeys());
        m_spline->SetKeyTime(index, time);
        Invalidate();
    }
    int GetKeyFlags(int index) override
    {
        assert(index >= 0 && index < GetNumKeys());
        return m_spline->key(index).flags;
    }
    void SetKeyFlags(int index, int flags) override
    {
        assert(index >= 0 && index < GetNumKeys());
        m_spline->key(index).flags = flags;
    }

    EUiAnimCurveType GetCurveType() override { assert(0); return eUiAnimCurveType_Unknown; }
    EUiAnimValue GetValueType() override { assert(0); return eUiAnimValue_Unknown; }

    void GetValue(float time, float& value) override { assert(0); }
    void GetValue([[maybe_unused]] float time, [[maybe_unused]] Vec3& value) override { assert(0); }
    void GetValue([[maybe_unused]] float time, [[maybe_unused]] Vec4& value) override { assert(0); }
    void GetValue([[maybe_unused]] float time, [[maybe_unused]] Quat& value) override { assert(0); }
    void GetValue([[maybe_unused]] float time, [[maybe_unused]] bool& value) override { assert(0); }
    void GetValue([[maybe_unused]] float time, [[maybe_unused]] AZ::Vector2& value) override { assert(0); }
    void GetValue([[maybe_unused]] float time, [[maybe_unused]] AZ::Vector3& value) override { assert(0); }
    void GetValue([[maybe_unused]] float time, [[maybe_unused]] AZ::Vector4& value) override { assert(0); }
    void GetValue([[maybe_unused]] float time, [[maybe_unused]] AZ::Color& value) override { assert(0); }

    void SetValue(float time, const float& value, bool bDefault = false) override { assert(0); }
    void SetValue([[maybe_unused]] float time, [[maybe_unused]] const Vec3& value, [[maybe_unused]] bool bDefault = false) override { assert(0); }
    void SetValue([[maybe_unused]] float time, [[maybe_unused]] const Vec4& value, [[maybe_unused]] bool bDefault = false) override { assert(0); }
    void SetValue([[maybe_unused]] float time, [[maybe_unused]] const Quat& value, [[maybe_unused]] bool bDefault = false) override { assert(0); }
    void SetValue([[maybe_unused]] float time, [[maybe_unused]] const bool& value, [[maybe_unused]] bool bDefault = false) override { assert(0); }
    void SetValue([[maybe_unused]] float time, [[maybe_unused]] const AZ::Vector2& value, [[maybe_unused]] bool bDefault = false) override { assert(0); }
    void SetValue([[maybe_unused]] float time, [[maybe_unused]] const AZ::Vector3& value, [[maybe_unused]] bool bDefault = false) override { assert(0); }
    void SetValue([[maybe_unused]] float time, [[maybe_unused]] const AZ::Vector4& value, [[maybe_unused]] bool bDefault = false) override { assert(0); }
    void SetValue([[maybe_unused]] float time, [[maybe_unused]] const AZ::Color& value, [[maybe_unused]] bool bDefault = false) override { assert(0); }

    void OffsetKeyPosition([[maybe_unused]] const Vec3& value) override { assert(0); };

    bool Serialize(IUiAnimationSystem* uiAnimationSystem, XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;
    bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset) override;

    void GetKeyInfo(int key, const char*& description, float& duration) override
    {
        description = 0;
        duration = 0;
    }

    //! Sort keys in track (after time of keys was modified).
    void SortKeys() override
    {
        m_spline->sort_keys();
    };

    //! Get track flags.
    int GetFlags() override { return m_flags; }

    //! Check if track is masked by mask
    bool IsMasked([[maybe_unused]] const uint32 mask) const override { return false; }

    //! Set track flags.
    void SetFlags(int flags) override
    {
        m_flags = flags;
        if (m_flags & eUiAnimTrackFlags_Loop)
        {
            m_spline->ORT(Spline::ORT_LOOP);
        }
        else if (m_flags & eUiAnimTrackFlags_Cycle)
        {
            m_spline->ORT(Spline::ORT_CYCLE);
        }
        else
        {
            m_spline->ORT(Spline::ORT_CONSTANT);
        }
    }

    void Invalidate()
    {
        m_spline->flag_set(Spline::MODIFIED);
    };

    void SetTimeRange(const Range& timeRange) override
    {
        m_spline->SetRange(timeRange.start, timeRange.end);
    }

    int FindKey(float time) override
    {
        // Find key with given time.
        int num = m_spline->num_keys();
        for (int i = 0; i < num; i++)
        {
            float keyt = m_spline->key(i).time;
            if (fabs(keyt - time) < MIN_TIME_PRECISION)
            {
                return i;
            }
        }
        return -1;
    }

    //! Create key at given time, and return its index.
    int CreateKey(float time) override
    {
        ValueType value;

        int nkey = GetNumKeys();

        if (nkey > 0)
        {
            GetValue(time, value);
        }
        else
        {
            value = m_defaultValue;
        }

        typename Spline::ValueType tmp;
        m_spline->ToValueType(value, tmp);
        return m_spline->InsertKey(time, tmp);
    }

    int CloneKey(int srcKey) override
    {
        return CopyKey(this, srcKey);
    }

    int CopyKey(IUiAnimTrack* pFromTrack, int nFromKey) override
    {
        ITcbKey key;
        pFromTrack->GetKey(nFromKey, &key);
        int nkey = GetNumKeys();
        SetNumKeys(nkey + 1);
        SetKey(nkey, &key);
        return nkey;
    }

    //! Get key at given time,
    //! If key not exist adds key at this time.
    void SetKeyAtTime(float time, IKey* key)
    {
        assert(key != 0);

        key->time = time;

        bool found = false;
        // Find key with given time.
        for (int i = 0; i < m_spline->num_keys(); i++)
        {
            float keyt = m_spline->key(i).time;
            if (fabs(keyt - time) < MIN_TIME_PRECISION)
            {
                key->flags = m_spline->key(i).flags;                // Reserve the flag value.
                SetKey(i, key);
                found = true;
                break;
            }
            //if (keyt > time)
            //break;
        }
        if (!found)
        {
            // Key with this time not found.
            // Create a new one.
            int keyIndex = CreateKey(time);
            // Reserve the flag value.
            key->flags = m_spline->key(keyIndex).flags;     // Reserve the flag value.
            SetKey(keyIndex, key);
        }
    }

    virtual void SetDefaultValue(const ValueType& value)
    {
        m_defaultValue = value;
    }

    ColorB GetCustomColor() const
    { return m_customColor; }
    void SetCustomColor(ColorB color)
    {
        m_customColor = color;
        m_bCustomColorSet = true;
    }
    bool HasCustomColor() const
    { return m_bCustomColorSet; }
    void ClearCustomColor()
    { m_bCustomColorSet = false; }

    static void Reflect(AZ::SerializeContext* serializeContext) {}

protected:

    void UpdateTrackValueRange(float newValue)
    {
        m_fMinKeyValue = (newValue < m_fMinKeyValue) ? newValue : m_fMinKeyValue;
        m_fMaxKeyValue = (newValue > m_fMaxKeyValue) ? newValue : m_fMaxKeyValue;
        if ((m_fMaxKeyValue - m_fMinKeyValue) < MIN_VALUE_RANGE)
        {
            // prevents fill sliders from being inoperable when min and max are identical (or close to it)
            m_fMaxKeyValue = (m_fMinKeyValue + MIN_VALUE_RANGE);
        }
    };

private:
    //! Spawns new instance of Tcb spline.
    void AllocSpline()
    {
        m_spline = aznew UiSpline::TrackSplineInterpolator<ValueType>;
    }

    int m_refCount;

    typedef UiSpline::TrackSplineInterpolator<ValueType> Spline;
    AZStd::intrusive_ptr<Spline> m_spline;
    ValueType m_defaultValue;

    //! Keys of float track.
    int m_flags;
    CUiAnimParamType m_nParamType;

    ColorB m_customColor;
    bool m_bCustomColorSet;

    float m_fMinKeyValue;
    float m_fMaxKeyValue;

    UiAnimParamData m_componentParamData;

    static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement) {};
};

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void TUiAnimSplineTrack<T>::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void TUiAnimSplineTrack<T>::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

template <class T>
inline bool TUiAnimSplineTrack<T>::Serialize(IUiAnimationSystem* uiAnimationSystem, XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        int num = xmlNode->getChildCount();

        int flags = m_flags;
        xmlNode->getAttr("Flags", flags);
        xmlNode->getAttr("defaultValue", m_defaultValue);
        SetFlags(flags);
        xmlNode->getAttr("HasCustomColor", m_bCustomColorSet);

        if (m_bCustomColorSet)
        {
            unsigned int abgr;
            xmlNode->getAttr("CustomColor", abgr);
            m_customColor = ColorB(abgr);
        }

        T value;
        SetNumKeys(num);
        for (int i = 0; i < num; i++)
        {
            ITcbKey key; // Must be inside loop.

            XmlNodeRef keyNode = xmlNode->getChild(i);
            keyNode->getAttr("time", key.time);

            if (keyNode->getAttr("value", value))
            {
                key.SetValue(value);
            }

            keyNode->getAttr("tens", key.tens);
            keyNode->getAttr("cont", key.cont);
            keyNode->getAttr("bias", key.bias);
            keyNode->getAttr("easeto", key.easeto);
            keyNode->getAttr("easefrom", key.easefrom);
            keyNode->getAttr("flags", key.flags);

            SetKey(i, &key);

            // In-/Out-tangent
            keyNode->getAttr("ds", m_spline->key(i).ds);
            keyNode->getAttr("dd", m_spline->key(i).dd);
        }

        if ((!num) && (!bLoadEmptyTracks))
        {
            return false;
        }
    }
    else
    {
        int num = GetNumKeys();
        xmlNode->setAttr("Flags", GetFlags());
        xmlNode->setAttr("defaultValue", m_defaultValue);
        xmlNode->setAttr("HasCustomColor", m_bCustomColorSet);
        if (m_bCustomColorSet)
        {
            xmlNode->setAttr("CustomColor", m_customColor.pack_abgr8888());
        }

        ITcbKey key;
        T value;
        for (int i = 0; i < num; i++)
        {
            GetKey(i, &key);
            XmlNodeRef keyNode = xmlNode->newChild("Key");
            keyNode->setAttr("time", key.time);

            key.GetValue(value);
            keyNode->setAttr("value", value);

            if (key.tens != 0)
            {
                keyNode->setAttr("tens", key.tens);
            }
            if (key.cont != 0)
            {
                keyNode->setAttr("cont", key.cont);
            }
            if (key.bias != 0)
            {
                keyNode->setAttr("bias", key.bias);
            }
            if (key.easeto != 0)
            {
                keyNode->setAttr("easeto", key.easeto);
            }
            if (key.easefrom != 0)
            {
                keyNode->setAttr("easefrom", key.easefrom);
            }

            int flags = key.flags;
            // Just save the in/out mask part. Others are for editing convenience.
            flags &= (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK);
            if (flags != 0)
            {
                keyNode->setAttr("flags", flags);
            }

            // We also have to save in-/out-tangents, because TCB infos are not used for custom tangent keys.
            keyNode->setAttr("ds", m_spline->key(i).ds);
            keyNode->setAttr("dd", m_spline->key(i).dd);
        }
    }
    return true;
}

template <class T>
inline bool TUiAnimSplineTrack<T>::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset)
{
    if (bLoading)
    {
        int numCur = GetNumKeys();
        int num = xmlNode->getChildCount();

        int type;
        xmlNode->getAttr("TrackType", type);

        if (type != GetCurveType())
        {
            return false;
        }

        T value;
        SetNumKeys(num + numCur);
        for (int i = 0; i < num; i++)
        {
            ITcbKey key; // Must be inside loop.

            XmlNodeRef keyNode = xmlNode->getChild(i);
            keyNode->getAttr("time", key.time);
            key.time += fTimeOffset;

            if (keyNode->getAttr("value", value))
            {
                key.SetValue(value);
            }

            keyNode->getAttr("tens", key.tens);
            keyNode->getAttr("cont", key.cont);
            keyNode->getAttr("bias", key.bias);
            keyNode->getAttr("easeto", key.easeto);
            keyNode->getAttr("easefrom", key.easefrom);
            keyNode->getAttr("flags", key.flags);

            SetKey(i + numCur, &key);

            if (bCopySelected)
            {
                SelectKey(i + numCur, true);
            }

            // In-/Out-tangent
            keyNode->getAttr("ds", m_spline->key(i + numCur).ds);
            keyNode->getAttr("dd", m_spline->key(i + numCur).dd);
        }
        SortKeys();
    }
    else
    {
        int num = GetNumKeys();
        xmlNode->setAttr("TrackType", GetCurveType());

        ITcbKey key;
        T value;
        for (int i = 0; i < num; i++)
        {
            GetKey(i, &key);

            if (!bCopySelected || IsKeySelected(i))
            {
                XmlNodeRef keyNode = xmlNode->newChild("Key");
                keyNode->setAttr("time", key.time);

                key.GetValue(value);
                keyNode->setAttr("value", value);

                if (key.tens != 0)
                {
                    keyNode->setAttr("tens", key.tens);
                }
                if (key.cont != 0)
                {
                    keyNode->setAttr("cont", key.cont);
                }
                if (key.bias != 0)
                {
                    keyNode->setAttr("bias", key.bias);
                }
                if (key.easeto != 0)
                {
                    keyNode->setAttr("easeto", key.easeto);
                }
                if (key.easefrom != 0)
                {
                    keyNode->setAttr("easefrom", key.easefrom);
                }

                int flags = key.flags;
                // Just save the in/out mask part. Others are for editing convenience.
                flags &= (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK);
                if (flags != 0)
                {
                    keyNode->setAttr("flags", flags);
                }

                // We also have to save in-/out-tangents, because TCB infos are not used for custom tangent keys.
                keyNode->setAttr("ds", m_spline->key(i).ds);
                keyNode->setAttr("dd", m_spline->key(i).dd);
            }
        }
    }
    return true;
}

// This is the current main.
#include "AnimSplineTrack_Vec2Specialization.h"
typedef TUiAnimSplineTrack<Vec2>        C2DSplineTrack;

#if 0
// Followings are deprecated and supported only for the backward compatibility.
#include "AnimSplineTrack_FloatSpecialization.h"
#include "AnimSplineTrack_Vec3Specialization.h"
#include "AnimSplineTrack_QuatSpecialization.h"
typedef TUiAnimSplineTrack<float>   CTcbFloatTrack;
typedef TUiAnimSplineTrack<Vec3>        CTcbVectorTrack;
typedef TUiAnimSplineTrack<Quat>        CTcbQuatTrack;
#endif
