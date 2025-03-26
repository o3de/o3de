/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "ConsoleTrack.h"

namespace Maestro
{

    void CConsoleTrack::SerializeKey(IConsoleKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            const char* str;
            str = keyNode->getAttr("command");
            key.command = str;
        }
        else
        {
            if (!key.command.empty())
            {
                keyNode->setAttr("command", key.command.c_str());
            }
        }
    }

    void CConsoleTrack::GetKeyInfo(int key, const char*& description, float& duration)
    {
        AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index %i is out of range", key);
        CheckValid();
        description = 0;
        duration = 0;
        if (!m_keys[key].command.empty())
        {
            description = m_keys[key].command.c_str();
        }
    }

    static bool ConsoleTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<IConsoleKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<IConsoleKey>, IAnimTrack>()
                ->Version(3, &ConsoleTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<IConsoleKey>::m_flags)
                ->Field("Range", &TAnimTrack<IConsoleKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<IConsoleKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<IConsoleKey>::m_keys)
                ->Field("Id", &TAnimTrack<IConsoleKey>::m_id);
        }
    }

    void CConsoleTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<IConsoleKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CConsoleTrack, TAnimTrack<IConsoleKey>>()->Version(1);
        }
    }

} // namespace Maestro
