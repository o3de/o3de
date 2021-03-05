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

#include "RenderDll_precompiled.h"
#include "NULL_Renderer.h"
#include "NULLRenderAuxGeom.h"

// CryTek undefs some defines from WinBase.h...so let's redefine them here
#if defined(WIN32) || defined(WIN64)
  #ifdef UNICODE
    #define RegisterClass  RegisterClassW
  #else
    #define RegisterClass  RegisterClassA
  #endif // !UNICODE
#endif

CNULLRenderAuxGeom* CNULLRenderAuxGeom::s_pThis = NULL;


#ifdef ENABLE_WGL_DEBUG_RENDERER

static const float W = 800.0f;
static const float H = 600.0f;
static const float THETA = 5.0f;
static const Vec3 VUP(0.0f, 0.0f, 1.0f);

bool CNULLRenderAuxGeom::s_active = false;
bool CNULLRenderAuxGeom::s_hidden = true;

void CNULLRenderAuxGeom::EnableOpenGL()
{
    CCamera& camera = gEnv->pSystem->GetViewCamera();
    camera.SetFrustum(W, H);

    const float FOV = camera.GetFov() / PI * 180.0f;
    const float PNR = camera.GetNearPlane();
    const float PFR = camera.GetFarPlane();

    PIXELFORMATDESCRIPTOR pfd;
    int format;

    // get the device context (DC)
    m_hdc = GetDC(m_hwnd);

    // set the pixel format for the DC
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;
    format = ChoosePixelFormat(m_hdc, &pfd);
    SetPixelFormat(m_hdc, format, &pfd);

    // create and enable the render context (RC)
    m_glrc = wglCreateContext(m_hdc);
    wglMakeCurrent(m_hdc, m_glrc);

    m_qobj = gluNewQuadric();

    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT, GL_FILL);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, W, H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, W / H, PNR, PFR);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void CNULLRenderAuxGeom::DisableOpenGL()
{
    gluDeleteQuadric(m_qobj);

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(m_glrc);
    ReleaseDC(m_hwnd, m_hdc);
}

#endif

CNULLRenderAuxGeom::CNULLRenderAuxGeom(CNULLRenderer& renderer)
    : m_renderer(&renderer)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    const char* wndClassName = "DebugRenderer";

    // register window class
    WNDCLASS wc;
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = wndClassName;
    RegisterClass(&wc);

    // create main window
    m_hwnd = CreateWindow(
            wndClassName, wndClassName,
            WS_CAPTION | WS_POPUP,
            0, 0, W, H,
            NULL, NULL, wc.hInstance, NULL);

    ShowWindow(m_hwnd, SW_HIDE);
    UpdateWindow(m_hwnd);

    EnableOpenGL();

    m_eye.Set(0.0f, 0.0f, 0.0f);
    m_dir.Set(0.0f, 1.0f, 0.0f);
    m_up.Set(0.0f, 0.0f, 1.0f);
    m_updateSystemView = true;

    REGISTER_COMMAND("r_debug_renderer_show_window", DebugRendererShowWindow, VF_NULL, "");
    REGISTER_COMMAND("r_debug_renderer_set_eye_pos", DebugRendererSetEyePos, VF_NULL, "");
    REGISTER_COMMAND("r_debug_renderer_update_system_view", DebugRendererUpdateSystemView, VF_NULL, "");
#endif
}

CNULLRenderAuxGeom::~CNULLRenderAuxGeom()
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    DisableOpenGL();
    DestroyWindow(m_hwnd);
#endif
}

void CNULLRenderAuxGeom::BeginFrame()
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);

    {
        m_dir.normalize();
        m_up.normalize();

        Vec3 right = m_dir ^ m_up;

        if (s_active)
        {
            Matrix34 m;

            m.SetIdentity();
            if (GetAsyncKeyState('W') & 0x8000)
            {
                m.AddTranslation(m_dir);
            }
            if (GetAsyncKeyState('S') & 0x8000)
            {
                m.AddTranslation(-m_dir);
            }
            if (GetAsyncKeyState('A') & 0x8000)
            {
                m.AddTranslation(-right);
            }
            if (GetAsyncKeyState('D') & 0x8000)
            {
                m.AddTranslation(right);
            }
            m_eye = m * m_eye;

            m.SetIdentity();
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
            {
                m.SetRotationAA(-PI / 180.0f * THETA, VUP); // !m_up
            }
            if (GetAsyncKeyState(VK_LEFT) & 0x8000)
            {
                m.SetRotationAA(PI / 180.0f * THETA, VUP); // !m_up
            }
            if (GetAsyncKeyState(VK_UP) & 0x8000)
            {
                m.SetRotationAA(PI / 180.0f * THETA, right);
            }
            if (GetAsyncKeyState(VK_DOWN) & 0x8000)
            {
                m.SetRotationAA(-PI / 180.0f * THETA, right);
            }
            m_up = m * m_up;
            m_dir = m * m_dir;
        }

        if (m_updateSystemView)
        {
            Matrix34 m(Matrix33::CreateOrientation(m_dir, m_up, 0), m_eye);
            gEnv->pSystem->GetViewCamera().SetMatrix(m);
        }
        else
        {
            const Matrix34& viewMatrix = gEnv->pSystem->GetViewCamera().GetMatrix();
            m_eye = viewMatrix.GetTranslation();
            m_dir = viewMatrix.GetColumn1();
            m_up  = viewMatrix.GetColumn2();
        }
    }

