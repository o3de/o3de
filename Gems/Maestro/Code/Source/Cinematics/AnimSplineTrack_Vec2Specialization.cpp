/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : 'Vec2' explicit specialization of the class template
//               TAnimSplineTrack
// Notice      : Should be included in AnimSplineTrack h only

#include "AnimSplineTrack.h"
#include "2DSpline.h"
#include <AzCore/Serialization/EditContext.h>


namespace spline
{
    template<>
    void SplineKey<Vec2>::Reflect(AZ::ReflectContext* context);
}

namespace Maestro
{

    template<>
    TAnimSplineTrack<Vec2>::TAnimSplineTrack()
        : m_refCount(0)
    {
        AllocSpline();
        m_flags = 0;
        m_defaultValue = Vec2(0, 0);
        m_fMinKeyValue = 0.0f;
        m_fMaxKeyValue = 0.0f;
        m_bCustomColorSet = false;
        m_node = nullptr;
        m_trackMultiplier = 1.0f;
    }

    template<>
    void TAnimSplineTrack<Vec2>::add_ref()
    {
        ++m_refCount;
    }

    template<>
    void TAnimSplineTrack<Vec2>::release()
    {
        if (--m_refCount <= 0)
        {
            delete this;
        }
    }

    template<>
    void TAnimSplineTrack<Vec2>::GetValue(float time, float& value, bool applyMultiplier) const
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

