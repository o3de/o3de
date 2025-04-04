/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Comment node animator class

/*
    CCommentContext stores information about comment track.
    The Comment Track is activated only in the editor.
*/


#pragma once

#include "TrackViewAnimNode.h"

class CTrackViewTrack;

struct CCommentContext
{
    CCommentContext()
        : m_nLastActiveKeyIndex(-1)
        , m_size(1.0f)
        , m_align(0)
        , m_color(0.f, 0.f, 0.f, 1.f)
    {
        sprintf_s(m_strFont, sizeof(m_strFont), "default");
        m_unitPos = Vec2(0.f, 0.f);
    }

    int m_nLastActiveKeyIndex;

    AZStd::string m_strComment;
    char m_strFont[64];
    Vec2 m_unitPos;
    AZ::Color m_color;
    float m_size;
    int m_align;
};

class CCommentNodeAnimator
    : public IAnimNodeAnimator
{
public:
    CCommentNodeAnimator(CTrackViewAnimNode* pCommentNode);
    void Animate(CTrackViewAnimNode* pNode, const SAnimContext& ac) override;
    void Render(CTrackViewAnimNode* pNode, const SAnimContext& ac) override;

private:
    virtual ~CCommentNodeAnimator();

    void AnimateCommentTextTrack(CTrackViewTrack* pTrack, const SAnimContext& ac);
    CTrackViewKeyHandle GetActiveKeyHandle(CTrackViewTrack* pTrack, float fTime);
    Vec2 GetScreenPosFromNormalizedPos(const Vec2& unitPos);
    void DrawText(const char* szFontName, float fSize, const Vec2& unitPos, const ColorF col, const char* szText, int align);

    CTrackViewAnimNode* m_pCommentNode;
    CCommentContext m_commentContext;
};
