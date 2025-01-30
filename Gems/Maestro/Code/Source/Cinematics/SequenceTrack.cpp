/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "SequenceTrack.h"

namespace Maestro
{

    /// @deprecated Serialization for Sequence Tracks in Component Entity Sequences now occur through AZ::SerializeContext and the Sequence
    /// Component
    void CSequenceTrack::SerializeKey(ISequenceKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            const char* szSelection;
            szSelection = keyNode->getAttr("node");
            key.szSelection = szSelection;

            AZ::u64 id64;
            if (keyNode->getAttr("sequenceEntityId", id64))
            {
                key.sequenceEntityId = AZ::EntityId(id64);
            }

            if (!keyNode->getAttr("overridetimes", key.bOverrideTimes))
            {
                key.bOverrideTimes = false;
            }

            if (key.bOverrideTimes == true)
            {
                if (!keyNode->getAttr("starttime", key.fStartTime))
                {
                    key.fStartTime = 0.0f;
                }
                if (!keyNode->getAttr("endtime", key.fEndTime))
                {
                    key.fEndTime = 0.0f;
                }
            }
            else
            {
                key.fStartTime = 0.0f;
                key.fEndTime = 0.0f;
            }
        }
        else
        {
            keyNode->setAttr("node", key.szSelection.c_str());
            AZ::u64 id64 = static_cast<AZ::u64>(key.sequenceEntityId);
            keyNode->setAttr("sequenceEntityId", id64);

            if (key.bOverrideTimes == true)
            {
                keyNode->setAttr("overridetimes", key.bOverrideTimes);
                keyNode->setAttr("starttime", key.fStartTime);
                keyNode->setAttr("endtime", key.fEndTime);
            }
        }
    }

    void CSequenceTrack::GetKeyInfo(int key, const char*& description, float& duration)
    {
        AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index %i is out of range", key);
        CheckValid();
        description = 0;
        duration = m_keys[key].fDuration;

        IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();

        IAnimSequence* sequence = movieSystem ? movieSystem->FindSequence(m_keys[key].sequenceEntityId) : nullptr;
        if (sequence)
        {
            description = sequence->GetName();
        }
    }

    static bool SequencTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<ISequenceKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<ISequenceKey>, IAnimTrack>()
                ->Version(3, &SequencTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<ISequenceKey>::m_flags)
                ->Field("Range", &TAnimTrack<ISequenceKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<ISequenceKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<ISequenceKey>::m_keys)
                ->Field("Id", &TAnimTrack<ISequenceKey>::m_id);
        }
    }

    void CSequenceTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<ISequenceKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CSequenceTrack, TAnimTrack<ISequenceKey>>()->Version(1);
        }
    }

} // namespace Maestro
