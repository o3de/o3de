/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/IDraw2d.h>
#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiTransformBus.h>

#include <AzFramework/Font/FontInterface.h>
#include <Atom/Bootstrap/BootstrapNotificationBus.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Public/ViewportContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Implementation of IDraw2d interface for 2D drawing in screen space
//
//! The CDraw2d class implements the IDraw2d interface for drawing 2D images, shapes and text.
//! Positions and sizes are specified in pixels in the associated 2D viewport.
class CDraw2d
    : public IDraw2d // LYSHINE_ATOM_TODO - keep around until gEnv->pLyShine is replaced by bus interface
    , public AZ::Render::Bootstrap::NotificationBus::Handler
{
public: // types

    struct RenderState
    {
        RenderState()
        {
            m_blendState.m_enable = true;
            m_blendState.m_blendSource = AZ::RHI::BlendFactor::AlphaSource;
            m_blendState.m_blendDest = AZ::RHI::BlendFactor::AlphaSourceInverse;

            m_depthState.m_enable = false;
        }

        AZ::RHI::TargetBlendState m_blendState;
        AZ::RHI::DepthState m_depthState;
    };

    //! Struct used to pass additional image options.
    //
    //! If this is not passed then the defaults are used
    struct ImageOptions
    {
        AZ::Vector3 color = AZ::Vector3(1.0f, 1.0f, 1.0f);
        Rounding pixelRounding = Rounding::Nearest;
        RenderState m_renderState;
    };

    //! Struct used to pass additional text options - mostly ones that do not change from call to call.
    //
    //! If this is not passed then the defaults below are used
    struct TextOptions
    {
        AZStd::string   fontName;               //!< default is "default"
        unsigned int    effectIndex;            //!< default is 0
        AZ::Vector3     color;                  //!< default is (1,1,1)
        HAlign          horizontalAlignment;    //!< default is HAlign::Left
        VAlign          verticalAlignment;      //!< default is VAlign::Top
        AZ::Vector2     dropShadowOffset;       //!< default is (0,0), zero offset means no drop shadow is drawn
        AZ::Color       dropShadowColor;        //!< default is (0,0,0,0), zero alpha means no drop shadow is drawn
        float           rotation;               //!< default is 0
        bool            depthTestEnabled;       //!< default is false
    };

    //! Used to pass in arrays of vertices (e.g. to DrawQuad)
    struct VertexPosColUV
    {
        VertexPosColUV() {}
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

    //! Constructor, constructed by the LyShine class
    CDraw2d(AZ::RPI::ViewportContextPtr viewportContext = nullptr);

    // IDraw2d

    ~CDraw2d() override;

    // ~IDraw2d

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
    void DrawImage(AZ::Data::Instance<AZ::RPI::Image> image, AZ::Vector2 position, AZ::Vector2 size, float opacity = 1.0f,
        float rotation = 0.0f, const AZ::Vector2* pivotPoint = nullptr, const AZ::Vector2* minMaxTexCoords = nullptr,
        ImageOptions* imageOptions = nullptr);

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
    void DrawImageAligned(AZ::Data::Instance<AZ::RPI::Image> image, AZ::Vector2 position, AZ::Vector2 size,
        HAlign horizontalAlignment, VAlign verticalAlignment,
        float opacity = 1.0f, float rotation = 0.0f, const AZ::Vector2* minMaxTexCoords = nullptr,
        ImageOptions* imageOptions = nullptr);

    //! Draw a textured quad where the position, color and uv of each point is specified explicitly
    //
    //! \param texId        The texture ID returned by ITexture::GetTextureID()
    //! \param verts        An array of 4 vertices, in clockwise order (e.g. top left, top right, bottom right, bottom left)
    //! \param pixelRounding Whether and how to round pixel coordinates
    //! \param renderState  Blend mode and depth state
    virtual void DrawQuad(AZ::Data::Instance<AZ::RPI::Image> image,
        VertexPosColUV* verts,
        Rounding pixelRounding = Rounding::Nearest,
        const RenderState& renderState = RenderState{});

    //! Draw a line
    //
    //! \param start        The start position
    //! \param end          The end position
    //! \param color        The color of the line
    //! \param pixelRounding Whether and how to round pixel coordinates
    //! \param renderState  Blend mode and depth state
    virtual void DrawLine(AZ::Vector2 start, AZ::Vector2 end, AZ::Color color,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        const RenderState& renderState = RenderState{});

    //! Draw a line with a texture so it can be dotted or dashed
    //
    //! \param texId        The texture ID returned by ITexture::GetTextureID()
    //! \param verts        An array of 2 vertices for the start and end points of the line
    //! \param pixelRounding Whether and how to round pixel coordinates
    //! \param renderState  Blend mode and depth state
    virtual void DrawLineTextured(AZ::Data::Instance<AZ::RPI::Image> image,
        VertexPosColUV* verts,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        const RenderState& renderState = RenderState{});
    //! Draw a text string. Only supports ASCII text.
    //
    //! The font and effect used to render the text are specified in the textOptions structure
    //! \param textString   A null terminated ASCII text string. May contain \n characters
    //! \param position     Position of the text in pixels. Alignment values in textOptions affect actual position
    //! \param pointSize    The size of the font to use
    //! \param opacity      The opacity (alpha value) to use to draw the text
    //! \param textOptions  Pointer to an options struct. If null the default options are used
    void DrawText(const char* textString, AZ::Vector2 position, float pointSize,
        float opacity = 1.0f, TextOptions* textOptions = nullptr);

    //! Draw a rectangular outline with a texture
    //
    //! \param image            The texture to be used for drawing the outline
    //! \param points           The rect's vertices (top left, top right, bottom right, bottom left)
    //! \param rightVec         Right vector. Specified because the rect's width/height could be 0 
    //! \param downVec          Down vector. Specified because the rect's width/height could be 0
    //! \param color            The color of the outline
    //! \param lineThickness    The thickness in pixels of the outline. If 0, it will be based on image height
    void DrawRectOutlineTextured(AZ::Data::Instance<AZ::RPI::Image> image,
        UiTransformInterface::RectPoints points,
        AZ::Vector2 rightVec,
        AZ::Vector2 downVec,
        AZ::Color color,
        uint32_t lineThickness = 0);

    //! Get the width and height (in pixels) that would be used to draw the given text string.
    //
    //! Pass the same parameter values that would be used to draw the string
    AZ::Vector2 GetTextSize(const char* textString, float pointSize, TextOptions* textOptions = nullptr);

    //! Get the width of the rendering viewport (in pixels).
    float GetViewportWidth() const;

    //! Get the height of the rendering viewport (in pixels).
    float GetViewportHeight() const;

    //! Get the default values that would be used if no image options were passed in
    //
    //! This is a convenient way to initialize the imageOptions struct
    virtual const ImageOptions& GetDefaultImageOptions() const;

    //! Get the default values that would be used if no text options were passed in
    //
    //! This is a convenient way to initialize the textOptions struct
    virtual const TextOptions& GetDefaultTextOptions() const;

    //! Render the primitives that have been deferred
    void RenderDeferredPrimitives();

    //! Specify whether to defer future primitives or render them right away
    void SetDeferPrimitives(bool deferPrimitives);

    //! Return whether future primitives will be deferred or rendered right away
    bool GetDeferPrimitives();

    //! Set sort key offset for following draws.
    void SetSortKey(int64_t key);

private:

    AZ_DISABLE_COPY_MOVE(CDraw2d);

    // AZ::Render::Bootstrap::NotificationBus overrides
    void OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene) override;

public: // static member functions

    //! Given a position and size and an alignment return the top left corner of the aligned quad
    static AZ::Vector2 Align(AZ::Vector2 position, AZ::Vector2 size, HAlign horizontalAlignment, VAlign verticalAlignment);

    //! Helper to load a texture
    static AZ::Data::Instance<AZ::RPI::Image> LoadTexture(const AZStd::string& pathName);

protected: // types and constants

    enum
    {
        MAX_VERTICES_IN_PRIM = 6
    };

    // Cached shader data
    struct Draw2dShaderData
    {
        AZ::RHI::ShaderInputImageIndex m_imageInputIndex;
        AZ::RHI::ShaderInputConstantIndex m_viewProjInputIndex;
    };

    class DeferredPrimitive
    {
    public:
        virtual ~DeferredPrimitive() {};
        virtual void Draw(AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw,
            const Draw2dShaderData& shaderData,
            AZ::RPI::ViewportContextPtr viewportContext) const = 0;
    };

    class DeferredQuad
        : public DeferredPrimitive
    {
    public:
        ~DeferredQuad() override {};
        void Draw(AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw,
            const Draw2dShaderData& shaderData,
            AZ::RPI::ViewportContextPtr viewportContext) const override;

        AZ::Vector2 m_points[4];
        AZ::Vector2 m_texCoords[4];
        uint32      m_packedColors[4];
        AZ::Data::Instance<AZ::RPI::Image> m_image;
        RenderState m_renderState;
    };

    class DeferredLine
        : public DeferredPrimitive
    {
    public:
        ~DeferredLine() override {};
        void Draw(AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw,
            const Draw2dShaderData& shaderData,
            AZ::RPI::ViewportContextPtr viewportContext) const override;

        AZ::Data::Instance<AZ::RPI::Image> m_image;
        AZ::Vector2 m_points[2];
        AZ::Vector2 m_texCoords[2];
        uint32      m_packedColors[2];
        RenderState m_renderState;
    };

    class DeferredText
        : public DeferredPrimitive
    {
    public:
        ~DeferredText() override {};
        void Draw(AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw,
            const Draw2dShaderData& shaderData,
            AZ::RPI::ViewportContextPtr viewportContext) const override;

        AzFramework::TextDrawParameters m_drawParameters;
        AzFramework::FontId m_fontId;
        std::string         m_string;
    };

    class DeferredRectOutline
        : public DeferredPrimitive
    {
    public:
        ~DeferredRectOutline() override {};
        void Draw(AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw,
            const Draw2dShaderData& shaderData,
            AZ::RPI::ViewportContextPtr viewportContext) const override;

        AZ::Data::Instance<AZ::RPI::Image> m_image;

        static constexpr int32 NUM_VERTS = 8;
        AZ::Vector2 m_verts2d[NUM_VERTS];
        AZ::Vector2 m_uvs[NUM_VERTS];

        AZ::Color m_color;
    };

protected: // member functions

    //! Rotate an array of points around the z-axis at the pivot point.
    //
    //! Angle is in degrees counter-clockwise
    void RotatePointsAboutPivot(AZ::Vector2* points, int numPoints, AZ::Vector2 pivot, float angle) const;

    //! Helper function to render a text string
    void DrawTextInternal(const char* textString, AzFramework::FontId fontId, unsigned int effectIndex,
        AZ::Vector2 position, float pointSize, AZ::Color color, float rotation,
        HAlign horizontalAlignment, VAlign verticalAlignment, bool depthTestEnabled);

    //! Draw or defer a quad
    void DrawOrDeferQuad(const DeferredQuad* quad);

    //! Draw or defer a line
    void DrawOrDeferLine(const DeferredLine* line);

    //! Draw or defer a text string
    void DrawOrDeferTextString(const DeferredText* text);

    //! Draw or defer a rect outline
    void DrawOrDeferRectOutline(const DeferredRectOutline* outlineRect);

    //! Get specified viewport context or default viewport context if not specified
    AZ::RPI::ViewportContextPtr GetViewportContext() const;

protected: // attributes

    ImageOptions m_defaultImageOptions;     //!< The default image options used if nullptr is passed
    TextOptions m_defaultTextOptions;       //!< The default text options used if nullptr is passed

    //! True if the actual render of the primitives should be deferred to a RenderDeferredPrimitives call
    bool m_deferCalls;

    std::vector<DeferredPrimitive*> m_deferredPrimitives;

    AZ::RPI::ViewportContextPtr m_viewportContext;
    AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> m_dynamicDraw;
    Draw2dShaderData m_shaderData;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Helper class for using the IDraw2d interface
//!
//! The Draw2dHelper class is an inline wrapper that provides the convenience feature of
//! automatically setting member options structures to their defaults and providing set functions.
class Draw2dHelper
{
public: // member functions

    //! Start a section of 2D drawing function calls that will render to the default viewport
    Draw2dHelper(bool deferCalls = false)
    {
        InitCommon(nullptr, deferCalls);
    }

    //! Start a section of 2D drawing function calls that will render to the viewport
    //! associated with the specified Draw2d object
    Draw2dHelper(CDraw2d* draw2d, bool deferCalls = false)
    {
        InitCommon(draw2d, deferCalls);
    }

    void InitCommon(CDraw2d* draw2d, bool deferCalls)
    {
        m_draw2d = draw2d;

        if (!m_draw2d)
        {
            // Set to default which is the game's draw 2d object
            m_draw2d = GetDefaultDraw2d();
        }

        if (m_draw2d)
        {
            m_previousDeferCalls = m_draw2d->GetDeferPrimitives();
            m_draw2d->SetDeferPrimitives(deferCalls);
            m_imageOptions = m_draw2d->GetDefaultImageOptions();
            m_textOptions = m_draw2d->GetDefaultTextOptions();
        }
    }

    //! End a section of 2D drawing function calls.
    ~Draw2dHelper()
    {
        if (m_draw2d)
        {
            m_draw2d->SetDeferPrimitives(m_previousDeferCalls);
        }
    }

    //! Draw a textured quad, optional rotation is counter-clockwise in degrees.
    //
    //! See IDraw2d:DrawImage for parameter descriptions
    void DrawImage(AZ::Data::Instance<AZ::RPI::Image> image, AZ::Vector2 position, AZ::Vector2 size, float opacity = 1.0f,
        float rotation = 0.0f, const AZ::Vector2* pivotPoint = nullptr, const AZ::Vector2* minMaxTexCoords = nullptr)
    {
        if (m_draw2d)
        {
            m_draw2d->DrawImage(image, position, size, opacity, rotation, pivotPoint, minMaxTexCoords, &m_imageOptions);
        }
    }

    //! Draw a textured quad where the position specifies the point specified by the alignment.
    //
    //! See IDraw2d:DrawImageAligned for parameter descriptions
    void DrawImageAligned(AZ::Data::Instance<AZ::RPI::Image> image, AZ::Vector2 position, AZ::Vector2 size,
        IDraw2d::HAlign horizontalAlignment, IDraw2d::VAlign verticalAlignment,
        float opacity = 1.0f, float rotation = 0.0f, const AZ::Vector2* minMaxTexCoords = nullptr)
    {
        if (m_draw2d)
        {
            m_draw2d->DrawImageAligned(image, position, size, horizontalAlignment, verticalAlignment,
                opacity, rotation, minMaxTexCoords, &m_imageOptions);
        }
    }

    //! Draw a textured quad where the position, color and uv of each point is specified explicitly
    //
    //! See IDraw2d:DrawQuad for parameter descriptions
    void DrawQuad(AZ::Data::Instance<AZ::RPI::Image> image, CDraw2d::VertexPosColUV* verts,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        const CDraw2d::RenderState& renderState = CDraw2d::RenderState{})
    {
        if (m_draw2d)
        {
            m_draw2d->DrawQuad(image, verts, pixelRounding, renderState);
        }
    }

    //! Draw a line
    //
    //! See IDraw2d:DrawLine for parameter descriptions
    void DrawLine(AZ::Vector2 start, AZ::Vector2 end, AZ::Color color,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        const CDraw2d::RenderState& renderState = CDraw2d::RenderState{})
    {
        if (m_draw2d)
        {
            m_draw2d->DrawLine(start, end, color, pixelRounding, renderState);
        }
    }

    //! Draw a line with a texture so it can be dotted or dashed
    //
    //! See IDraw2d:DrawLineTextured for parameter descriptions
    void DrawLineTextured(AZ::Data::Instance<AZ::RPI::Image> image, CDraw2d::VertexPosColUV* verts,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        const CDraw2d::RenderState& renderState = CDraw2d::RenderState{})
    {
        if (m_draw2d)
        {
            m_draw2d->DrawLineTextured(image, verts, pixelRounding, renderState);
        }
    }

    //! Draw a rect outline with a texture
    //
    //! See IDraw2d:DrawRectOutlineTextured for parameter descriptions
    void DrawRectOutlineTextured(AZ::Data::Instance<AZ::RPI::Image> image,
        UiTransformInterface::RectPoints points,
        AZ::Vector2 rightVec,
        AZ::Vector2 downVec,
        AZ::Color color,
        uint32_t lineThickness = 0)
    {
        if (m_draw2d)
        {
            m_draw2d->DrawRectOutlineTextured(image, points, rightVec, downVec, color, lineThickness);
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
    void SetImageBlendMode(const AZ::RHI::TargetBlendState& blendState) { m_imageOptions.m_renderState.m_blendState = blendState; }

    //! Set the color used for DrawImage and other image drawing.
    void SetImageColor(AZ::Vector3 color) { m_imageOptions.color = color; }

    //! Set whether images are rounded to have the points on exact pixel boundaries.
    void SetImagePixelRounding(IDraw2d::Rounding round) { m_imageOptions.pixelRounding = round; }

    //! Set the base state (that blend mode etc is combined with) used for images, default is GS_NODEPTHTEST.
    void SetImageDepthState(const AZ::RHI::DepthState& depthState) { m_imageOptions.m_renderState.m_depthState = depthState; }

    //! Set the text font.
    void SetTextFont(AZStd::string_view fontName) { m_textOptions.fontName = fontName; }

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

    //! Set wheter to enable depth test for the text
    void SetTextDepthTestEnabled(bool enabled)
    {
        m_textOptions.depthTestEnabled = enabled;
    }

public: // static member functions

    //! Helper to get the default IDraw2d interface
    static CDraw2d* GetDefaultDraw2d()
    {
        if (gEnv && gEnv->pLyShine) // LYSHINE_ATOM_TODO - remove pLyShine and use bus interface
        {
            IDraw2d* draw2d = gEnv->pLyShine->GetDraw2d();
            return reinterpret_cast<CDraw2d*>(draw2d);
        }

        return nullptr;
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

    CDraw2d::ImageOptions   m_imageOptions; //!< image options are stored locally and updated by member functions
    CDraw2d::TextOptions    m_textOptions;  //!< text options are stored locally and updated by member functions
    CDraw2d* m_draw2d;
    bool m_previousDeferCalls;
};
