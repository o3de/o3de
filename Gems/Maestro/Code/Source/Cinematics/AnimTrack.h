/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/std/sort.h>
#include <IMovieSystem.h>


namespace Maestro
{

    /** General templated track for event type keys.
            KeyType class must be derived from IKey.
    */
    template<class KeyType>
    class TAnimTrack : public IAnimTrack
    {
    public:
        AZ_CLASS_ALLOCATOR(TAnimTrack, AZ::SystemAllocator);
        AZ_RTTI((TAnimTrack, "{D6E0F0E3-8843-46F0-8484-7B6E130409AE}", KeyType), IAnimTrack);

        TAnimTrack();

        EAnimCurveType GetCurveType() const override
        {
            return eAnimCurveType_Unknown;
        }

        AnimValueType GetValueType() const override
        {
            return kAnimValueUnknown;
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

        int GetSubTrackCount() const override
        {
            return 0;
        }

        IAnimTrack* GetSubTrack([[maybe_unused]] int nIndex) const override
        {
            return 0;
        }

        AZStd::string GetSubTrackName([[maybe_unused]] int nIndex) const override
        {
            return AZStd::string();
        }

        void SetSubTrackName([[maybe_unused]] int nIndex, [[maybe_unused]] const char* name) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        const CAnimParamType& GetParameterType() const override
        {
            return m_nParamType;
        }

        void SetParameterType(CAnimParamType type) override
        {
            m_nParamType = type;
        }

        //////////////////////////////////////////////////////////////////////////
        // for intrusive_ptr support
        void add_ref() override;
        void release() override;
        //////////////////////////////////////////////////////////////////////////

        bool IsKeySelected(int keyIndex) const override
        {
            if (keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(false, "Key index (%d) is out of range (0 .. %d)", keyIndex, GetNumKeys());
                return false;
            }

            if (m_keys[keyIndex].flags & AKEY_SELECTED)
            {
                return true;
            }
            return false;
        }

        void SelectKey(int keyIndex, bool select) override
        {
            if (keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(false, "Key index (%d) is out of range (0 .. %d)", keyIndex, GetNumKeys());
                return;
            }

            if (select)
            {
                m_keys[keyIndex].flags |= AKEY_SELECTED;
            }
            else
            {
                m_keys[keyIndex].flags &= ~AKEY_SELECTED;
            }
        }

        bool IsSortMarkerKey(int keyIndex) const override
        {
            if (keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(false, "Key index (%d) is out of range (0 .. %d)", keyIndex, GetNumKeys());
                return false;
            }

            if (m_keys[keyIndex].flags & AKEY_SORT_MARKER)
            {
                return true;
            }
            return false;
        }

        void SetSortMarkerKey(int keyIndex, bool enabled) override
        {
            if (keyIndex < 0 || keyIndex >= GetNumKeys())
            {
                AZ_Assert(false, "Key index (%d) is out of range (0 .. %d)", keyIndex, GetNumKeys());
                return;
            }

            if (enabled)
            {
                m_keys[keyIndex].flags |= AKEY_SORT_MARKER;
            }
            else
            {
                m_keys[keyIndex].flags &= ~AKEY_SORT_MARKER;
            }
        }

        //! Return number of keys in track.
        int GetNumKeys() const override
        {
            return static_cast<int>(m_keys.size());
        }

        //! Return true if keys exists in this track
        bool HasKeys() const override
        {
            return !m_keys.empty();
        }

        //! Set number of keys in track.
        //! If needed adds empty keys at end or remove keys from end.
        void SetNumKeys(int numKeys) override
        {
            m_keys.resize(numKeys);
        }

        //! Remove specified key.
        void RemoveKey(int keyIndex) override;

        int CreateKey(float time) override;
        int CloneKey(int srcKeyIndex, float timeOffset) override;
        int CopyKey(IAnimTrack* pFromTrack, int fromKeyIndex) override;

        //! Get key at specified location.
        //! @param key Must be valid pointer to compatible key structure, to be filled with specified key location.
        void GetKey(int keyIndex, IKey* key) const override;

        //! Get time of specified key.
        //! @return key time.
        float GetKeyTime(int keyIndex) const override;

        //! Get minimal legal time delta between keys.
        //! @return Minimal legal time delta between keys.
        float GetMinKeyTimeDelta() const override { return 0.01f; };

        //! Find key at given time.
        //! @return Index of found key, or -1 if key with this time not found.
        int FindKey(float time) const override;

        //! Get flags of specified key.
        //! @return key time.
        int GetKeyFlags(int keyIndex) override;

        //! Set key at specified location.
        //! @param key Must be valid pointer to compatible key structure.
        void SetKey(int keyIndex, IKey* key) override;

        //! Set time of specified key.
        void SetKeyTime(int keyIndex, float time) override;

        //! Set flags of specified key.
        void SetKeyFlags(int keyIndex, int flags) override;

        //! Sort keys in track (after time of keys was modified).
        void SortKeys() override;

        //! Get track flags.
        int GetFlags() const override
        {
            return m_flags;
        }

        //! Check if track is masked
        bool IsMasked([[maybe_unused]] const uint32 mask) const override
        {
            return false;
        }

        //! Set track flags.
        void SetFlags(int flags) override
        {
            m_flags = flags;
        }

        //////////////////////////////////////////////////////////////////////////
        // Get track value at specified time.
        // Interpolates keys if needed.
        //////////////////////////////////////////////////////////////////////////
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

        //////////////////////////////////////////////////////////////////////////
        // Set track value at specified time.
        // Adds new keys if required.
        //////////////////////////////////////////////////////////////////////////
        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const float& value, [[maybe_unused]] bool bDefault = false, [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue( [[maybe_unused]] float time, [[maybe_unused]] const AZ::Vector3& value, [[maybe_unused]] bool bDefault = false, [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue( [[maybe_unused]] float time, [[maybe_unused]] const AZ::Vector4& value, [[maybe_unused]] bool bDefault = false, [[maybe_unused]] bool applyMultiplier = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue( [[maybe_unused]] float time, [[maybe_unused]] const AZ::Quaternion& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const bool& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue( [[maybe_unused]] float time, [[maybe_unused]] const AssetBlends<AZ::Data::AssetData>& value, [[maybe_unused]] bool bDefault = false) override
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

        /** Assign active time range for this track.
         */
        void SetTimeRange(const Range& timeRange) override
        {
            m_timeRange = timeRange;
        }

        Range GetTimeRange() const override
        {
            return m_timeRange;
        }

        /** Serialize this animation track to XML.
                Do not override this method, prefer to override SerializeKey.
        */
        bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true) override;

        bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected = false, float fTimeOffset = 0) override;

        /** Serialize single key of this track.
                Override this in derived classes.
                Do not save time attribute, it is already saved in Serialize of the track.
        */
        virtual void SerializeKey(KeyType& key, XmlNodeRef& keyNode, bool bLoading) = 0;

        void InitPostLoad([[maybe_unused]] IAnimSequence* sequence) override { }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        /** Get last key before specified time.
                @return Index of key, or -1 if such key not exist.
        */
        virtual int GetActiveKey(float time, KeyType* key);

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

        int m_refCount;

        typedef AZStd::vector<KeyType> Keys;
        Keys m_keys;
        Range m_timeRange;

        CAnimParamType m_nParamType;
        int m_currKey : 31;
        float m_lastTime;
        int m_flags;

#ifdef MOVIESYSTEM_SUPPORT_EDITING
        ColorB m_customColor;
        bool m_bCustomColorSet;
#endif

        float m_fMinKeyValue;
        float m_fMaxKeyValue;

        IAnimNode* m_node;

        float m_trackMultiplier = 1.0f;

        unsigned int m_id = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::add_ref()
    {
        ++m_refCount;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::release()
    {
        if (--m_refCount <= 0)
        {
            delete this;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline TAnimTrack<KeyType>::TAnimTrack()
        : m_refCount(0)
    {
        m_currKey = 0;
        m_flags = 0;
        m_lastTime = -1;
        m_node = nullptr;

#ifdef MOVIESYSTEM_SUPPORT_EDITING
        m_bCustomColorSet = false;
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::RemoveKey(int keyIndex)
    {
        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        m_keys.erase(m_keys.begin() + keyIndex);

        SortKeys();
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::GetKey(int keyIndex, IKey* key) const
    {
        if (!key || keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(key, "Invalid key pointer.");
            AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        *(KeyType*)key = m_keys[keyIndex];
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::SetKey(int keyIndex, IKey* key)
    {
        if (!key || keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(key, "Invalid key pointer.");
            AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(),"Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        m_keys[keyIndex] = *(KeyType*)key;

        SortKeys();
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline float TAnimTrack<KeyType>::GetKeyTime(int keyIndex) const
    {
        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d)", keyIndex, GetNumKeys());
            return -1.0f;
        }

        return m_keys[keyIndex].time;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::SetKeyTime(int keyIndex, float time)
    {
        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        if (((m_timeRange.end - m_timeRange.start) > AZ::Constants::Tolerance) && (time < m_timeRange.start || time > m_timeRange.end))
        {
            AZ_WarningOnce("AnimTrack", false, "SetKeyTime(%d, %f): Key time is out of range (%f .. %f) in track (%s), clamped.",
                keyIndex, time, m_timeRange.start, m_timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
            AZStd::clamp(time, m_timeRange.start, m_timeRange.end);
        }

        const int exitingIndex = FindKey(time);
        if (exitingIndex >= 0)
        {
            AZ_Error("AnimTrack", exitingIndex == keyIndex, "SetKeyTime(%d, %f): A key with this time exists in track (%s).",
                keyIndex, time, (GetNode() ? GetNode()->GetName() : ""));
            return;
        }

        m_keys[keyIndex].time = time;

        SortKeys();
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::FindKey(float time) const
    {
        for (int i = 0; i < (int)m_keys.size(); i++)
        {
            if (AZStd::abs(m_keys[i].time - time) < AZ::Constants::Tolerance)
            {
                return i;
            }
        }
        return -1;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::GetKeyFlags(int keyIndex)
    {
        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return 0;
        }

        return m_keys[keyIndex].flags;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::SetKeyFlags(int keyIndex, int flags)
    {
        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        m_keys[keyIndex].flags = flags;

        SortKeys();
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::SortKeys()
    {
        AZStd::sort(m_keys.begin(), m_keys.end());
    }

    //////////////////////////////////////////////////////////////////////////
    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    template<class KeyType>
    inline bool TAnimTrack<KeyType>::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        if (bLoading)
        {
            int num = xmlNode->getChildCount();

            Range timeRange;
            int flags = m_flags;
            xmlNode->getAttr("Flags", flags);
            xmlNode->getAttr("StartTime", timeRange.start);
            xmlNode->getAttr("EndTime", timeRange.end);
            SetFlags(flags);
            SetTimeRange(timeRange);

#ifdef MOVIESYSTEM_SUPPORT_EDITING
            xmlNode->getAttr("HasCustomColor", m_bCustomColorSet);
            if (m_bCustomColorSet)
            {
                unsigned int abgr;
                xmlNode->getAttr("CustomColor", abgr);
                m_customColor = ColorB(abgr);
            }
#endif

            SetNumKeys(num);
            for (int i = 0; i < num; i++)
            {
                XmlNodeRef keyNode = xmlNode->getChild(i);
                keyNode->getAttr("time", m_keys[i].time);

                SerializeKey(m_keys[i], keyNode, bLoading);
            }

            xmlNode->getAttr("Id", m_id);

            if ((!num) && (!bLoadEmptyTracks))
            {
                return false;
            }
        }
        else
        {
            SortKeys();
            xmlNode->setAttr("Flags", GetFlags());
            xmlNode->setAttr("StartTime", m_timeRange.start);
            xmlNode->setAttr("EndTime", m_timeRange.end);
#ifdef MOVIESYSTEM_SUPPORT_EDITING
            xmlNode->setAttr("HasCustomColor", m_bCustomColorSet);
            if (m_bCustomColorSet)
            {
                xmlNode->setAttr("CustomColor", m_customColor.pack_abgr8888());
            }
#endif

            for (int i = 0; i < GetNumKeys(); i++)
            {
                XmlNodeRef keyNode = xmlNode->newChild("Key");
                keyNode->setAttr("time", m_keys[i].time);

                SerializeKey(m_keys[i], keyNode, bLoading);
            }

            xmlNode->setAttr("Id", m_id);
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline bool TAnimTrack<KeyType>::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset)
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
                XmlNodeRef keyNode = xmlNode->getChild(i);
                keyNode->getAttr("time", m_keys[i + numCur].time);
                m_keys[i + numCur].time += fTimeOffset;

                SerializeKey(m_keys[i + numCur], keyNode, bLoading);
                if (bCopySelected)
                {
                    m_keys[i + numCur].flags |= AKEY_SELECTED;
                }
            }
        }
        else
        {
            int num = GetNumKeys();
            xmlNode->setAttr("TrackType", GetCurveType());

            // CheckValid();

            for (int i = 0; i < num; i++)
            {
                if (!bCopySelected || GetKeyFlags(i) & AKEY_SELECTED)
                {
                    XmlNodeRef keyNode = xmlNode->newChild("Key");
                    keyNode->setAttr("time", m_keys[i].time);

                    SerializeKey(m_keys[i], keyNode, bLoading);
                }
            }
        }

        SortKeys();

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::CreateKey(float time)
    {
        if ((m_timeRange.end - m_timeRange.start > AZ::Constants::Tolerance) && (time < m_timeRange.start || time > m_timeRange.end))
        {
            AZ_Warning("AnimSplineTrack", false, "CreateKey(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                time, m_timeRange.start, m_timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
            AZStd::clamp(time, m_timeRange.start, m_timeRange.end);
        }

        const auto existingKeyIndex = FindKey(time);
        if (existingKeyIndex >= 0)
        {
            AZ_Error("AnimTrack", false, "CreateKey(%f) : A key (%d) with this time exists in track (%s).",
                time, existingKeyIndex, (GetNode() ? GetNode()->GetName() : ""));
            return -1;
        }

        KeyType key;
        const int nkey = GetNumKeys();
        SetNumKeys(nkey + 1);
        key.time = time;
        SetKey(nkey, &key);

        SortKeys();

        return FindKey(time);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::CloneKey(int srcKeyIndex, float timeOffset)
    {
        const auto numKeys = GetNumKeys();
        if (srcKeyIndex < 0 || srcKeyIndex >= numKeys)
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", srcKeyIndex, numKeys);
            return -1;
        }

        KeyType key;
        GetKey(srcKeyIndex, &key);

        if (AZStd::abs(timeOffset) < GetMinKeyTimeDelta())
        {
            timeOffset = timeOffset >= 0.0f ? GetMinKeyTimeDelta() : -GetMinKeyTimeDelta();
        }

        key.time += timeOffset;

        const auto existingKeyIndex = FindKey(key.time);
        if (existingKeyIndex >= 0)
        {
            AZ_Error("AnimTrack", false, "CloneKey(%d, %f): A key at this time already exists in this track (%s).", srcKeyIndex, key.time, (GetNode() ? GetNode()->GetName() : ""));
            return -1;
        }

        SetNumKeys(numKeys + 1);
        SetKey(numKeys, &key);

        SortKeys();

        return FindKey(key.time);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::CopyKey(IAnimTrack* pFromTrack, int fromKeyIndex)
    {
        if (!pFromTrack)
        {
            AZ_Assert(false, "Expected valid track pointer.");
            return -1;
        }

        if (fromKeyIndex < 0 || fromKeyIndex >= pFromTrack->GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", fromKeyIndex, GetNumKeys());
            return -1;
        }

        KeyType key;
        pFromTrack->GetKey(fromKeyIndex, &key);

        if (this == pFromTrack)
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
                AZ_Error("AnimTrack", false, "CopyKey(%s, %d): A key at time (%f) with index (%d) already exists in track (%s).",
                    (pFromTrack->GetNode() ? pFromTrack->GetNode()->GetName() : ""), fromKeyIndex, key.time, existingKeyIndex, (GetNode() ? GetNode()->GetName() : ""));
                return -1;
            }
        }
        else
        {
            const auto existingKeyIndex = FindKey(key.time);
            if (existingKeyIndex >= 0)
            {
                AZ_Error("AnimTrack", false, "CopyKey(%s, %d): A key at time (%f) with index (%d) already exists in this track (%s).",
                    (pFromTrack->GetNode() ? pFromTrack->GetNode()->GetName() : ""), fromKeyIndex, key.time, existingKeyIndex, (GetNode() ? GetNode()->GetName() : ""));
                return -1;
            }
        }

        const auto numKeys = GetNumKeys();
        SetNumKeys(numKeys + 1);
        SetKey(numKeys, &key);

        SortKeys();

        return FindKey(key.time);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::GetActiveKey(float time, KeyType* key)
    {
        if (!key)
        {
            return -1;
        }

        const int numKeys = GetNumKeys();
        if (numKeys == 0)
        {
            m_lastTime = time;
            m_currKey = -1;
            return m_currKey;
        }

        bool bTimeWrap = false;

        if ((m_flags & eAnimTrackFlags_Cycle) || (m_flags & eAnimTrackFlags_Loop))
        {
            // Warp time.
            const char* desc = 0;
            float duration = 0;
            GetKeyInfo(numKeys - 1, desc, duration);
            float endtime = GetKeyTime(numKeys - 1) + duration;
            time = fmod_tpl(time, endtime);
            if (time < m_lastTime)
            {
                // Time is wrapped.
                bTimeWrap = true;
            }
        }
        m_lastTime = time;

        // Time is before first key.
        if (m_keys[0].time > time)
        {
            if (bTimeWrap)
            {
                // If time wrapped, active key is last key.
                m_currKey = numKeys - 1;
                *key = m_keys[m_currKey];
            }
            else
            {
                m_currKey = -1;
            }
            return m_currKey;
        }

        if (m_currKey < 0)
        {
            m_currKey = 0;
        }

        // Start from current key.
        int i;
        for (i = m_currKey; i < numKeys; i++)
        {
            if (time >= m_keys[i].time)
            {
                if ((i >= numKeys - 1) || (time < m_keys[i + 1].time))
                {
                    m_currKey = i;
                    *key = m_keys[m_currKey];
                    return m_currKey;
                }
            }
            else
            {
                break;
            }
        }

        // Start from beginning.
        for (i = 0; i < numKeys; i++)
        {
            if (time >= m_keys[i].time)
            {
                if ((i >= numKeys - 1) || (time < m_keys[i + 1].time))
                {
                    m_currKey = i;
                    *key = m_keys[m_currKey];
                    return m_currKey;
                }
            }
            else
            {
                break;
            }
        }
        m_currKey = -1;
        return m_currKey;
    }

} // namespace Maestro
