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
#include "CommentTrack.h"

//-----------------------------------------------------------------------------
CCommentTrack::CCommentTrack()
{
}

//-----------------------------------------------------------------------------
void CCommentTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    static char desc[128];
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = m_keys[key].m_duration;

    cry_strcpy(desc, m_keys[key].m_strComment.c_str());

    description = desc;
}

//-----------------------------------------------------------------------------
void CCommentTrack::SerializeKey(ICommentKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        // Load only in the editor for loading performance.
        if (gEnv->IsEditor())
        {
            XmlString xmlComment;
            keyNode->getAttr("comment", xmlComment);
            key.m_strComment = xmlComment;
            keyNode->getAttr("duration", key.m_duration);
            const char* strFont = keyNode->getAttr("font");
            key.m_strFont = strFont;

            Vec4 color(key.m_color.GetR(), key.m_color.GetG(), key.m_color.GetB(), key.m_color.GetA());
            keyNode->getAttr("color", color);
            key.m_color.Set(color.x, color.y, color.z, color.w);
            
            keyNode->getAttr("size", key.m_size);
            int alignment = 0;
            keyNode->getAttr("align", alignment);
            key.m_align = (ICommentKey::ETextAlign)(alignment);
        }
    }
    else
    {
        XmlString xmlComment(key.m_strComment.c_str());
        keyNode->setAttr("comment", xmlComment);
        keyNode->setAttr("duration", key.m_duration);
        if (!key.m_strFont.empty())
        {
            keyNode->setAttr("font", key.m_strFont.c_str());
        }
        Vec4 color(key.m_color.GetR(), key.m_color.GetG(), key.m_color.GetB(), key.m_color.GetA());
        keyNode->setAttr("color", color);
        keyNode->setAttr("size", key.m_size);
        keyNode->setAttr("align", (int)key.m_align);
    }
}


//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<ICommentKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TAnimTrack<ICommentKey> >()
        ->Version(2)
        ->Field("Flags", &TAnimTrack<ICommentKey>::m_flags)
        ->Field("Range", &TAnimTrack<ICommentKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<ICommentKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<ICommentKey>::m_keys)
        ->Field("Id", &TAnimTrack<ICommentKey>::m_id);
}

//////////////////////////////////////////////////////////////////////////
void CCommentTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<ICommentKey>::Reflect(serializeContext);

    serializeContext->Class<CCommentTrack, TAnimTrack<ICommentKey> >()
        ->Version(1);
}
