/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Font class.


#pragma once


#if !defined(USE_NULLFONT_ALWAYS)

#include <vector>
#include <CryCommon/IRenderer.h>
#include "AtomFont.h"

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/containers/map.h>
#include <AzFramework/Font/FontInterface.h>

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/WindowContext.h>

struct ISystem;

namespace AZ
{
    class FontTexture;

    struct FontDeleter
    {
        void operator () (const AZStd::intrusive_refcount<AZStd::atomic_uint, FontDeleter>* ptr) const;
    };

    using TextDrawContext = STextDrawContext;

    //! FFont is the implementation of IFFont used to draw text with a particular font (e.g. Consolas Italic)
    //! FFont manages creation of a gpu texture to cache the font and generates draw commands that use that texture.
    //! FFont's are managed by AtomFont as either individual font instances or a font family
    //! that collects all the variations (italic, bold, bold italic, normal).
    class FFont
        : public IFFont
        , public AZStd::intrusive_refcount<AZStd::atomic_uint, FontDeleter>
        , public AzFramework::FontDrawInterface
    {
        using ref_count = AZStd::intrusive_refcount<AZStd::atomic_uint, FontDeleter>;
        friend FontDeleter;
    public:
        //! Determines how characters of different sizes should be handled during render.
        enum class SizeBehavior
        {
            Scale,      //!< Default behavior; glyphs rendered at different sizes are rendered on scaled geometry
            Rerender    //!< Similar to Scale, but the glyph in the font texture is re-rendered to match the target
                        //!< size, as long as the size isn't greater than the maximum glyph/slot resolution as
                        //!< configured for the font texture in the font XML.
        };

        //! The hinting visual algorithm to be used (when hinting is enabled)
        enum class HintStyle
        {
            Normal, //!< Default hinting behavior provided by font renderer
            Light   //!< Produces fuzzier glyphs but more accurately tracks glyph shape
        };

        //! Chooses whether hinting info should be obtained from the font, turned off entirely, or automatically generated
        enum class HintBehavior
        {
            Default,    //!< Obtain hinting data from font itself
            AutoHint,   //!< Procedurally derive hinting information from glyph
            NoHinting,  //!< Disable hinting entirely
        };

        //! Simple struct used to communicate font hinting parameters to font renderer.
        struct FontHintParams
        {
            FontHintParams()
                : hintStyle(HintStyle::Normal)
                , hintBehavior(HintBehavior::Default) { }

            HintStyle hintStyle;
            HintBehavior hintBehavior;
        };

        struct FontRenderingPass
        {
            ColorB m_color = {255, 255, 255, 255};
            Vec2 m_posOffset = {0,0};
            int m_blendSrc = GS_BLSRC_SRCALPHA;
            int m_blendDest = GS_BLDST_ONEMINUSSRCALPHA;
        };

        struct FontEffect
        {
            AZStd::string m_name;
            std::vector<FontRenderingPass> m_passes;

            FontEffect(const char* name)
                : m_name(name)
            {
                assert(name);
            }

            FontRenderingPass* AddPass()
            {
                m_passes.push_back(FontRenderingPass());
                return &m_passes[m_passes.size() - 1];
            }

            void ClearPasses()
            {
                m_passes.resize(0);
            }
        };

        typedef std::vector<FontEffect> FontEffects;
        typedef FontEffects::iterator FontEffectsIterator;

        struct FontShaderData
        {
            AZ::RHI::ShaderInputNameIndex m_imageInputIndex = "m_texture";
            AZ::RHI::ShaderInputNameIndex m_viewProjInputIndex = "m_worldToProj";
        };

    public:
        /////////////////////////////////////////////////////////////////////////////////////////////////
        // IFFont interface
        int32_t AddRef() override;
        int32_t Release() override;
        bool Load(const char* fontFilePath, unsigned int width, unsigned int height, unsigned int widthNumSlots, unsigned int heightNumSlots, unsigned int flags, float sizeRatio) override;
        bool Load(const char* xmlFile) override;
        void Free() override;
        void DrawString(float x, float y, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        void DrawString(float x, float y, float z, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        Vec2 GetTextSize(const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        size_t GetTextLength(const char* str, const bool asciiMultiLine) const override;
        void WrapText(AZStd::string& result, float maxWidth, const char* str, const TextDrawContext& ctx) override;
        void GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const override;
        unsigned int GetEffectId(const char* effectName) const override;
        unsigned int GetNumEffects() const override;
        const char* GetEffectName(unsigned int effectId) const override;
        Vec2 GetMaxEffectOffset(unsigned int effectId) const override;
        bool DoesEffectHaveTransparency(unsigned int effectId) const override;
        void AddCharsToFontTexture(const char* chars, int glyphSizeX = ICryFont::defaultGlyphSizeX, int glyphSizeY = ICryFont::defaultGlyphSizeY) override;
        Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph, const TextDrawContext& ctx) const override;
        float GetAscender(const TextDrawContext& ctx) const override;
        float GetBaseline(const TextDrawContext& ctx) const override;
        float GetSizeRatio() const override;

        uint32_t GetNumQuadsForText(const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        uint32_t WriteTextQuadsToBuffers(SVF_P2F_C4B_T2F_F4B* verts, uint16_t* indices, uint32_t maxQuads, float x, float y, float z, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        int GetFontTextureId() override {return -1;} // old Cry Interface, disable
        uint32_t GetFontTextureVersion() override;
        /////////////////////////////////////////////////////////////////////////////////////////////////////

        // AzFramework::FontDrawInterface implementation
        void DrawScreenAlignedText2d(
            const AzFramework::TextDrawParameters& params,
            AZStd::string_view text) override;

        void DrawScreenAlignedText3d(
            const AzFramework::TextDrawParameters& params,
            AZStd::string_view text) override;

        AZ::Vector2 GetTextSize(
            const AzFramework::TextDrawParameters& params,
            AZStd::string_view text) override;

    public:
        FFont(AtomFont* atomFont, const char* fontName);

        FontTexture* GetFontTexture() const { return m_fontTexture; }
        const AZStd::string& GetName() const { return m_name; }

        FontEffect* AddEffect(const char* effectName);
        FontEffect* GetDefaultEffect();

        AZ::Data::Instance<AZ::RPI::Image> GetFontImage() { return m_fontAttachmentImage; }

    private:
        virtual ~FFont();
        bool InitTexture();
        bool InitCache();

        void Prepare(const char* str, bool updateTexture, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize);
        void DrawStringUInternal(
            const RHI::Viewport& viewport, 
            RPI::ViewportContextPtr viewportContext, 
            float x, 
            float y, 
            float z, 
            const char* str, 
            const bool asciiMultiLine, 
            const TextDrawContext& ctx);
        Vec2 GetTextSizeUInternal(const RHI::Viewport& viewport, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx);
        Vec2 GetKerningInternal(const RHI::Viewport& viewport, uint32_t leftGlyph, uint32_t rightGlyph, const TextDrawContext& ctx) const;
        float GetBaselineInternal(const RHI::Viewport& viewport, const TextDrawContext& ctx) const;

        // returns true if add operation was successful, false otherwise
        using AddFunction = AZStd::function<bool(const Vec3&, const Vec3&, const Vec3&, const Vec3&, const Vec2&, const Vec2&, const Vec2&, const Vec2&, uint32_t)>;

        //! This function is used by both DrawStringUInternal and WriteTextQuadsToBuffers
        //! To do this is takes a function pointer that implement the appropriate AddQuad behavior
        int CreateQuadsForText(
            const RHI::Viewport& viewport, 
            float x,
            float y,
            float z,
            const char* str,
            const bool asciiMultiLine,
            const TextDrawContext& ctx,
            AddFunction AddQuad);

        struct TextScaleInfoInternal
        {
            TextScaleInfoInternal(const Vec2& _scale, float _rcpCellWidth)
                : scale(_scale)
                , rcpCellWidth(_rcpCellWidth) { }

            Vec2 scale;
            float rcpCellWidth;
        };

        TextScaleInfoInternal CalculateScaleInternal(const RHI::Viewport& viewport, const TextDrawContext& ctx) const;

        Vec2 GetRestoredFontSize(const TextDrawContext& ctx) const;

        bool UpdateTexture();

        void ScaleCoord(const RHI::Viewport& viewport, float& x, float& y) const;

        RPI::WindowContextSharedPtr GetDefaultWindowContext() const;
        RPI::ViewportContextPtr GetDefaultViewportContext() const;

        struct DrawParameters
        {
            TextDrawContext m_ctx;
            AZ::Vector2 m_position;
            AZ::Vector2 m_size;
            AZ::RPI::ViewportContextPtr m_viewportContext;
            AZ::RHI::Viewport m_viewport;
        };
        DrawParameters ExtractDrawParameters(const AzFramework::TextDrawParameters& params, AZStd::string_view text, bool forceCalculateSize);

    private:
        static constexpr uint32_t NumBuffers = 2;
        static constexpr float WindowScaleWidth = 800.0f;
        static constexpr float WindowScaleHeight = 600.0f;
        AZStd::string m_name;
        AZStd::string m_curPath;

        AZ::Name m_dynamicDrawContextName = AZ::Name(AZ::AtomFontDynamicDrawContextName);

        FontTexture* m_fontTexture = nullptr;

        size_t m_fontBufferSize = 0;
        AZStd::unique_ptr<uint8_t[]> m_fontBuffer;

        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_fontAttachmentImage;
        AZ::RHI::Ptr<AZ::RHI::Image>     m_fontImage;
        uint32_t m_fontImageVersion = 0;

        AtomFont* m_atomFont = nullptr;

        bool m_fontTexDirty = false;

        FontEffects m_effects;

        // Atom data
        AZStd::mutex                        m_vertexDataMutex;
        SVF_P3F_C4B_T2F*                    m_vertexBuffer = nullptr;
        uint16_t                            m_vertexCount = 0;

        uint16_t*                           m_indexBuffer = nullptr;
        uint16_t                            m_indexCount = 0;

        FontShaderData                      m_fontShaderData;

        bool m_monospacedFont = false; //!< True if this font is fixed/monospaced, false otherwise (obtained from FreeType)

        float m_sizeRatio = IFFontConstants::defaultSizeRatio;
        SizeBehavior m_sizeBehavior = SizeBehavior::Scale;   //!< Changes how glyphs rendered at different sizes are rendered.
        FontHintParams m_fontHintParams; //!< How the font should be hinted when its loaded and rendered to the font texture

        static constexpr char LogName[] = "AtomFont::FFont";
    };

    inline float FFont::GetSizeRatio() const
    {
        return m_sizeRatio;
    }

    inline void FontDeleter::operator () (const AZStd::intrusive_refcount<AZStd::atomic_uint, FontDeleter>* ptr) const
    {
        /// Recover the mutable parent object pointer from the refcount base class.
        FFont* font = const_cast<FFont*>(static_cast<const FFont*>(ptr));
        if (font && font->m_atomFont)
        {
            font->m_atomFont->UnregisterFont(font->m_name.c_str());
            font->m_atomFont = nullptr;
        }

        delete font;
    }
}

#endif
