/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Cry_Color.h"
#include "IRenderer.h"

struct SAuxGeomRenderFlags;

enum EBoundingBoxDrawStyle
{
    eBBD_Faceted,
    eBBD_Extremes_Color_Encoded
};

// Summary:
//   Auxiliary geometry render interface.
// Description:
//   Used mostly for debugging, editor purposes, the Auxiliary geometry render
//   interface provide functions to render 2d geometry and also text.
struct IRenderAuxGeom
{
    // <interfuscator:shuffle>
    virtual ~IRenderAuxGeom(){}
    // Summary:
    //   Sets render flags.
    virtual void SetRenderFlags(const SAuxGeomRenderFlags& renderFlags) = 0;
    // Summary:
    //   Gets render flags.
    virtual SAuxGeomRenderFlags GetRenderFlags() = 0;

    // 2D/3D rendering function

    // Summary:
    //   Draws a point.
    // Arguments:
    //   v          - Vector storing the position of the point.
    //   col        - Color used to draw the point.
    //   size       - Size of the point drawn.
    virtual void DrawPoint(const Vec3& v, const ColorB& col, uint8 size = 1) = 0;
    // Summary:
    //   Draws n points.
    // Arguments:
    //   v          - Pointer to a list of vector storing the position of the points.
    //   numPoints  - Number of point we will find starting from the area memory defined by v.
    //   col        - Color used to draw the points.
    //   size       - Size of the points drawn.
    //##@{
    virtual void DrawPoints(const Vec3* v, uint32 numPoints, const ColorB& col, uint8 size = 1) = 0;
    virtual void DrawPoints(const Vec3* v, uint32 numPoints, const ColorB* col, uint8 size = 1) = 0;
    //##@}

