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
    void AddGizmo(CGizmo* gizmo);
    void RemoveGizmo(CGizmo* gizmo);

    int GetGizmoCount() const override;
    CGizmo* GetGizmoByIndex(int nIndex) const override;

    void Display(DisplayContext& dc);
    bool HitTest(HitContext& hc);

    void DeleteAllTransformManipulators();

private:
    typedef std::set<CGizmoPtr> Gizmos;
    Gizmos m_gizmos;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_GIZMOMANAGER_H
