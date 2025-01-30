/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "LookAtTrack.h"

namespace Maestro
{

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    bool CLookAtTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        if (bLoading)
        {
            xmlNode->getAttr("AnimationLayer", m_iAnimationLayer);
        }
        else
        {
            xmlNode->setAttr("AnimationLayer", m_iAnimationLayer);
        }

        return TAnimTrack<ILookAtKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    }

    void CLookAtTrack::SerializeKey(ILookAtKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            const char* szSelection;
            f32 smoothTime;
            if (!keyNode->getAttr("smoothTime", smoothTime))
            {
                smoothTime = 0.0f;
            }

            const char* lookPose = keyNode->getAttr("lookPose");
            if (lookPose)
            {
                key.lookPose = lookPose;
            }

            szSelection = keyNode->getAttr("node");
            if (szSelection)
            {
                key.szSelection = szSelection;
            }

            key.smoothTime = smoothTime;
        }
        else
        {
            keyNode->setAttr("node", key.szSelection.c_str());
            keyNode->setAttr("smoothTime", key.smoothTime);
            keyNode->setAttr("lookPose", key.lookPose.c_str());
        }
    }

    void CLookAtTrack::GetKeyInfo(int key, const char*& description, float& duration)
    {
        AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index %i is out of range", key);
        CheckValid();
        description = 0;
        duration = m_keys[key].fDuration;
        if (!m_keys[key].szSelection.empty())
        {
            description = m_keys[key].szSelection.c_str();
        }
    }

    static bool LookAtTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<ILookAtKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<ILookAtKey>, IAnimTrack>()
                ->Version(3, &LookAtTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<ILookAtKey>::m_flags)
                ->Field("Range", &TAnimTrack<ILookAtKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<ILookAtKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<ILookAtKey>::m_keys)
                ->Field("Id", &TAnimTrack<ILookAtKey>::m_id);
        }
    }

    void CLookAtTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<ILookAtKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CLookAtTrack, TAnimTrack<ILookAtKey>>()->Version(1)->Field(
                "AnimationLayer", &CLookAtTrack::m_iAnimationLayer);
        }
    }

} // namespace Maestro
