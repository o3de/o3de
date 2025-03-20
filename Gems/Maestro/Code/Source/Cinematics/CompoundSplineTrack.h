/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

namespace Maestro
{

    //////////////////////////////////////////////////////////////////////////
    class CCompoundSplineTrack : public IAnimTrack
    {
    public:
        AZ_CLASS_ALLOCATOR(CCompoundSplineTrack, AZ::SystemAllocator);
        AZ_RTTI(CCompoundSplineTrack, "{E6B88EF4-6DB7-48E7-9758-DF6C9E40D4D2}", IAnimTrack);

        static constexpr int MaxSubtracks = 4;

        CCompoundSplineTrack(int nDims, AnimValueType inValueType, CAnimParamType subTrackParamTypes[MaxSubtracks], bool expanded);
        CCompoundSplineTrack();

        //////////////////////////////////////////////////////////////////////////
        // for intrusive_ptr support
        void add_ref() override;
        void release() override;
        //////////////////////////////////////////////////////////////////////////

        void SetNode(IAnimNode* node) override;
        // Return Animation Node that owns this Track.
        IAnimNode* GetNode() const override
        {
            return m_node;
        }

        int GetSubTrackCount() const override
        {
            return m_nDimensions;
        }

        IAnimTrack* GetSubTrack(int nIndex) const override;
        AZStd::string GetSubTrackName(int nIndex) const override;
        void SetSubTrackName(int nIndex, const char* name) override;

        EAnimCurveType GetCurveType() const override
        {
            return eAnimCurveType_BezierFloat;
        }
        AnimValueType GetValueType() const override
        {
            return m_valueType;
        }

        const CAnimParamType& GetParameterType() const override
        {
            return m_nParamType;
        }

        void SetParameterType(CAnimParamType type) override
        {
            m_nParamType = type;
        }

        //! Return number of all keys in all sub-tracks.
        int GetNumKeys() const override;

        void SetNumKeys([[maybe_unused]] int numKeys) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        //! Return true if a key exists in sub-tracks.
        bool HasKeys() const override;

        //! Remove single key from a sub-track by index. Index is extended, belongs to (0 .. total number of keys in sub-tracks).
        void RemoveKey(int keyIndex) override;

        //! Get summary info about sub-tracks keys by key index. Index is elemental, should be inside keys counts in sub-tracks.
        //! @param keyIndex The index specifying keys.
        //! @param description Concatenation of short human readable text descriptions of keys.
        //! @param duration Zeroed.
        void GetKeyInfo(int keyIndex, const char*& description, float& duration) const override;

        //! Create keys at given time, and return summary index.
        //! @return First index of new keys created in sub-tracks, or -1 if keys are not created (for example, if a key at this time exists).
        int CreateKey(float time) override;

        int CloneKey([[maybe_unused]] int srcKeyIndex) override
        {
            AZ_Assert(false, "Not expected to be used");
            return 0;
        }

        int CopyKey([[maybe_unused]] IAnimTrack* pFromTrack, [[maybe_unused]] int fromKeyIndex) override
        {
            AZ_Assert(false, "Not expected to be used");
            return 0;
        }

        void GetKey([[maybe_unused]] int keyIndex, [[maybe_unused]] IKey* key) const override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        //! Get time of a key found in sub-tracks with specified key index.
        //! Index is extended, includes count of all keys existing in sub-tracks before the needed key.
        //! @return Key time, or -1 if a key with the index is not found.
        float GetKeyTime(int keyIndex) const override;

        float GetMinKeyTimeDelta() const override { return 0.01f; };

        //! Find a key at given time within minimal time delta between keys, in a first track with such key.
        //! @return Index of the found key (includes count of all keys scanned before the key was found),
        //!         or -1 if a key with this time is not found.
        int FindKey(float time) const override;

        int GetKeyFlags([[maybe_unused]] int keyIndex) override
        {
            AZ_Assert(false, "Not expected to be used");
            return 0;
        }

        void SetKey([[maybe_unused]] int keyIndex, [[maybe_unused]] IKey* key) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        //! Set time of a key found in sub-tracks with specified key index.
        //! Index is extended, includes count of all keys existing in sub-tracks before the needed key.
        void SetKeyTime(int keyIndex, float time) override;

        void SetKeyFlags([[maybe_unused]] int keyIndex, [[maybe_unused]] int flags) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SortKeys() override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        //! Check if all keys in sub-tracks corresponding to the given index are selected (actually, keys with the time of a key found).
        //! Index is extended, includes count of all keys existing in sub-tracks before the needed key.
        //! @return True if all keys in sub-tracks selected (have selected flag).
        bool IsKeySelected(int keyIndex) const override;

