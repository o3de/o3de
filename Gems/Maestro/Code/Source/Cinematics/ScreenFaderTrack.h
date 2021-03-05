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

#ifndef CRYINCLUDE_CRYMOVIE_SCREENFADERTRACK_H
#define CRYINCLUDE_CRYMOVIE_SCREENFADERTRACK_H
#pragma once


#include "IMovieSystem.h"
#include "AnimTrack.h"

class CScreenFaderTrack
    : public TAnimTrack<IScreenFaderKey>
{
public:
    AZ_CLASS_ALLOCATOR(CScreenFaderTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CScreenFaderTrack, "{3279BB19-D32D-482E-BD6E-C2DCD8858328}", IAnimTrack);

    //-----------------------------------------------------------------------------
    //!
    CScreenFaderTrack();
    ~CScreenFaderTrack();

    //-----------------------------------------------------------------------------
    //! IAnimTrack Method Overriding.
    //-----------------------------------------------------------------------------
    virtual void GetKeyInfo(int key, const char*& description, float& duration);
    virtual void SerializeKey(IScreenFaderKey& key, XmlNodeRef& keyNode, bool bLoading);
    void SetFlags(int flags) override;

    void PreloadTextures();
    ITexture* GetActiveTexture() const;
    void SetScreenFaderTrackDefaults();

    bool IsTextureVisible() const {return m_bTextureVisible; };
    void SetTextureVisible(bool bVisible){m_bTextureVisible = bVisible; };
    Vec4 GetDrawColor() const {return m_drawColor; };
    void SetDrawColor(Vec4 vDrawColor){m_drawColor = vDrawColor; };
    int GetLastTextureID() const { return m_lastTextureID; };
    void SetLastTextureID(int nTextureID){ m_lastTextureID = nTextureID; };
    bool SetActiveTexture(int index);

    static void Reflect(AZ::SerializeContext* serializeContext);

private:
    void ReleasePreloadedTextures();

    std::vector<ITexture*> m_preloadedTextures;
    bool m_bTextureVisible;
    Vec4 m_drawColor;
    int m_lastTextureID;
};

#endif // CRYINCLUDE_CRYMOVIE_SCREENFADERTRACK_H
