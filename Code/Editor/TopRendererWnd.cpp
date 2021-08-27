/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TopRendererWnd.h"
// Editor
#include "Settings.h"

#include "DisplaySettings.h"
#include "ViewManager.h"

// Size of the surface texture
#define SURFACE_TEXTURE_WIDTH 512

#define MARKER_SIZE 6.0f
#define MARKER_DIR_SIZE 10.0f
#define SELECTION_RADIUS 30.0f

#define GL_RGBA 0x1908
#define GL_BGRA 0x80E1

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

QTopRendererWnd::QTopRendererWnd(QWidget* parent)
    : Q2DViewport(parent)
{
    ////////////////////////////////////////////////////////////////////////
    // Set the window type member of the base class to the correct type and
    // create the initial surface texture
    ////////////////////////////////////////////////////////////////////////

    if (gSettings.viewports.bTopMapSwapXY)
    {
        SetAxis(VPA_YX);
    }
    else
    {
        SetAxis(VPA_XY);
    }

    m_bShowHeightmap = false;
    m_bShowStatObjects = false;
    m_bLastShowHeightmapState = false;

    ////////////////////////////////////////////////////////////////////////
    // Surface texture
    ////////////////////////////////////////////////////////////////////////

    m_textureSize.setWidth(gSettings.viewports.nTopMapTextureResolution);
    m_textureSize.setHeight(gSettings.viewports.nTopMapTextureResolution);

    m_heightmapSize = QSize(1, 1);

    m_terrainTextureId = 0;

    m_vegetationTextureId = 0;
    m_bFirstTerrainUpdate = true;
    m_bShowWater = false;

    m_bContentsUpdated = false;

    m_gridAlpha = 0.3f;
    m_colorGridText = QColor(255, 255, 255);
    m_colorAxisText = QColor(255, 255, 255);
    m_colorBackground = QColor(128, 128, 128);

    //For this viewport 250 is a better max zoom. 
    //Anything more than that and the viewport is too small to actually 
    //paint a heightmap outside of a very high res 4K+ monitor.
    m_maxZoom = 250.0f;
}

//////////////////////////////////////////////////////////////////////////
QTopRendererWnd::~QTopRendererWnd()
{
    ////////////////////////////////////////////////////////////////////////
    // Destroy the attached render and free the surface texture
    ////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::SetType(EViewportType type)
{
    m_viewType = type;
    m_axis = VPA_YX;
    SetAxis(m_axis);
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::ResetContent()
{
    Q2DViewport::ResetContent();

    // Reset texture ids.
    m_terrainTextureId = 0;
    m_vegetationTextureId = 0;
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::UpdateContent(int flags)
{
    if (gSettings.viewports.bTopMapSwapXY)
    {
        SetAxis(VPA_YX);
    }
    else
    {
        SetAxis(VPA_XY);
    }

    Q2DViewport::UpdateContent(flags);
    if (!GetIEditor()->GetDocument())
    {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::Draw([[maybe_unused]] DisplayContext& dc)
{
    ////////////////////////////////////////////////////////////////////////
    // Perform the rendering for this window
    ////////////////////////////////////////////////////////////////////////
    if (!m_bContentsUpdated)
    {
        UpdateContent(0xFFFFFFFF);
    }

    ////////////////////////////////////////////////////////////////////////
    // Render the 2D map
    ////////////////////////////////////////////////////////////////////////
     // ToDo: Remove TopRendererWnd or update to work with Atom: LYN-3671
    /*if (!m_terrainTextureId)
    {
        //GL_BGRA_EXT
        if (m_terrainTexture.IsValid())
        {
            m_terrainTextureId = m_renderer->DownLoadToVideoMemory((unsigned char*)m_terrainTexture.GetData(), m_textureSize.width(), m_textureSize.height(), eTF_R8G8B8A8, eTF_R8G8B8A8, 0, 0, 0);
        }
    }

    if (m_terrainTextureId && m_terrainTexture.IsValid())
    {
        m_renderer->UpdateTextureInVideoMemory(m_terrainTextureId, (unsigned char*)m_terrainTexture.GetData(), 0, 0, m_textureSize.width(), m_textureSize.height(), eTF_R8G8B8A8);
    }

    if (m_bShowStatObjects)
    {
        if (m_vegetationTexture.IsValid())
        {
            int w = m_vegetationTexture.GetWidth();
            int h = m_vegetationTexture.GetHeight();
            uint32* tex = m_vegetationTexture.GetData();
            if (!m_vegetationTextureId)
            {
                m_vegetationTextureId = m_renderer->DownLoadToVideoMemory((unsigned char*)tex, w, h, eTF_R8G8B8A8, eTF_R8G8B8A8, 0, 0, FILTER_NONE);
            }
            else
            {
                int px = m_vegetationTexturePos.x();
                int py = m_vegetationTexturePos.y();
                m_renderer->UpdateTextureInVideoMemory(m_vegetationTextureId, (unsigned char*)tex, px, py, w, h, eTF_R8G8B8A8);
            }
            m_vegetationTexture.Release();
        }
    }


    // Reset states
    m_renderer->ResetToDefault();

    dc.DepthTestOff();

    Matrix34 tm = GetScreenTM();

    if (m_axis == VPA_YX)
    {
        float s[4], t[4];

        s[0] = 0;
        t[0] = 0;
        s[1] = 1;
        t[1] = 0;
        s[2] = 1;
        t[2] = 1;
        s[3] = 0;
        t[3] = 1;
        m_renderer->DrawImageWithUV(tm.m03, tm.m13, 0, tm.m01 * m_heightmapSize.width(), tm.m10 * m_heightmapSize.height(), m_terrainTextureId, s, t);
    }
    else
    {
        float s[4], t[4];
        s[0] = 0;
        t[0] = 0;
        s[1] = 0;
        t[1] = 1;
        s[2] = 1;
        t[2] = 1;
        s[3] = 1;
        t[3] = 0;
        m_renderer->DrawImageWithUV(tm.m03, tm.m13, 0, tm.m00 * m_heightmapSize.width(), tm.m11 * m_heightmapSize.height(), m_terrainTextureId, s, t);
    }

    dc.DepthTestOn();

    Q2DViewport::Draw(dc);*/
}

//////////////////////////////////////////////////////////////////////////
Vec3    QTopRendererWnd::ViewToWorld(const QPoint& vp, bool* collideWithTerrain, bool onlyTerrain, bool bSkipVegetation, bool bTestRenderMesh, bool* collideWithObject) const
{
    Vec3 wp = Q2DViewport::ViewToWorld(vp, collideWithTerrain, onlyTerrain, bSkipVegetation, bTestRenderMesh, collideWithObject);
    wp.z = GetIEditor()->GetTerrainElevation(wp.x, wp.y);
    return wp;
}

#include <moc_TopRendererWnd.cpp>
