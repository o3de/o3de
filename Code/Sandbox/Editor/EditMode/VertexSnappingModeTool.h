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

#ifndef CRYINCLUDE_EDITOR_EDITMODE_VERTEXSNAPPINGMODETOOL_H
#define CRYINCLUDE_EDITOR_EDITMODE_VERTEXSNAPPINGMODETOOL_H
#pragma once

#include "EditTool.h"
#include "Objects/BaseObject.h"

class CKDTree;
struct IDisplayViewport;

class CVertexSnappingModeTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CVertexSnappingModeTool();
    ~CVertexSnappingModeTool();

    static const GUID& GetClassID();

    static void RegisterTool(CRegistrationContext& rc);

    void Display(DisplayContext& dc);
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

protected:

    void DrawVertexCubes(DisplayContext& dc, const Matrix34& tm, IStatObj* pStatObj);
    void DeleteThis(){ delete this; }
    Vec3 GetCubeSize(IDisplayViewport* pView, const Vec3& pos) const;

private:

    using CEditTool::HitTest;
    bool HitTest(CViewport* view, const QPoint& point, CBaseObject* pExcludedObj, Vec3& outHitPos, CBaseObjectPtr& pOutHitObject, std::vector<CBaseObjectPtr>& outObjects);
    CKDTree* GetKDTree(CBaseObject* pObject);

    enum EVertexSnappingStatus
    {
        eVSS_SelectFirstVertex,
        eVSS_MoveSelectVertexToAnotherVertex
    };
    EVertexSnappingStatus m_modeStatus;

    struct SSelectionInfo
    {
        SSelectionInfo()
        {
            m_pObject = NULL;
            m_vPos = Vec3(0, 0, 0);
        }
        CBaseObjectPtr m_pObject;
        Vec3 m_vPos;
    };

    /// Info on object being moved (when in eVSS_MoveSelectVertexToAnotherVertex mode).
    SSelectionInfo m_SelectionInfo;

    /// Objects that mouse is over
    std::vector<CBaseObjectPtr> m_Objects;

    /// Position of vertex that mouse is hitting.
    /// Invalid when m_bHit is false.
    Vec3 m_vHitVertex;

    /// Whether the mouse hit test succeeded
    bool m_bHit;

    /// Object that mouse is hitting
    CBaseObjectPtr m_pHitObject;

    /// Boxes to render for debug drawing
    std::vector<AABB> m_DebugBoxes;

    /// For each object, a tree containing its vertices.
    std::map<CBaseObjectPtr, CKDTree*> m_ObjectKdTreeMap;
};
#endif // CRYINCLUDE_EDITOR_EDITMODE_VERTEXSNAPPINGMODETOOL_H
