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

// Description : Dummy font implementation (dedicated server)


#ifndef CRYINCLUDE_CRYFONT_NULLFONT_H
#define CRYINCLUDE_CRYFONT_NULLFONT_H
#pragma once


#if defined(USE_NULLFONT)

#include <IFont.h>

class CNullFont
    : public IFFont
{
public:
    CNullFont() {}
    virtual ~CNullFont() {}

    virtual int32 AddRef() { return 0; };
    virtual int32 Release() { return 0; };

    virtual bool Load([[maybe_unused]] const char* pFontFilePath, [[maybe_unused]] unsigned int width, [[maybe_unused]] unsigned int height, [[maybe_unused]] unsigned int widthNumSlots, [[maybe_unused]] unsigned int heightNumSlots, [[maybe_unused]] unsigned int flags, [[maybe_unused]] float sizeRatio) { return true; }
    virtual bool Load([[maybe_unused]] const char* pXMLFile) { return true; }
    virtual void Free() {}

    virtual void DrawString([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] const char* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) {}
    virtual void DrawString([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z, [[maybe_unused]] const char* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) {}

    virtual void DrawStringW([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] const wchar_t* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) {}
    virtual void DrawStringW([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z, [[maybe_unused]] const wchar_t* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) {}

    virtual Vec2 GetTextSize([[maybe_unused]] const char* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) { return Vec2(0.0f, 0.0f); }
    virtual Vec2 GetTextSizeW([[maybe_unused]] const wchar_t* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) { return Vec2(0.0f, 0.0f); }

    virtual size_t GetTextLength([[maybe_unused]] const char* pStr, [[maybe_unused]] const bool asciiMultiLine) const { return 0; }
    virtual size_t GetTextLengthW([[maybe_unused]] const wchar_t* pStr, [[maybe_unused]] const bool asciiMultiLine) const { return 0; }

    virtual void WrapText(string& result, [[maybe_unused]] float maxWidth, const char* pStr, [[maybe_unused]] const STextDrawContext& ctx) { result = pStr; }
    virtual void WrapTextW(wstring& result, [[maybe_unused]] float maxWidth, const wchar_t* pStr, [[maybe_unused]] const STextDrawContext& ctx) { result = pStr; }

    virtual void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}

    virtual void GetGradientTextureCoord([[maybe_unused]] float& minU, [[maybe_unused]] float& minV, [[maybe_unused]] float& maxU, [[maybe_unused]] float& maxV) const {}

    virtual unsigned int GetEffectId([[maybe_unused]] const char* pEffectName) const { return 0; }
    virtual unsigned int GetNumEffects() const { return 0; }
    virtual const char* GetEffectName([[maybe_unused]] unsigned int effectId) const { return nullptr; }
    virtual Vec2 GetMaxEffectOffset([[maybe_unused]] unsigned int effectId) const { return Vec2(); }
    virtual bool DoesEffectHaveTransparency([[maybe_unused]] unsigned int effectId) const { return false; }

    virtual void AddCharsToFontTexture([[maybe_unused]] const char* pChars, [[maybe_unused]] int glyphSizeX, [[maybe_unused]] int glyphSizeY) override {}
    virtual Vec2 GetKerning([[maybe_unused]] uint32_t leftGlyph, [[maybe_unused]] uint32_t rightGlyph, [[maybe_unused]] const STextDrawContext& ctx) const override { return Vec2(); }
    virtual float GetAscender([[maybe_unused]] const STextDrawContext& ctx) const override { return 0.0f; }
    virtual float GetBaseline([[maybe_unused]] const STextDrawContext& ctx) const override { return 0.0f; }
    virtual float GetSizeRatio() const override { return IFFontConstants::defaultSizeRatio; }
    virtual uint32 GetNumQuadsForText([[maybe_unused]] const char* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) { return 0; }
    virtual uint32 WriteTextQuadsToBuffers([[maybe_unused]] SVF_P2F_C4B_T2F_F4B* verts, [[maybe_unused]] uint16* indices, [[maybe_unused]] uint32 maxQuads, [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z, [[maybe_unused]] const char* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) { return 0; }
    virtual int GetFontTextureId() { return -1; }
    virtual uint32 GetFontTextureVersion() { return 0; }
};

class CCryNullFont
    : public ICryFont
{
public:
    virtual void Release() {}
    virtual IFFont* NewFont([[maybe_unused]] const char* pFontName) { return &ms_nullFont; }
    virtual IFFont* GetFont([[maybe_unused]] const char* pFontName) const { return &ms_nullFont; }
    virtual FontFamilyPtr LoadFontFamily([[maybe_unused]] const char* pFontFamilyName) override { CRY_ASSERT(false); return nullptr; }
    virtual FontFamilyPtr GetFontFamily([[maybe_unused]] const char* pFontFamilyName) override { CRY_ASSERT(false); return nullptr; }
    virtual void AddCharsToFontTextures(FontFamilyPtr pFontFamily, [[maybe_unused]] const char* pChars, [[maybe_unused]] int glyphSizeX, [[maybe_unused]] int glyphSizeY) override {};
    virtual void SetRendererProperties([[maybe_unused]] IRenderer* pRenderer) {}
    virtual void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}
    virtual string GetLoadedFontNames() const { return ""; }
    virtual void OnLanguageChanged() override { }
    virtual void ReloadAllFonts() override { } 

private:
    static CNullFont ms_nullFont;
};

#endif // USE_NULLFONT

#endif // CRYINCLUDE_CRYFONT_NULLFONT_H
