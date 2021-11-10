/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_OBJECTS_GIZMOMANAGER_H
#define CRYINCLUDE_EDITOR_OBJECTS_GIZMOMANAGER_H
#pragma once


#include "IGizmoManager.h"


typedef _smart_ptr<CGizmo> CGizmoPtr;

/** GizmoManager manages set of currently active Gizmo objects.
*/
class CGizmoManager
    : public IGizmoManager
{
public:
    void AddGizmo(CGizmo* gizmo) override;
    void RemoveGizmo(CGizmo* gizmo) override;

    int GetGizmoCount() const override;
    CGizmo* GetGizmoByIndex(int nIndex) const override;

    void Display(DisplayContext& dc) override;
    bool HitTest(HitContext& hc) override;

    void DeleteAllTransformManipulators();

private:
    typedef std::set<CGizmoPtr> Gizmos;
    Gizmos m_gizmos;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_GIZMOMANAGER_H
