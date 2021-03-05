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

// Description : Ruler helper for Editor to determine distances


#ifndef CRYINCLUDE_EDITOR_UTIL_RULER_H
#define CRYINCLUDE_EDITOR_UTIL_RULER_H
#pragma once


#include "RulerPoint.h"

//! The Ruler utility helps to determine distances between user-specified points
class CRuler
{
public:
    CRuler();
    ~CRuler();

    //! Returns if ruler has queued paths in the path agent
    bool HasQueuedPaths() const;

    //! Activate the ruler
    void SetActive(bool bActive);
    bool IsActive() const { return m_bActive; }

    //! Update
    void Update();

    //! Mouse callback handling from viewport
    bool MouseCallback(CViewport* pView, EMouseEvent event, QPoint& point, int flags);

private:
    //! Mouse callback helpers
    void OnMouseMove(CViewport* pView, QPoint& point, int flags);
    void OnLButtonUp(CViewport* pView, QPoint& point, int flags);

    //! Returns world point based on mouse point
    void UpdateRulerPoint(CViewport* pView, const QPoint& point, CRulerPoint& rulerPoint, bool bRequestPath);

    //! Request a path using the path agent
    void RequestPath();

    bool IsObjectSelectMode(CViewport* pView) const;

private:
    bool m_bActive;
    GUID m_MouseOverObject;

    // Base point
    CRulerPoint m_startPoint;
    CRulerPoint m_endPoint;

    float m_sphereScale;
    float m_sphereTrans;
};

#endif // CRYINCLUDE_EDITOR_UTIL_RULER_H
