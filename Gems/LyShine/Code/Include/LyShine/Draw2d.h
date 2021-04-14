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

#include <LyShine/IDraw2d.h>
#include <IRenderer.h>
#include <stack>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Implementation of IDraw2d interface for 2D drawing in screen space
//
//! The CDraw2d class implements the IDraw2d interface for drawing 2D images, shapes and text.
//! Positions and sizes are specified in pixels in the current 2D viewport.
class CDraw2d
    : public IDraw2d
{
public: // member functions

    //! Constructor, constructed by the LyShine class
    CDraw2d();

    // IDraw2d

    ~CDraw2d() override;

    //! Start a section of 2D drawing function calls. This will set appropriate render state.
    void BeginDraw2d(bool deferCalls = false) override;

    //! Start a section of 2D drawing function calls. This will set appropriate render state.
    //! This variant allows the viewport size to be specified
    void BeginDraw2d(AZ::Vector2 viewportSize, bool deferCalls = false) override;

    //! End a section of 2D drawing function calls. This will reset some render state.
    void EndDraw2d() override;

    //! Draw a textured quad, optional rotation is counter-clockwise in degrees.
    void DrawImage(int texId, AZ::Vector2 position, AZ::Vector2 size, float opacity = 1.0f,
        float rotation = 0.0f, const AZ::Vector2* pivotPoint = nullptr, const AZ::Vector2* minMaxTexCoords = nullptr,
        ImageOptions* imageOptions = nullptr) override;

    //! Draw a textured quad where the position specifies the point specified by the alignment. Rotation is around that point.
    void DrawImageAligned(int texId, AZ::Vector2 position, AZ::Vector2 size,
        HAlign horizontalAlignment, VAlign verticalAlignment,
        float opacity = 1.0f, float rotation = 0.0f, const AZ::Vector2* minMaxTexCoords = nullptr,
        ImageOptions* imageOptions = nullptr) override;

    //! Draw a textured quad where the position, color and uv of each point is specified explicitly
    void DrawQuad(int texId, VertexPosColUV* verts, int blendMode, Rounding pixelRounding, int baseState) override;

    //! Draw a line
    void DrawLine(AZ::Vector2 start, AZ::Vector2 end, AZ::Color color, int blendMode, Rounding pixelRounding, int baseState) override;

    //! Draw a line textured
    void DrawLineTextured(int texId, VertexPosColUV* verts, int blendMode, Rounding pixelRounding, int baseState) override;

    //! Draw a text string. Only supports ASCII text.
    void DrawText(const char* textString, AZ::Vector2 position, float pointSize,
        float opacity = 1.0f, TextOptions* textOptions = nullptr) override;

    //! Get the width and height (in pixels) that would be used to draw the given text string.
    AZ::Vector2 GetTextSize(const char* textString, float pointSize, TextOptions* textOptions = nullptr) override;

    //! Get the width of the rendering viewport (in pixels).
    float GetViewportWidth() const override;

    //! Get the height of the rendering viewport (in pixels).
    float GetViewportHeight() const override;

    //! Get the default values that would be used if no image options were passed in
    const ImageOptions& GetDefaultImageOptions() const override;

    //! Get the default values that would be used if no text options were passed in
    const TextOptions& GetDefaultTextOptions() const override;

    // ~IDraw2d

    //! Render the primitives that have been deferred
    void RenderDeferredPrimitives();

private:

    AZ_DISABLE_COPY_MOVE(CDraw2d);

public: // static member functions

    //! Given a position and size and an alignment return the top left corner of the aligned quad
    static AZ::Vector2 Align(AZ::Vector2 position, AZ::Vector2 size, HAlign horizontalAlignment, VAlign verticalAlignment);

protected: // types and constants

    enum
    {
        MAX_VERTICES_IN_PRIM = 6
    };

    class DeferredPrimitive
    {
    public:
        virtual ~DeferredPrimitive() {};
        virtual void Draw() const = 0;
    };

    class DeferredQuad
        : public DeferredPrimitive
    {
    public:
        ~DeferredQuad() override {};
        void Draw() const override;

        AZ::Vector2 m_points[4];
        AZ::Vector2 m_texCoords[4];
        uint32      m_packedColors[4];
        int         m_texId;
        int         m_state;
    };

    class DeferredLine
        : public DeferredPrimitive
    {
    public:
        ~DeferredLine() override {};
        void Draw() const override;

        int         m_texId;
        AZ::Vector2 m_points[2];
        AZ::Vector2 m_texCoords[2];
        uint32      m_packedColors[2];
        int         m_state;
    };

    class DeferredText
        : public DeferredPrimitive
    {
    public:
        ~DeferredText() override {};
        void Draw() const override;

        STextDrawContext    m_fontContext;
        IFFont*             m_font;
        AZ::Vector2         m_position;
        std::string         m_string;
    };

protected: // member functions

    //! Rotate an array of points around the z-axis at the pivot point.
    //
    //! Angle is in degrees counter-clockwise
    void RotatePointsAboutPivot(AZ::Vector2* points, int numPoints, AZ::Vector2 pivot, float angle) const;

    //! Helper function to render a text string
    void DrawTextInternal(const char* textString, IFFont* font, unsigned int effectIndex,
        AZ::Vector2 position, float pointSize, AZ::Color color, float rotation,
        HAlign horizontalAlignment, VAlign verticalAlignment, int baseState);

    //! Draw or defer a quad
    void DrawOrDeferQuad(const DeferredQuad* quad);

    //! Draw or defer a line
    void DrawOrDeferLine(const DeferredLine* line);

protected: // attributes

    ImageOptions m_defaultImageOptions;     //!< The default image options used if nullptr is passed
    TextOptions m_defaultTextOptions;       //!< The default text options used if nullptr is passed

    bool m_deferCalls;                      //!< True if the actual render of the primitives should be deferred until end of frame

    std::vector<DeferredPrimitive*> m_deferredPrimitives;

    //! These two data members allows nested calls to BeginDraw2d/EndDraw2d. We will begin 2D mode only on the
    //! outermost call to BeginDraw2d with deferCalls set to false and will end 2D mode on the corresposnding
    //! call to EndDraw2d. The stack is used to detect that corresponding call and we need the level it occurred
    //! to know when to end 2D mode.
    int m_nestLevelAtWhichStarted2dMode;
    std::stack<bool> m_deferCallsFlagStack;

private:
    TransformationMatrices m_backupSceneMatrices;
};
