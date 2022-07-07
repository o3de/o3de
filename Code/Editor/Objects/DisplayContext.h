/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : DisplayContext definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_DISPLAYCONTEXT_H
#define CRYINCLUDE_EDITOR_OBJECTS_DISPLAYCONTEXT_H
#pragma once


#include "SandboxAPI.h"
#include <Cry_Color.h>
#include <Cry_Geo.h>
#include <AzCore/std/containers/vector.h>

#include <QColor>

#define DC_DEFAULT_DOTLINE_STEPS 10
#define DC_UNIT_DEGREE 1

// forward declarations.
struct IDisplayViewport;
struct IRenderer;
struct IRenderAuxGeom;
struct IIconManager;
class CDisplaySettings;
class QPoint;

enum DisplayFlags
{
    DISPLAY_2D = 0x01,
    DISPLAY_HIDENAMES = 0x02,
    DISPLAY_BBOX = 0x04,
    DISPLAY_TRACKS = 0x08,
    DISPLAY_TRACKTICKS = 0x010,
    DISPLAY_WORLDSPACEAXIS = 0x020, //!< Set if axis must be displayed in world space.
    DISPLAY_LINKS = 0x040,
    DISPLAY_DEGRADATED = 0x080, //!< Display Objects in degradated quality (When moving/modifying).
    DISPLAY_SELECTION_HELPERS = 0x100,  //!< Display advanced selection helpers.
};

/*!
 *  DisplayContex is a structure passed to BaseObject Display method.
 *  It contains everything the object should know to display itself in a view.
 *  All fields must be filled before passing that structure to Display call.
 */
struct SANDBOX_API DisplayContext
{
    enum ETextureIconFlags
    {
        TEXICON_ADDITIVE     = 0x0001,
        TEXICON_ALIGN_BOTTOM = 0x0002,
        TEXICON_ALIGN_TOP    = 0x0004,
        TEXICON_ON_TOP       = 0x0008,
    };

    CDisplaySettings* settings;
    IDisplayViewport* view;
    IRenderAuxGeom* pRenderAuxGeom;
    IIconManager* pIconManager;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AABB    box;    // Bounding box of volume that need to be repainted.
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    int flags;

    //! Ctor.
    DisplayContext();
    // Helper methods.
    void SetView(IDisplayViewport* pView);
    IDisplayViewport* GetView() const { return view; }
    void Flush2D();

    //////////////////////////////////////////////////////////////////////////
    // Draw functions
    //////////////////////////////////////////////////////////////////////////
    //! Set current materialc color.
    void SetColor(float r, float g, float b, float a = 1)
    {
        m_color4b = ColorB(
            static_cast<uint8>(r * 255.0f), static_cast<uint8>(g * 255.0f), static_cast<uint8>(b * 255.0f), static_cast<uint8>(a * 255.0f));
    };
    void SetColor(const Vec3& color, float a = 1)
    {
        m_color4b = ColorB(
            static_cast<uint8>(color.x * 255.0f), static_cast<uint8>(color.y * 255.0f), static_cast<uint8>(color.z * 255.0f),
            static_cast<uint8>(a * 255.0f));
    };
    void SetColor(const AZ::Vector3& color, float a = 1)
    {
        m_color4b = ColorB(
            static_cast<uint8>(color.GetX() * 255.0f), static_cast<uint8>(color.GetY() * 255.0f), static_cast<uint8>(color.GetZ() * 255.0f),
            static_cast<uint8>(a * 255.0f));
    };
    void SetColor(const QColor& rgb, float a)
    {
        m_color4b = ColorB(
            static_cast<uint8>(rgb.red()), static_cast<uint8>(rgb.green()), static_cast<uint8>(rgb.blue()), static_cast<uint8>(a * 255.0f));
    };
    void SetColor(const QColor& color)
    {
        m_color4b = ColorB(
            static_cast<uint8>(color.red()), static_cast<uint8>(color.green()), static_cast<uint8>(color.blue()),
            static_cast<uint8>(color.alpha()));
    };
    void SetColor(const ColorB& color)
    {
        m_color4b = color;
    };
    void SetAlpha(float a = 1) { m_color4b.a = static_cast<uint8>(a * 255.0f); };
    ColorB GetColor() const { return m_color4b; }

    void SetSelectedColor(float fAlpha = 1);
    void SetFreezeColor();

    //! Get color to draw selectin of object.
    QColor GetSelectedColor();
    QColor GetFreezeColor();

