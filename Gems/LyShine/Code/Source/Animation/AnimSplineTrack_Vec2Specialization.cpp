/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AnimSplineTrack.h"
#include "2DSpline.h"
#include <LyShine/UiBase.h>
#include <AzCore/Serialization/SerializeContext.h>

template <>
TUiAnimSplineTrack<Vec2>::TUiAnimSplineTrack()
    : m_refCount(0)
{
    AllocSpline();
    m_flags = 0;
    m_defaultValue = Vec2(0, 0);
    m_fMinKeyValue = 0.0f;
    m_fMaxKeyValue = 0.0f;
    m_bCustomColorSet = false;
}

template <>
void TUiAnimSplineTrack<Vec2>::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
template <>
void TUiAnimSplineTrack<Vec2>::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

template <>
void TUiAnimSplineTrack<Vec2>::GetValue(float time, float& value)
{
    if (GetNumKeys() == 0)
    {
        value = m_defaultValue.y;
    }
    else
    {
        Spline::ValueType tmp;
        m_spline->Interpolate(time, tmp);
        value = tmp[0];
    }
}
template <>
EUiAnimCurveType TUiAnimSplineTrack<Vec2>::GetCurveType() { return eUiAnimCurveType_BezierFloat; }
template <>
EUiAnimValue TUiAnimSplineTrack<Vec2>::GetValueType() { return eUiAnimValue_Float; }
template <>
void TUiAnimSplineTrack<Vec2>::SetValue(float time, const float& value, bool bDefault)
{
    if (!bDefault)
    {
        I2DBezierKey key;
        key.value = Vec2(time, value);
        SetKeyAtTime(time, &key);
    }
    else
    {
        m_defaultValue = Vec2(time, value);
    }
}

template <>
void TUiAnimSplineTrack<Vec2>::GetKey(int index, IKey* key) const
{
    assert(index >= 0 && index < GetNumKeys());
    assert(key != 0);
    Spline::key_type& k = m_spline->key(index);
    I2DBezierKey* bezierkey = (I2DBezierKey*)key;
    bezierkey->time = k.time;
    bezierkey->flags = k.flags;

    bezierkey->value = k.value;
}

template <>
void TUiAnimSplineTrack<Vec2>::SetKey(int index, IKey* key)
{
    assert(index >= 0 && index < GetNumKeys());
    assert(key != 0);
    Spline::key_type& k = m_spline->key(index);
    I2DBezierKey* bezierkey = (I2DBezierKey*)key;
    k.time = bezierkey->time;
    k.flags = bezierkey->flags;
    k.value = bezierkey->value;
    UpdateTrackValueRange(k.value.y);
    Invalidate();
}

//! Create key at given time, and return its index.
template <>
int TUiAnimSplineTrack<Vec2>::CreateKey(float time)
{
    float value;

    int nkey = GetNumKeys();

    if (nkey > 0)
    {
        GetValue(time, value);
    }
    else
    {
        value = m_defaultValue.y;
    }

    UpdateTrackValueRange(value);

    Spline::ValueType tmp;
    tmp[0] = value;
    tmp[1] = 0;
    return m_spline->InsertKey(time, tmp);
}

template <>
int TUiAnimSplineTrack<Vec2>::CopyKey(IUiAnimTrack* pFromTrack, int nFromKey)
{
    // This small time offset is applied to prevent the generation of singular tangents.
    float timeOffset = 0.01f;
    I2DBezierKey key;
    pFromTrack->GetKey(nFromKey, &key);
    float t = key.time + timeOffset;
    int newIndex =  CreateKey(t);
    key.time = key.value.x = t;
    SetKey(newIndex, &key);
    return newIndex;
}

