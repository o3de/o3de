/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
    EViewportType GetType() const override { return ET_ViewportMap; }
    void SetType(EViewportType type) override;

    void ResetContent() override;
    void UpdateContent(int flags) override;

    //! Map viewport position to world space position.
    virtual Vec3    ViewToWorld(const QPoint& vp, bool* collideWithTerrain = nullptr, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false, bool* collideWithObject = nullptr) const override;

    void SetShowWater(bool bShow) { m_bShowWater = bShow; }
    bool GetShowWater() const { return m_bShowWater; };

    void SetAutoScaleGreyRange(bool bAutoScale) { m_bAutoScaleGreyRange = bAutoScale; }
    bool GetAutoScaleGreyRange() const { return m_bAutoScaleGreyRange; };

protected:
    // Draw everything.
    void Draw(DisplayContext& dc) override;

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