    // Summary:
    //   Draws a line.
    // Arguments:
    //   v0         - Starting vertex of the line.
    //   colV0      - Color of the first vertex.
    //   v1         - Ending vertex of the line.
    //   colV1      - Color of the second vertex.
    //   thickness  - Thickness of the line.
    virtual void DrawLine(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, float thickness = 1.0f) = 0;
    // Summary:
    //   Draws n lines.
    // Arguments:
    //   v          - List of vertexes belonging to the lines we want to draw.
    //   numPoints  - Number of the points we will find starting from the area memory defined by v.
    //   col        - Color of the vertexes.
    //   thickness  - Thickness of the line.
    //##@{
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const ColorB& col, float thickness = 1.0f) = 0;
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const ColorB* col, float thickness = 1.0f) = 0;
    //##@}

    // Summary:
    //   Draws n lines.
    // Arguments:
    //   v          - List of vertexes belonging to the lines we want to draw.
    //   numPoints  - Number of the points we will find starting from the area memory defined by v.
    //   ind        -
    //   numIndices -
    //   col        - Color of the vertexes.
    //   thickness  - Thickness of the line.
    //##@{
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col, float thickness = 1.0f) = 0;
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col, float thickness = 1.0f) = 0;
    //##@}

    // Summary:
    //   Draws a polyline.
    // Arguments:
    //   v          - List of vertexes belonging to the polyline we want to draw.
    //   numPoints  - Number of the points we will find starting from the area memory defined by v.
    //   closed     - If true a line between the last vertex and the first one is drawn.
    //   col        - Color of the vertexes.
    //   thickness  - Thickness of the line.
    //##@{
    virtual void DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB& col, float thickness = 1.0f) = 0;
    virtual void DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB* col, float thickness = 1.0f) = 0;
    //##@}

    // Summary:
    //   Draws a triangle.
    // Arguments:
    //   v0         - First vertex of the triangle.
    //   colV0      - Color of the first vertex of the triangle.
    //   v1         - Second vertex of the triangle.
    //   colV1      - Color of the second vertex of the triangle.
    //   v2         - Third vertex of the triangle.
    //   colV2      - Color of the third vertex of the triangle.
    virtual void DrawTriangle(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, const Vec3& v2, const ColorB& colV2) = 0;
    // Summary:
    //   Draws n triangles.
    // Arguments:
    //   v          - List of vertexes belonging to the sequence of triangles we have to draw.
    //   numPoints  - Number of the points we will find starting from the area memory defined by v.
    //   col        - Color of the vertexes.
    //##@{
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB& col) = 0;
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB* col) = 0;
    //##@}
    // Summary:
    //   Draws n triangles.
    // Arguments:
    //   v          - List of vertexes belonging to the sequence of triangles we have to draw.
    //   numPoints  - Number of the points we will find starting from the area memory defined by v.
    //   ind        -
    //   numIndices -
    //   col        - Color of the vertexes.
    //##@{
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col) = 0;
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col) = 0;
    //##@}

    // Summary:
    //   Draws a quad on the xz plane.
    // Arguments:
    //   pos        - Center position of the quad
    //   width      - Width of the quad.
    //   height     - Height of the quad.
    //   matWorld   - World matrix to transform the quad.
    //   col        - Color of the quad.
    //   drawShaded - True if you want to draw the quad shaded, false otherwise.
    virtual void DrawQuad(float width, float height, const Matrix34& matWorld, const ColorB& col, bool drawShaded = true) = 0;

    // Summary:
    //   Draws a Axis-aligned Bounding Boxes (AABB).
    //##@{
    virtual void DrawAABB(const AABB& aabb, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle) = 0;
    virtual void DrawAABBs(const AABB* aabbs, uint32 aabbCount, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle) = 0;
    virtual void DrawAABB(const AABB& aabb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle) = 0;
    //##@}

    // Summary:
    //   Draws a Oriented Bounding Boxes (AABB).
    //##@{
    virtual void DrawOBB(const OBB& obb, const Vec3& pos, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle) = 0;
    virtual void DrawOBB(const OBB& obb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle) = 0;
    //##@}

    // Summary:
    //   Draws a sphere.
    // Arguments:
    //   pos        - Center of the sphere.
    //   radius     - Radius of the sphere.
    //   col        - Color of the sphere.
    //   drawShaded - True if you want to draw the sphere shaded, false otherwise.
    virtual void DrawSphere(const Vec3& pos, float radius, const ColorB& col, bool drawShaded = true) = 0;

    // Summary:
    //   Draws a disk.
    // Arguments:
    //   pos        - Center of the disk.
    //   radius     - Radius of the disk.
    //   col        - Color of the disk.
    //   drawShaded - True if you want to draw the disk shaded, false otherwise.
    virtual void DrawDisk(const Vec3& pos, const Vec3& dir, float radius, const ColorB& col, bool drawShaded = true) = 0;

    // Summary:
    //   Draws a cone.
    // Arguments:
    //   pos        - Center of the base of the cone.
    //   dir        - Direction of the cone.
    //   radius     - Radius of the base of the cone.
    //   height     - Height of the cone.
    //   col        - Color of the cone.
    //   drawShaded - True if you want to draw the cone shaded, false otherwise.
    virtual void DrawCone(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded = true) = 0;
    // Summary:
    //   Draws a cylinder.
    // Arguments:
    //   pos        - Center of the base of the cylinder.
    //   dir        - Direction of the cylinder.
    //   radius     - Radius of the base of the cylinder.
    //   height     - Height of the cylinder.
    //   col        - Color of the cylinder.
    //   drawShaded - True if you want to draw the cylinder shaded, false otherwise.
    virtual void DrawCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded = true) = 0;

    // Summary:
    //   Draws bones.
    //##@{
    virtual void DrawBone(const Vec3& rParent, const Vec3& rBone, ColorB col) = 0;
    //##@}

    // Summary:
    //   Draws Text.
    //##@{
    virtual void RenderText(Vec3 pos, SDrawTextInfo& ti, const char* format, va_list args) = 0;

    // Summary:
    //   Draws 3d Label.
    //##@{
    void Draw3dLabel(Vec3 pos, float font_size, const ColorF& color, const char* label_text, ...) PRINTF_PARAMS(5, 6)
    {
        va_list args;
        va_start(args, label_text);

        SDrawTextInfo ti;
        ti.xscale = ti.yscale = font_size;
        ti.color[0] = color[0];
        ti.color[1] = color[1];
        ti.color[2] = color[2];
        ti.color[3] = color[3];
        ti.flags = eDrawText_2D | eDrawText_FixedSize | eDrawText_Monospace;

        RenderText(pos, ti, label_text, args);

        va_end(args);
    }

    void Draw2dLabelInternal(float x, float y, float font_size, const float* pfColor, int flags, const char* format, va_list args)
    {
        SDrawTextInfo ti;
        ti.xscale = ti.yscale = font_size;
        ti.flags = (eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | flags);
        if (pfColor)
        {
            ti.color[0] = pfColor[0];
            ti.color[1] = pfColor[1];
            ti.color[2] = pfColor[2];
            ti.color[3] = pfColor[3];
        }

        RenderText(Vec3(x, y, 0.5f), ti, format, args);
    }

    void Draw2dLabel(float x, float y, float font_size, const float* pfColor, bool bCenter, const char* label_text, ...) PRINTF_PARAMS(7, 8)
    {
        va_list args;
        va_start(args, label_text);
        Draw2dLabelInternal(x, y, font_size, pfColor, (bCenter ? eDrawText_Center : eDrawText_Left), label_text, args);
        va_end(args);
    }

    void Draw2dLabel(float x, float y, float font_size, const ColorF& fColor, bool bCenter, const char* label_text, ...) PRINTF_PARAMS(7, 8)
    {
        const AZStd::array<float, 4> color = fColor.GetAsArray();
        va_list args;
        va_start(args, label_text);
        Draw2dLabelInternal(x, y, font_size, color.data(), (bCenter ? eDrawText_Center : eDrawText_Left), label_text, args);
        va_end(args);
    }

    void Draw2dLabelCustom(float x, float y, float font_size, const float* pfColor, int flags, const char* label_text, ...) PRINTF_PARAMS(7, 8)
    {
        va_list args;
        va_start(args, label_text);
        Draw2dLabelInternal(x, y, font_size, pfColor, flags, label_text, args);
        va_end(args);
    }

    void Draw2dLabelCustom(float x, float y, float font_size, const ColorF& fColor, int flags, const char* label_text, ...) PRINTF_PARAMS(7, 8)
    {
        const AZStd::array<float, 4> color = fColor.GetAsArray();
        va_list args;
        va_start(args, label_text);
        Draw2dLabelInternal(x, y, font_size, color.data(), flags, label_text, args);
        va_end(args);
    }
    //##@}

    // Summary:
    //   If possible flushes all elements stored on the buffer to rendering system.
    //   Note 1: rendering system may start processing flushed commands immediately or postpone it till Commit() call
    //   Note 2: worker threads's commands are always postponed till Commit() call
    //
    virtual void Flush() = 0;
    // </interfuscator:shuffle>

    // Summary:
    //   Flushes yet unprocessed elements and notifies rendering system that issuing rendering commands for current frame is done and frame is ready to be drawn
    //   Thus Commit() guarantees that all previously issued commands will appear on the screen
    //   Each thread rendering AUX geometry MUST call Commit() at the end of drawing cycle/frame
    //   "frames" indicate how many frames current commands butch must be presented on screen unless there till next butch is ready.
    //   for render and main thread this parameter has no effect
    virtual void Commit(uint frames = 0) = 0;
    // </interfuscator:shuffle>

    /**
    * Processes and resets the underlying vertex buffer
    */
    virtual void Process() = 0;
};

