/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IGIZMOMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDE_IGIZMOMANAGER_H
#pragma once

class CGizmo;
struct DisplayContext;
struct HitContext;

/** GizmoManager manages set of currently active Gizmo objects.
*/
struct IGizmoManager
{
    virtual ~IGizmoManager() = default;
    
    virtual void AddGizmo(CGizmo* gizmo) = 0;
    virtual void RemoveGizmo(CGizmo* gizmo) = 0;

    virtual int GetGizmoCount() const = 0;
    virtual CGizmo* GetGizmoByIndex(int nIndex) const = 0;

    virtual void Display(DisplayContext& dc) = 0;
    virtual bool HitTest(HitContext& hc) = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IGIZMOMANAGER_H
