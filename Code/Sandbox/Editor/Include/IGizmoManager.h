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
