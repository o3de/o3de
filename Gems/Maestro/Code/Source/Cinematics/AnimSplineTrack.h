/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include "2DSpline.h"

namespace Maestro
{

    /*!
            Templated class that used as a base for all Tcb spline tracks.
     */
    template<class ValueType>
    class TAnimSplineTrack : public IAnimTrack
    {
    public:
        AZ_CLASS_ALLOCATOR(TAnimSplineTrack, AZ::SystemAllocator);
        AZ_RTTI((TAnimSplineTrack, "{6D72D5F6-61A7-43D4-9104-8F7DCCC19E10}", ValueType), IAnimTrack);

        static constexpr void DeprecatedTypeNameVisitor(const AZ::DeprecatedTypeNameCallback& visitCallback)
        {
            // TAnimSplineTrack previously restricted the typename to 128 bytes
            AZStd::array<char, 128> deprecatedName{};

            // The old TAnimSplineTrack TypeName mistakenly used Vec2 as the template parameter and not ValueType
            // Also the extra space before the '>' is due to the AZ::Internal::AggregateTypes template
            // always adding a space after each argument
            AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), "TAnimSplineTrack<Vec2 >");

            if (visitCallback)
            {
                visitCallback(deprecatedName.data());
            }
        }

        TAnimSplineTrack()
            : m_refCount(0)
        {
            AllocSpline();
            m_flags = 0;
            m_bCustomColorSet = false;
            m_fMinKeyValue = 0.0f;
            m_fMaxKeyValue = 0.0f;
            m_node = nullptr;
            m_trackMultiplier = 1.0f;
        }

        ~TAnimSplineTrack()
        {
            m_spline.reset();
        }

        //////////////////////////////////////////////////////////////////////////
        // for intrusive_ptr support
        void add_ref() override;
        void release() override;
        //////////////////////////////////////////////////////////////////////////

        int GetSubTrackCount() const override
        {
            return 0;
        }

        IAnimTrack* GetSubTrack([[maybe_unused]] int nIndex) const override
        {
            return nullptr;
        }

        AZStd::string GetSubTrackName([[maybe_unused]] int nIndex) const override
        {
            return AZStd::string();
        }

        void SetSubTrackName([[maybe_unused]] int nIndex, [[maybe_unused]] const char* name) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetNode(IAnimNode* node) override
        {
            m_node = node;
        }

        // Return Animation Node that owns this Track.
        IAnimNode* GetNode() const override
        {
            return m_node;
        }

        const CAnimParamType& GetParameterType() const override
        {
            return m_nParamType;
        }

        void SetParameterType(CAnimParamType type) override
        {
            m_nParamType = type;
        }

        void GetKeyValueRange(float& fMin, float& fMax) const override
        {
            fMin = m_fMinKeyValue;
            fMax = m_fMaxKeyValue;
        }

        void SetKeyValueRange(float fMin, float fMax) override
        {
            m_fMinKeyValue = fMin;
            m_fMaxKeyValue = fMax;
        }

        ISplineInterpolator* GetSpline() const override
        {
            return m_spline.get();
        }

        bool IsKeySelected(int keyIndex) const override
        {
            if (!(m_spline) || keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
                AZ_Assert((m_spline), "Invalid spline.");
                return false;
            }

            return m_spline->IsKeySelectedAtAnyDimension(keyIndex);
        }

        void SelectKey(int keyIndex, bool select) override
        {
            if (!(m_spline) || keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
                AZ_Assert((m_spline), "Invalid spline.");
                return;
            }

            m_spline->SelectKeyAllDimensions(keyIndex, select);
        }

        int GetNumKeys() const override
        {
            return (m_spline) ? m_spline->num_keys() : 0;
        }

        void SetNumKeys(int numKeys) override
        {
            if (!m_spline)
            {
                AZ_Assert(false, "Invalid spline.");
                return;
            }

            m_spline->resize(numKeys);
        }

        bool HasKeys() const override
        {
            return GetNumKeys() > 0;
        }

        void RemoveKey(int keyIndex) override
        {
            if (!(m_spline) || keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
                AZ_Assert((m_spline), "Invalid spline.");
                return;
            }

            m_spline->erase(keyIndex);

            Invalidate();
            SortKeys();
        }

        void GetKey(int keyIndex, IKey* key) const override
        {
            if (!(m_spline) || keyIndex < 0 || keyIndex >= GetNumKeys() || !key)
            {
                AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
                AZ_Assert((m_spline), "Invalid spline.");
                AZ_Assert(key, "Key pointer is null");
                return;
            }

            typename Spline::key_type& k = m_spline->key(keyIndex);
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

        void SetKey(int keyIndex, IKey* key) override
        {
            if (!(m_spline) || keyIndex < 0 || keyIndex >= GetNumKeys() || !key)
            {
                AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
                AZ_Assert((m_spline), "Invalid spline.");
                AZ_Assert(key, "Key pointer is null");
                return;
            }

            typename Spline::key_type& k = m_spline->key(keyIndex);
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
            SortKeys();
        }

        float GetKeyTime(int keyIndex) const override
        {
            if (!(m_spline) || keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, m_spline->num_keys());
                AZ_Assert((m_spline), "Invalid spline.");
                return -1.0f;
            }

            return m_spline->time(keyIndex);
        }

        void SetKeyTime(int keyIndex, float time) override
        {
            if (!(m_spline) || keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, m_spline->num_keys());
                AZ_Assert((m_spline), "Invalid spline.");
                return;
            }

            const Range timeRange(GetTimeRange());
            if (((timeRange.end - timeRange.start) > AZ::Constants::Tolerance) && (time < timeRange.start || time > timeRange.end))
            {
                AZ_WarningOnce("AnimSplineTrack", false, "SetKeyTime(%d, %f): Key time is out of range (%f .. %f) in track (%s), clamped.",
                    keyIndex, time, timeRange.start, timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
                AZStd::clamp(time, timeRange.start, timeRange.end);
            }

            const int existingKeyIndex = FindKey(time);
            if (existingKeyIndex >= 0)
            {
                AZ_Error("AnimSplineTrack", existingKeyIndex == keyIndex, "SetKeyTime(%d, %f): A key with this time exists in track (%s).",
                    keyIndex, time, (GetNode() ? GetNode()->GetName() : ""));
                return;
            }

            m_spline->SetKeyTime(keyIndex, time);

            Invalidate();
            SortKeys();
        }

        int GetKeyFlags(int keyIndex) override
        {
            if (!(m_spline) || keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, m_spline->num_keys());
                AZ_Assert((m_spline), "Invalid spline.");
                return 0;
            }

            return m_spline->key(keyIndex).flags;
        }

        void SetKeyFlags(int keyIndex, int flags) override
        {
            if (!(m_spline) || keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, m_spline->num_keys());
                AZ_Assert((m_spline), "Invalid spline.");
                return;
            }

            m_spline->key(keyIndex).flags = flags;
        }

        EAnimCurveType GetCurveType() const override
        {
            AZ_Assert(false, "Not expected to be used");
            return eAnimCurveType_Unknown;
        }

        AnimValueType GetValueType() const override
        {
            AZ_Assert(false, "Not expected to be used");
            return static_cast<AnimValueType>(0xFFFFFFFF);
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] float& value, [[maybe_unused]] bool applyMultiplier = false) const override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] AZ::Vector3& value, [[maybe_unused]] bool applyMultiplier = false) const override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] AZ::Vector4& value, [[maybe_unused]] bool applyMultiplier = false) const override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] AZ::Quaternion& value) const override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] bool& value) const override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] AssetBlends<AZ::Data::AssetData>& value) const override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] AZStd::string& value) const override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const float& value,  [[maybe_unused]] bool bDefault = false, [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const AZ::Vector3& value, [[maybe_unused]] bool bDefault = false, [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const AZ::Vector4& value, [[maybe_unused]] bool bDefault = false, [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const AZ::Quaternion& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const bool& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const AssetBlends<AZ::Data::AssetData>& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const AZStd::string& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void OffsetKeyPosition([[maybe_unused]] const AZ::Vector3& value) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void UpdateKeyDataAfterParentChanged(
            [[maybe_unused]] const AZ::Transform& oldParentWorldTM, [[maybe_unused]] const AZ::Transform& newParentWorldTM) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;
        bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset) override;

        void GetKeyInfo([[maybe_unused]] int keyIndex, const char*& description, float& duration) const override
        {
            description = 0;
            duration = 0;
            AZ_Assert(false, "Not expected to be used");
        }

        //! Sort keys in track (after time of keys was modified).
        void SortKeys() override
        {
            if (m_spline)
            {
                m_spline->sort_keys();
            }
        }

        //! Get track flags.
        int GetFlags() const override
        {
            return m_flags;
        }

        //! Check if track is masked by mask
        bool IsMasked([[maybe_unused]] const uint32 mask) const override
        {
            return false;
        }

        //! Set track flags.
        void SetFlags(int flags) override
        {
            if (!m_spline)
            {
                AZ_Assert(false, "Invalid spline.");
                return;
            }

            m_flags = flags;
            if (m_flags & eAnimTrackFlags_Loop)
            {
                m_spline->ORT(Spline::ORT_LOOP);
            }
            else if (m_flags & eAnimTrackFlags_Cycle)
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
            if (m_spline)
            {
                m_spline->flag_set(Spline::MODIFIED);
            }
        }

        void SetTimeRange(const Range& timeRange) override
        {
            if (m_spline)
            {
                m_spline->SetRange(timeRange.start, timeRange.end);
                Invalidate();
            }
        }

        Range GetTimeRange() const override
        {
            Range timeRange = {0.0f, 1.0f};

            if (m_spline)
            {
                timeRange.start = m_spline->GetRangeStart();
                timeRange.end   = m_spline->GetRangeEnd();
            }

            return timeRange;
        }

        float GetMinKeyTimeDelta() const override
        {
            return 0.01f;
        };

        int FindKey(float time) const override
        {
            if (!m_spline)
            {
                AZ_Assert(false, "Invalid spline.");
                return -1;
            }

            // Find key with given time.
            for (int i = 0; i < GetNumKeys(); ++i)
            {
                float keyt = m_spline->key(i).time;
                if (AZStd::abs(keyt - time) < AZ::Constants::Tolerance)
                {
                    return i;
                }
            }
            return -1;
        }

        //! Create key at given time, and return its index.
        int CreateKey(float time) override
        {
            const Range timeRange(GetTimeRange());
            if (((timeRange.end - timeRange.start) > AZ::Constants::Tolerance) && (time < timeRange.start || time > timeRange.end))
            {
                AZ_WarningOnce("AnimSplineTrack", false, "CreateKey(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                    time, timeRange.start, timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
                AZStd::clamp(time, timeRange.start, timeRange.end);
            }

            const auto existingKeyIndex = FindKey(time);
            if (existingKeyIndex >= 0)
            {
                AZ_Error("AnimSplineTrack", false, "CreateKey(%f): Key (%d) with this time exists in track (%s).",
                    time, existingKeyIndex, (GetNode() ? GetNode()->GetName() : ""));
                return -1;
            }

            ValueType value;

            const int numKeys = GetNumKeys();
            if (numKeys > 0)
            {
                GetValue(time, value);
            }
            else
            {
                value = m_defaultValue;
            }

            typename Spline::ValueType tmp;
            m_spline->ToValueType(value, tmp);
            const auto newKeyIndex = m_spline->InsertKey(time, tmp);

            Invalidate();
            SortKeys();

            return newKeyIndex;
        }

        int CloneKey(int srcKeyIndex, float timeOffset) override
        {
            const auto numKeys = GetNumKeys();
            if (srcKeyIndex < 0 || srcKeyIndex >= numKeys)
            {
                AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", srcKeyIndex, numKeys);
                return -1;
            }

            ITcbKey key;
            GetKey(srcKeyIndex, &key);

            if (AZStd::abs(timeOffset) < GetMinKeyTimeDelta())
            {
                timeOffset = timeOffset >= 0.0f ? GetMinKeyTimeDelta() : -GetMinKeyTimeDelta();
            }

            key.time += timeOffset;

            const auto existingKeyIndex = FindKey(key.time);
            if (existingKeyIndex >= 0)
            {
                AZ_Error("AnimSplineTrack", false, "CloneKey(%d, %f): A key at this time already exists in this track (%s).", srcKeyIndex, key.time, (GetNode() ? GetNode()->GetName() : ""));
                return -1;
            }

            SetNumKeys(numKeys + 1);
            SetKey(numKeys, &key);

            SortKeys();

            return FindKey(key.time);
        }

        int CopyKey(IAnimTrack* pFromTrack, int fromKeyIndex) override
        {
            if (!pFromTrack)
            {
                AZ_Assert(false, "Expected valid track pointer.");
                return -1;
            }

            const int numKeysFromTrack = pFromTrack->GetNumKeys();
            if (fromKeyIndex < 0 || fromKeyIndex >= numKeysFromTrack)
            {
                AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", fromKeyIndex, m_spline->num_keys());
                return -1;
            }

            ITcbKey key;
            pFromTrack->GetKey(fromKeyIndex, &key);

            if (this == pFromTrack) // Used in CloneKey()
            {
                // Shift key time to avoid fully equal keys, with offset selected bigger than minimal legal key time delta
                const float timeOffset = GetMinKeyTimeDelta() * 1.1f;
                const auto timeRange = GetTimeRange();
                bool allowToAddKey = timeRange.end - timeRange.start > timeOffset;
                if (allowToAddKey)
                {
                    key.time += timeOffset;
                    if (key.time > timeRange.end)
                    {
                        key.time -= timeOffset * 2.0f;
                        allowToAddKey = key.time >= timeRange.start;
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

            const int numKeys = GetNumKeys();
            SetNumKeys(numKeys + 1);
            SetKey(numKeys, &key);

            Invalidate();
            SortKeys();

            return FindKey(key.time);
        }

        //! Get key at given time,
        //! If key does not exist, adds key at this time.
        void SetKeyAtTime(float time, IKey* key)
        {
            if (!key)
            {
                AZ_Assert(key, "Expected valid key pointer.");
                return;
            }

            const Range timeRange(GetTimeRange());
            if (((timeRange.end - timeRange.start) > AZ::Constants::Tolerance) && (time < timeRange.start || time > timeRange.end))
            {
                AZ_WarningOnce("AnimSplineTrack", false, "SetKeyAtTime(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                    time, timeRange.start, timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
                AZStd::clamp(time, timeRange.start, timeRange.end);
            }

            key->time = time;

            bool found = false;
            // Find key with given time.
            for (int i = 0; i < m_spline->num_keys(); i++)
            {
                float keyt = m_spline->key(i).time;
                if (AZStd::abs(keyt - time) < GetMinKeyTimeDelta())
                {
                    key->flags = m_spline->key(i).flags; // Reserve the flag value.
                    SetKey(i, key);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                // Key with this time not found.
                // Create a new one.
                int keyIndex = CreateKey(time);
                // Reserve the flag value.
                key->flags = m_spline->key(keyIndex).flags; // Reserve the flag value.
                SetKey(keyIndex, key);
            }

            Invalidate();
            SortKeys();
        }

        virtual void SetDefaultValue(const ValueType& value)
        {
            m_defaultValue = value;
        }

#ifdef MOVIESYSTEM_SUPPORT_EDITING
        ColorB GetCustomColor() const override
        {
            return m_customColor;
        }

        void SetCustomColor(ColorB color) override
        {
            m_customColor = color;
            m_bCustomColorSet = true;
        }

        bool HasCustomColor() const override
        {
            return m_bCustomColorSet;
        }

        void ClearCustomColor() override
        {
            m_bCustomColorSet = false;
        }
#endif

        void SetMultiplier(float trackMultiplier) override
        {
            if (AZStd::abs(trackMultiplier) > AZ::Constants::Tolerance)
            {
                m_trackMultiplier = trackMultiplier;
            }
        }

        void SetExpanded([[maybe_unused]] bool expanded) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        bool GetExpanded() const override
        {
            return false;
        }

        unsigned int GetId() const override
        {
            return m_id;
        }

        void SetId(unsigned int id) override
        {
            m_id = id;
        }

        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
        }

    protected:
        void UpdateTrackValueRange(float newValue)
        {
            m_fMinKeyValue = (newValue < m_fMinKeyValue) ? newValue : m_fMinKeyValue;
            m_fMaxKeyValue = (newValue > m_fMaxKeyValue) ? newValue : m_fMaxKeyValue;
            if ((m_fMaxKeyValue - m_fMinKeyValue) < s_MinValueRange)
            {
                // prevents fill sliders from being inoperable when min and max are identical (or close to it)
                m_fMaxKeyValue = (m_fMinKeyValue + s_MinValueRange);
            }
        }

    private:
        //! Spawns new instance of Tcb spline.
        void AllocSpline()
        {
            m_spline = aznew spline::TrackSplineInterpolator<ValueType>;
        }

        int m_refCount;
        typedef spline::TrackSplineInterpolator<ValueType> Spline;
        AZStd::intrusive_ptr<Spline> m_spline;
        ValueType m_defaultValue;

        //! Keys of float track.
        int m_flags;
        CAnimParamType m_nParamType;

        ColorB m_customColor;
        bool m_bCustomColorSet;

        float m_fMinKeyValue;
        float m_fMaxKeyValue;

        IAnimNode* m_node;

        float m_trackMultiplier = 1.0f;

        unsigned int m_id = 0;

        static constexpr float s_MinValueRange = 1.0f; // prevents fill sliders from being inoperable on the first key frame
    };

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    inline void TAnimSplineTrack<T>::add_ref()
    {
        ++m_refCount;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    inline void TAnimSplineTrack<T>::release()
    {
        if (--m_refCount <= 0)
        {
            delete this;
        }
    }

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    template<class T>
    inline bool TAnimSplineTrack<T>::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
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

            xmlNode->getAttr("Id", m_id);

            if ((!num) && (!bLoadEmptyTracks))
            {
                return false;
            }

            Invalidate();
            SortKeys();
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

            xmlNode->setAttr("Id", m_id);
        }
        return true;
    }

    template<class T>
    inline bool TAnimSplineTrack<T>::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset)
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

        Invalidate();
        SortKeys();

        return true;
    }

    template<>
    TAnimSplineTrack<Vec2>::TAnimSplineTrack();
    template<>
    void TAnimSplineTrack<Vec2>::GetValue(float time, float& value, bool applyMultiplier) const;
    template<>
    EAnimCurveType TAnimSplineTrack<Vec2>::GetCurveType() const;
    template<>
    AnimValueType TAnimSplineTrack<Vec2>::GetValueType() const;
    template<>
    void TAnimSplineTrack<Vec2>::SetValue(float time, const float& value, bool bDefault, bool applyMultiplier);
    template<>
    void TAnimSplineTrack<Vec2>::GetKey(int keyIndex, IKey* key) const;

    template<>
    void TAnimSplineTrack<Vec2>::SetKey(int keyIndex, IKey* key);

    //! Create key at given time, and return its index.
    template<>
    int TAnimSplineTrack<Vec2>::CreateKey(float time);

    template<>
    int TAnimSplineTrack<Vec2>::CloneKey(int srcKeyIndex, float timeOffset);

    template<>
    int TAnimSplineTrack<Vec2>::CopyKey(IAnimTrack* pFromTrack, int fromKeyIndex);

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    template<>
    bool TAnimSplineTrack<Vec2>::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    template<>
    bool TAnimSplineTrack<Vec2>::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset);

    template<>
    void TAnimSplineTrack<Vec2>::GetKeyInfo(int keyIndex, const char*& description, float& duration) const;

    template<>
    void TAnimSplineTrack<Vec2>::add_ref();

    template<>
    void TAnimSplineTrack<Vec2>::release();

    template<>
    void TAnimSplineTrack<Vec2>::Reflect(AZ::ReflectContext* context);

    using C2DSplineTrack = TAnimSplineTrack<Vec2>;

} // namespace Maestro
