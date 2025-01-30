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

        EAnimCurveType GetCurveType() override
        {
            return eAnimCurveType_Unknown;
        }

        AnimValueType GetValueType() override
        {
            return kAnimValueUnknown;
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

        bool IsKeySelected(int key) const override
        {
            AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index is out of range");
            if (m_keys[key].flags & AKEY_SELECTED)
            {
                return true;
            }
            return false;
        }

        void SelectKey(int key, bool select) override
        {
            AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index is out of range");
            if (select)
            {
                m_keys[key].flags |= AKEY_SELECTED;
            }
            else
            {
                m_keys[key].flags &= ~AKEY_SELECTED;
            }
        }

        bool IsSortMarkerKey(unsigned int key) const override
        {
            AZ_Assert(key < m_keys.size(), "key index is out of range");
            if (m_keys[key].flags & AKEY_SORT_MARKER)
            {
                return true;
            }
            return false;
        }

        void SetSortMarkerKey(unsigned int key, bool enabled) override
        {
            AZ_Assert(key < m_keys.size(), "key index is out of range");
            if (enabled)
            {
                m_keys[key].flags |= AKEY_SORT_MARKER;
            }
            else
            {
                m_keys[key].flags &= ~AKEY_SORT_MARKER;
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
        void RemoveKey(int num) override;

        int CreateKey(float time) override;
        int CloneKey(int fromKey) override;
        int CopyKey(IAnimTrack* pFromTrack, int nFromKey) override;

        //! Get key at specified location.
        //! @param key Must be valid pointer to compatible key structure, to be filled with specified key location.
        void GetKey(int index, IKey* key) const override;

        //! Get time of specified key.
        //! @return key time.
        float GetKeyTime(int index) const override;

        //! Find key at given time.
        //! @return Index of found key, or -1 if key with this time not found.
        int FindKey(float time) override;

        //! Get flags of specified key.
        //! @return key time.
        int GetKeyFlags(int index) override;

        //! Set key at specified location.
        //! @param key Must be valid pointer to compatible key structure.
        void SetKey(int index, IKey* key) override;

        //! Set time of specified key.
        void SetKeyTime(int index, float time) override;

        //! Set flags of specified key.
        void SetKeyFlags(int index, int flags) override;

        //! Sort keys in track (after time of keys was modified).
        void SortKeys() override;

        //! Get track flags.
        int GetFlags() override
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
        void GetValue([[maybe_unused]] float time, [[maybe_unused]] float& value, [[maybe_unused]] bool applyMultiplier = false) override
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

        //////////////////////////////////////////////////////////////////////////
        // Set track value at specified time.
        // Adds new keys if required.
        //////////////////////////////////////////////////////////////////////////
        void SetValue(
            [[maybe_unused]] float time,
            [[maybe_unused]] const float& value,
            [[maybe_unused]] bool bDefault = false,
            [[maybe_unused]] bool applyMultiplier = false) override
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

        /** Assign active time range for this track.
         */
        void SetTimeRange(const Range& timeRange) override
        {
            m_timeRange = timeRange;
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

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        /** Get last key before specified time.
                @return Index of key, or -1 if such key not exist.
        */
        int GetActiveKey(float time, KeyType* key);

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
        void CheckValid()
        {
            if (m_bModified)
            {
                SortKeys();
            }
        }

        void Invalidate()
        {
            m_bModified = -1;
        }

        int m_refCount;

        typedef AZStd::vector<KeyType> Keys;
        Keys m_keys;
        Range m_timeRange;

        CAnimParamType m_nParamType;
        int m_currKey : 31;
        int m_bModified : 1;
        float m_lastTime;
        int m_flags;

#ifdef MOVIESYSTEM_SUPPORT_EDITING
        ColorB m_customColor;
        bool m_bCustomColorSet;
#endif

        float m_fMinKeyValue;
        float m_fMaxKeyValue;

        IAnimNode* m_node;

        float m_trackMultiplier;

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
        m_bModified = 0;
        m_node = nullptr;

#ifdef MOVIESYSTEM_SUPPORT_EDITING
        m_bCustomColorSet = false;
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::RemoveKey(int index)
    {
        AZ_Assert(index >= 0 && index < (int)m_keys.size(), "Key index is out of range");
        m_keys.erase(m_keys.begin() + index);
        Invalidate();
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::GetKey(int index, IKey* key) const
    {
        AZ_Assert(index >= 0 && index < (int)m_keys.size(), "Key index is out of range");
        AZ_Assert(key != 0, "Key cannot be null!");
        *(KeyType*)key = m_keys[index];
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::SetKey(int index, IKey* key)
    {
        AZ_Assert(index >= 0 && index < (int)m_keys.size(), "Key index is out of range");
        AZ_Assert(key != 0, "Key cannot be null!");
        m_keys[index] = *(KeyType*)key;
        Invalidate();
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline float TAnimTrack<KeyType>::GetKeyTime(int index) const
    {
        AZ_Assert(index >= 0 && index < (int)m_keys.size(), "Key index is out of range");
        return m_keys[index].time;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::SetKeyTime(int index, float time)
    {
        AZ_Assert(index >= 0 && index < (int)m_keys.size(), "Key index is out of range");
        m_keys[index].time = time;
        Invalidate();
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::FindKey(float time)
    {
        for (int i = 0; i < (int)m_keys.size(); i++)
        {
            if (m_keys[i].time == time)
            {
                return i;
            }
        }
        return -1;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::GetKeyFlags(int index)
    {
        AZ_Assert(index >= 0 && index < (int)m_keys.size(), "Key index is out of range");
        return m_keys[index].flags;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::SetKeyFlags(int index, int flags)
    {
        AZ_Assert(index >= 0 && index < (int)m_keys.size(), "Key index is out of range");
        m_keys[index].flags = flags;
        Invalidate();
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline void TAnimTrack<KeyType>::SortKeys()
    {
        AZStd::sort(m_keys.begin(), m_keys.end());
        m_bModified = 0;
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
            int num = GetNumKeys();
            CheckValid();
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

            for (int i = 0; i < num; i++)
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
            SortKeys();
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
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::CreateKey(float time)
    {
        KeyType key;
        int nkey = GetNumKeys();
        SetNumKeys(nkey + 1);
        key.time = time;
        SetKey(nkey, &key);

        return nkey;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::CloneKey(int fromKey)
    {
        KeyType key;
        GetKey(fromKey, &key);
        int nkey = GetNumKeys();
        SetNumKeys(nkey + 1);
        SetKey(nkey, &key);
        return nkey;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::CopyKey(IAnimTrack* pFromTrack, int nFromKey)
    {
        KeyType key;
        pFromTrack->GetKey(nFromKey, &key);
        int nkey = GetNumKeys();
        SetNumKeys(nkey + 1);
        SetKey(nkey, &key);
        return nkey;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class KeyType>
    inline int TAnimTrack<KeyType>::GetActiveKey(float time, KeyType* key)
    {
        CheckValid();

        if (key == nullptr)
        {
            return -1;
        }

        int nkeys = static_cast<int>(m_keys.size());
        if (nkeys == 0)
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
            GetKeyInfo(nkeys - 1, desc, duration);
            float endtime = GetKeyTime(nkeys - 1) + duration;
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
                m_currKey = nkeys - 1;
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
        for (i = m_currKey; i < nkeys; i++)
        {
            if (time >= m_keys[i].time)
            {
                if ((i >= nkeys - 1) || (time < m_keys[i + 1].time))
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
        for (i = 0; i < nkeys; i++)
        {
            if (time >= m_keys[i].time)
            {
                if ((i >= nkeys - 1) || (time < m_keys[i + 1].time))
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
