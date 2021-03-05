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
#pragma once

#include <LyShine/ILyShine.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

// Forward declarations
struct IFFont;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Class for 2D drawing in screen space
//
//! The IDraw2d interface allows drawing images and text in 2D.
//! Positions and sizes are specified in pixels in the current 2D viewport.
//! The BeginDraw2d method should be called before calling the Draw methods to enter 2D mode
//! and the EndDraw2d method should be called after calling the Draw methods to exit 2D mode.
//! There is a helper class Draw2dHelper that encapsulates this in its constructor and destructor.
class IDraw2d
{
public: // types

    //! Horizontal alignment can be used for both text and image drawing
    enum class HAlign
    {
        Left,
        Center,
        Right,
    };

    //! Vertical alignment can be used for both text and image drawing
    enum class VAlign
    {
        Top,
        Center,
        Bottom,
    };

    //! Used for specifying how to round positions to an exact pixel position for pixel-perfect rendering
    enum class Rounding
    {
        None,
        Nearest,
        Down,
        Up
    };

    enum
    {
        //! Limit imposed by FFont. This is the max number of characters including the null terminator.
        MAX_TEXT_STRING_LENGTH = 1024,
    };

    enum : int
    {
        //! Constant that indicates the built-in default value should be used
        UseDefault = -1
    };

    //! Struct used to pass additional image options.
    //
    //! If this is not passed then the defaults below are used
    struct ImageOptions
    {
        int         blendMode;      //!< default is GS_BLSRC_SRCALPHA|GS_BLDST_ONEMINUSSRCALPHA
        AZ::Vector3 color;          //!< default is (1,1,1)
        Rounding    pixelRounding;  //!< default is Rounding::Nearest
        int         baseState;      //!< Additional flags for SetState. Default is GS_NODEPTHTEST
    };

    //! Struct used to pass additional text options - mostly ones that do not change from call to call.
    //
    //! If this is not passed then the defaults below are used
    struct TextOptions
    {
        IFFont*         font;                   //!< default is "default"
        unsigned int    effectIndex;            //!< default is 0
        AZ::Vector3     color;                  //!< default is (1,1,1)
        HAlign          horizontalAlignment;    //!< default is HAlign::Left
        VAlign          verticalAlignment;      //!< default is VAlign::Top
        AZ::Vector2     dropShadowOffset;       //!< default is (0,0), zero offset means no drop shadow is drawn
        AZ::Color       dropShadowColor;        //!< default is (0,0,0,0), zero alpha means no drop shadow is drawn
        float           rotation;               //!< default is 0
        int             baseState;              //!< Additional flags for SetState. Default is GS_NODEPTHTEST
    };

    //! Used to pass in arrays of vertices (e.g. to DrawQuad)
    struct VertexPosColUV
    {
        VertexPosColUV(){}
        VertexPosColUV(const AZ::Vector2& inPos, const AZ::Color& inColor, const AZ::Vector2& inUV)
        {
            position = inPos;
            color = inColor;
            uv = inUV;
        }

        AZ::Vector2         position;   //!< 2D position of vertex
        AZ::Color           color;      //!< Float color
        AZ::Vector2         uv;         //!< Texture coordinate
    };

public: // member functions

    //! Implement virtual destructor just for safety.
    virtual ~IDraw2d() {}

    //! Start a section of 2D drawing function calls. This will set appropriate render state.
    //
    //! \param deferCalls   If true then actual render calls are deferred until the end of the frame
    virtual void BeginDraw2d(bool deferCalls = false) = 0;

    //! Start a section of 2D drawing function calls. This will set appropriate render state.
    //! This variant allows the viewport size to be specified
    //
    //! \param viewportSize The size of the viewport being rendered to
    //! \param deferCalls   If true then actual render calls are deferred until the end of the frame
    virtual void BeginDraw2d(AZ::Vector2 viewportSize, bool deferCalls = false) = 0;

    //! End a section of 2D drawing function calls. This will reset some render state.
    virtual void EndDraw2d() = 0;

