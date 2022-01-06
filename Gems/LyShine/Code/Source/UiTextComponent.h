/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <string>
#include <LyShine/IDraw2d.h>
#include <LyShine/UiBase.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiLayoutCellDefaultBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiAnimateEntityBus.h>
#include <LyShine/UiAssetTypes.h>
#include <LyShine/UiRenderFormats.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/set.h>

#include <Atom/RPI.Reflect/Image/Image.h>

// Only needed for internal unit-testing
#include <LyShine.h>

#include <IFont.h>
#include <ILocalizationManager.h>
#include <TextureAtlas/TextureAtlasBus.h>
#include <TextureAtlas/TextureAtlasNotificationBus.h>
#include <TextureAtlas/TextureAtlas.h>

// Forward declaractions
namespace TextMarkup
{
    struct Tag;

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    void UnitTest();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTextComponent
    : public AZ::Component
    , public UiVisualBus::Handler
    , public UiRenderBus::Handler
    , public UiTextBus::Handler
    , public UiClickableTextBus::Handler
    , public UiAnimateEntityBus::Handler
    , public UiTransformChangeNotificationBus::Handler
    , public UiLayoutCellDefaultBus::Handler
    , public FontNotificationBus::Handler
    , public LanguageChangeNotificationBus::Handler
    , public UiCanvasPixelAlignmentNotificationBus::Handler
    , public TextureAtlasNamespace::TextureAtlasNotificationBus::Handler
{
public: //types

    using FontEffectComboBoxVec = AZStd::vector < AZStd::pair<unsigned int, AZStd::string> >;

    //! An inline image to be displayed within the text
    struct InlineImage
    {
        enum VAlign
        {
            Baseline,
            Top,
            Center,
            Bottom
        };

        InlineImage(const AZStd::string& texturePathname,
            float height,
            float scale,
            VAlign vAlign,
            float yOffset,
            float leftPadding,
            float rightPadding);
        ~InlineImage();

        bool OnAtlasLoaded(const TextureAtlasNamespace::TextureAtlas* atlas);
        bool OnAtlasUnloaded(const TextureAtlasNamespace::TextureAtlas* atlas);

        AZ::Data::Instance<AZ::RPI::Image> m_texture;
        AZ::Vector2 m_size;
        VAlign m_vAlign;
        float m_yOffset;
        float m_leftPadding;
        float m_rightPadding;
        AZStd::string m_filepath;
        const TextureAtlasNamespace::TextureAtlas* m_atlas;
        TextureAtlasNamespace::AtlasCoordinates m_coordinates;
    };

    using InlineImageContainer = AZStd::list < InlineImage* >;

    //! Atomic unit of font "state" for drawing text in the renderer.
    //! A single line of text can be divided amongst multiple draw batches,
    //! allowing that line of text to be rendered with different font
    //! stylings, which is used to support FontFamily rendering.
    struct DrawBatch
    {
        enum Type
        {
            Text,
            Image
        };

        //! Overflow information based on available width. Used for wrapping
        struct OverflowInfo
        {
            int overflowIndex;                      //!< the index of the character that overflowed
            bool overflowCharIsSpace;               //!< indicates whether the character that overflowed is a space
            float widthUntilOverflowOrTotalWidth;   //!< the width of the batch up until the overflow, or the total width if no overflow
            float overflowCharWidth;                //!< the width of the overflow character
            int lastSpaceIndex;                     //!< the index of the last space character that hasn't overflowed
            bool isSpaceAtEnd;                      //!< indicates whether the space character is the last character to not overflow
        };

        DrawBatch();

        Type GetType() const;

        //! Calculate and store the size of the batch content
        void CalculateSize(const STextDrawContext& ctx, bool excludeTrailingSpace);
        //! Calculate and store the y offset of the batch from the text y position
        void CalculateYOffset(float fontSize, float baseline);
        //! Get the number of characters that the batch contains. An image is considered to be one character
        int GetNumChars() const;
        //! Get overflow information based on the available width. Used for wrapping
        bool GetOverflowInfo(const STextDrawContext& ctx,
            float availableWidth, bool skipFirstChar, OverflowInfo& overflowInfoOut);
        //! Split the batch at a specified character index
        void Split(int atCharIndex, DrawBatch& newDrawBatchOut);
        bool IsClickable() const { return !action.empty() || !data.empty(); }

        AZ::Vector3 color;

        AZStd::string text;

        AZStd::string action; //!< Only used for clickable text. Parsed from "action" attribute in anchor tag (markup).

        AZStd::string data; //!< Only used for clickable text. Parsed from "data" attribute in anchor tag (markup).

        IFFont* font = nullptr;

        InlineImage* image = nullptr;

        AZ::Vector2 size; //!< The size in pixels of the batch content

        float yOffset; //!< While calculating, the yOffset is set to the offset from the text draw y position.
                       //!< Once all batches in the line are calculated, the yOffset will become the offset
                       //!< from the y draw position of the batch line

        int clickableId = -1; //!< Only used for clickable text. Each parse anchor tag gets assigned
                              //!< a unique ID that's shared amongst all draw batches that belong to
                              //!< the anchor.
    };

    using DrawBatchContainer = AZStd::list < DrawBatch >;

    //! A single line of text that can be composed of multiple DrawBatch objects
    struct DrawBatchLine
    {
        DrawBatchLine();

        //! Check whether the line is overflowing and split it into two lines if it is overflowing
        bool CheckAndSplitLine(const STextDrawContext& ctx,
            float maxWidth,
            DrawBatchLine& newDrawBatchLineOut);

        DrawBatchContainer drawBatchList;   //!< DrawBatches that the line is composed of
        AZ::Vector2 lineSize;                      //!< Pixel size of entire line of text
    };

    using DrawBatchLineContainer = AZStd::list < DrawBatchLine >;
    using FontFamilyRefSet = AZStd::set < FontFamilyPtr >;

    //! A collection of batch lines used for multi-line rendering of DrawBatch objects.
    //! A single line of text contains a list of batches, and multi-line rendering requires
    //! a list of multiple lines of draw batches.
    //!
    //! Since different Font Familys can be referenced batch-to-batch, we hold a strong
    //! reference (shared_ptr) for each Font Family that's referenced. Once this struct
    //! goes out of scope, or is cleared, the references are freed.
    struct DrawBatchLines
    {
        ~DrawBatchLines();

        //! Clears the batch lines list and releases any Font Family references.
        void Clear();

        DrawBatchLineContainer batchLines;  //!< List of batch lines for drawing, each implicitly separated by a newline.
        FontFamilyRefSet fontFamilyRefs;    //!< Set of strongly referenced Font Family objects used by draw batches.
        InlineImageContainer inlineImages;  //!< List of images used by draw batches.
        float height;                       //!< The accumulated height of all the batch lines.
        float baseline;                     //!< The baseline to use when aligning images. Offset from the y draw position of the text.
        AZ::Vector2 fontSizeScale = AZ::Vector2(1.0f, 1.0f);  //!< A scale that gets applied to the font size when using shrink-to-fit.
        bool m_fontEffectHasTransparency = false; //! True if any of the font effects used in the draw batch lines have an alpha less than 1.
    };

    //! Simple container for left/right AZ::Vector2 offsets.
    struct LineOffsets
    {
        LineOffsets()
            : left(AZ::Vector2::CreateZero())
            , right(AZ::Vector2::CreateZero())
            , batchLineLength(0.0f) {}

        AZ::Vector2 left;
        AZ::Vector2 right;
        float batchLineLength;
    };

public: // member functions

    AZ_COMPONENT(UiTextComponent, LyShine::UiTextComponentUuid, AZ::Component);

    UiTextComponent();
    ~UiTextComponent() override;

    // UiVisualInterface
    void ResetOverrides() override;
    void SetOverrideColor(const AZ::Color& color) override;
    void SetOverrideAlpha(float alpha) override;
    void SetOverrideFont(FontFamilyPtr fontFamily) override;
    void SetOverrideFontEffect(unsigned int fontEffectIndex) override;
    // ~UiVisualInterface

    // UiRenderInterface
    void Render(LyShine::IRenderGraph* renderGraph) override;
    // ~UiRenderInterface

    // UiTextInterface
    AZStd::string GetText() override;
    void SetText(const AZStd::string& text) override;
    AZStd::string GetTextWithFlags(GetTextFlags flags = GetAsIs) override;
    void SetTextWithFlags(const AZStd::string& text, SetTextFlags flags = SetAsIs) override;
    AZ::Color GetColor() override;
    void SetColor(const AZ::Color& color) override;
    LyShine::PathnameType GetFont() override;
    void SetFont(const LyShine::PathnameType& fontPath) override;
    int GetFontEffect() override;
    void SetFontEffect(int effectIndex) override;
    AZStd::string GetFontEffectName(int effectIndex) override;
    void SetFontEffectByName(const AZStd::string& effectName) override;
    float GetFontSize() override;
    void SetFontSize(float size) override;
    void GetTextAlignment(IDraw2d::HAlign& horizontalAlignment,
        IDraw2d::VAlign& verticalAlignment) override;
    void SetTextAlignment(IDraw2d::HAlign horizontalAlignment,
        IDraw2d::VAlign verticalAlignment) override;
    IDraw2d::HAlign GetHorizontalTextAlignment() override;
    void SetHorizontalTextAlignment(IDraw2d::HAlign alignment) override;
    IDraw2d::VAlign GetVerticalTextAlignment() override;
    void SetVerticalTextAlignment(IDraw2d::VAlign alignment) override;
    float GetCharacterSpacing() override;
    //! Expects 1/1000th ems, where 1 em = font size. This will also affect text size, which can lead to
    //! formatting changes (with word-wrap enabled for instance).
    void SetCharacterSpacing(float characterSpacing) override;
    float GetLineSpacing() override;
    //! Expects pixels.
    void SetLineSpacing(float lineSpacing) override;
    int GetCharIndexFromPoint(AZ::Vector2 point, bool mustBeInBoundingBox) override;
    int GetCharIndexFromCanvasSpacePoint(AZ::Vector2 point, bool mustBeInBoundingBox) override;
    AZ::Vector2 GetPointFromCharIndex(int index) override;
    AZ::Color GetSelectionColor() override;
    void GetSelectionRange(int& startIndex, int& endIndex) override;
    void SetSelectionRange(int startIndex, int endIndex, const AZ::Color& selectionColor) override;
    void ClearSelectionRange() override;
    AZ::Vector2 GetTextSize() override;
    float GetTextWidth() override;
    float GetTextHeight() override;
    void GetTextBoundingBox(int startIndex, int endIndex, UiTransformInterface::RectPointsArray& rectPoints) override;
    DisplayedTextFunction GetDisplayedTextFunction() const override { return m_displayedTextFunction; }
    void SetDisplayedTextFunction(const DisplayedTextFunction& displayedTextFunction) override;
    OverflowMode GetOverflowMode() override;
    void SetOverflowMode(OverflowMode overflowMode) override;
    WrapTextSetting GetWrapText() override;
    void SetWrapText(WrapTextSetting wrapSetting) override;
    ShrinkToFit GetShrinkToFit() override;
    void SetShrinkToFit(ShrinkToFit shrinkToFit) override;
    void ResetCursorLineHint() override { m_cursorLineNumHint = -1; }
    bool GetIsMarkupEnabled() override;
    void SetIsMarkupEnabled(bool isEnabled) override;
    float GetMinimumShrinkScale() override;
    void SetMinimumShrinkScale(float minShrinkScale) override;
    // ~UiTextInterface

    // UiClickableTextInterface
    void GetClickableTextRects(UiClickableTextInterface::ClickableTextRects& clickableTextRects) override;
    void SetClickableTextColor(int id, const AZ::Color& color) override;
    // ~UiClickableTextInterface

    // UiAnimateEntityInterface
    void PropertyValuesChanged() override;
    // ~UiAnimateEntityInterface

    // UiTransformChangeNotificationBus
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    void OnTransformToViewportChanged() override;
    // ~UiTransformChangeNotificationBus

    // UiLayoutCellDefaultInterface
    float GetMinWidth() override;
    float GetMinHeight() override;
    float GetTargetWidth(float maxWidth) override;
    float GetTargetHeight(float maxHeight) override;
    float GetExtraWidthRatio() override;
    float GetExtraHeightRatio() override;
    // ~UiLayoutCellDefaultInterface

    // FontNotifications
    void OnFontsReloaded() override;
    // ~FontNotifications

    // LanguageChangeNotification
    void LanguageChanged() override;
    // ~LanguageChangeNotification

    // UiCanvasTextPixelAlignmentNotification
    void OnCanvasTextPixelAlignmentChange() override;
    // ~UiCanvasTextPixelAlignmentNotification

    // TextureAtlasNotifications
    void OnAtlasLoaded(const TextureAtlasNamespace::TextureAtlas* atlas) override;
    void OnAtlasUnloaded(const TextureAtlasNamespace::TextureAtlas* atlas) override;
    // ~TextureAtlasNotifications

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    static void UnitTest(CLyShine* lyshine, IConsoleCmdArgs* cmdArgs);
    static void UnitTestLocalization(CLyShine* lyshine, IConsoleCmdArgs* cmdArgs);
#endif

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
        provided.push_back(AZ_CRC("UiTextService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
        incompatible.push_back(AZ_CRC("UiTextService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    //! Called when we know the font needs to be changed
    void ChangeFont(const AZStd::string& fontFileName);

    //! Implementation of getting bounding box for the given displayed text
    void GetTextBoundingBoxPrivate(const DrawBatchLines& drawBatchLines, int startIndex, int endIndex, UiTransformInterface::RectPointsArray& rectPoints);

    //! Get the bounding rectangle of the text, in untransformed canvas space
    void GetTextRect(UiTransformInterface::RectPoints& rect);

    //! Similar to GetTextRect, but allows getting a rect for only a portion of text (via textSize).
    //!
    //! This method is particularly useful for multi-line text, where text selection can
    //! vary line-by-line, or across multiple lines of text, in which case you only want
    //! rects for a portion of the displayed text, rather than all of it (which GetTextRect
    //! does).
    void GetTextRect(UiTransformInterface::RectPoints& rect, const AZ::Vector2& textSize);

    //! ChangeNotify callback for text string change
    void OnTextChange();

    //! ChangeNotify callback for color change
    void OnColorChange();

    //! ChangeNotify callback for alignment change
    void OnAlignmentChange();

    //! ChangeNotify callback for overflow settings change
    void OnOverflowChange();

    //! ChangeNotify callback for font size change
    void OnFontSizeChange();

    //! ChangeNotify callback for font pathname change
    AZ::u32 OnFontPathnameChange();

    //! ChangeNotify callback for font effect change
    void OnFontEffectChange();

    //! ChangeNotify callback for text wrap setting change
    void OnWrapTextSettingChange();

    //! ChangeNotify callback for shrink-to-fit setting change
    void OnShrinkToFitChange();

    //! ChangeNotify callback for "minimum shrink scale" setting change
    void OnMinShrinkScaleChange();

    //! ChangeNotify callback for char spacing change
    void OnCharSpacingChange();

    //! ChangeNotify callback for line spacing change
    void OnLineSpacingChange();

    //! ChangeNotify callback for markup enabled change
    void OnMarkupEnabledChange();

    //! Populate the list for the font effect combo box in the properties pane
    FontEffectComboBoxVec PopulateFontEffectList();

    //! Returns the amount of pixels the displayed text is adjusted for clipping.
    //!
    //! Returns zero if text is not large enough to be clipped or clipping
    //! isn't enabled.
    //!
    //! \note This does not simply return m_clipoffset. This method calculates
    //! and assigns new values to m_clipOffset and m_clipOffsetMultiplier and
    //! returns their product.
    float CalculateHorizontalClipOffset();

    //! Mark draw batches dirty
    void MarkDrawBatchLinesDirty(bool invalidateLayout);

    //! Calculate the DrawBatchLines if needed and return a const ref
    const DrawBatchLines& GetDrawBatchLines();

    //! Calculates draw batch lines
    void CalculateDrawBatchLines(
        UiTextComponent::DrawBatchLines& drawBatchLinesOut,
        bool forceNoWrap = false,
        float availableWidth = -1.0f,
        bool excludeTrailingSpaceWidth = true
        );

    //! Renders the text to the render cache
    void RenderToCache(float alpha);

    //! Add DrawBatch lines to the render graph for rendering
    void RenderDrawBatchLines(
        const UiTextComponent::DrawBatchLines& drawBatchLines,
        const AZ::Vector2& pos,
        const UiTransformInterface::RectPoints& points,
        const AZ::Matrix4x4& transformToViewport,
        STextDrawContext& fontContext);

    //! Update the text render batches in the case of a font texture change
    void UpdateTextRenderBatchesForFontTextureChange();

    //! Returns a prototypical STextDrawContext to be used when interacting with IFont routines..
    STextDrawContext GetTextDrawContextPrototype(int requestFontSize, const AZ::Vector2& fontSizeScale) const;

    //! Recomputes draw batch lines as appropriate depending on current options when text width properties are modified
    void OnTextWidthPropertyChanged();

    //! Handles overflow and shrink-to-text settings to text
    void HandleOverflowText(UiTextComponent::DrawBatchLines& drawBatchLinesOut);

    //! Handles shrink-to-fit for text, if applicable.
    void HandleShrinkToFit(UiTextComponent::DrawBatchLines& drawBatchLinesOut, float availableHeight = -1.0f);

    //! Handles the "uniform" shrink-to-fit setting.
    void HandleUniformShrinkToFitWithScale(UiTextComponent::DrawBatchLines& drawBatchLinesOut, const AZ::Vector2& scaleVec);

    //! Handles the shrink-to-fit setting for word-wrapped text.
    void HandleShrinkToFitWithWrapping(UiTextComponent::DrawBatchLines& drawBatchLinesOut, const AZ::Vector2& currentElementSize, const AZ::Vector2& textSize);

    //! Handles "width only" word-wrapped shrink-to-fit text.
    void HandleWidthOnlyShrinkToFitWithWrapping(UiTextComponent::DrawBatchLines& drawBatchLinesOut, const AZ::Vector2& currentElementSize, int maxLinesElementCanHold);

    //! Handles "uniform" word-wrapped shrink-to-fit text.
    void HandleUniformShrinkToFitWithWrapping(UiTextComponent::DrawBatchLines& drawBatchLinesOut, const AZ::Vector2& currentElementSize, int maxLinesElementCanHold);

    //! Inserts ellipsis into overflowing text
    void HandleEllipsis(UiTextComponent::DrawBatchLines& drawBatchLinesOut, float availableHeight = -1.0f);

    using DrawBatchLineIters = AZStd::vector<DrawBatchLineContainer::iterator>;

    //! Returns the draw batch line to ellipsis and the following lines to truncate (if any).
    void GetLineToEllipsisAndLinesToTruncate(UiTextComponent::DrawBatchLines& drawBatchLinesOut,
        DrawBatchLineContainer::iterator* lineToEllipsis, DrawBatchLineIters& linesToRemove, const AZ::Vector2& currentElementSize);

    using DrawBatchStartPosPair = AZStd::pair<DrawBatch*, float>;
    using DrawBatchStartPositions = AZStd::vector<DrawBatchStartPosPair>;

    //! Returns the "starting" pixel position for each batch on the given line
    void GetDrawBatchStartPositions(DrawBatchStartPositions& startPositions, DrawBatchLine* lineToEllipsis, const AZ::Vector2& currentElementSize);

    //! Returns the draw batch that will have ellipsis inserted, along with required position information to do so.
    DrawBatch* GetDrawBatchToEllipseAndPositions(const char* ellipseText,
        const STextDrawContext& ctx,
        const AZ::Vector2& currentElementSize,
        DrawBatchStartPositions* startPositions,
        float* drawBatchStartPos,
        float* ellipsisPos);

    //! Removes all draw batches following the given DrawBatch on the given DrawBatchLine.
    void TruncateDrawBatches(DrawBatchLine* lineToTruncate, const DrawBatch* truncateAfterBatch);

    //! Given a draw batch, get the character index where ellipsis should be inserted in the string.
    int GetStartEllipseIndexInDrawBatch(const DrawBatch* drawBatchToEllipse,
        const STextDrawContext& ctx,
        const float drawBatchStartPos,
        const float ellipsePos);

    //! Inserts the ellipse text into the given draw batch and updates batch and line sizing information
    void InsertEllipsisText(const char* ellipseText,
        const int ellipsisCharPos,
        DrawBatch* drawBatchToEllipse);

    //! Ensures that all draw batches on the given batch line have valid font pointers
    //!
    //! This is primarily used for ellipsis overflow handling, making it easier to make
    //! assumptions about which font to use when inserting ellipsis text for a given
    //! batch (when that batch is an image batch).
    void SetBatchLineFontPointers(DrawBatchLine* batchLine);

    //! Returns true if the given text rect overflows the given element size, false otherwise
    bool GetTextOverflowsBounds(const AZ::Vector2& textSize, const AZ::Vector2& elementSize) const;

    //! Compute the text size from the already computed draw batch lines
    AZ::Vector2 GetTextSizeFromDrawBatchLines(const UiTextComponent::DrawBatchLines& drawBatchLines) const;

    //! Localize the given text string
    AZStd::string GetLocalizedText(const AZStd::string& text);

    //! Given rect points and number of lines of text to display, returns the position to display text.
    //!
    //! The number of lines of text determines the Y offset of the first line to display. For
    //! top-aligned text, this offset will be zero (regardless of the number of lines of text)
    //! because the first line to display will always be displayed at the top of the rect, while
    //! bottom-aligned text will be offset by the number of lines to display, and vertically
    //! centered text will be offset by half of that amount.
    //!
    //! Example: if horizontal alignment is "left" and vertical alignment is
    //! "top", this will simply return the top-left point of the rect.
    //!
    //! This assumes the given rect points are axis-aligned.
    AZ::Vector2 CalculateAlignedPositionWithYOffset(const UiTransformInterface::RectPoints& points);

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

    AZ_DISABLE_COPY_MOVE(UiTextComponent);

    //! Calculates the left and right offsets for cursor placement and text selection bounds.
    void GetOffsetsFromSelectionInternal(LineOffsets& top, LineOffsets& middle, LineOffsets& bottom, int selectionStart, int selectionEnd);

private: // member functions

    //! Given an index into the displayed string, returns the line number that the character is displayed on.
    int GetLineNumberFromCharIndex(const DrawBatchLines& drawBatchLines, const int soughtIndex) const;

    //! Invalidates the parent and this element's layout
    void InvalidateLayout() const;

    //! Refresh the transform properties in the editor's properties pane
    void CheckLayoutFitterAndRefreshEditorTransformProperties() const;

    //! Mark the render cache as dirty, this should be done when any change is made that invalidated the cached data
    void MarkRenderCacheDirty();

    //! Mark the render graph as dirty, this should be done when any change is made affects the structure of the graph
    void MarkRenderGraphDirty();

    //! Clear the render cache
    void ClearRenderCache();

    //! Clear the render cache memory allocations
    void FreeRenderCacheMemory();

    //! Checks if clipping is enabled for handling overflow, or if specific conditions are met when using ellipsis.
    //!
    //! When ellipsis overflow handling is enabled, content will become clipped when the text
    //! overflows vertically and only one line is displayed.
    bool ShouldClip();

    //! Calculate m_requestFontSize if needed then return it
    int GetRequestFontSize();

private: // types

    struct RenderCacheBatch
    {
        AZ::Vector2         m_position;
        AZStd::string       m_text;
        ColorB              m_color;
        IFFont*             m_font;
        uint32              m_fontTextureVersion;
        LyShine::UiPrimitive      m_cachedPrimitive;
    };

    struct RenderCacheImageBatch
    {
        AZ::Data::Instance<AZ::RPI::Image>  m_texture;
        LyShine::UiPrimitive             m_cachedPrimitive;
    };

    struct RenderCacheData
    {
        bool                                    m_isDirty = true;
        STextDrawContext                        m_fontContext;
        AZStd::vector<RenderCacheBatch*>        m_batches;
        AZStd::vector<RenderCacheImageBatch*>   m_imageBatches;
    };

private: // data

    AZStd::string m_text;
    AZStd::string m_locText;                        //!< Language-specific localized text (if applicable), keyed by m_text. May contain word-wrap formatting (if enabled).

    DrawBatchLines m_drawBatchLines;                //!< Lists of DrawBatches across multiple lines for rendering text.

    AZ::Color m_color;
    float m_alpha;
    float m_fontSize;
    int m_requestFontSize;                                  //!< The size to request glyphs to be rendered at within the font texture
    IDraw2d::HAlign m_textHAlignment;
    IDraw2d::VAlign m_textVAlignment;
    float m_charSpacing;                            //!< The spacing (aka "tracking") between characters, defined in 1/1000th ems. 1em is equal to the
                                                    //!< font size. In GetTextDrawContextPrototype, this value ultimately gets converted to pixels and
                                                    //!< stored in STextDrawContext::m_tracking. This value and STextDrawContext::m_tracking aren't
                                                    //!< necessarily 1:1, just as m_fontSize and STextDrawContext::m_size aren't necessarily 1:1.
                                                    //!< Although the component values of m_charSpacing and m_fontSize are unaffected by scaling,
                                                    //!< scaling (such as scaling performed by shrink-to-fit overflow handling) is applied to these
                                                    //!< values and the resulting scaled value is stored in STextDrawContext for rendering. As a result,
                                                    //!< it's possible for the value of m_charSpacing to never change, but STextDrawContext::m_tracking
                                                    //!< can vary in value independently of m_charSpacing as the font size (and/or scaled font size)
                                                    //!< changes over time. See also DrawBatchLines::fontSizeScale.
    float m_lineSpacing;

    float m_currFontSize;                           //!< Needed for PropertyValuesChanged method, used for UI animation
    float m_currCharSpacing;                        //!< Needed for PropertyValuesChanged method, used for UI animation

    AzFramework::SimpleAssetReference<LyShine::FontAsset> m_fontFilename;
    IFFont* m_font;
    FontFamilyPtr m_fontFamily;
    unsigned int m_fontEffectIndex;
    DisplayedTextFunction m_displayedTextFunction;  //!< Function object that returns a string to be used for rendering/display.

    AZ::Color m_overrideColor;
    float m_overrideAlpha;
    FontFamilyPtr m_overrideFontFamily;
    unsigned int m_overrideFontEffectIndex;

    bool m_isColorOverridden;
    bool m_isAlphaOverridden;
    bool m_isFontFamilyOverridden;
    bool m_isFontEffectOverridden;

    AZ::Color m_textSelectionColor;                 //!< color for a selection box drawn as background for a range of text

    int m_selectionStart;                           //!< UTF8 character/element index in the displayed string. This index
                                                    //! marks the beggining of a text selection, such as when this component
                                                    //! is associated with a text input component. If the displayed string
                                                    //! contains UTF8 multi-byte characters, then this index will not
                                                    //!< match 1:1 with an index into the raw string buffer.

    int m_selectionEnd;                             //!< UTF8 character/element index in the displayed string. This index
                                                    //! marks the end of a text selection, such as when this component
                                                    //! is associated with a text input component. If the displayed string
                                                    //! contains UTF8 multi-byte characters, then this index will not
                                                    //!< match 1:1 with an index into the raw string buffer.

    int m_cursorLineNumHint;
    OverflowMode m_overflowMode;                    //!< How text should "fit" within the element
    WrapTextSetting m_wrapTextSetting;              //!< Drives text-wrap setting
    ShrinkToFit m_shrinkToFit = ShrinkToFit::None;  //!< Whether text should shrink to fit element bounds when it overflows
    float m_minShrinkScale = 0.0f;                  //!< Limits the scale applied to text when text overflows and ShrinkToFit is used.
    float m_clipOffset;                             //!< Amount of pixels to adjust text draw call to account for clipping rect
    float m_clipOffsetMultiplier;                   //!< Used to adjust clip offset based on horizontal alignment settings

    bool m_isMarkupEnabled;                         //!< Enables markup in the text string. If false string will not be XML parsed

    RenderCacheData m_renderCache;                  //! Cached render data used to optimize rendering when nothing is changing frame to frame

    bool m_areDrawBatchLinesDirty = true;           //!< Indicates whether m_drawBatchLines needs regenerating before next use
    bool m_isRequestFontSizeDirty = true;           //!< Indicates whether m_requestFontSize needs calculating before next use

    bool m_textNeedsXmlValidation = true;           //!< Indicates whether any XML parsing warnings should be displayed when next parsed
};