template <>
bool TUiAnimSplineTrack<Vec2>::Serialize([[maybe_unused]] IUiAnimationSystem* uiAnimationSystem, XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
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

        SetNumKeys(num);
        for (int i = 0; i < num; i++)
        {
            I2DBezierKey key; // Must be inside loop.

            XmlNodeRef keyNode = xmlNode->getChild(i);
            if (!keyNode->getAttr("time", key.time))
            {
                CryLog("[UI_ANIMATION:TUiAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing time information.");
                return false;
            }
            if (!keyNode->getAttr("value", key.value))
            {
                CryLog("[UI_ANIMATION:TUiAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing value information.");
                return false;
            }
            //assert(key.time == key.value.x);

            keyNode->getAttr("flags", key.flags);

            SetKey(i, &key);

            // In-/Out-tangent
            if (!keyNode->getAttr("ds", m_spline->key(i).ds))
            {
                CryLog("[UI_ANIMATION:TUiAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing ds spline information.");
                return false;
            }

            if (!keyNode->getAttr("dd", m_spline->key(i).dd))
            {
                CryLog("[UI_ANIMATION:TUiAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:dd spline information.");
                return false;
            }
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
        I2DBezierKey key;
        for (int i = 0; i < num; i++)
        {
            GetKey(i, &key);
            XmlNodeRef keyNode = xmlNode->newChild("Key");
            assert(key.time == key.value.x);
            keyNode->setAttr("time", key.time);
            keyNode->setAttr("value", key.value);

            int flags = key.flags;
            // Just save the in/out/unify mask part. Others are for editing convenience.
            flags &= (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK | SPLINE_KEY_TANGENT_UNIFY_MASK);
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

template <>
bool TUiAnimSplineTrack<Vec2>::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset)
{
    if (bLoading)
    {
        int numCur = GetNumKeys();
        int num = xmlNode->getChildCount();

        unsigned int type;
        xmlNode->getAttr("TrackType", type);

        if (type != GetCurveType())
        {
            return false;
        }

        SetNumKeys(num + numCur);
        for (int i = 0; i < num; i++)
        {
            I2DBezierKey key; // Must be inside loop.

            XmlNodeRef keyNode = xmlNode->getChild(i);
            keyNode->getAttr("time", key.time);
            keyNode->getAttr("value", key.value);
            assert(key.time == key.value.x);
            key.time += fTimeOffset;
            key.value.x += fTimeOffset;

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

        I2DBezierKey key;
        for (int i = 0; i < num; i++)
        {
            GetKey(i, &key);
            assert(key.time == key.value.x);

            if (!bCopySelected || IsKeySelected(i))
            {
                XmlNodeRef keyNode = xmlNode->newChild("Key");
                keyNode->setAttr("time", key.time);
                keyNode->setAttr("value", key.value);

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

//////////////////////////////////////////////////////////////////////////
template<>
void TUiAnimSplineTrack<Vec2>::GetKeyInfo(int index, const char*& description, float& duration)
{
    duration = 0;

    static char str[64];
    description = str;
    assert(index >= 0 && index < GetNumKeys());
    Spline::key_type& k = m_spline->key(index);
    sprintf_s(str, "%.2f", k.value.y);
}

//////////////////////////////////////////////////////////////////////////
namespace UiSpline
{
    using BezierSplineVec2 = BezierSpline<Vec2, SplineKeyEx<Vec2> >;
    using TSplineBezierBasisVec2 = TSpline<SplineKeyEx<Vec2>, spline::BezierBasis>;


    // Implement Reflection functions for Spline full template specializations in a cpp file
    //////////////////////////////////////////////////////////////////////////
    template <>
    void TSplineBezierBasisVec2::Reflect(AZ::ReflectContext* context);

    //////////////////////////////////////////////////////////////////////////
    template <>
    void BezierSplineVec2::Reflect(AZ::ReflectContext* context);
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_DECL(TrackSplineInterpolator<Vec2>);
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(TrackSplineInterpolator<Vec2>, "TrackSplineInterpolator<Vec2>", "{38F814D4-6041-4442-9704-9F68E996D55B}");
    AZ_TYPE_INFO_SPECIALIZE(SplineKey<Vec2>, "{E2301E81-6BAF-4A17-886C-76F1A9C37118}");
    AZ_TYPE_INFO_SPECIALIZE(SplineKeyEx<Vec2>, "{1AE37C63-D5C2-4E65-A08B-7020E7696233}");
    AZ_TYPE_INFO_SPECIALIZE(BezierSplineVec2, "{EC8BA7BD-EF3B-453A-8017-CD1BF5B7C011}");
    AZ_TYPE_INFO_SPECIALIZE(TSplineBezierBasisVec2, "{B661D05E-B912-4BD9-B102-FA82938243A9}");

    template<>
    void SplineKey<Vec2>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<SplineKey<Vec2> >()
                ->Version(1)
                ->Field("time", &SplineKey<Vec2>::time)
                ->Field("flags", &SplineKey<Vec2>::flags)
                ->Field("value", &SplineKey<Vec2>::value)
                ->Field("ds", &SplineKey<Vec2>::ds)
                ->Field("dd", &SplineKey<Vec2>::dd);
        }
    }

    template<>
    void SplineKeyEx<Vec2>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<SplineKeyEx<Vec2>, SplineKey<Vec2> >()
                ->Version(1)
                ;
        }
    }

    template <>
    void TSplineBezierBasisVec2::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<TSplineBezierBasisVec2>()
                ->Version(1)
                ->Field("Keys", &BezierSplineVec2::m_keys);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template <>
    void BezierSplineVec2::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            TSplineBezierBasisVec2::Reflect(serializeContext);

            serializeContext->Class<BezierSplineVec2, TSplineBezierBasisVec2>()
                ->Version(1)
                ;
        }
    }

    void TrackSplineInterpolator<Vec2>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<TrackSplineInterpolator<Vec2>,
                UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >()
                ->Version(1)
                ;
        }
    }
}

