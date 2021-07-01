/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
