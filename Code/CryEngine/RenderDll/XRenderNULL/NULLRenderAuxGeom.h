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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERNULL_NULLRENDERAUXGEOM_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERNULL_NULLRENDERAUXGEOM_H
#pragma once
#include "../Common/RenderAuxGeom.h"

class CNULLRenderer;
class ICrySizer;

#if (defined(WIN32) || defined(WIN64)) && !defined(_RELEASE)
#define ENABLE_WGL_DEBUG_RENDERER
#endif

#ifdef ENABLE_WGL_DEBUG_RENDERER
#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#endif

class CNULLRenderAuxGeom
    : public IRenderAuxGeom
{
public:
    // interface
    virtual void SetRenderFlags([[maybe_unused]] const SAuxGeomRenderFlags& renderFlags) {}
    virtual SAuxGeomRenderFlags GetRenderFlags() { return SAuxGeomRenderFlags(); }

    virtual void Flush() {}
    virtual void Commit([[maybe_unused]] uint frames = 0) {}
    virtual void Process() {}

    virtual void DrawPoint(const Vec3& v, const ColorB& col, uint8 size = 1);
    virtual void DrawPoints(const Vec3* v, uint32 numPoints, const ColorB& col, uint8 size = 1);
    virtual void DrawPoints(const Vec3* v, uint32 numPoints, const ColorB* col, uint8 size = 1);

    virtual void DrawLine(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, float thickness = 1.0f);
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const ColorB& col, float thickness = 1.0f);
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const ColorB* col, float thickness = 1.0f);
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col, float thickness = 1.0f);
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col, float thickness = 1.0f);
    virtual void DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB& col, float thickness = 1.0f);
    virtual void DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB* col, float thickness = 1.0f);

    virtual void DrawTriangle(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, const Vec3& v2, const ColorB& colV2);
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB& col);
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB* col);
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col);
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col);

    virtual void DrawQuad([[maybe_unused]] float width, [[maybe_unused]] float height, [[maybe_unused]] const Matrix34& matWorld, [[maybe_unused]] const ColorB& col, [[maybe_unused]] bool drawShaded = true) {}

    virtual void DrawAABB([[maybe_unused]] const AABB& aabb, [[maybe_unused]] bool bSolid, [[maybe_unused]] const ColorB& col, [[maybe_unused]] const EBoundingBoxDrawStyle& bbDrawStyle) {}
    virtual void DrawAABBs([[maybe_unused]] const AABB* aabb, [[maybe_unused]] uint32 aabbCount, [[maybe_unused]] bool bSolid, [[maybe_unused]] const ColorB& col, [[maybe_unused]] const EBoundingBoxDrawStyle& bbDrawStyle) {}
    virtual void DrawAABB([[maybe_unused]] const AABB& aabb, [[maybe_unused]] const Matrix34& matWorld, [[maybe_unused]] bool bSolid, [[maybe_unused]] const ColorB& col, [[maybe_unused]] const EBoundingBoxDrawStyle& bbDrawStyle) {}

    virtual void DrawOBB([[maybe_unused]] const OBB& obb, [[maybe_unused]] const Vec3& pos, [[maybe_unused]] bool bSolid, [[maybe_unused]] const ColorB& col, [[maybe_unused]] const EBoundingBoxDrawStyle& bbDrawStyle) {}
    virtual void DrawOBB([[maybe_unused]] const OBB& obb, [[maybe_unused]] const Matrix34& matWorld, [[maybe_unused]] bool bSolid, [[maybe_unused]] const ColorB& col, [[maybe_unused]] const EBoundingBoxDrawStyle& bbDrawStyle) {}

    virtual void DrawSphere(const Vec3& pos, float radius, const ColorB& col, bool drawShaded = true);
    virtual void DrawDisk([[maybe_unused]] const Vec3& pos, [[maybe_unused]] const Vec3& dir, [[maybe_unused]] float radius, [[maybe_unused]] const ColorB& col, [[maybe_unused]] bool drawShaded = true) {}
    virtual void DrawCone([[maybe_unused]] const Vec3& pos, [[maybe_unused]] const Vec3& dir, [[maybe_unused]] float radius, [[maybe_unused]] float height, [[maybe_unused]] const ColorB& col, [[maybe_unused]] bool drawShaded = true) {}
    virtual void DrawCylinder([[maybe_unused]] const Vec3& pos, [[maybe_unused]] const Vec3& dir, [[maybe_unused]] float radius, [[maybe_unused]] float height, [[maybe_unused]] const ColorB& col, [[maybe_unused]] bool drawShaded = true) {}

    virtual void DrawBone([[maybe_unused]] const Vec3& rParent, [[maybe_unused]] const Vec3& rBone, [[maybe_unused]] ColorB col) {}

    virtual void RenderText([[maybe_unused]] Vec3 pos, [[maybe_unused]] SDrawTextInfo& ti, [[maybe_unused]] const char* forma, [[maybe_unused]] va_list args) {}

