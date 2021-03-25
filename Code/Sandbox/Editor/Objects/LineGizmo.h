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
    virtual void SetName(const char* sName);
    virtual void GetWorldBounds(AABB& bbox);
    virtual void Display(DisplayContext& dc);
    virtual bool HitTest(HitContext& hc);

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