#endif
}

void CNULLRenderAuxGeom::EndFrame()
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);

    if (!s_hidden)
    {
        glLoadIdentity();

        Vec3 at = m_eye + m_dir;
        gluLookAt(m_eye.x, m_eye.y, m_eye.z, at.x, at.y, at.z, m_up.x, m_up.y, m_up.z);

        glClearColor(0.0f, 0.0f, 0.3f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glColor3f(1.0f, 0.0f, 0.0f);
        gluSphere(m_qobj, 1.0f, 32, 32);

        glInterleavedArrays(GL_C3F_V3F, 0, &m_points[0]);
        glDrawArrays(GL_POINTS, 0, m_points.size());

        glInterleavedArrays(GL_C3F_V3F, 0, &m_lines[0]);
        glDrawArrays(GL_LINES, 0, m_lines.size() * 2);

        for (size_t i = 0; i < m_polyLines.size(); ++i)
        {
            const SPolyLine& polyline = m_polyLines[i];
            glInterleavedArrays(GL_C3F_V3F, 0, &polyline.points[0]);
            glDrawArrays(GL_LINE_STRIP, 0, polyline.points.size());
        }

        glInterleavedArrays(GL_C3F_V3F, 0, &m_triangles[0]);
        glDrawArrays(GL_TRIANGLES, 0, m_triangles.size() * 3);

        for (size_t i = 0; i < m_spheres.size(); ++i)
        {
            glColor3fv(m_spheres[i].p.color);
            glPushMatrix();
            glLoadIdentity();
            glTranslatef(m_spheres[i].p.vertex[0], m_spheres[i].p.vertex[1], m_spheres[i].p.vertex[2]);
            gluSphere(m_qobj, m_spheres[i].r, 32, 32);
            glPopMatrix();
        }

        glFlush();

        SwapBuffers(m_hdc);
    }

    m_points.resize(0);
    m_lines.resize(0);
    m_polyLines.resize(0);
    m_triangles.resize(0);
    m_spheres.resize(0);
#endif
}

void CNULLRenderAuxGeom::DrawPoint([[maybe_unused]] const Vec3& v, [[maybe_unused]] const ColorB& col, [[maybe_unused]] uint8 size /* = 1  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    m_points.push_back(SPoint(v, col));
#endif
}

void CNULLRenderAuxGeom::DrawPoints([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const ColorB* col, [[maybe_unused]] uint8 size /* = 1  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    for (uint32 i = 0; i < numPoints; ++i)
    {
        m_points.push_back(SPoint(v[i], *col));
    }
#endif
}

void CNULLRenderAuxGeom::DrawPoints([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const ColorB& col, [[maybe_unused]] uint8 size /* = 1  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    for (uint32 i = 0; i < numPoints; ++i)
    {
        m_points.push_back(SPoint(v[i], col));
    }
#endif
}

void CNULLRenderAuxGeom::DrawLine([[maybe_unused]] const Vec3& v0, [[maybe_unused]] const ColorB& colV0, [[maybe_unused]] const Vec3& v1, [[maybe_unused]] const ColorB& colV1, [[maybe_unused]] float thickness /* = 1::0f  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    m_lines.push_back(SLine(SPoint(v0, colV0), SPoint(v1, colV1)));
#endif
}

void CNULLRenderAuxGeom::DrawLines([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const ColorB& col, [[maybe_unused]] float thickness /* = 1::0f  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert((numPoints >= 2) && (0 == (numPoints & 1)));
    for (uint32 i = 0; i < numPoints; i += 2)
    {
        m_lines.push_back(SLine(SPoint(v[i], col), SPoint(v[i + 1], col)));
    }
#endif
}

void CNULLRenderAuxGeom::DrawLines([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const ColorB* col, [[maybe_unused]] float thickness /* = 1::0f  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert((numPoints >= 2) && (0 == (numPoints & 1)));
    for (uint32 i = 0; i < numPoints; i += 2)
    {
        m_lines.push_back(SLine(SPoint(v[i], *col), SPoint(v[i + 1], *col)));
    }
#endif
}

void CNULLRenderAuxGeom::DrawLines([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const vtx_idx* ind, [[maybe_unused]] uint32 numIndices, [[maybe_unused]] const ColorB& col, [[maybe_unused]] float thickness /* = 1::0f  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert(numPoints >= 2);
    assert((numIndices >= 2) && (0 == (numIndices & 1)));
    for (uint32 i = 0; i < numIndices; i += 2)
    {
        vtx_idx i0 = ind[i], i1 = ind[i + 1];
        assert(i0 < numPoints && i1 < numPoints);
        m_lines.push_back(SLine(SPoint(v[i0], col), SPoint(v[i1], col)));
    }
#endif
}

void CNULLRenderAuxGeom::DrawLines([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const vtx_idx* ind, [[maybe_unused]] uint32 numIndices, [[maybe_unused]] const ColorB* col, [[maybe_unused]] float thickness /* = 1::0f  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert(numPoints >= 2);
    assert((numIndices >= 2) && (0 == (numIndices & 1)));
    for (uint32 i = 0; i < numIndices; i += 2)
    {
        vtx_idx i0 = ind[i], i1 = ind[i + 1];
        assert(i0 < numPoints && i1 < numPoints);
        m_lines.push_back(SLine(SPoint(v[i0], *col), SPoint(v[i1], *col)));
    }
#endif
}

void CNULLRenderAuxGeom::DrawPolyline([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] bool closed, [[maybe_unused]] const ColorB& col, [[maybe_unused]] float thickness /* = 1::0f  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert(numPoints >= 2);
    assert(!closed || numPoints >= 3);   // if "closed" then we need at least three vertices
    m_polyLines.resize(m_polyLines.size() + 1);
    SPolyLine& polyline = m_polyLines[m_polyLines.size() - 1];
    for (uint32 i = 0; i < numPoints; ++i)
    {
        polyline.points.push_back(SPoint(v[i], col));
    }
    if (closed)
    {
        polyline.points.push_back(SPoint(v[0], col));
    }
#endif
}

void CNULLRenderAuxGeom::DrawPolyline([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] bool closed, [[maybe_unused]] const ColorB* col, [[maybe_unused]] float thickness /* = 1::0f  */)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert(numPoints >= 2);
    assert(!closed || numPoints >= 3);   // if "closed" then we need at least three vertices
    m_polyLines.resize(m_polyLines.size() + 1);
    SPolyLine& polyline = m_polyLines[m_polyLines.size() - 1];
    for (uint32 i = 0; i < numPoints; ++i)
    {
        polyline.points.push_back(SPoint(v[i], *col));
    }
    if (closed)
    {
        polyline.points.push_back(SPoint(v[0], *col));
    }
