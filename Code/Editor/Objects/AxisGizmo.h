/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_OBJECTS_AXISGIZMO_H
#define CRYINCLUDE_EDITOR_OBJECTS_AXISGIZMO_H
#pragma once

#include "BaseObject.h"
#include "Gizmo.h"
#include "Include/ITransformManipulator.h"

// forward declarations.
struct DisplayContext;
class CAxisHelper;

/** Gizmo of Objects animation track.
*/
class SANDBOX_API CAxisGizmo
    : public CGizmo
    , public ITransformManipulator
    , public CBaseObject::EventListener
{
public:
    CAxisGizmo();
    // Creates axis gizmo linked to an object.
    CAxisGizmo(CBaseObject* object);
    ~CAxisGizmo();

    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CGizmo
    //////////////////////////////////////////////////////////////////////////
    void GetWorldBounds(AABB& bbox) override;
    void Display(DisplayContext& dc) override;
    bool HitTest(HitContext& hc) override;
    const Matrix34& GetMatrix() const override;
    //////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////
    // ITransformManipulator implementation.
    //////////////////////////////////////////////////////////////////////////
    Matrix34 GetTransformation(RefCoordSys coordSys, IDisplayViewport* view = nullptr) const override;
    void SetTransformation(RefCoordSys coordSys, const Matrix34& tm) override;
    bool HitTestManipulator(HitContext& hc) override;
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int nFlags) override;
    void SetAlwaysUseLocal(bool on) override
    { m_bAlwaysUseLocal = on; }
    //////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////
    void SetWorldBounds(const AABB& bbox);

    void DrawAxis(DisplayContext& dc);

    static int GetGlobalAxisGizmoCount() { return m_axisGizmoCount; }

    CBaseObjectPtr GetBaseObject() const override { return m_object; }

private:
    void OnObjectEvent(CBaseObject* object, int event) override;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    CBaseObjectPtr m_object;
    AABB m_bbox;
    std::unique_ptr<CAxisHelper> m_pAxisHelper;

    bool m_bDragging;
    QPoint m_cMouseDownPos;
    Vec3 m_initPos;

    int m_highlightAxis;

    Matrix34 m_localTM;
    Matrix34 m_parentTM;
    Matrix34 m_userTM;

    bool m_bAlwaysUseLocal;
    RefCoordSys m_coordSysBackUp;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    static int m_axisGizmoCount;
};
#endif // CRYINCLUDE_EDITOR_OBJECTS_AXISGIZMO_H