// Notes:
//   Don't change the xxxShift values blindly as they affect the rendering output
//   that is two primitives have to be rendered after 3d primitives, alpha blended
//   geometry have to be rendered after opaque ones, etc.
//   This also applies to the individual flags in EAuxGeomPublicRenderflags_*!
// Remarks:
//   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
//   Check RenderAuxGeom.h in ../RenderDll/Common
enum EAuxGeomPublicRenderflagBitMasks
{
    e_Mode2D3DShift             = 31,
    e_Mode2D3DMask              = 0x1 << e_Mode2D3DShift,

    e_AlphaBlendingShift        = 29,
    e_AlphaBlendingMask         = 0x3 << e_AlphaBlendingShift,

    e_DrawInFrontShift          = 28,
    e_DrawInFrontMask           = 0x1 << e_DrawInFrontShift,

    e_FillModeShift             = 26,
    e_FillModeMask              = 0x3 << e_FillModeShift,

    e_CullModeShift             = 24,
    e_CullModeMask              = 0x3 << e_CullModeShift,

    e_DepthWriteShift           = 23,
    e_DepthWriteMask            = 0x1 << e_DepthWriteShift,

    e_DepthTestShift            = 22,
    e_DepthTestMask             = 0x1 << e_DepthTestShift,

    e_PublicParamsMask      = e_Mode2D3DMask | e_AlphaBlendingMask | e_DrawInFrontMask | e_FillModeMask |
        e_CullModeMask | e_DepthWriteMask | e_DepthTestMask
};

