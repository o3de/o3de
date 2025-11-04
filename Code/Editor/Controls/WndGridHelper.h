/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_WNDGRIDHELPER_H
#define CRYINCLUDE_EDITOR_CONTROLS_WNDGRIDHELPER_H
#pragma once

#include <QPoint>
#include <QRect>
#include "Cry_Vector2.h"
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Vector2.h>

class CWndGridHelper
{
public:
    AZ::Vector2 zoom;
    AZ::Vector2 origin;
    AZ::Vector2 step;
    AZ::Vector2 pixelsPerGrid;
    int nMajorLines;
    QRect rect;
    QPoint nMinPixelsPerGrid;
    QPoint nMaxPixelsPerGrid;
    
    QPoint firstGridLine;
    QPoint numGridLines;

    CWndGridHelper()
    {
        zoom = AZ::Vector2(1, 1);
        step = AZ::Vector2(10, 10);
        pixelsPerGrid = AZ::Vector2(10, 10);
        origin = AZ::Vector2(0, 0);
        nMajorLines = 10;
        nMinPixelsPerGrid = QPoint(50, 10);
        nMaxPixelsPerGrid = QPoint(100, 20);
        firstGridLine = QPoint(0, 0);
        numGridLines = QPoint(0, 0);
    }

    //////////////////////////////////////////////////////////////////////////
    Vec2 ClientToWorld(const QPoint& point)
    {
        Vec2 v;
        v.x = (point.x() - rect.left()) / zoom.GetX() + origin.GetX();
        v.y = (point.y() - rect.top()) / zoom.GetY() + origin.GetY();
        return v;
    }
    
    QPoint WorldToClient(Vec2 v)
    {
        QPoint p(aznumeric_cast<int>(floor((v.x - origin.GetX()) * zoom.GetX() + 0.5f) + rect.left()),
            aznumeric_cast<int>(floor((v.y - origin.GetY()) * zoom.GetY() + 0.5f) + rect.top()));
        return p;
    }

    void SetOrigin(Vec2 neworigin)
    {
        origin = AZ::Vector2(neworigin.x, neworigin.y);
    }
    void SetZoom(Vec2 newzoom)
    {
        zoom = AZ::Vector2(newzoom.x, newzoom.y);
    }
    void SetZoom(AZ::Vector2 newzoom, const QPoint& center)
    {

        if (newzoom.GetX() < 0.01f)
        {
            newzoom.SetX(0.01f);
        }
        if (newzoom.GetY() < 0.01f)
        {
            newzoom.SetY(0.01f);
        }

        // Zoom to mouse position.
        float ofsx = origin.GetX();
        float ofsy = origin.GetY();

        AZ::Vector2 z1 = zoom;
        AZ::Vector2 z2 = newzoom;

        zoom = newzoom;

        // Calculate new offset to center zoom on mouse.
        float x2 = aznumeric_cast<float>(center.x() - rect.left());
        float y2 = aznumeric_cast<float>(center.y() - rect.top());
        ofsx = -(x2 / z2.GetX() - x2 / z1.GetX() - ofsx);
        ofsy = -(y2 / z2.GetY() - y2 / z1.GetY() - ofsy);
        origin.SetX(ofsx);
        origin.SetY(ofsy);
    }
    void SetZoom(Vec2 newzoom, const QPoint& center)
    {
        SetZoom(AZ::Vector2(newzoom.x, newzoom.y), center);
    }
    void CalculateGridLines()
    {
        pixelsPerGrid = zoom;

        step = AZ::Vector2(1.00f, 1.00f);
        nMajorLines = 2;

        int griditers;
        if (pixelsPerGrid.GetX() <= nMinPixelsPerGrid.x())
        {
            griditers = 0;
            while (pixelsPerGrid.GetX() <= nMinPixelsPerGrid.x() && griditers++ < 1000)
            {
                step.SetX(step.GetX() * nMajorLines);
                pixelsPerGrid.SetX(step.GetX() * zoom.GetX());
            }
        }
        else
        {
            griditers = 0;
            while (pixelsPerGrid.GetX() >= nMaxPixelsPerGrid.x() && griditers++ < 1000)
            {
                step.SetX(step.GetX() / nMajorLines);
                pixelsPerGrid.SetX(step.GetX() * zoom.GetX());
            }
        }

        if (pixelsPerGrid.GetY() <= nMinPixelsPerGrid.y())
        {
            griditers = 0;
            while (pixelsPerGrid.GetY() <= nMinPixelsPerGrid.y() && griditers++ < 1000)
            {
                step.SetY(step.GetY() * nMajorLines);
                pixelsPerGrid.SetY(step.GetY() * zoom.GetY());
            }
        }
        else
        {
            griditers = 0;
            while (pixelsPerGrid.GetY() >= nMaxPixelsPerGrid.y() && griditers++ < 1000)
            {
                step.SetY(step.GetY() / nMajorLines);
                pixelsPerGrid.SetY(step.GetY() * zoom.GetY());
            }
        }

        firstGridLine.rx() = aznumeric_cast<int>(origin.GetX() / step.GetX());
        firstGridLine.ry() = aznumeric_cast<int>(origin.GetY() / step.GetY());

        numGridLines.rx() = aznumeric_cast<int>((rect.width() / zoom.GetX()) / step.GetX() + 1);
        numGridLines.ry() = aznumeric_cast<int>((rect.height() / zoom.GetY()) / step.GetY() + 1);
    }
    int GetGridLineX(int nGridLineX) const
    {
        return aznumeric_cast<int>(floor((nGridLineX * step.GetX() - origin.GetX()) * zoom.GetX() + 0.5f));
    }
    int GetGridLineY(int nGridLineY) const
    {
        return aznumeric_cast<int>(floor((nGridLineY * step.GetY() - origin.GetY()) * zoom.GetY() + 0.5f));
    }
    float GetGridLineXValue(int nGridLineX) const
    {
        return (nGridLineX * step.GetX());
    }
    float GetGridLineYValue(int nGridLineY) const
    {
        return (nGridLineY * step.GetY());
    }
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_WNDGRIDHELPER_H
