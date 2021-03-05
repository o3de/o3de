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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_ITEXTURESTREAMER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_ITEXTURESTREAMER_H
#pragma once


struct STexStreamingInfo;
struct STexStreamPrepState;

typedef DynArray<CTexture*> TStreamerTextureVec;

class ITextureStreamer
{
public:
    enum EApplyScheduleFlags
    {
        eASF_InOut = 1,
        eASF_Prep = 2,

        eASF_Full = 3,
    };

public:
    ITextureStreamer();
    virtual ~ITextureStreamer() {}

public:
    virtual void BeginUpdateSchedule();
    virtual void ApplySchedule(EApplyScheduleFlags asf);

    virtual bool BeginPrepare(CTexture* pTexture, const char* sFilename, uint32 nFlags) = 0;
    virtual void EndPrepare(STexStreamPrepState*& pState) = 0;

    virtual void Precache(CTexture* pTexture) = 0;
    virtual void UpdateMip(CTexture* pTexture, const float fMipFactor, const int nFlags, const int nUpdateId, const int nCounter) = 0;

    virtual void OnTextureDestroy(CTexture* pTexture) = 0;

    void Relink(CTexture* pTexture);
    void Unlink(CTexture* pTexture);

    virtual void FlagOutOfMemory() = 0;
    virtual void Flush() = 0;

    virtual bool IsOverflowing() const = 0;
    virtual float GetBias() const { return 0.0f; }

    int GetMinStreamableMip() const;
    int GetMinStreamableMipWithSkip() const;

    void StatsFetchTextures(std::vector<CTexture*>& out);
    bool StatsWouldUnload(const CTexture* pTexture);

protected:
    void SyncTextureList();

    TStreamerTextureVec& GetTextures()
    {
        return m_textures;
    }

private:
    size_t StatsComputeRequiredMipMemUsage();

private:
    TStreamerTextureVec m_pendingRelinks;
    TStreamerTextureVec m_pendingUnlinks;
    TStreamerTextureVec m_textures;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_ITEXTURESTREAMER_H
