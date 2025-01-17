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
        IAnimNode* GetNode() override
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
                AZ_Assert(false, "Key index %i is out of range", num);
            }
        }

        void GetKey(int index, IKey* key) const override
        {
            AZ_Assert(index >= 0 && index < GetNumKeys(), "Key index %i is out of range", index);
            AZ_Assert(key, "Key is null");
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
            AZ_Assert(index >= 0 && index < GetNumKeys(), "Key index %i is out of range", index);
            AZ_Assert(key, "Key is null");
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
            AZ_Assert(index >= 0 && index < GetNumKeys(), "Key index %i is out of range", index);
            return m_spline->time(index);
        }

        void SetKeyTime(int index, float time) override
        {
            AZ_Assert(index >= 0 && index < GetNumKeys(), "Key index %i is out of range", index);
            m_spline->SetKeyTime(index, time);
            Invalidate();
        }

        int GetKeyFlags(int index) override
        {
            AZ_Assert(index >= 0 && index < GetNumKeys(), "Key index %i is out of range", index);
            return m_spline->key(index).flags;
        }

        void SetKeyFlags(int index, int flags) override
        {
            AZ_Assert(index >= 0 && index < GetNumKeys(), "Key index %i is out of range", index);
            m_spline->key(index).flags = flags;
        }

        EAnimCurveType GetCurveType() override
        {
            AZ_Assert(false, "Not expected to be used");
            return eAnimCurveType_Unknown;
        }

        AnimValueType GetValueType() override
        {
            AZ_Assert(false, "Not expected to be used");
            return static_cast<AnimValueType>(0xFFFFFFFF);
        }

        void GetValue(float time, float& value, bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue(
            [[maybe_unused]] float time, [[maybe_unused]] AZ::Vector3& value, [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue(
            [[maybe_unused]] float time, [[maybe_unused]] AZ::Vector4& value, [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] AZ::Quaternion& value) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] bool& value) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] AssetBlends<AZ::Data::AssetData>& value) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue(float time, const float& value, bool bDefault = false, bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue(
            [[maybe_unused]] float time,
            [[maybe_unused]] const AZ::Vector3& value,
            [[maybe_unused]] bool bDefault = false,
            [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue(
            [[maybe_unused]] float time,
            [[maybe_unused]] const AZ::Vector4& value,
            [[maybe_unused]] bool bDefault = false,
            [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue(
            [[maybe_unused]] float time, [[maybe_unused]] const AZ::Quaternion& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const bool& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue(
            [[maybe_unused]] float time,
            [[maybe_unused]] const AssetBlends<AZ::Data::AssetData>& value,
            [[maybe_unused]] bool bDefault = false) override
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

        void GetKeyInfo(int key, const char*& description, float& duration) override
        {
            description = 0;
            duration = 0;
        }

        //! Sort keys in track (after time of keys was modified).
        void SortKeys() override
        {
            m_spline->sort_keys();
        }

        //! Get track flags.
        int GetFlags() override
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
            m_spline->flag_set(Spline::MODIFIED);
        }

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
                if (fabs(keyt - time) < MinTimePrecision)
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

        int CopyKey(IAnimTrack* pFromTrack, int nFromKey) override
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
            AZ_Assert(key, "Key is null");

            key->time = time;

            bool found = false;
            // Find key with given time.
            for (int i = 0; i < m_spline->num_keys(); i++)
            {
                float keyt = m_spline->key(i).time;
                if (fabs(keyt - time) < MinTimePrecision)
                {
                    key->flags = m_spline->key(i).flags; // Reserve the flag value.
                    SetKey(i, key);
                    found = true;
                    break;
                }
                // if (keyt > time)
                // break;
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
            m_trackMultiplier = trackMultiplier;
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
            if ((m_fMaxKeyValue - m_fMinKeyValue) < MinValueRange)
            {
                // prevents fill sliders from being inoperable when min and max are identical (or close to it)
                m_fMaxKeyValue = (m_fMinKeyValue + MinValueRange);
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

        float m_trackMultiplier;

        unsigned int m_id = 0;

        static constexpr float MinTimePrecision = 0.01f;
        static constexpr float MinValueRange = 1.0f; // prevents fill sliders from being inoperable on the first key frame
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

    template<>
    TAnimSplineTrack<Vec2>::TAnimSplineTrack();
    template<>
    void TAnimSplineTrack<Vec2>::GetValue(float time, float& value, bool applyMultiplier);
    template<>
    EAnimCurveType TAnimSplineTrack<Vec2>::GetCurveType();
    template<>
    AnimValueType TAnimSplineTrack<Vec2>::GetValueType();
    template<>
    void TAnimSplineTrack<Vec2>::SetValue(float time, const float& value, bool bDefault, bool applyMultiplier);
    template<>
    void TAnimSplineTrack<Vec2>::GetKey(int index, IKey* key) const;

    template<>
    void TAnimSplineTrack<Vec2>::SetKey(int index, IKey* key);

    //! Create key at given time, and return its index.
    template<>
    int TAnimSplineTrack<Vec2>::CreateKey(float time);

    template<>
    int TAnimSplineTrack<Vec2>::CopyKey(IAnimTrack* pFromTrack, int nFromKey);

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    template<>
    bool TAnimSplineTrack<Vec2>::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    template<>
    bool TAnimSplineTrack<Vec2>::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset);

    template<>
    void TAnimSplineTrack<Vec2>::GetKeyInfo(int index, const char*& description, float& duration);

    template<>
    void TAnimSplineTrack<Vec2>::add_ref();

    template<>
    void TAnimSplineTrack<Vec2>::release();

    template<>
    void TAnimSplineTrack<Vec2>::Reflect(AZ::ReflectContext* context);

    using C2DSplineTrack = TAnimSplineTrack<Vec2>;

} // namespace Maestro