public:
    static CNULLRenderAuxGeom* Create(CNULLRenderer& renderer)
    {
        if (s_pThis == NULL)
        {
            s_pThis = new CNULLRenderAuxGeom(renderer);
        }
        return s_pThis;
    }

public:
    ~CNULLRenderAuxGeom();

    void BeginFrame();
    void EndFrame();

private:
    CNULLRenderAuxGeom(CNULLRenderer& renderer);

    CNULLRenderer* m_renderer;

    static CNULLRenderAuxGeom* s_pThis;

#ifdef ENABLE_WGL_DEBUG_RENDERER
    static void DebugRendererShowWindow([[maybe_unused]] IConsoleCmdArgs* args)
    {
        if (!s_pThis)
        {
            return;
        }

        ShowWindow(s_pThis->m_hwnd, s_hidden ? SW_SHOWNA : SW_HIDE);
        s_hidden = !s_hidden;
    }

    static void DebugRendererSetEyePos(IConsoleCmdArgs* args)
    {
        if (!s_pThis)
        {
            return;
        }

        if (args->GetArgCount() != 4)
        {
            return;
        }

        Vec3 pos;
        pos.x = (float)atof(args->GetArg(1));
        pos.y = (float)atof(args->GetArg(2));
        pos.z = (float)atof(args->GetArg(3));

        s_pThis->m_eye = pos;
    }

    static void DebugRendererUpdateSystemView(IConsoleCmdArgs* args)
    {
        if (!s_pThis)
        {
            return;
        }

        if (args->GetArgCount() != 2)
        {
            return;
        }

        s_pThis->m_updateSystemView = ((int)atoi(args->GetArg(1)) != 0);
    }

    HWND m_hwnd;
    HDC m_hdc;
    HGLRC m_glrc;

    GLUquadricObj* m_qobj;

    // Window Procedure
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
            return 0;

        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            return 0;

        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_INACTIVE)
            {
                s_active = true;
            }
            else
            {
                s_active = false;
            }
            return 0;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }

    void EnableOpenGL();
    void DisableOpenGL();

    struct SPoint // GL_C3F_V3F
    {
        GLfloat color[3]; // rgb
        GLfloat vertex[3]; // xyz

        SPoint() {}

        SPoint(const Vec3& v, const ColorB& c)
        {
            color[0] = c.r / 255.f;
            color[1] = c.g / 255.f;
            color[2] = c.b / 255.f;
            vertex[0] = v.x;
            vertex[1] = v.y;
            vertex[2] = v.z;
        }

        const SPoint& operator=(const SPoint& rhs)
        {
            memcpy(color, rhs.color, sizeof(color));
            memcpy(vertex, rhs.vertex, sizeof(vertex));
            return *this;
        }
    };

    std::vector<SPoint> m_points;

    struct SLine
    {
        SPoint points[2];

        SLine() {}
        SLine(const SPoint& p0, const SPoint& p1)
        {
            points[0] = p0;
            points[1] = p1;
        }
    };

    std::vector<SLine> m_lines;

    struct SPolyLine
    {
        std::vector<SPoint> points;
    };

    std::vector<SPolyLine> m_polyLines;

    struct STriangle
    {
        SPoint points[3];

        STriangle() {}
        STriangle(const SPoint& p0, const SPoint& p1, const SPoint& p2)
        {
            points[0] = p0;
            points[1] = p1;
            points[2] = p2;
        }
    };

    std::vector<STriangle> m_triangles;

    struct SSphere
    {
        SPoint p;
        float r;

        SSphere() {}
        SSphere(const SPoint& p_, float r_)
            : p(p_)
            , r(r_) {}
    };

    std::vector<SSphere> m_spheres;

    Vec3 m_eye, m_dir, m_up;
    bool m_updateSystemView;

    static bool s_active;
    static bool s_hidden;
#endif
};

#endif // NULL_RENDER_AUX_GEOM_H