#endif
}

void CNULLRenderAuxGeom::DrawTriangle([[maybe_unused]] const Vec3& v0, [[maybe_unused]] const ColorB& colV0, [[maybe_unused]] const Vec3& v1, [[maybe_unused]] const ColorB& colV1, [[maybe_unused]] const Vec3& v2, [[maybe_unused]] const ColorB& colV2)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    m_triangles.push_back(STriangle(SPoint(v0, colV0), SPoint(v1, colV1), SPoint(v2, colV2)));
#endif
}

void CNULLRenderAuxGeom::DrawTriangles([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const ColorB& col)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert((numPoints >= 3) && (0 == (numPoints % 3)));
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    for (size_t i = 0; i < numPoints; i += 3)
    {
        m_triangles.push_back(STriangle(SPoint(v[i], col), SPoint(v[i + 1], col), SPoint(v[i + 2], col)));
    }
#endif
}

void CNULLRenderAuxGeom::DrawTriangles([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const ColorB* col)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert((numPoints >= 3) && (0 == (numPoints % 3)));
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    for (size_t i = 0; i < numPoints; i += 3)
    {
        m_triangles.push_back(STriangle(SPoint(v[i], *col), SPoint(v[i + 1], *col), SPoint(v[i + 2], *col)));
    }
#endif
}

void CNULLRenderAuxGeom::DrawTriangles([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const vtx_idx* ind, [[maybe_unused]] uint32 numIndices, [[maybe_unused]] const ColorB& col)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert(numPoints >= 3);
    assert((numIndices >= 3) && (0 == (numIndices % 3)));
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    for (size_t i = 0; i < numIndices; i += 3)
    {
        vtx_idx i0 = ind[i], i1 = ind[i + 1], i2 = ind[i + 2];
        assert(i0 < numPoints && i1 < numPoints && i2 < numPoints);
        m_triangles.push_back(STriangle(SPoint(v[i0], col), SPoint(v[i1], col), SPoint(v[i2], col)));
    }
#endif
}

void CNULLRenderAuxGeom::DrawTriangles([[maybe_unused]] const Vec3* v, [[maybe_unused]] uint32 numPoints, [[maybe_unused]] const vtx_idx* ind, [[maybe_unused]] uint32 numIndices, [[maybe_unused]] const ColorB* col)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    assert(numPoints >= 3);
    assert((numIndices >= 3) && (0 == (numIndices % 3)));
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    for (size_t i = 0; i < numIndices; i += 3)
    {
        vtx_idx i0 = ind[i], i1 = ind[i + 1], i2 = ind[i + 2];
        assert(i0 < numPoints && i1 < numPoints && i2 < numPoints);
        m_triangles.push_back(STriangle(SPoint(v[i0], *col), SPoint(v[i1], *col), SPoint(v[i2], *col)));
    }
#endif
}

void CNULLRenderAuxGeom::DrawSphere([[maybe_unused]] const Vec3& pos, [[maybe_unused]] float radius, [[maybe_unused]] const ColorB& col, [[maybe_unused]] bool drawShaded)
{
#ifdef ENABLE_WGL_DEBUG_RENDERER
    m_spheres.push_back(SSphere(SPoint(pos, col), radius));
#endif
}

