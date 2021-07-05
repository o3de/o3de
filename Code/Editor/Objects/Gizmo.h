/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "BaseObject.h"

// forward declarations.
#ifndef CRYINCLUDE_EDITOR_OBJECTS_GIZMO_H
#define CRYINCLUDE_EDITOR_OBJECTS_GIZMO_H
#pragma once
struct DisplayContext;
struct HitContext;

enum EGizmoFlags
{
    EGIZMO_SELECTABLE = 0x0001, //! If set gizmo can be selected by clicking.
    EGIZMO_HIDDEN = 0x0002,         //! If set gizmo hidden and should not be displayed.
    EGIZMO_TRANSFORM_MANIPULATOR = 0x0004, //! This gizmo is a transform manipulator.
};

/** Any helper object that BaseObjects can use to display some usefull information like tracks.
        Gizmo's life time should be controlled by thier owning BaseObjects.
*/
class SANDBOX_API CGizmo
    : public CRefCountBase
{
public:
    CGizmo();
    virtual ~CGizmo();

    virtual void SetName([[maybe_unused]] const char* sName) {};
    virtual const char* GetName([[maybe_unused]] const char* sName) { return ""; };

    //! Set gizmo object flags.
    void SetFlags(uint32 flags) { m_flags = flags; }
    //! Get gizmo object flags.
    uint32 GetFlags() const { return m_flags; }

    /** Get bounding box of Gizmo in world space.
        @param bbox Returns bounding box.
    */
    virtual void GetWorldBounds(AABB& bbox) = 0;

    /** Set transformation matrix of this gizmo.
    */
    virtual void SetMatrix(const Matrix34& tm);

    /** Get transformation matrix of this gizmo.
    */
    virtual const Matrix34& GetMatrix() const { return m_matrix; }

    /** Display Gizmo in the viewport.
    */
    virtual void Display(DisplayContext& dc) = 0;

    /** Performs hit testing on gizmo object.
    */
    virtual bool HitTest([[maybe_unused]] HitContext& hc) { return false; };

    //! Is this gizmo need to be deleted?.
    bool IsDelete() const { return m_bDelete; }
    //! Set this gizmo to be deleted.
    void DeleteThis();

    virtual CBaseObjectPtr GetBaseObject() const {  return NULL; }


protected:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    mutable Matrix34 m_matrix;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    bool m_bDelete; // This gizmo is marked for deletion.
    uint32 m_flags;
};
#endif // CRYINCLUDE_EDITOR_OBJECTS_GIZMO_H