        if (applyMultiplier && m_trackMultiplier != 1.0f && m_trackMultiplier > AZ::Constants::Tolerance)
        {
            value /= m_trackMultiplier;
        }
    }

    template<>
    EAnimCurveType TAnimSplineTrack<Vec2>::GetCurveType() const
    {
        return eAnimCurveType_BezierFloat;
    }

    template<>
    AnimValueType TAnimSplineTrack<Vec2>::GetValueType() const
    {
        return kAnimValueDefault;
    }

    template<>
    void TAnimSplineTrack<Vec2>::SetValue(float time, const float& value, bool bDefault, bool applyMultiplier)
    {
        const Range timeRange(GetTimeRange().start, GetTimeRange().end);
        if ((timeRange.end - timeRange.start > AZ::Constants::Tolerance) && (time < timeRange.start || time > timeRange.end))
        {
            AZ_WarningOnce("AnimSplineTrack", false, "SetValue(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                time, timeRange.start, timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
            AZStd::clamp(time, timeRange.start, timeRange.end);
        }

        if (!bDefault)
        {
            I2DBezierKey key;
            if (applyMultiplier && m_trackMultiplier != 1.0f)
            {
                key.value = Vec2(time, value * m_trackMultiplier);
            }
            else
            {
                key.value = Vec2(time, value);
            }
            SetKeyAtTime(time, &key);
        }
        else
        {
            if (applyMultiplier && m_trackMultiplier != 1.0f)
            {
                m_defaultValue.set(time, value * m_trackMultiplier);
            }
            else
            {
                m_defaultValue.set(time, value);
            }
        }
    }

    template<>
    void TAnimSplineTrack<Vec2>::GetKey(int keyIndex, IKey* key) const
    {
        if (keyIndex < 0 || keyIndex >= GetNumKeys() || !key || !(m_spline))
        {
            AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            AZ_Assert(key, "Key is null");
            AZ_Assert((m_spline), "Invalid spline.");
            return;
        }

        Spline::key_type& k = m_spline->key(keyIndex);
        I2DBezierKey* bezierkey = (I2DBezierKey*)key;
        bezierkey->time = k.time;
        bezierkey->flags = k.flags;

        bezierkey->value = k.value;
    }

    template<>
    void TAnimSplineTrack<Vec2>::SetKey(int keyIndex, IKey* key)
    {
        if (keyIndex < 0 || keyIndex >= GetNumKeys() || !key || !(m_spline))
        {
            AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            AZ_Assert(key, "Key is null");
            AZ_Assert((m_spline), "Invalid spline.");
            return;
        }

        Spline::key_type& k = m_spline->key(keyIndex);
        I2DBezierKey* bezierkey = (I2DBezierKey*)key;
        k.time = bezierkey->time;
        k.flags = bezierkey->flags;
        k.value = bezierkey->value;
        UpdateTrackValueRange(k.value.y);

        Invalidate();
        SortKeys();
    }

    //! Create key at given time, and return its index.
    template<>
    int TAnimSplineTrack<Vec2>::CreateKey(float time)
    {
        const Range timeRange(GetTimeRange());
        if ((timeRange.end - timeRange.start > AZ::Constants::Tolerance) && (time < timeRange.start || time > timeRange.end))
        {
            AZ_WarningOnce("AnimSplineTrack", false, "CreateKey(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                time, timeRange.start, timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
            AZStd::clamp(time, timeRange.start, timeRange.end);
        }

        float value;

        const int numKeys = GetNumKeys();

        bool bUseDefault = true;
        if (numKeys > 0)
        {
            SortKeys();
            // Check that no keys exist at the same time
            auto prevKeyIdx = numKeys - 1;
            while ((prevKeyIdx > 0) && (GetKeyTime(prevKeyIdx) > time))
            {
                --prevKeyIdx;
            }
            float dt = AZStd::abs(time - GetKeyTime(prevKeyIdx)); // delta time to previous key
            if (prevKeyIdx < numKeys - 1) // Is there next key?
            {
                dt = AZStd::min(dt, AZStd::abs(time - GetKeyTime(prevKeyIdx + 1))); // delta time to closest key
            }
            if (dt < GetMinKeyTimeDelta())
            {
                AZ_Error("AnimSplineTrack", false, "CreateKey(%f): A key at this time exists in track (%s).", time, (GetNode() ? GetNode()->GetName() : ""));
                return -1; // a key is too close in time, reject adding a key
            }
            // Check if Default Value was recently updated, and is closer in time than existing keys
            bUseDefault = AZStd::abs(time - m_defaultValue.x) < dt;
        }

        if (bUseDefault)
        {
            value = m_defaultValue.y;
        }
        else
        {
            GetValue(time, value);
        }

        UpdateTrackValueRange(value);

        Spline::ValueType tmp;
        tmp[0] = value;
        tmp[1] = 0.f;
        tmp[2] = 0.f;
        tmp[3] = 0.f;
        m_spline->InsertKey(time, tmp);

        Invalidate();
        SortKeys();

        return FindKey(time);
    }

    template<>
    int TAnimSplineTrack<Vec2>::CloneKey(int srcKeyIndex, float timeOffset)
    {
        const auto numKeys = GetNumKeys();
        if (srcKeyIndex < 0 || srcKeyIndex >= numKeys)
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", srcKeyIndex, numKeys);
            return -1;
        }

        I2DBezierKey key;
        GetKey(srcKeyIndex, &key);

        // Key time offset must be big enough to prevent generation of singular tangents
        const auto minTimeOffset = AZStd::max(0.01f, GetMinKeyTimeDelta() * 1.1f);
        if (AZStd::abs(timeOffset) < minTimeOffset)
        {
            timeOffset = timeOffset >= 0.0f ? minTimeOffset : minTimeOffset;
        }

        key.time += timeOffset;

        const auto timeRange = GetTimeRange();
        AZStd::clamp(key.time, timeRange.start, timeRange.end);

        // Check that no key is too close
        for (int i = 0; i < numKeys; ++i)
        {
            I2DBezierKey aKey;
            GetKey(i, &aKey);

            const auto dt = AZStd::abs(aKey.time - key.time);
            if (dt < minTimeOffset)
            {
                AZ_Error("AnimSplineTrack", false,
                    "CloneKey(%d, %f): A key at time (%f) with index (%d) in this track (%s) is too close to cloned key time (%f).",
                    srcKeyIndex, timeOffset, aKey.time, i, (GetNode() ? GetNode()->GetName() : ""), key.time);
                return -1;
            }
        }

        const auto newIndex = CreateKey(key.time);
        if (newIndex < 0)
        {
            return -1;
        }

        key.value.x = key.time;
        SetKey(newIndex, &key);

        SortKeys();

        return FindKey(key.time);
    }

    template<>
    int TAnimSplineTrack<Vec2>::CopyKey(IAnimTrack* pFromTrack, int fromKeyIndex)
    {
        if (!pFromTrack ||  fromKeyIndex < 0 || fromKeyIndex >= pFromTrack->GetNumKeys())
        {
            AZ_Assert(pFromTrack != nullptr, "Expected valid track pointer.");
            AZ_Assert(fromKeyIndex >= 0 && fromKeyIndex < pFromTrack->GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", fromKeyIndex, GetNumKeys());
            return -1;
        }

        I2DBezierKey key;
        pFromTrack->GetKey(fromKeyIndex, &key);

        if (this == pFromTrack)
        {
            // Shift key time to prevent generation of singular tangents, with offset selected bigger than minimal legal key time delta.
            const float timeOffset = AZStd::max(0.01f, GetMinKeyTimeDelta() * 1.1f);
            const auto timeRange = GetTimeRange();
            bool allowToAddKey = timeRange.end - timeRange.start > timeOffset;
            if (allowToAddKey)
            {
                key.time += timeOffset;
                if (key.time > timeRange.end)
                {
                    key.time -= timeOffset * 2.0f;
                    if (key.time < timeRange.start)
                    {
                        allowToAddKey = false;
                    }
                }
            }
            if (!allowToAddKey)
            {
                AZ_Error("AnimSplineTrack", false, "CopyKey(%s, %d): Too narrow time range (%f .. %f) to clone key in this track.",
                    (GetNode() ? GetNode()->GetName() : ""), fromKeyIndex, timeRange.start, timeRange.end);
                return -1;
            }

            const auto existingKeyIndex = FindKey(key.time);
            if (existingKeyIndex >= 0)
            {
                AZ_Error("AnimSplineTrack", false, "CopyKey(%s, %d): A key at time (%f) with index (%d) already exists in this track.",
                    (GetNode() ? GetNode()->GetName() : ""), fromKeyIndex, key.time, existingKeyIndex);
                return -1;
            }
        }
        else
        {
            const auto existingKeyIndex = FindKey(key.time);
            if (existingKeyIndex >= 0)
            {
                AZ_Error("AnimSplineTrack", false, "CopyKey(%s, %d): A key at time (%f) with index (%d) already exists in this track (%s).",
                    (pFromTrack->GetNode() ? pFromTrack->GetNode()->GetName() : ""), fromKeyIndex, key.time, existingKeyIndex, (GetNode() ? GetNode()->GetName() : ""));
                return -1;
            }
        }

        const auto newIndex = CreateKey(key.time);
        if (newIndex < 0)
        {
            return -1;
        }

        key.value.x = key.time;
        SetKey(newIndex, &key);

        SortKeys();

        return FindKey(key.time);
    }

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    template<>
    bool TAnimSplineTrack<Vec2>::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
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
                    CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing time information.");
                    return false;
                }
                if (!keyNode->getAttr("value", key.value))
                {
                    CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing value information.");
                    return false;
                }

                keyNode->getAttr("flags", key.flags);

                SetKey(i, &key);

                // In-/Out-tangent
                if (!keyNode->getAttr("ds", m_spline->key(i).ds))
                {
                    CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing ds spline information.");
                    return false;
                }

                if (!keyNode->getAttr("dd", m_spline->key(i).dd))
                {
                    CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:dd spline information.");
                    return false;
                }
                // now that tangents are loaded, compute the relative angle and size for later unified Tangent manipulations
                m_spline->key(i).ComputeThetaAndScale();
            }

            xmlNode->getAttr("Id", m_id);

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
                AZ_Assert(key.time == key.value.x, "Invalid Bezier key at %i", i);
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
            xmlNode->setAttr("Id", m_id);
        }
        return true;
    }

    template<>
    bool TAnimSplineTrack<Vec2>::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset)
    {
        bool result = true;
        if (bLoading)
        {
            int numKeys = GetNumKeys();
            int numNewKeys = xmlNode->getChildCount();

            unsigned int type;
            xmlNode->getAttr("TrackType", type);

            if (type != GetCurveType())
            {
                return false;
            }

            SetNumKeys(numNewKeys + numKeys);
            for (int i = 0; i < numNewKeys; i++)
            {
                I2DBezierKey key; // Must be inside loop.

                XmlNodeRef keyNode = xmlNode->getChild(i);
                keyNode->getAttr("time", key.time);
                keyNode->getAttr("value", key.value);
                if (AZStd::abs(key.time - key.value.x) > AZ::Constants::FloatEpsilon)
                {
                    AZ_Assert(false, "Invalid Bezier key at %i", i);
                    result = false;
                    continue;
                }

                key.time += fTimeOffset;
                key.value.x += fTimeOffset;

                keyNode->getAttr("flags", key.flags);

                SetKey(i + numKeys, &key);

                const int newKeyIndex = FindKey(key.time);
                if (newKeyIndex < 0)
                {
                    AZ_Assert(false, "Failed to set a new key.");
                    result = false;
                    continue;
                }

                if (bCopySelected)
                {
                    SelectKey(newKeyIndex, true);
                }

                // In-/Out-tangent
                keyNode->getAttr("ds", m_spline->key(newKeyIndex).ds);
                keyNode->getAttr("dd", m_spline->key(newKeyIndex).dd);
            }
        }
        else
        {
            int numKeys = GetNumKeys();
            xmlNode->setAttr("TrackType", GetCurveType());

            I2DBezierKey key;
            for (int i = 0; i < numKeys; i++)
            {
                GetKey(i, &key);
                if (AZStd::abs(key.time - key.value.x) > AZ::Constants::FloatEpsilon)
                {
                    AZ_Assert(false, "Invalid Bezier key at %i", i);
                    result = false;
                    continue;
                }

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

        SortKeys();

        return result;
    }

    template<>
    void TAnimSplineTrack<Vec2>::GetKeyInfo(int keyIndex, const char*& description, float& duration) const
    {
        duration = 0;

        static char str[64];
        description = str;

        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        Spline::key_type& k = m_spline->key(keyIndex);
        azsnprintf(str, AZ_ARRAY_SIZE(str), "%.2f", k.value.y);
    }

} // namespace Maestro


namespace spline
{
    using BezierSplineVec2 = BezierSpline<Vec2, SplineKeyEx<Vec2>>;
    using TSplineBezierBasisVec2 = TSpline<SplineKeyEx<Vec2>, BezierBasis>;

    template <>
    void TSplineBezierBasisVec2::Reflect(AZ::ReflectContext* context);

    template <>
    void BezierSplineVec2::Reflect(AZ::ReflectContext* context);

    AZ_TYPE_INFO_SPECIALIZE(TrackSplineInterpolator<Vec2>, "{173AC8F0-FD63-4583-8D38-F43FE59F2209}");

    AZ_TYPE_INFO_SPECIALIZE(SplineKeyEx<Vec2>, "{96BCA307-A4D5-43A0-9985-08A29BCCCB30}");

    AZ_TYPE_INFO_SPECIALIZE(BezierSplineVec2, "{EE318F13-A608-4047-85B3-3D40745A19C7}");
    AZ_TYPE_INFO_SPECIALIZE(TSplineBezierBasisVec2, "{B638C840-C1D7-483A-B04E-B22DA539DB8D}");

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
                ->Version(1);
        }
    }

    void TrackSplineInterpolator<Vec2>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<TrackSplineInterpolator<Vec2>, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >()
                ->Version(1);
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

    template <>
    void BezierSplineVec2::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            TSplineBezierBasisVec2::Reflect(serializeContext);

            serializeContext->Class<BezierSplineVec2, TSplineBezierBasisVec2>()
                ->Version(1);
        }
    }
} // namespace spline

