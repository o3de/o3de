/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

struct DisplayContext;
class CBaseObjectsCache;
class QPoint;
struct AABB;
class CViewport;

// Viewport functionality required for DisplayContext
struct IDisplayViewport
{
    virtual void Update() = 0;
    virtual float GetScreenScaleFactor(const Vec3& position) const = 0;

    /**
     * Gets the distance of the point on screen to the line defined by the two points, converted to screenspace.
     * @param lineP1 The first point of the line, in world space.
     * @param lineP2 The second point of the line, in world space.
     * @param point The point to check the distance from the line. This point is in screen space.
     * @return The distance of the point to the line.
    */
    virtual float GetDistanceToLine(const Vec3& lineP1, const Vec3& lineP2, const QPoint& point) const = 0;

    enum EAxis
    {
        AXIS_NONE,
        AXIS_X,
        AXIS_Y,
        AXIS_Z
    };
    virtual void GetPerpendicularAxis(EAxis* axis, bool* is2D) const = 0;

    virtual const Matrix34& GetViewTM() const = 0;
    virtual const Matrix34& GetScreenTM() const = 0;
    virtual QPoint WorldToView(const Vec3& worldPoint) const = 0;
    virtual Vec3 WorldToView3D(const Vec3& worldPoint, int flags = 0) const = 0;
    virtual Vec3 ViewToWorld(const QPoint& vp, bool* collideWithTerrain = nullptr, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false, bool* collideWithObject = nullptr) const = 0;
    virtual void ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const = 0;
    virtual void setRay(QPoint& vp, Vec3& raySrc, Vec3& rayDir) = 0;

    virtual float GetAspectRatio() const = 0;

    virtual bool IsBoundsVisible(const AABB& box) const = 0;

    virtual void ScreenToClient(QPoint& pt) const = 0;
    virtual void GetDimensions(int* width, int* height) const = 0;

    virtual CViewport *asCViewport() { return nullptr; }
};
