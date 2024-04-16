/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Color.h>
#include <LyShine/UiBase.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/IDraw2d.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTextInterface
    : public AZ::ComponentBus
{
public: // types

    //! Callback type for retrieving displayed text.
    typedef AZStd::function<AZStd::string(const AZStd::string&)> DisplayedTextFunction;

    //! Determines how text overflow should behave
    enum class OverflowMode
    {
        OverflowText,   //!< text contents aren't impacted by element size (and vice versa)
        ClipText,       //!< clips text contents to fit width of element
        Ellipsis        //!< truncates displayed text to fit element and appends ellipsis to the text
    };

    //! Provides values for determining whether text is wrapped or not
    enum class WrapTextSetting
    {
        NoWrap,
        Wrap
    };

    //! Controls whether text should shrink (scale down) to prevent overflowing.
    //!
    //! The amount of scale applied to text can be limited by minimum shrink scale.
    enum class ShrinkToFit
    {
        None,       //!< No shrinking is applied to text
        Uniform,    //!< Shrink text uniformly along X/Y dimension (maintains aspect ratio)
        WidthOnly   //!< Only scales text along X axis (width)
    };

    //! Determines what processing should be performed on text before returning
    enum GetTextFlags
    {
        GetAsIs         = 0,
        GetLocalized    = 1 << 0
    };

    //! Determines how text should be assigned
    enum SetTextFlags
    {
        SetAsIs         = 0,
        SetEscapeMarkup = 1 << 0,
        SetLocalized    = 1 << 1
    };

public: // member functions

    virtual ~UiTextInterface() {}

    //! Returns the unaltered contents of the string contained within the text component.
    //! \return Unaltered string contents of this text component
    virtual AZStd::string GetText() = 0;
    virtual void SetText(const AZStd::string& text) = 0;

    virtual AZStd::string GetTextWithFlags(GetTextFlags flags = GetAsIs) = 0;
    virtual void SetTextWithFlags(const AZStd::string& text, SetTextFlags flags = SetAsIs) = 0;

    virtual AZ::Color GetColor() = 0;
    virtual void SetColor(const AZ::Color& color) = 0;

    //! Returns font object used by the displayed text.
    virtual LyShine::PathnameType GetFont() = 0;
    virtual void SetFont(const LyShine::PathnameType& fontPath) = 0;

    virtual int GetFontEffect() = 0;
    virtual void SetFontEffect(int effectIndex) = 0;

    virtual AZStd::string GetFontEffectName(int effectIndex) = 0;
    virtual void SetFontEffectByName(const AZStd::string& effectName) = 0;

    virtual float GetFontSize() = 0;
    virtual void SetFontSize(float size) = 0;

    virtual void GetTextAlignment(IDraw2d::HAlign& horizontalAlignment,
        IDraw2d::VAlign& verticalAlignment) = 0;
    virtual void SetTextAlignment(IDraw2d::HAlign horizontalAlignment,
        IDraw2d::VAlign verticalAlignment) = 0;

    virtual IDraw2d::HAlign GetHorizontalTextAlignment() = 0;
    virtual void SetHorizontalTextAlignment(IDraw2d::HAlign alignment) = 0;
    virtual IDraw2d::VAlign GetVerticalTextAlignment() = 0;
    virtual void SetVerticalTextAlignment(IDraw2d::VAlign alignment) = 0;

    virtual float GetCharacterSpacing() = 0;
    virtual void SetCharacterSpacing(float characterSpacing) = 0;
    virtual float GetLineSpacing() = 0;
    virtual void SetLineSpacing(float lineSpacing) = 0;

    //! Given a point in viewport space, return the character index in the string
    //! \param point a point in viewport space
    //! \param mustBeInBoundingBox if true the given point be be contained in the bounding box of
    //!  actual text characters (not the element). If false it can be anywhere (even outside the element)
    //!  and is projected onto a text position (for drag select for example).
    //! \return -1 if second param is true and point is outside box
    //!  0 if to left of first char, 1 if between first and second char,
    //!  length of string if to right of last char
    virtual int GetCharIndexFromPoint(AZ::Vector2 point, bool mustBeInBoundingBox) = 0;

    //! Given a point in untransformed canvas space, return the character index in the string
    //! \param point a point in untransformed canvas space
    //! \param mustBeInBoundingBox if true the given point be be contained in the bounding box of
    //!  actual text characters (not the element). If false it can be anywhere (even outside the element)
    //!  and is projected onto a text position (for drag select for example).
    //! \return -1 if second param is true and point is outside box
    //!  0 if to left of first char, 1 if between first and second char,
    //!  length of string if to right of last char
    virtual int GetCharIndexFromCanvasSpacePoint(AZ::Vector2 point, bool mustBeInBoundingBox) = 0;

    //! Returns the XY coord of the rendered character position at a given index.
    //! Imagining a rect encompassing the character width and line height,
    //! the returned coordinate is the upper-left corner of the rect.
    //! \param index Index into displayed string.
    virtual AZ::Vector2 GetPointFromCharIndex(int index) = 0;

    virtual AZ::Color GetSelectionColor() = 0;
    virtual void GetSelectionRange(int& startIndex, int& endIndex) = 0;

    //! Set a range of the text string to be shown as selected
    //!
    //! If startIndex and endIndex are the same then a one pixel wide vertical bar is highlighted.
    //!
    //! The provided start and end indices are "character" indices into a UTF8 string. For
    //! example, an index of 1 could actually be mapped at buffer index 2 if the first character
    //! in the UTF8 string is a multi-byte character of size 2.
    //!
    //! \param startIndex 0 means starting at the left edge first character
    //! \param endIndex if equal to UTF8 text string length that means up to the right edge of the last char
    //! \param selectionColor the selection color (for box drawn behind text)
    virtual void SetSelectionRange(int startIndex, int endIndex, const AZ::Color& selectionColor) = 0;

    //! Clear any text selection range that has been applied to this text
    virtual void ClearSelectionRange() = 0;

    //! Get the width and height of the text
    virtual AZ::Vector2 GetTextSize() = 0;

    //! Get the width of the text
    virtual float GetTextWidth() = 0;

    //! Get the height of the text
    virtual float GetTextHeight() = 0;

    //! Get the bounding box (in viewport space, so it can be rotated) of the given text range
    //! If startIndex and endIndex are the same then a rect is still returned that is one pixel wide
    //! \param startIndex 0 means starting at the first character
    //! \param endIndex if equal to text string length that means including the last char
    //! \param rectPoints Output parameter that gets populated with up to three RectPoints for multi-line text selection geometry.
    virtual void GetTextBoundingBox(int startIndex, int endIndex, UiTransformInterface::RectPointsArray& rectPoints) = 0;

    //! Returns the function object used to manipulate a given string to condition it for rendering.
    //! The default displayed text function for a UiTextComponent is pass-through (the passed string
    //! is returned as-is).
    virtual DisplayedTextFunction GetDisplayedTextFunction() const = 0;

    //! Allows setting a DisplayedTextFunction object to be called prior to rendering.
    //! The string contents of a UiTextComponent can be modified for rendering without
    //! changing the actual contents of the text component (GetText()). This functionality
    //! is useful in some situations, like password hiding, where the displayed text should
    //! be different from the stored text.
    virtual void SetDisplayedTextFunction(const DisplayedTextFunction& displayedTextFunction) = 0;

    //! Gets the overflow behavior of this component
    virtual OverflowMode GetOverflowMode() = 0;

    //! Sets the overflow setting of this component.
    virtual void SetOverflowMode(OverflowMode overflowMode) = 0;

    //! Gets the text wrapping setting of this component
    virtual WrapTextSetting GetWrapText() = 0;

    //! Sets the text wrapping setting of this component
    virtual void SetWrapText(WrapTextSetting wrapSetting) = 0;

    //! Gets the "shrink-to-fit" value of this component
    virtual ShrinkToFit GetShrinkToFit() = 0;

    //! Sets the "shrink-to-fit" value of this component
    virtual void SetShrinkToFit(ShrinkToFit shrinkToFit) = 0;

    //! Typically triggered when input mechanism (keyboard vs. mouse) changes/alternates.
    virtual void ResetCursorLineHint() = 0;

    //! Get whether markup is enabled. If true then the text string is parsed for XML markup.
    virtual bool GetIsMarkupEnabled() = 0;

    //! Set whether markup is enabled. If true then the text string is parsed for XML markup.
    virtual void SetIsMarkupEnabled(bool isEnabled) = 0;

    //! Gets the minimum shrink scale when using ShrinkToFit.
    virtual float GetMinimumShrinkScale() = 0;

    //! Sets the minimum shrink scale when using ShrinkToFit.
    virtual void SetMinimumShrinkScale(float minShrinkScale) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiTextInterface> UiTextBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTextNotifications : public AZ::ComponentBus
{
public:
    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const bool EnableEventQueue = true;
    //////////////////////////////////////////////////////////////////////////

public: // member functions
    virtual ~UiTextNotifications()
    {
    }

    //! Notify listeners that the text has been changed
    virtual void OnLayoutInvalidated()
    {
    }
};
using UiTextNotificationsBus = AZ::EBus<UiTextNotifications>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface that describes "clickable" areas of text for a text component.
//!
//! Text components can contain anchor tags that allow user interaction with text.
//! These anchor tags define clickable areas that can be stylized (via color) and
//! provide information to listeners regarding the text.
class UiClickableTextInterface
    : public AZ::ComponentBus
{
public: // types

    //! Defines an area of interaction for clickable text, along with metadata.
    struct ClickableTextRect
    {
        UiTransformInterface::Rect rect; //!< "Click area" for text.

        AZStd::string action; //!< "action" string from markup.

        AZStd::string data; //!< "data" string from markup.

        int id; //!< Identifier that uniquely identifies clickable text from
                //!< a single anchor tag. The clickable text from markup can
                //!< be word-wrapped to subsequent lines, in which case the
                //!< identifier will identify all clickable text coming from
                //!< a single anchor tag. This allows word-wrapped clickable
                //!< text to be treated as a single clickable link, 
                //!< especially useful for hover/color styling.
    };

    using ClickableTextRects = AZStd::vector<ClickableTextRect>;

public: // member functions

    virtual ~UiClickableTextInterface() {}

    //! Populates given clickable text rects for a text component.
    virtual void GetClickableTextRects(UiClickableTextInterface::ClickableTextRects& clickableTextRects) = 0;

    //! Sets the drawbatch color for clickable text, given by the ID.
    virtual void SetClickableTextColor(int id, const AZ::Color& color) = 0;

public: // static member data

    //! Only one component on an entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiClickableTextInterface> UiClickableTextBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiClickableTextNotifications
    : public AZ::ComponentBus
{
public:

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const bool EnableEventQueue = true;
    //////////////////////////////////////////////////////////////////////////

public: // member functions

    virtual ~UiClickableTextNotifications() {}

    //! Notify listeners that the text has been prepared for render.
    virtual void OnClickableTextChanged() {}
};

typedef AZ::EBus<UiClickableTextNotifications> UiClickableTextNotificationsBus;

