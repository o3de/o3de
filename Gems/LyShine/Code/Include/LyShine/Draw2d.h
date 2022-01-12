/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/IDraw2d.h>

#include <AzFramework/Font/FontInterface.h>
#include <Atom/Bootstrap/BootstrapNotificationBus.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/ViewportContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Implementation of IDraw2d interface for 2D drawing in screen space
//
//! The CDraw2d class implements the IDraw2d interface for drawing 2D images, shapes and text.
//! Positions and sizes are specified in pixels in the associated 2D viewport.
class CDraw2d
    : public IDraw2d
    , public AZ::Render::Bootstrap::NotificationBus::Handler
{
public: // types

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
        ImageOptions* imageOptions = nullptr) override;

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
        ImageOptions* imageOptions = nullptr) override;

    //! Draw a textured quad where the position, color and uv of each point is specified explicitly
    //
    //! \param texId        The texture ID returned by ITexture::GetTextureID()
    //! \param verts        An array of 4 vertices, in clockwise order (e.g. top left, top right, bottom right, bottom left)
    //! \param pixelRounding Whether and how to round pixel coordinates
    //! \param renderState  Blend mode and depth state
    void DrawQuad(AZ::Data::Instance<AZ::RPI::Image> image,
        VertexPosColUV* verts,
        Rounding pixelRounding = Rounding::Nearest,
        bool clamp = false,
        const RenderState& renderState = RenderState{}) override;

    //! Draw a line
    //
    //! \param start        The start position
    //! \param end          The end position
    //! \param color        The color of the line
    //! \param pixelRounding Whether and how to round pixel coordinates
    //! \param renderState  Blend mode and depth state
    void DrawLine(AZ::Vector2 start, AZ::Vector2 end, AZ::Color color,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        const RenderState& renderState = RenderState{}) override;

    //! Draw a line with a texture so it can be dotted or dashed
    //
    //! \param texId        The texture ID returned by ITexture::GetTextureID()
    //! \param verts        An array of 2 vertices for the start and end points of the line
    //! \param pixelRounding Whether and how to round pixel coordinates
    //! \param renderState  Blend mode and depth state
    void DrawLineTextured(AZ::Data::Instance<AZ::RPI::Image> image,
        VertexPosColUV* verts,
        IDraw2d::Rounding pixelRounding = IDraw2d::Rounding::Nearest,
        const RenderState& renderState = RenderState{}) override;
    //! Draw a text string. Only supports ASCII text.
    //
    //! The font and effect used to render the text are specified in the textOptions structure
    //! \param textString   A null terminated ASCII text string. May contain \n characters
    //! \param position     Position of the text in pixels. Alignment values in textOptions affect actual position
    //! \param pointSize    The size of the font to use
    //! \param opacity      The opacity (alpha value) to use to draw the text
    //! \param textOptions  Pointer to an options struct. If null the default options are used
    void DrawText(const char* textString, AZ::Vector2 position, float pointSize,
        float opacity = 1.0f, TextOptions* textOptions = nullptr) override;

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
        uint32_t lineThickness = 0) override;

    //! Get the width and height (in pixels) that would be used to draw the given text string.
    //
    //! Pass the same parameter values that would be used to draw the string
    AZ::Vector2 GetTextSize(const char* textString, float pointSize, TextOptions* textOptions = nullptr) override;

    //! Get the width of the rendering viewport (in pixels).
    float GetViewportWidth() const override;

    //! Get the height of the rendering viewport (in pixels).
    float GetViewportHeight() const override;

    //! Get dpi scale factor
    float GetViewportDpiScalingFactor() const override;

    //! Get the default values that would be used if no image options were passed in
    //
    //! This is a convenient way to initialize the imageOptions struct
    const ImageOptions& GetDefaultImageOptions() const override;

    //! Get the default values that would be used if no text options were passed in
    //
    //! This is a convenient way to initialize the textOptions struct
    const TextOptions& GetDefaultTextOptions() const override;

    //! Render the primitives that have been deferred
    void RenderDeferredPrimitives() override;

    //! Specify whether to defer future primitives or render them right away
    void SetDeferPrimitives(bool deferPrimitives) override;

    //! Return whether future primitives will be deferred or rendered right away
    bool GetDeferPrimitives() override;

    //! Set sort key offset for following draws.
    void SetSortKey(int64_t key) override;

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
        AZ::RPI::ShaderVariantId m_shaderOptionsClamp;
        AZ::RPI::ShaderVariantId m_shaderOptionsWrap;
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
        bool m_clamp;
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