    //! Draw a textured quad with the top left corner at the given position.
    //
    //! The image is drawn with the color specified by SetShapeColor and the opacity
    //! passed as an argument.
    //! If rotation is non-zero then the quad is rotated. If the pivot point is
    //! provided then the points of the quad are rotated about that point, otherwise
    //! they are rotated about the top left corner of the quad.
    //! \param texId    The texture ID returned by ITexture::GetTextureID()
    //! \param position Position of the top left corner of the quad (before rotation) in pixels
    //! \param size     The width and height of the quad. Use texture width and height to avoid minification,
    //!                 magnification or stretching (assuming the minMaxTexCoords are left to the default)
    //! \param opacity  The alpha value used when blending
    //! \param rotation Angle of rotation in degrees counter-clockwise
    //! \param pivotPoint       The point about which the quad is rotated
    //! \param minMaxTexCoords  An optional two component array. The first component is the UV coord for the top left
    //!                         point of the quad and the second is the UV coord of the bottom right point of the quad
    //! \param imageOptions     Optional struct specifying options that tend to be the same from call to call
    virtual void DrawImage(int texId, AZ::Vector2 position, AZ::Vector2 size, float opacity = 1.0f,
        float rotation = 0.0f, const AZ::Vector2* pivotPoint = nullptr, const AZ::Vector2* minMaxTexCoords = nullptr,
        ImageOptions* imageOptions = nullptr) = 0;

    //! Draw a textured quad where the position specifies the point specified by the alignment.
    //
    //! Rotation is always around the position.
    //! \param texId    The texture ID returned by ITexture::GetTextureID()
    //! \param position Position align point of the quad (before rotation) in pixels
    //! \param size     The width and height of the quad. Use texture width and height to avoid minification,
    //!                 magnification or stretching (assuming the minMaxTexCoords are left to the default)
    //! \param horizontalAlignment  Specifies how the quad is horizontally aligned to the given position
    //! \param verticalAlignment    Specifies how the quad is vertically aligned to the given position
    //! \param opacity  The alpha value used when blending
    //! \param rotation Angle of rotation in degrees counter-clockwise
    //! \param minMaxTexCoords  An optional two component array. The first component is the UV coord for the top left
    //!                         point of the quad and the second is the UV coord of the bottom right point of the quad
    //! \param imageOptions     Optional struct specifying options that tend to be the same from call to call
    virtual void DrawImageAligned(int texId, AZ::Vector2 position, AZ::Vector2 size,
        HAlign horizontalAlignment, VAlign verticalAlignment,
        float opacity = 1.0f, float rotation = 0.0f, const AZ::Vector2* minMaxTexCoords = nullptr,
        ImageOptions* imageOptions = nullptr) = 0;

    //! Draw a textured quad where the position, color and uv of each point is specified explicitly
    //
    //! \param texId        The texture ID returned by ITexture::GetTextureID()
    //! \param verts        An array of 4 vertices, in clockwise order (e.g. top left, top right, bottom right, bottom left)
    //! \param blendMode    UseDefault means default blend mode (currently GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA)
    //! \param pixelRounding Whether and how to round pixel coordinates
    //! \param baseState     Additional render state to pass to or into value passed to renderer SetState
    virtual void DrawQuad(int texId, VertexPosColUV* verts,
        int blendMode = UseDefault,
        Rounding pixelRounding = Rounding::Nearest,
        int baseState = UseDefault) = 0;

    //! Draw a line
    //
    //! \param start        The start position
    //! \param end          The end position
    //! \param color        The color of the line
    //! \param blendMode    UseDefault means default blend mode (currently GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA)
    //! \param pixelRounding Whether and how to round pixel coordinates
    //! \param baseState     Additional render state to pass to or into value passed to renderer SetState
    virtual void DrawLine(AZ::Vector2 start, AZ::Vector2 end, AZ::Color color,
        int blendMode = UseDefault,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        int baseState = UseDefault) = 0;

    //! Draw a line with a texture so it can be dotted or dashed
    //
    //! \param texId        The texture ID returned by ITexture::GetTextureID()
    //! \param verts        An array of 2 vertices for the start and end points of the line
    //! \param blendMode    UseDefault means default blend mode (currently GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA)
    //! \param pixelRounding Whether and how to round pixel coordinates
    //! \param baseState     Additional render state to pass to or into value passed to renderer SetState
    virtual void DrawLineTextured(int texId, VertexPosColUV* verts,
        int blendMode = UseDefault,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        int baseState = UseDefault) = 0;

    //! Draw a text string. Only supports ASCII text.
    //
    //! The font and effect used to render the text are specified in the textOptions structure
    //! \param textString   A null terminated ASCII text string. May contain \n characters
    //! \param position     Position of the text in pixels. Alignment values in textOptions affect actual position
    //! \param pointSize    The size of the font to use
    //! \param opacity      The opacity (alpha value) to use to draw the text
    //! \param textOptions  Pointer to an options struct. If null the default options are used
    virtual void DrawText(const char* textString, AZ::Vector2 position, float pointSize,
        float opacity = 1.0f, TextOptions* textOptions = nullptr) = 0;