// When TUiAnimSplineTrack<Vec2> is deserialized, a spline instance
// is first created in the TUiAnimSplineTrack<Vec2> constructor (via AllocSpline()),
// then the pointer is overwritten when "Spline" field is deserialized.
// To prevent a memory leak, m_spline is now an intrusive pointer, so that if/when
// the "Spline" field is deserialized, the old object will be deleted.
static bool TUiAnimSplineTrackVec2VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    bool converted = false;
    if (classElement.GetVersion() == 1)
    {
        int splineElementIdx = classElement.FindElement(AZ_CRC_CE("Spline"));
        if (splineElementIdx != -1)
        {
            // Find & copy the raw pointer node
            AZ::SerializeContext::DataElementNode& splinePtrNodeRef = classElement.GetSubElement(splineElementIdx);
            AZ::SerializeContext::DataElementNode splinePtrNodeCopy = splinePtrNodeRef;

            // Reset the node, then convert it to an intrusive pointer
            splinePtrNodeRef = AZ::SerializeContext::DataElementNode();
            const bool result = splinePtrNodeRef.Convert<AZStd::intrusive_ptr<UiSpline::TrackSplineInterpolator<Vec2>>>(context, "Spline");
            if (result)
            {
                // Use the standard name used with the smart pointers serialization
                // (smart pointers are serialized as containers with one element);
                // Set the intrusive pointer to the raw pointer value
                splinePtrNodeCopy.SetName(AZ::SerializeContext::IDataContainer::GetDefaultElementName());
                splinePtrNodeRef.AddElement(splinePtrNodeCopy);

                converted = true;
            }
        }
    }

    // Did not convert. Discard unknown versions if failed to convert, and hope for the best
    AZ_Assert(converted, "Failed to convert TUiAnimSplineTrack<Vec2> version %d to the current version", classElement.GetVersion());
    return converted;
}

template<>
void TUiAnimSplineTrack<Vec2>::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
    {
        UiSpline::SplineKey<Vec2>::Reflect(serializeContext);
        UiSpline::SplineKeyEx<Vec2>::Reflect(serializeContext);

        UiSpline::TrackSplineInterpolator<Vec2>::Reflect(serializeContext);
        UiSpline::BezierSplineVec2::Reflect(serializeContext);


        serializeContext->Class<TUiAnimSplineTrack<Vec2> >()
            ->Version(2, &TUiAnimSplineTrackVec2VersionConverter)
            ->Field("Flags", &TUiAnimSplineTrack<Vec2>::m_flags)
            ->Field("DefaultValue", &TUiAnimSplineTrack<Vec2>::m_defaultValue)
            ->Field("ParamType", &TUiAnimSplineTrack<Vec2>::m_nParamType)
            ->Field("ParamData", &TUiAnimSplineTrack<Vec2>::m_componentParamData)
            ->Field("Spline", &TUiAnimSplineTrack<Vec2>::m_spline);
    }
}
