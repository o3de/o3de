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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_TRACKGIZMO_H
#define CRYINCLUDE_EDITOR_OBJECTS_TRACKGIZMO_H
#pragma once


#include "Gizmo.h"

// forward declarations.
struct DisplayContext;
class CTrackViewAnimNode;
class CTrackViewTrack;

/** Gizmo of Objects animation track.
*/
class CTrackGizmo
    : public CGizmo
{
public:
    CTrackGizmo();
    ~CTrackGizmo();

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CGizmo
    //////////////////////////////////////////////////////////////////////////
    virtual void GetWorldBounds(AABB& bbox);
    virtual void Display(DisplayContext& dc);
    virtual bool HitTest(HitContext& hc);
    virtual void SetMatrix(const Matrix34& tm);

    //////////////////////////////////////////////////////////////////////////
    void SetAnimNode(CTrackViewAnimNode* pNode);
    void DrawAxis(DisplayContext& dc, const Vec3& pos);

    void DrawKeys(DisplayContext& dc, CTrackViewTrack* pTrack, CTrackViewTrack* pKeysTrack);

private:
    CTrackViewAnimNode* m_pAnimNode;
    AABB m_worldBbox;
    bool m_keysSelected;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_TRACKGIZMO_H
