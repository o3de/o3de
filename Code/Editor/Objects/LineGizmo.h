/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_OBJECTS_LINEGIZMO_H
#define CRYINCLUDE_EDITOR_OBJECTS_LINEGIZMO_H
#pragma once

#include "BaseObject.h"
#include "Gizmo.h"

// forward declarations.
struct DisplayContext;

/** Gizmo of link line connecting two Objects.
*/
class CLineGizmo
    : public CGizmo
    , public CBaseObject::EventListener
{
public:
    CLineGizmo();
    ~CLineGizmo();

    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CGizmo
    //////////////////////////////////////////////////////////////////////////
    void SetName(const char* sName) override;
    void GetWorldBounds(AABB& bbox) override;
    void Display(DisplayContext& dc) override;
    bool HitTest(HitContext& hc) override;

    //////////////////////////////////////////////////////////////////////////
    void SetObjects(CBaseObject* pObject1, CBaseObject* pObject2, const QString& boneName = "");
    void SetColor(const Vec3& color1, const Vec3& color2, float alpha1 = 1.0f, float alpha2 = 1.0f);

private:
    void OnObjectEvent(CBaseObject* object, int event) override;
    void CalcBounds();

    CBaseObjectPtr m_object[2];
    Vec3 m_point[2];
    AABB m_bbox;
    ColorF m_color[2];
    QString m_name;
    QString m_boneName;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_LINEGIZMO_H

