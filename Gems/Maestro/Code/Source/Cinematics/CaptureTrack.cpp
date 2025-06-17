/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include "CaptureTrack.h"

namespace Maestro
{

    void CCaptureTrack::SerializeKey(ICaptureKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            const char* desc;
            keyNode->getAttr("time", key.time);
            keyNode->getAttr("flags", key.flags);
            keyNode->getAttr("duration", key.duration);
            keyNode->getAttr("timeStep", key.timeStep);
            desc = keyNode->getAttr("folder");
            key.folder = desc;
            keyNode->getAttr("once", key.once);
            desc = keyNode->getAttr("prefix");
            if (desc)
            {
                key.prefix = desc;
            }
        }
        else
        {
            keyNode->setAttr("time", key.time);
            keyNode->setAttr("flags", key.flags);
            keyNode->setAttr("duration", key.duration);
            keyNode->setAttr("timeStep", key.timeStep);
            keyNode->setAttr("folder", key.folder.c_str());
            keyNode->setAttr("once", key.once);
            keyNode->setAttr("prefix", key.prefix.c_str());
        }
    }

    void CCaptureTrack::GetKeyInfo(int keyIndex, const char*& description, float& duration) const
    {
        description = 0;
        duration = 0;

        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        static char buffer[256];
        duration = m_keys[keyIndex].once ? 0 : m_keys[keyIndex].duration;
        char prefix[64] = "Frame";
        if (!m_keys[keyIndex].prefix.empty())
        {
            azstrcpy(prefix, AZ_ARRAY_SIZE(prefix), m_keys[keyIndex].prefix.c_str());
        }
        description = buffer;
        if (!m_keys[keyIndex].folder.empty())
        {
            azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "[%s], %.3f, %s", prefix, m_keys[keyIndex].timeStep, m_keys[keyIndex].folder.c_str());
        }
        else
        {
            azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "[%s], %.3f", prefix, m_keys[keyIndex].timeStep);
        }
    }

    static bool CaptureTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<ICaptureKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<ICaptureKey>, IAnimTrack>()
                ->Version(3, &CaptureTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<ICaptureKey>::m_flags)
                ->Field("Range", &TAnimTrack<ICaptureKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<ICaptureKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<ICaptureKey>::m_keys)
                ->Field("Id", &TAnimTrack<ICaptureKey>::m_id);
        }
    }

    void CCaptureTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<ICaptureKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CCaptureTrack, TAnimTrack<ICaptureKey>>()->Version(1);
        }
    }

} // namespace Maestro