        //! Select keys (set selected flag) to all keys at time corresponding to time of a key found with given index.
        //! Index is extended, includes count of all keys existing in sub-tracks before the needed key.
        void SelectKey(int keyIndex, bool select) override;

        int GetFlags() const override
        {
            return m_flags;
        }

        bool IsMasked([[maybe_unused]] const uint32 mask) const override
        {
            return false;
        }

        void SetFlags(int flags) override
        {
            m_flags = flags;
        }

        //////////////////////////////////////////////////////////////////////////
        // Get track value at specified time.
        // Interpolates keys if needed.
        //////////////////////////////////////////////////////////////////////////
        void GetValue(float time, float& value, bool applyMultiplier = false) const override;
        void GetValue(float time, AZ::Vector3& value, bool applyMultiplier = false) const override;
        void GetValue(float time, AZ::Vector4& value, bool applyMultiplier = false) const override;
        void GetValue(float time, AZ::Quaternion& value) const override;
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
        void SetValue(float time, const float& value, bool bDefault = false, bool applyMultiplier = false) override;
        void SetValue(float time, const AZ::Vector3& value, bool bDefault = false, bool applyMultiplier = false) override;
        void SetValue(float time, const AZ::Vector4& value, bool bDefault = false, bool applyMultiplier = false) override;
        void SetValue(float time, const AZ::Quaternion& value, bool bDefault = false) override;
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

        void SetValue(
            [[maybe_unused]] float time, [[maybe_unused]] const AZStd::string& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void OffsetKeyPosition(const AZ::Vector3& value) override;
        void UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM) override;

        void SetTimeRange(const Range& timeRange) override;
        Range GetTimeRange() const override;

        bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true) override;

        bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected = false, float fTimeOffset = 0) override;

        void InitPostLoad([[maybe_unused]] IAnimSequence* sequence) override;

        int NextKeyByTime(int keyIndex) const override;

        void SetSubTrackName(const int idx, const AZStd::string& name)
        {
            if (idx < 0 || idx >= GetSubTrackCount())
            {
                AZ_Assert(false, "Subtrack index (%d) is out of range (0 .. %d).", idx, GetSubTrackCount());
                return;
            }
            m_subTrackNames[idx] = name;
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

        void GetKeyValueRange(float& fMin, float& fMax) const override
        {
            if (GetSubTrackCount() > 0)
            {
                m_subTracks[0]->GetKeyValueRange(fMin, fMax);
            }
        }

        void SetKeyValueRange(float fMin, float fMax) override
        {
            for (int i = 0; i < m_nDimensions; ++i)
            {
                m_subTracks[i]->SetKeyValueRange(fMin, fMax);
            }
        }

        void SetMultiplier(float trackMultiplier) override
        {
            for (int i = 0; i < m_nDimensions; ++i)
            {
                m_subTracks[i]->SetMultiplier(trackMultiplier);
            }
        }

        void SetExpanded(bool expanded) override;
        bool GetExpanded() const override;

        unsigned int GetId() const override;
        void SetId(unsigned int id) override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        void RenameSubTracksIfNeeded(); // Called in ctor, and then sub-track names are serialized.
        void SetKeyValueRanges(); // Key Value Range in a sub-track is not serialized, set it depending on the m_valueType and sub-track's m_nParamType.


        int m_refCount;
        AnimValueType m_valueType;
        int m_nDimensions;
        AZStd::vector<AZStd::intrusive_ptr<IAnimTrack>> m_subTracks;
        int m_flags;
        CAnimParamType m_nParamType;
        AZStd::vector<AZStd::string> m_subTrackNames;

#ifdef MOVIESYSTEM_SUPPORT_EDITING
        ColorB m_customColor;
        bool m_bCustomColorSet;
#endif

        float PreferShortestRotPath(float degree, float degree0) const;

        // Return index of a track having a key with keyIndex within total number of keys in sub-tracks;
        // also return keyIndex in a selected sub-track.
        int GetSubTrackIndex(int& keyIndex) const;

        IAnimNode* m_node;
        bool m_expanded;
        unsigned int m_id = 0;

        static constexpr float s_MinTimePrecision = 0.01f;
    };

    //////////////////////////////////////////////////////////////////////////
    inline void CCompoundSplineTrack::add_ref()
    {
        ++m_refCount;
    }

    //////////////////////////////////////////////////////////////////////////
    inline void CCompoundSplineTrack::release()
    {
        if (--m_refCount <= 0)
        {
            delete this;
        }
    }

} // namespace Maestro
