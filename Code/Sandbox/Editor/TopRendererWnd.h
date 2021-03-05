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

#ifndef CRYINCLUDE_EDITOR_TOPRENDERERWND_H
#define CRYINCLUDE_EDITOR_TOPRENDERERWND_H

#pragma once

#if !defined(Q_MOC_RUN)
#include "Util/Image.h"
#include "2DViewport.h"
#endif

class QMenu;

// Predeclare because of friend declaration
class QTopRendererWnd
    : public Q2DViewport
{
    Q_OBJECT
public:
    QTopRendererWnd(QWidget* parent = nullptr);
    virtual ~QTopRendererWnd();

    static const GUID& GetClassID()
    {
        return QtViewport::GetClassID<QTopRendererWnd>();
    }

    /** Get type of this viewport.
    */
    virtual EViewportType GetType() const { return ET_ViewportMap; }
    virtual void SetType(EViewportType type);

    virtual void ResetContent();
    virtual void UpdateContent(int flags);

    //! Map viewport position to world space position.
    virtual Vec3    ViewToWorld(const QPoint& vp, bool* collideWithTerrain = nullptr, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false, bool* collideWithObject = nullptr) const override;

    void SetShowWater(bool bShow) { m_bShowWater = bShow; }
    bool GetShowWater() const { return m_bShowWater; };

    void SetAutoScaleGreyRange(bool bAutoScale) { m_bAutoScaleGreyRange = bAutoScale; }
    bool GetAutoScaleGreyRange() const { return m_bAutoScaleGreyRange; };

protected:
    // Draw everything.
    virtual void Draw(DisplayContext& dc);

private:
    bool m_bContentsUpdated;

    //CImage* m_pSurfaceTexture;
    int m_terrainTextureId;

    QSize m_textureSize;

    // Size of heightmap in meters.
    QSize m_heightmapSize;

    CImageEx m_terrainTexture;

    CImageEx m_vegetationTexture;
    QPoint m_vegetationTexturePos;
    QSize m_vegetationTextureSize;
    int     m_vegetationTextureId;
    bool m_bFirstTerrainUpdate;

public:
    // Display options.
    bool m_bDisplayLabels;
    bool m_bShowHeightmap;
    bool m_bLastShowHeightmapState;
    bool m_bShowStatObjects;
    bool m_bShowWater;
    bool m_bAutoScaleGreyRange;

    friend class QTopRendererWnd;
};

#endif // CRYINCLUDE_EDITOR_TOPRENDERERWND_H
