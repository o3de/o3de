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

// Description : Definition of PickObjectTool, tool used to pick objects.

#ifndef CRYINCLUDE_EDITOR_MATERIAL_MATERIALPICKTOOL_H
#define CRYINCLUDE_EDITOR_MATERIAL_MATERIALPICKTOOL_H
#pragma once

#include "EditTool.h"

//////////////////////////////////////////////////////////////////////////
class CMaterialPickTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CMaterialPickTool();

    static const GUID& GetClassID();

    static void RegisterTool(CRegistrationContext& rc);

    //////////////////////////////////////////////////////////////////////////
    // CEditTool implementation
    //////////////////////////////////////////////////////////////////////////
    virtual bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual void Display(DisplayContext& dc);
    //////////////////////////////////////////////////////////////////////////

protected:

    bool OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point);
    void SetMaterial(_smart_ptr<IMaterial> pMaterial);

    virtual ~CMaterialPickTool();
    // Delete itself.
    void DeleteThis() { delete this; };

    _smart_ptr<IMaterial> m_pMaterial;
    QString m_displayString;
    QPoint m_Mouse2DPosition;
    SRayHitInfo m_HitInfo;
};


#endif // CRYINCLUDE_EDITOR_MATERIAL_MATERIALPICKTOOL_H