// Notes:
//   e_Mode2D renders in normalized [0.. 1] screen space.
//   Don't change the xxxShift values blindly as they affect the rendering output
//   that is two primitives have to be rendered after 3d primitives, alpha blended
//   geometry have to be rendered after opaque ones, etc.
//   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
// Remarks:
//   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
//   Check RenderAuxGeom.h in ../RenderDll/Common
// See also:
//   EAuxGeomPublicRenderflagBitMasks
enum EAuxGeomPublicRenderflags_Mode2D3D
{
    e_Mode3D                            = 0x0 << e_Mode2D3DShift,
    e_Mode2D                            = 0x1 << e_Mode2D3DShift,
};

// Notes:
//   Don't change the xxxShift values blindly as they affect the rendering output
//   that is two primitives have to be rendered after 3d primitives, alpha blended
//   geometry have to be rendered after opaque ones, etc.
//   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
// Remarks:
//   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
//   Check RenderAuxGeom.h in ../RenderDll/Common
// See also:
//   EAuxGeomPublicRenderflagBitMasks
enum EAuxGeomPublicRenderflags_AlphaBlendMode
{
    e_AlphaNone                     = 0x0 << e_AlphaBlendingShift,
    e_AlphaAdditive             = 0x1 << e_AlphaBlendingShift,
    e_AlphaBlended              = 0x2 << e_AlphaBlendingShift,
};

// Notes:
//   Don't change the xxxShift values blindly as they affect the rendering output
//   that is two primitives have to be rendered after 3d primitives, alpha blended
//   geometry have to be rendered after opaque ones, etc.
//   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
// Remarks:
//   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
//   Check RenderAuxGeom.h in ../RenderDll/Common
// See also:
//   EAuxGeomPublicRenderflagBitMasks
enum EAuxGeomPublicRenderflags_DrawInFrontMode
{
    e_DrawInFrontOff            = 0x0 << e_DrawInFrontShift,
    e_DrawInFrontOn             = 0x1 << e_DrawInFrontShift,
};

// Notes:
//   Don't change the xxxShift values blindly as they affect the rendering output
//   that is two primitives have to be rendered after 3d primitives, alpha blended
//   geometry have to be rendered after opaque ones, etc.
//   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
// Remarks:
//   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
//   Check RenderAuxGeom.h in ../RenderDll/Common
// See also:
//   EAuxGeomPublicRenderflagBitMasks
enum EAuxGeomPublicRenderflags_FillMode
{
    e_FillModeSolid             = 0x0 << e_FillModeShift,
    e_FillModeWireframe     = 0x1 << e_FillModeShift,
    e_FillModePoint             = 0x2 << e_FillModeShift,
};

// Notes:
//   Don't change the xxxShift values blindly as they affect the rendering output
//   that is two primitives have to be rendered after 3d primitives, alpha blended
//   geometry have to be rendered after opaque ones, etc.
//   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
// Remarks:
//   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
//   Check RenderAuxGeom.h in ../RenderDll/Common
// See also:
//   EAuxGeomPublicRenderflagBitMasks
enum EAuxGeomPublicRenderflags_CullMode
{
    e_CullModeNone              = 0x0 << e_CullModeShift,
    e_CullModeFront             = 0x1 << e_CullModeShift,
    e_CullModeBack              = 0x2 << e_CullModeShift,
};

// Notes:
//   Don't change the xxxShift values blindly as they affect the rendering output
//   that is two primitives have to be rendered after 3d primitives, alpha blended
//   geometry have to be rendered after opaque ones, etc.
//   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
// Remarks:
//   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
//   Check RenderAuxGeom.h in ../RenderDll/Common
// See also:
//   EAuxGeomPublicRenderflagBitMasks
enum EAuxGeomPublicRenderflags_DepthWrite
{
    e_DepthWriteOn              = 0x0 << e_DepthWriteShift,
    e_DepthWriteOff             = 0x1 << e_DepthWriteShift,
};