    //! Get the width and height (in pixels) that would be used to draw the given text string.
    //
    //! Pass the same parameter values that would be used to draw the string
    virtual AZ::Vector2 GetTextSize(const char* textString, float pointSize, TextOptions* textOptions = nullptr) = 0;

    //! Get the width of the rendering viewport (in pixels).
    //
    //! If rendering full screen this is the native width from IRenderer
    virtual float GetViewportWidth() const = 0;

    //! Get the height of the rendering viewport (in pixels).
    //
    //! If rendering full screen this is the native width from IRenderer
    virtual float GetViewportHeight() const = 0;

    //! Get the default values that would be used if no image options were passed in
    //
    //! This is a convenient way to initialize the imageOptions struct
    virtual const ImageOptions& GetDefaultImageOptions() const = 0;

    //! Get the default values that would be used if no text options were passed in
    //
    //! This is a convenient way to initialize the textOptions struct
    virtual const TextOptions& GetDefaultTextOptions() const = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Helper class for using the IDraw2d interface
//!
//! The Draw2dHelper class is an inline wrapper that provides two convenience features:
//! 1. It automatically calls BeginDraw2d/EndDraw2d in its construction/destruction.
//! 2. It automatically sets member options structures to their defaults and provides set functions
//!    to set them.
class Draw2dHelper
{
public: // member functions

    //! Start a section of 2D drawing function calls. This will set appropriate render state.
    Draw2dHelper(bool deferCalls = false)
        : m_draw2d(GetDraw2d())
    {
        if (m_draw2d)
        {
            m_draw2d->BeginDraw2d(deferCalls);
            m_imageOptions = m_draw2d->GetDefaultImageOptions();
            m_textOptions = m_draw2d->GetDefaultTextOptions();
        }
    }

    //! End a section of 2D drawing function calls. This will reset some render state.
    ~Draw2dHelper()
    {
        if (m_draw2d)
        {
            m_draw2d->EndDraw2d();
        }
    }

    //! Draw a textured quad, optional rotation is counter-clockwise in degrees.
    //
    //! See IDraw2d:DrawImage for parameter descriptions
    void DrawImage(int texId, AZ::Vector2 position, AZ::Vector2 size, float opacity = 1.0f,
        float rotation = 0.0f, const AZ::Vector2* pivotPoint = nullptr, const AZ::Vector2* minMaxTexCoords = nullptr)
    {
        if (m_draw2d)
        {
            m_draw2d->DrawImage(texId, position, size, opacity, rotation, pivotPoint, minMaxTexCoords, &m_imageOptions);
        }
    }

    //! Draw a textured quad where the position specifies the point specified by the alignment.
    //
    //! See IDraw2d:DrawImageAligned for parameter descriptions
    void DrawImageAligned(int texId, AZ::Vector2 position, AZ::Vector2 size,
        IDraw2d::HAlign horizontalAlignment, IDraw2d::VAlign verticalAlignment,
        float opacity = 1.0f, float rotation = 0.0f, const AZ::Vector2* minMaxTexCoords = nullptr)
    {
        if (m_draw2d)
        {
            m_draw2d->DrawImageAligned(texId, position, size, horizontalAlignment, verticalAlignment,
                opacity, rotation, minMaxTexCoords, &m_imageOptions);
        }
    }

    //! Draw a textured quad where the position, color and uv of each point is specified explicitly
    //
    //! See IDraw2d:DrawQuad for parameter descriptions
    void DrawQuad(int texId, IDraw2d::VertexPosColUV* verts, int blendMode = IDraw2d::UseDefault,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        int baseState = IDraw2d::UseDefault)
    {
        if (m_draw2d)
        {
            m_draw2d->DrawQuad(texId, verts, blendMode, pixelRounding, baseState);
        }
    }

    //! Draw a line
    //
    //! See IDraw2d:DrawLine for parameter descriptions
    void DrawLine(AZ::Vector2 start, AZ::Vector2 end, AZ::Color color, int blendMode = IDraw2d::UseDefault,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        int baseState = IDraw2d::UseDefault)
    {
        if (m_draw2d)
        {
            m_draw2d->DrawLine(start, end, color, blendMode, pixelRounding, baseState);
        }
    }

    //! Draw a line with a texture so it can be dotted or dashed
    //
    //! See IDraw2d:DrawLineTextured for parameter descriptions
    void DrawLineTextured(int texId, IDraw2d::VertexPosColUV* verts, int blendMode = IDraw2d::UseDefault,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        int baseState = IDraw2d::UseDefault)
    {
        if (m_draw2d)
        {
            m_draw2d->DrawLineTextured(texId, verts, blendMode, pixelRounding, baseState);
        }
    }

