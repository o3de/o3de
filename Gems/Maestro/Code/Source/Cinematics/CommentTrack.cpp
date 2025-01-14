/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "CommentTrack.h"

namespace Maestro
{

    CCommentTrack::CCommentTrack()
    {
    }

    void CCommentTrack::GetKeyInfo(int key, const char*& description, float& duration)
    {
        static char desc[128];
        AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index %i is out of range", key);
        CheckValid();
        description = 0;
        duration = m_keys[key].m_duration;

        azstrcpy(desc, AZ_ARRAY_SIZE(desc), m_keys[key].m_strComment.c_str());

        description = desc;
    }

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

    static bool CommentTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<ICommentKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<ICommentKey>, IAnimTrack>()
                ->Version(3, &CommentTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<ICommentKey>::m_flags)
                ->Field("Range", &TAnimTrack<ICommentKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<ICommentKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<ICommentKey>::m_keys)
                ->Field("Id", &TAnimTrack<ICommentKey>::m_id);
        }
    }

    void CCommentTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<ICommentKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CCommentTrack, TAnimTrack<ICommentKey>>()->Version(1);
        }
    }

} // namespace Maestro