    // Draw 3D quad.
    void DrawQuad(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& p4);
    void DrawQuad(float width, float height);
    void DrawQuadGradient(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& p4, ColorB firstColor, ColorB secondColor);
    void DrawWireQuad(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& p4);
    void DrawWireQuad(float width, float height);
    // Draw 3D Triangle.
    void DrawTri(const Vec3& p1, const Vec3& p2, const Vec3& p3);
    void DrawTriangles(const AZStd::vector<Vec3>& vertices, const ColorB& color);
    void DrawTrianglesIndexed(const AZStd::vector<Vec3>& vertices, const AZStd::vector<vtx_idx>& indices, const ColorB& color);
    // Draw wireframe box.
    void DrawWireBox(const Vec3& min, const Vec3& max);
    void DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max);
    // Draw filled box
    void DrawSolidBox(const Vec3& min, const Vec3& max);
    void DrawSolidOBB(const Vec3& center, const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ, const Vec3& halfExtents);
    void DrawPoint(const Vec3& p, int nSize = 1);
    void DrawLine(const Vec3& p1, const Vec3& p2);
    void DrawLine(const Vec3& p1, const Vec3& p2, const ColorF& col1, const ColorF& col2);
    void DrawLine(const Vec3& p1, const Vec3& p2, const QColor& rgb1, const QColor& rgb2);
    void DrawLines(const AZStd::vector<Vec3>& vertices, const ColorF& color);
    void DrawPolyLine(const Vec3* pnts, int numPoints, bool cycled = true);

    // Vera, Confetti
    void DrawDottedLine(const Vec3& p1, const Vec3& p2, const ColorF& col1, const ColorF& col2, const float numOfSteps = DC_DEFAULT_DOTLINE_STEPS);
    void DrawWireQuad2d(const QPoint& p1, const QPoint& p2, float z);
    void DrawLine2d(const QPoint& p1, const QPoint& p2, float z);
    void DrawLine2dGradient(const QPoint& p1, const QPoint& p2, float z, ColorB firstColor, ColorB secondColor);
    void DrawWireCircle2d(const QPoint& center, float radius, float z);

    // Draw circle from lines on terrain, position is in world space.
    void DrawTerrainCircle(const Vec3& worldPos, float radius, float height);
    void DrawTerrainCircle(const Vec3& center, float radius, float angle1, float angle2, float height);

    /// DrawArc
    /// Draws an arc around the specified position from a given angle across the angular length given by sweepAngleDegrees
    /// it orients the arc around the index of the given basis axis.
    /// \param pos World space position on which to center the arc.
    /// \param radius Radius that defines the size of the arc.
    /// \param startAngleDegrees Angle in degrees measured clockwise from the basis axis to the starting point of the arc.
    /// \param sweepAngleDegreees Angle in degrees measured clockwise from the startAngle parameter to ending point of the arc.
    /// \param angularStepDegrees Defines the distance between vertices, a small value will result in a greater number of vertices.
    /// \param referenceAxis Axis on which to align the arc (0 for X, 1 for Y, 2 for Z)
    void DrawArc(const Vec3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, int referenceAxis = 2);


    /// DrawArc
    /// Draws an arc around the specified position from a given angle across the angular length given by sweepAngleDegrees
    /// oriented around the specified axis.
    /// \param pos World space position on which to center the arc.
    /// \param radius Radius that defines the size of the arc.
    /// \param startAngleDegrees Angle in degrees measured clockwise from the basis axis to the starting point of the arc.
    /// \param sweepAngleDegreees Angle in degrees measured clockwise from the startAngle parameter to ending point of the arc.
    /// \param angularStepDegrees Defines the distance between vertices, a small value will result in a greater number of vertices.
    /// \param fixedAxis Normal axis on which to align the arc.
    void DrawArc(const Vec3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const Vec3& fixedAxis);

    //Vera, Confetti:
    //Draw an arc and an arrow at the end of the arc
    void DrawArcWithArrow(const Vec3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const Vec3& fixedAxis);

    // Draw circle.
    void DrawCircle(const Vec3& pos, float radius, int nUnchangedAxis = 2 /*z axis*/);

    void DrawHalfDottedCircle(const Vec3& pos, float radius, const Vec3& viewPos, int nUnchangedAxis = 2 /*z axis*/);

    // Vera, Confetti :
    // Draw a dotted circle.
    void DrawDottedCircle(const Vec3& pos, float radius, const Vec3& nUnchangedAxis, int numberOfArrows = 0, float stepDegree = DC_UNIT_DEGREE);

    // Draw cylinder.
    void DrawCylinder(const Vec3& p1, const Vec3& p2, float radius, float height);

    void DrawCone(const Vec3& pos, const Vec3& dir, float radius, float height, bool drawShaded = true);

    /// DrawWireCylinder
    /// \param center Center of cylinder.
    /// \param axis Axis along which cylinder is tall.
    /// \param radius Radius of cylinder.
    /// \param height Total height of cylinder.
    void DrawWireCylinder(const Vec3& center, const Vec3& axis, float radius, float height);

    /// DrawSolidCylinder
    /// \param center Center of cylinder.
    /// \param axis Axis along which cylinder is tall.
    /// \param radius Radius of cylinder.
    /// \param height Total height of cylinder.
    void DrawSolidCylinder(const Vec3& center, const Vec3& axis, float radius, float height, bool drawShaded = true);

    /// DrawWireCapsule
    /// \param pos Center of capsule.
    /// \param axis Axis along which capsule is tall.
    /// \param radius Radius of capsule.
    /// \param heightStraightSection Height of capsule's straight section (does not include caps).
    void DrawWireCapsule(const Vec3& center, const Vec3& axis, float radius, float heightStraightSection);

    //! Draw rectangle on top of terrain.
    //! Coordinates are in world space.
    void DrawTerrainRect(float x1, float y1, float x2, float y2, float height);

    void DrawTerrainLine (Vec3 worldPos1, Vec3 worldPos2);

    void DrawWireSphere(const Vec3& pos, float radius);
    void DrawWireSphere(const Vec3& pos, const Vec3 radius);

    void DrawWireDisk(const Vec3& pos, const Vec3& dir, float radius);

    void PushMatrix(const Matrix34& tm);
    void PopMatrix();
    const Matrix34& GetMatrix();

    // Draw special 3D objects.
    void DrawBall(const Vec3& pos, float radius, bool drawShaded = true);
    void DrawDisk(const Vec3& pos, const Vec3& dir, float radius);

    //! Draws 3d arrow.
    void DrawArrow(const Vec3& src, const Vec3& trg, float fHeadScale = 1, bool b2SidedArrow = false);

    // Draw texture label in 2d view coordinates.
    // w,h in pixels.
    void DrawTextureLabel(const Vec3& pos, int nWidth, int nHeight, int nTexId, int nTexIconFlags = 0, int srcOffsetX = 0, int scrOffsetY = 0, bool bDistanceScaleIcons = false, float fDistanceScale = 1.0f);

    void RenderObject(int objectType, const Vec3& pos, float scale);
    void RenderObject(int objectType, const Matrix34& tm);

    void DrawTextLabel(const Vec3& pos, float size, const char* text, const bool bCenter = false, int srcOffsetX = 0, int scrOffsetY = 0);
    void Draw2dTextLabel(float x, float y, float size, const char* text, bool bCenter = false);
    void SetLineWidth(float width);

    //! Is given bbox visible in this display context.
    bool IsVisible(const AABB& bounds);

    //! Gets current render state.
    uint32 GetState() const;
    //! Set a new render state.
    //! \param returns previous render state.
    uint32 SetState(uint32 state);
    //! Set a new render state flags.
    //! \param returns previous render state.
    uint32 SetStateFlag(uint32 state);
    //! Clear specified flags in render state.
    //! \param returns previous render state.
    uint32 ClearStateFlag(uint32 state);

    void DepthTestOff();
    void DepthTestOn();

    void DepthWriteOff();
    void DepthWriteOn();

    void CullOff();
    void CullOn();

    // Enables drawing helper lines in front of usual geometry, adds a small z offset to all drawn lines.
    bool SetDrawInFrontMode(bool bOn);

    // Description:
    //   Changes fill mode.
    // Arguments:
    //    nFillMode is one of the values from EAuxGeomPublicRenderflags_FillMode
    int  SetFillMode(int nFillMode);

    //! Convert position to world space.
    Vec3 ToWorldSpacePosition(const Vec3& v) { return m_matrixStack[m_currentMatrix].TransformPoint(v); }

    //! Convert direction to world space (translation is not considered)
    Vec3 ToWorldSpaceVector(const Vec3& v) { return m_matrixStack[m_currentMatrix].TransformVector(v); }

    float ToWorldSpaceMaxScale(float value);

    float GetLineWidth() const              {   return m_thickness; }

private:

    void InternalDrawLine(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1);

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    ColorB m_color4b;
    uint32 m_renderState;
    float m_thickness;
    float m_width;
    float m_height;

    int m_currentMatrix;
    //! Matrix stack.
    Matrix34 m_matrixStack[32];

    struct STextureLabel
    {
        float x, y, z; // 2D position (z in world space).
        float w, h;  // Width height.
        int nTexId; // Texture id.
        int flags;  // ETextureIconFlags
        float color[4];
    };
    std::vector<STextureLabel> m_textureLabels;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_DISPLAYCONTEXT_H
