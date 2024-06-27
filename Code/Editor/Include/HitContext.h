/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Hit testing for editor viewport operations


#ifndef CRYINCLUDE_EDITOR_INCLUDE_HITCONTEXT_H
#define CRYINCLUDE_EDITOR_INCLUDE_HITCONTEXT_H
#pragma once

struct IDisplayViewport;
struct AABB;

#include <QRect>
#include <platform.h>

//! Flags used in HitContext for nSubObjFlags member.
enum ESubObjHitFlags
{
    //! When set all hit elements will be selected.
    SO_HIT_SELECT = BIT(1),
    //! Only test selected elements for hit.
    SO_HIT_TEST_SELECTED = BIT(2),
    //! Only hit test point2d, not rectangle
    //! Will only test/select 1 closest element
    SO_HIT_POINT = BIT(3),
    //! Adds hit elements to previously selected ones.
    SO_HIT_SELECT_ADD = BIT(4),
    //! Remove hit elements from previously selected ones.
    SO_HIT_SELECT_REMOVE = BIT(5),
    //! Output flag, set if selection was changed.
    SO_HIT_SELECTION_CHANGED = BIT(6),
    //! Hit testing to highlight sub object-element.
    SO_HIT_HIGHLIGHT_ONLY = BIT(7),
    //! This hit test is not for editing sub-objects.
    //! (ex. for moving an object by its face-normal)
    SO_HIT_NO_EDIT = BIT(8),
    // Check hit with vertices.
    SO_HIT_ELEM_VERTEX = BIT(10),
    // Check hit with edges.
    SO_HIT_ELEM_EDGE = BIT(11),
    // Check hit with faces.
    SO_HIT_ELEM_FACE = BIT(12),
    // Check hit with polygons.
    SO_HIT_ELEM_POLYGON = BIT(13)
};

#define SO_HIT_ELEM_ALL (SO_HIT_ELEM_VERTEX | SO_HIT_ELEM_EDGE | SO_HIT_ELEM_FACE | SO_HIT_ELEM_POLYGON)

//! Collision structure passed to HitTest function.
struct HitContext
{
    //! Viewport that originates hit testing.
    IDisplayViewport* view;
    //! 2D point on view that is used for hit testing.
    QPoint point2d;
    //! 2D Selection rectangle (Only when HitTestRect)
    QRect rect;
    //! Optional limiting bounding box for hit testing.
    AABB* bounds;

    //! Testing performed in 2D viewport.
    bool b2DViewport;
    //! True if axis collision must be ignored.
    bool bIgnoreAxis;
    //! Hit test only gizmo objects
    bool bOnlyGizmo;
    //! Test objects using advanced selection helpers.
    bool bUseSelectionHelpers;

    // Input parameters.

    //! Ray origin.
    Vec3 raySrc;
    //! Ray direction.
    Vec3 rayDir;
    //! Relaxation parameter for hit testing.
    float distanceTolerance;
    //! Sub object hit testing flags, @see ESubObjHitFlags
    int nSubObjFlags;

    // Output parameters.

    //! true if this hit should have less priority then non weak hits.
    //! (exp: Ray hit entity bounding box but not entity geometry.)
    bool weakHit;
    //! constrain axis if hit AxisGizmo.
    int axis;
    //! if hit axis gizmo, 1 - move mode, 2 - rotate mode, 3 - scale mode, 4 - rotate circle mode
    int manipulatorMode;
    //! distance to the object from src.
    float dist;
    //! For linking tool
    const char* name;
    //! true if this hit was from the object icon
    bool iconHit;

    HitContext()
    {
        rect = QRect();
        b2DViewport = false;
        view = 0;
        point2d = QPoint();
        axis = 0;
        distanceTolerance = 0;
        raySrc(0, 0, 0);
        rayDir(0, 0, 0);
        dist = 0;
        weakHit = false;
        manipulatorMode = 0;
        nSubObjFlags = 0;
        bounds = 0;
        bIgnoreAxis = false;
        bOnlyGizmo = false;
        bUseSelectionHelpers = false;
        name = nullptr;
        iconHit = false;
    }
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_HITCONTEXT_H
