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

// Description : Object edit mode describe viewport input behavior when operating on objects.


#ifndef CRYINCLUDE_EDITOR_EDITMODE_OBJECTMODE_H
#define CRYINCLUDE_EDITOR_EDITMODE_OBJECTMODE_H
#pragma once

// {87109FED-BDB5-4874-936D-338400079F58}
DEFINE_GUID(OBJECT_MODE_GUID, 0x87109fed, 0xbdb5, 0x4874, 0x93, 0x6d, 0x33, 0x84, 0x0, 0x7, 0x9f, 0x58);

#include "EditTool.h"

class CBaseObject;
class CDeepSelection;
/*!
*   CObjectMode is an abstract base class for All Editing   Tools supported by Editor.
*   Edit tools handle specific editing modes in viewports.
*/
class SANDBOX_API CObjectMode
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CObjectMode(QObject* parent = nullptr);
    virtual ~CObjectMode();

    static const GUID& GetClassID() { return OBJECT_MODE_GUID; }

    // Registration function.
    static void RegisterTool(CRegistrationContext& rc);

    //////////////////////////////////////////////////////////////////////////
    // CEditTool implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void BeginEditParams([[maybe_unused]] IEditor* ie, [[maybe_unused]] int flags) {};
    virtual void EndEditParams();
    virtual void Display(struct DisplayContext& dc);
    virtual void DisplaySelectionPreview(struct DisplayContext& dc);
    virtual void DrawSelectionPreview(struct DisplayContext& dc, CBaseObject* drawObject);
    void DisplayExtraLightInfo(struct DisplayContext& dc);

    virtual bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnSetCursor([[maybe_unused]] CViewport* vp) { return false; };

    virtual void OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const Vec3& value) override;

    bool IsUpdateUIPanel() override { return true; }

protected:
    enum ECommandMode
    {
        NothingMode = 0,
        ScrollZoomMode,
        SelectMode,
        MoveMode,
        RotateMode,
        ScaleMode,
        ScrollMode,
        ZoomMode,
    };

    virtual bool OnLButtonDown(CViewport* view, int nFlags, const QPoint& point);
    virtual bool OnLButtonDblClk(CViewport* view, int nFlags, const QPoint& point);
    virtual bool OnLButtonUp(CViewport* view, int nFlags, const QPoint& point);
    virtual bool OnRButtonDown(CViewport* view, int nFlags, const QPoint& point);
    virtual bool OnRButtonUp(CViewport* view, int nFlags, const QPoint& point);
    virtual bool OnMButtonDown(CViewport* view, int nFlags, const QPoint& point);
    virtual bool OnMouseMove(CViewport* view, int nFlags, const QPoint& point);
    virtual bool OnMouseLeave(CViewport* view);

    void SetCommandMode(ECommandMode mode) { m_commandMode = mode; }
    ECommandMode GetCommandMode() const { return m_commandMode; }

    //! Ctrl-Click in move mode to move selected objects to given pos.
    void MoveSelectionToPos(CViewport* view, Vec3& pos, bool align, const QPoint& point);
    void SetObjectCursor(CViewport* view, CBaseObject* hitObj, bool bChangeNow = false);

    virtual void DeleteThis() { delete this; };

    void UpdateStatusText();
    void AwakeObjectAtPoint(CViewport* view, const QPoint& point);

    void HideMoveByFaceNormGizmo();
    void HandleMoveByFaceNormal(HitContext& hitInfo);
    void UpdateMoveByFaceNormGizmo(CBaseObject* pHitObject);

protected:

    bool m_openContext;

private:
    void    CheckDeepSelection(HitContext& hitContext, CViewport* view);
    Vec3& GetScale(const CViewport* view, const QPoint& point, Vec3& OutScale);

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QPoint m_cMouseDownPos;
    bool m_bDragThresholdExceeded;
    ECommandMode m_commandMode;

    GUID m_MouseOverObject;
    typedef std::vector<GUID> TGuidContainer;
    TGuidContainer m_PreviewGUIDs;

    _smart_ptr<CDeepSelection> m_pDeepSelection;

    bool m_bMoveByFaceNormManipShown;
    CBaseObject* m_pHitObject;

    bool m_bTransformChanged;

    QPoint m_prevMousePos = QPoint(0, 0);

    Vec3 m_lastValidMoveVector = Vec3(0, 0, 0);
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};



#endif // CRYINCLUDE_EDITOR_EDITMODE_OBJECTMODE_H
