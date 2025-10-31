/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "SoundTrack.h"

namespace Maestro
{

    void CSoundTrack::SerializeKey(ISoundKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            const char* sTemp;
            sTemp = keyNode->getAttr("StartTrigger");
            key.sStartTrigger = sTemp;

            sTemp = keyNode->getAttr("StopTrigger");
            key.sStopTrigger = sTemp;

            float fDuration = 0.0f;

            if (keyNode->getAttr("Duration", fDuration))
            {
                key.fDuration = fDuration;
            }

            keyNode->getAttr("CustomColor", key.customColor);
        }
        else
        {
            keyNode->setAttr("StartTrigger", key.sStartTrigger.c_str());
            keyNode->setAttr("StopTrigger", key.sStopTrigger.c_str());
            keyNode->setAttr("Duration", key.fDuration);
            keyNode->setAttr("CustomColor", key.customColor);
        }
    }

    void CSoundTrack::GetKeyInfo(int keyIndex, const char*& description, float& duration) const
    {
        description = 0;
        duration = 0;

        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        duration = m_keys[keyIndex].fDuration;

        if (!m_keys[keyIndex].sStartTrigger.empty())
        {
            description = m_keys[keyIndex].sStartTrigger.c_str();
        }
    }

    static bool SoundTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<ISoundKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<ISoundKey>, IAnimTrack>()
                ->Version(3, &SoundTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<ISoundKey>::m_flags)
                ->Field("Range", &TAnimTrack<ISoundKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<ISoundKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<ISoundKey>::m_keys)
                ->Field("Id", &TAnimTrack<ISoundKey>::m_id);
        }
    }

    void CSoundTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<ISoundKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CSoundTrack, TAnimTrack<ISoundKey>>()->Version(1);
        }
    }

} // namespace Maestro
