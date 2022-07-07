/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Dummy font implementation (dedicated server)

#pragma once

#define USE_NULLFONT

#if defined(USE_NULLFONT)

#include <IFont.h>

namespace AZ
{
    class AtomNullFFont
        : public IFFont
    {
    public:
        using TextDrawContext = STextDrawContext;

        AtomNullFFont() {}
        ~AtomNullFFont() override {}

        int32_t AddRef() override { return 0; };
        int32_t Release() override { return 0; };

        bool Load([[maybe_unused]] const char* fontFilePath, [[maybe_unused]] unsigned int width, [[maybe_unused]] unsigned int height, [[maybe_unused]] unsigned int widthNumSlots, [[maybe_unused]] unsigned int heightNumSlots, [[maybe_unused]] unsigned int flags, [[maybe_unused]] float sizeRatio) override { return true; }
        bool Load([[maybe_unused]] const char* xmlFile) override { return true; }
        void Free() override {}

        void DrawString([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] const char* str, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const TextDrawContext& ctx) override {}
        void DrawString([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z, [[maybe_unused]] const char* str, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const TextDrawContext& ctx) override {}

        Vec2 GetTextSize([[maybe_unused]] const char* str, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const TextDrawContext& ctx) override { return Vec2(0.0f, 0.0f); }

        size_t GetTextLength([[maybe_unused]] const char* str, [[maybe_unused]] const bool asciiMultiLine) const override { return 0; }

        void WrapText(AZStd::string& result, [[maybe_unused]] float maxWidth, const char* str, [[maybe_unused]] const TextDrawContext& ctx) override { result = str; }

        void GetGradientTextureCoord([[maybe_unused]] float& minU, [[maybe_unused]] float& minV, [[maybe_unused]] float& maxU, [[maybe_unused]] float& maxV) const override {}

        unsigned int GetEffectId([[maybe_unused]] const char* effectName) const override { return 0; }
        unsigned int GetNumEffects() const override { return 0; }
        const char* GetEffectName([[maybe_unused]] unsigned int effectId) const override { return nullptr; }
        Vec2 GetMaxEffectOffset([[maybe_unused]] unsigned int effectId) const override { return Vec2(); }
        bool DoesEffectHaveTransparency([[maybe_unused]] unsigned int effectId) const override { return false; }

        void AddCharsToFontTexture([[maybe_unused]] const char* chars, [[maybe_unused]] int glyphSizeX, [[maybe_unused]] int glyphSizeY) override {}
        Vec2 GetKerning([[maybe_unused]] uint32_t leftGlyph, [[maybe_unused]] uint32_t rightGlyph, [[maybe_unused]] const TextDrawContext& ctx) const override { return Vec2(); }
        float GetAscender([[maybe_unused]] const TextDrawContext& ctx) const override { return 0.0f; }
        float GetBaseline([[maybe_unused]] const TextDrawContext& ctx) const override { return 0.0f; }
        float GetSizeRatio() const override { return IFFontConstants::defaultSizeRatio; }
        uint32_t GetNumQuadsForText([[maybe_unused]] const char* str, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const TextDrawContext& ctx) override { return 0; }
        uint32_t WriteTextQuadsToBuffers([[maybe_unused]] SVF_P2F_C4B_T2F_F4B* verts, [[maybe_unused]] uint16_t* indices, [[maybe_unused]] uint32_t maxQuads, [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z, [[maybe_unused]] const char* str, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const TextDrawContext& ctx) override { return 0; }
        int GetFontTextureId() override { return -1; }
        uint32_t GetFontTextureVersion() override { return 0; }
    };

    class AtomNullFont
        : public ICryFont
    {
    public:
        virtual void Release() override {}
        virtual IFFont* NewFont([[maybe_unused]] const char* fontName) override { return &NullFFont; }
        virtual IFFont* GetFont([[maybe_unused]] const char* fontName) const override { return &NullFFont; }
        virtual FontFamilyPtr LoadFontFamily([[maybe_unused]] const char* fontFamilyName) override { CRY_ASSERT(false); return nullptr; }
        virtual FontFamilyPtr GetFontFamily([[maybe_unused]] const char* fontFamilyName) override { CRY_ASSERT(false); return nullptr; }
        virtual void AddCharsToFontTextures([[maybe_unused]] FontFamilyPtr fontFamily, [[maybe_unused]] const char* chars, [[maybe_unused]] int glyphSizeX, [[maybe_unused]] int glyphSizeY) override {};
        virtual AZStd::string GetLoadedFontNames() const override { return ""; }
        virtual void OnLanguageChanged() override { }
        virtual void ReloadAllFonts() override { } 

    private:
        static AtomNullFFont NullFFont;
    };
}
#endif // USE_NULLFONT