// Notes:
//   Don't change the xxxShift values blindly as they affect the rendering output
//   that is two primitives have to be rendered after 3d primitives, alpha blended
//   geometry have to be rendered after opaque ones, etc.
//   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
// Remarks:
//   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
//   Check RenderAuxGeom.h in ../RenderDll/Common
// See also:
//   EAuxGeomPublicRenderflagBitMasks
enum EAuxGeomPublicRenderflags_DepthTest
{
    e_DepthTestOn               = 0x0 << e_DepthTestShift,
    e_DepthTestOff              = 0x1 << e_DepthTestShift,
};


enum EAuxGeomPublicRenderflags_Defaults
{
    // Default render flags for 3d primitives.
    e_Def3DPublicRenderflags = e_Mode3D | e_AlphaNone | e_DrawInFrontOff | e_FillModeSolid |
        e_CullModeBack | e_DepthWriteOn | e_DepthTestOn,
};


struct SAuxGeomRenderFlags
{
    uint32 m_renderFlags;

    SAuxGeomRenderFlags();
    SAuxGeomRenderFlags(const SAuxGeomRenderFlags& rhs);
    SAuxGeomRenderFlags(uint32 renderFlags);
    SAuxGeomRenderFlags& operator =(const SAuxGeomRenderFlags& rhs);
    SAuxGeomRenderFlags& operator =(uint32 rhs);

    bool operator ==(const SAuxGeomRenderFlags& rhs) const;
    bool operator ==(uint32 rhs) const;
    bool operator !=(const SAuxGeomRenderFlags& rhs) const;
    bool operator !=(uint32 rhs) const;

    void SetDrawInFrontMode(const EAuxGeomPublicRenderflags_DrawInFrontMode& state);

    // Summary:
    //   Sets the flags for the filling mode.
    void SetFillMode(const EAuxGeomPublicRenderflags_FillMode& state);

    // Summary:
    //   Sets the flags for the culling mode.
    void SetCullMode(const EAuxGeomPublicRenderflags_CullMode& state);
};


inline
SAuxGeomRenderFlags::SAuxGeomRenderFlags()
    : m_renderFlags(e_Def3DPublicRenderflags)
{
}

inline
SAuxGeomRenderFlags::SAuxGeomRenderFlags(const SAuxGeomRenderFlags& rhs)
    : m_renderFlags(rhs.m_renderFlags)
{
}


inline
SAuxGeomRenderFlags::SAuxGeomRenderFlags(uint32 renderFlags)
    : m_renderFlags(renderFlags)
{
}


inline SAuxGeomRenderFlags&
SAuxGeomRenderFlags::operator =(const SAuxGeomRenderFlags& rhs)
{
    m_renderFlags = rhs.m_renderFlags;
    return(*this);
}


inline SAuxGeomRenderFlags&
SAuxGeomRenderFlags::operator =(uint32 rhs)
{
    m_renderFlags = rhs;
    return(*this);
}


inline bool
SAuxGeomRenderFlags::operator ==(const SAuxGeomRenderFlags& rhs) const
{
    return(m_renderFlags == rhs.m_renderFlags);
}


inline bool
SAuxGeomRenderFlags::operator ==(uint32 rhs) const
{
    return(m_renderFlags == rhs);
}


inline bool
SAuxGeomRenderFlags::operator !=(const SAuxGeomRenderFlags& rhs) const
{
    return(m_renderFlags != rhs.m_renderFlags);
}


inline bool
SAuxGeomRenderFlags::operator !=(uint32 rhs) const
{
    return(m_renderFlags != rhs);
}


inline void
SAuxGeomRenderFlags::SetDrawInFrontMode(const EAuxGeomPublicRenderflags_DrawInFrontMode& state)
{
    m_renderFlags &= ~e_DrawInFrontMask;
    m_renderFlags |= state;
}

inline void
SAuxGeomRenderFlags::SetFillMode(const EAuxGeomPublicRenderflags_FillMode& state)
{
    m_renderFlags &= ~e_FillModeMask;
    m_renderFlags |= state;
}

inline void
SAuxGeomRenderFlags::SetCullMode(const EAuxGeomPublicRenderflags_CullMode& state)
{
    m_renderFlags &= ~e_CullModeMask;
    m_renderFlags |= state;
}
