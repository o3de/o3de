/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "ConsoleTrack.h"

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
void CConsoleTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = 0;
    if (!m_keys[key].command.empty())
    {
        description = m_keys[key].command.c_str();
    }
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<IConsoleKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TAnimTrack<IConsoleKey> >()
        ->Version(2)
        ->Field("Flags", &TAnimTrack<IConsoleKey>::m_flags)
        ->Field("Range", &TAnimTrack<IConsoleKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<IConsoleKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<IConsoleKey>::m_keys)
        ->Field("Id", &TAnimTrack<IConsoleKey>::m_id);
}

//////////////////////////////////////////////////////////////////////////
void CConsoleTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<IConsoleKey>::Reflect(serializeContext);

    serializeContext->Class<CConsoleTrack, TAnimTrack<IConsoleKey> >()
        ->Version(1);
}
