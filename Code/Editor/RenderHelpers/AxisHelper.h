/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_RENDERHELPERS_AXISHELPER_H
#define CRYINCLUDE_EDITOR_RENDERHELPERS_AXISHELPER_H
#pragma once

#include "IEditor.h" // for AxisConstrains and RefCoordSys

struct DisplayContext;
struct HitContext;
struct IDisplayViewport;

//////////////////////////////////////////////////////////////////////////
struct SGizmoParameters
{
    bool axisGizmoText;
    bool enabled;
    float axisGizmoScale;
    AxisConstrains axisConstraint;
    RefCoordSys referenceCoordSys;

    SGizmoParameters()
        : axisGizmoText(false)
        , enabled(true)
        , axisGizmoScale(1.0f)
        , axisConstraint(AXIS_TERRAIN)
        , referenceCoordSys(COORDS_LOCAL)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
// Helper Axis object.
//////////////////////////////////////////////////////////////////////////
class CAxisHelper
{
public:
    enum EHelperMode
    {
        MOVE_MODE    = 0x1,
        SCALE_MODE   = 0x2,
        ROTATE_MODE  = 0x4,
        SELECT_MODE  = 0x8,
        ROTATE_CIRCLE_MODE = 0x10,
    };

    CAxisHelper();

    void SetMode(int nModeFlags);
    void DrawAxis(const Matrix34& worldTM, const SGizmoParameters& setup, DisplayContext& dc);
    void DrawDome(const Matrix34& worldTM, const SGizmoParameters& setup, DisplayContext& dc, AABB& objectBox);
    bool HitTest(const Matrix34& worldTM, const SGizmoParameters& setup, HitContext& hc);

    bool HitTestForRotationCircle(const Matrix34& worldTM, IDisplayViewport* view, const QPoint& pos, float fHitWidth, Vec3* pOutHitPos = NULL, Vec3* pOutHitNormal = NULL);

    void SetHighlightAxis(int axis) { m_highlightAxis = axis; };
    int  GetHighlightAxis() const { return m_highlightAxis; };

private:
    void Prepare(const Matrix34& worldTM, const SGizmoParameters& setup, IDisplayViewport* view);
    float GetDistance2D(IDisplayViewport* view, const QPoint& p, const Vec3& wp);

    int m_nModeFlags;
    int m_highlightAxis;
    int m_highlightMode;
    int m_currentMode;
    bool m_bHighlightRotationCircle;

    float m_fScreenScale;
    bool m_bNeedX;
    bool m_bNeedY;
    bool m_bNeedZ;
    float m_size;
    Matrix34 m_matrix;
};

#endif // CRYINCLUDE_EDITOR_RENDERHELPERS_AXISHELPER_H