namespace Maestro
{

    // When TAnimSplineTrack<Vec2> is deserialized, a spline instance
    // is first created in the TUiAnimSplineTrack<Vec2> constructor (via AllocSpline()),
    // then the pointer is overwritten when "Spline" field is deserialized.
    // To prevent a memory leak, m_spline is now an intrusive pointer, so that if/when
    // the "Spline" field is deserialized, the old object will be deleted.
    static bool TAnimSplineTrackVec2VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        bool result = true;
        if (classElement.GetVersion() < 5)
        {
            classElement.AddElement(context, "BaseClass1", azrtti_typeid<IAnimTrack>());

            if (classElement.GetVersion() == 1)
            {
                bool converted = false;

                int splineElementIdx = classElement.FindElement(AZ_CRC_CE("Spline"));
                if (splineElementIdx != -1)
                {
                    // Find & copy the raw pointer node
                    AZ::SerializeContext::DataElementNode& splinePtrNodeRef = classElement.GetSubElement(splineElementIdx);
                    AZ::SerializeContext::DataElementNode splinePtrNodeCopy = splinePtrNodeRef;

                    // Reset the node, then convert it to an intrusive pointer
                    splinePtrNodeRef = AZ::SerializeContext::DataElementNode();
                    if (splinePtrNodeRef.Convert<AZStd::intrusive_ptr<spline::TrackSplineInterpolator<Vec2>>>(context, "Spline"))
                    {
                        // Use the standard name used with the smart pointers serialization
                        // (smart pointers are serialized as containers with one element);
                        // Set the intrusive pointer to the raw pointer value
                        splinePtrNodeCopy.SetName(AZ::SerializeContext::IDataContainer::GetDefaultElementName());
                        splinePtrNodeRef.AddElement(splinePtrNodeCopy);

                        converted = true;
                    }
                }

                // Did not convert. Discard unknown versions if failed to convert, and hope for the best
                AZ_Assert(
                    converted, "Failed to convert TUiAnimSplineTrack<Vec2> version %d to the current version", classElement.GetVersion());
                result = converted;
            }
        }

