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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_AXISGIZMO_H
#define CRYINCLUDE_EDITOR_OBJECTS_AXISGIZMO_H
#pragma once

#include "BaseObject.h"
#include "Gizmo.h"
#include "Include/ITransformManipulator.h"

// forward declarations.
struct DisplayContext;
class CAxisHelper;
class CAxisHelperExtended;

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
    virtual void GetWorldBounds(AABB& bbox);
    virtual void Display(DisplayContext& dc);
    virtual bool HitTest(HitContext& hc);
    virtual const Matrix34& GetMatrix() const;
    //////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////
    // ITransformManipulator implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual Matrix34 GetTransformation(RefCoordSys coordSys, IDisplayViewport* view = nullptr) const;
    virtual void SetTransformation(RefCoordSys coordSys, const Matrix34& tm);
    virtual bool HitTestManipulator(HitContext& hc);
    virtual bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int nFlags);
    virtual void SetAlwaysUseLocal(bool on)
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
    std::unique_ptr<CAxisHelperExtended> m_pAxisHelperExtended;

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