    //! Draw a text string. Only supports ASCII text.
    //
    //! See IDraw2d:DrawText for parameter descriptions
    void DrawText(const char* textString, AZ::Vector2 position, float pointSize, float opacity = 1.0f)
    {
        if (m_draw2d)
        {
            m_draw2d->DrawText(textString, position, pointSize, opacity, &m_textOptions);
        }
    }

    //! Get the width and height (in pixels) that would be used to draw the given text string.
    //
    //! See IDraw2d:GetTextSize for parameter descriptions
    AZ::Vector2 GetTextSize(const char* textString, float pointSize)
    {
        if (m_draw2d)
        {
            return m_draw2d->GetTextSize(textString, pointSize, &m_textOptions);
        }
        else
        {
            return AZ::Vector2(0, 0);
        }
    }

    // State management

    //! Set the blend mode used for images, default is GS_BLSRC_SRCALPHA|GS_BLDST_ONEMINUSSRCALPHA.
    void SetImageBlendMode(int mode) { m_imageOptions.blendMode = mode; }

    //! Set the color used for DrawImage and other image drawing.
    void SetImageColor(AZ::Vector3 color) { m_imageOptions.color = color; }

    //! Set whether images are rounded to have the points on exact pixel boundaries.
    void SetImagePixelRounding(IDraw2d::Rounding round) { m_imageOptions.pixelRounding = round; }

    //! Set the base state (that blend mode etc is combined with) used for images, default is GS_NODEPTHTEST.
    void SetImageBaseState(int state) { m_imageOptions.baseState = state; }

    //! Set the text font.
    void SetTextFont(IFFont* font) { m_textOptions.font = font; }

    //! Set the text font effect index.
    void SetTextEffectIndex(unsigned int effectIndex) { m_textOptions.effectIndex = effectIndex; }

    //! Set the text color.
    void SetTextColor(AZ::Vector3 color) { m_textOptions.color = color; }

    //! Set the text alignment.
    void SetTextAlignment(IDraw2d::HAlign horizontalAlignment, IDraw2d::VAlign verticalAlignment)
    {
        m_textOptions.horizontalAlignment = horizontalAlignment;
        m_textOptions.verticalAlignment = verticalAlignment;
    }

    //! Set a drop shadow for text drawing. An alpha of zero disables drop shadow.
    void SetTextDropShadow(AZ::Vector2 offset, AZ::Color color)
    {
        m_textOptions.dropShadowOffset = offset;
        m_textOptions.dropShadowColor = color;
    }

    //! Set a rotation for the text. The text rotates around its position (taking into account alignment).
    void SetTextRotation(float rotation)
    {
        m_textOptions.rotation = rotation;
    }

    //! Set the base state (that blend mode etc is combined with) used for text, default is GS_NODEPTHTEST.
    void SetTextBaseState(int state) { m_textOptions.baseState = state; }

public: // static member functions

    //! Helper to get the IDraw2d interface
    static IDraw2d* GetDraw2d() { return (gEnv && gEnv->pLyShine) ? gEnv->pLyShine->GetDraw2d() : nullptr; }

    //! Get the width of the rendering viewport (in pixels).
    static float GetViewportWidth()
    {
        IDraw2d* draw2d = GetDraw2d();
        return (draw2d) ? draw2d->GetViewportWidth() : 0.0f;
    }

    //! Get the height of the rendering viewport (in pixels).
    static float GetViewportHeight()
    {
        IDraw2d* draw2d = GetDraw2d();
        return (draw2d) ? draw2d->GetViewportHeight() : 0.0f;
    }

    //! Round the X and Y coordinates of a point using the given rounding policy
    template<typename T>
    static T RoundXY(T value, IDraw2d::Rounding roundingType)
    {
        T result = value;

        switch (roundingType)
        {
        case IDraw2d::Rounding::None:
            // nothing to do
            break;
        case IDraw2d::Rounding::Nearest:
            result.SetX(floor(value.GetX() + 0.5f));
            result.SetY(floor(value.GetY() + 0.5f));
            break;
        case IDraw2d::Rounding::Down:
            result.SetX(floor(value.GetX()));
            result.SetY(floor(value.GetY()));
            break;
        case IDraw2d::Rounding::Up:
            result.SetX(ceil(value.GetX()));
            result.SetY(ceil(value.GetY()));
            break;
        }

        return result;
    }

protected: // attributes

    IDraw2d::ImageOptions   m_imageOptions; //!< image options are stored locally and updated by member functions
    IDraw2d::TextOptions    m_textOptions;  //!< text options are stored locally and updated by member functions
    IDraw2d* m_draw2d;
};