        return result;
    }

    template<>
    void TAnimSplineTrack<Vec2>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            spline::SplineKey<Vec2>::Reflect(serializeContext);
            spline::SplineKeyEx<Vec2>::Reflect(serializeContext);

            spline::TrackSplineInterpolator<Vec2>::Reflect(serializeContext);
            spline::BezierSplineVec2::Reflect(serializeContext);

            serializeContext->Class<TAnimSplineTrack<Vec2>, IAnimTrack>()
                ->Version(5, &TAnimSplineTrackVec2VersionConverter)
                ->Field("Flags", &TAnimSplineTrack<Vec2>::m_flags)
                ->Field("DefaultValue", &TAnimSplineTrack<Vec2>::m_defaultValue)
                ->Field("ParamType", &TAnimSplineTrack<Vec2>::m_nParamType)
                ->Field("Spline", &TAnimSplineTrack<Vec2>::m_spline)
                ->Field("Id", &TAnimSplineTrack<Vec2>::m_id);

            AZ::EditContext* ec = serializeContext->GetEditContext();

            // Preventing the default value from being pushed to slice to keep it from dirtying the slice when updated internally
            if (ec)
            {
                ec->Class<TAnimSplineTrack<Vec2>>("TAnimSplineTrack Vec2", "Specialization track for Vec2 AnimSpline")
                    ->DataElement(AZ::Edit::UIHandlers::Vector2, &TAnimSplineTrack<Vec2>::m_defaultValue, "DefaultValue", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                    ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushable);
            }
        }
    }

} // namespace Maestro
