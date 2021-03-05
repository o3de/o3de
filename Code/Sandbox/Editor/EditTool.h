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

#ifndef CRYINCLUDE_EDITOR_EDITTOOL_H
#define CRYINCLUDE_EDITOR_EDITTOOL_H

#pragma once

#if !defined(Q_MOC_RUN)
#include "QtViewPaneManager.h"
#endif

class CViewport;
struct IClassDesc;
struct ITransformManipulator;
struct HitContext;

enum EEditToolType
{
    EDIT_TOOL_TYPE_PRIMARY,
    EDIT_TOOL_TYPE_SECONDARY,
};

/*!
 *  CEditTool is an abstract base class for All Editing Tools supported by Editor.
 *  Edit tools handle specific editing modes in viewports.
 */
class SANDBOX_API CEditTool
    : public QObject
{
    Q_OBJECT
public:
    explicit CEditTool(QObject* parent = nullptr);

    //////////////////////////////////////////////////////////////////////////
    // For reference counting.
    //////////////////////////////////////////////////////////////////////////
    void AddRef() { m_nRefCount++; };
    void Release()
    {
        AZ_Assert(m_nRefCount > 0, "Negative ref count");
        if (--m_nRefCount == 0)
        {
            DeleteThis();
        }
    };

    //! Returns class description for this tool.
    IClassDesc* GetClassDesc() const { return m_pClassDesc; }

    virtual void SetParentTool(CEditTool* pTool);
    virtual CEditTool* GetParentTool();

    virtual EEditToolType GetType() { return EDIT_TOOL_TYPE_PRIMARY; }
    virtual EOperationMode GetMode() { return eOperationModeNone; }

    // Abort tool.
    virtual void Abort();

    // Accept tool.
    virtual void Accept([[maybe_unused]] bool resetPosition = false) {}

    //! Status text displayed when this tool is active.
    void SetStatusText(const QString& text) { m_statusText = text; };
    QString GetStatusText() { return m_statusText; };

    // Description:
    //    Activates tool.
    // Arguments:
    //    pPreviousTool - Previously active edit tool.
    // Return:
    //    True if the tool can be activated,
    virtual bool Activate([[maybe_unused]] CEditTool* pPreviousTool) { return true; };

    //! Used to pass user defined data to edit tool from ToolButton.
    virtual void SetUserData([[maybe_unused]] const char* key, [[maybe_unused]] void* userData) {};

    //! Called when user starts using this tool.
    //! Flags is comnination of ObjectEditFlags flags.
    virtual void BeginEditParams([[maybe_unused]] IEditor* ie, [[maybe_unused]] int flags) {};
    //! Called when user ends using this tool.
    virtual void EndEditParams() {};

    // Called each frame to display tool for given viewport.
    virtual void Display(struct DisplayContext& dc) = 0;

    //! Mouse callback sent from viewport.
    //! Returns true if event processed by callback, and all other processing for this event should abort.
    //! Return false if event was not processed by callback, and other processing for this event should occur.
    //! @param view Viewport that sent this callback.
    //! @param event Indicate what kind of event occured in viewport.
    //! @param point 2D coordinate in viewport where event occured.
    //! @param flags Additional flags (MK_LBUTTON,etc..) or from (MouseEventFlags) specified by viewport when calling callback.
    virtual bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags) = 0;

    //! Called when key in viewport is pressed while using this tool.
    //! Returns true if event processed by callback, and all other processing for this event should abort.
    //! Returns false if event was not processed by callback, and other processing for this event should occur.
    //! @param view Viewport where key was pressed.
    //! @param nChar Specifies the virtual key code of the given key. For a list of standard virtual key codes, see Winuser.h
    //! @param nRepCnt Specifies the repeat count, that is, the number of times the keystroke is repeated as a result of the user holding down the key.
    //! @param nFlags   Specifies the scan code, key-transition code, previous key state, and context code, (see WM_KEYDOWN)
    virtual bool OnKeyDown([[maybe_unused]] CViewport* view, [[maybe_unused]] uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags) { return false; };

    //! Called when key in viewport is released while using this tool.
    //! Returns true if event processed by callback, and all other processing for this event should abort.
    //! Returns false if event was not processed by callback, and other processing for this event should occur.
    //! @param view Viewport where key was pressed.
    //! @param nChar Specifies the virtual key code of the given key. For a list of standard virtual key codes, see Winuser.h
    //! @param nRepCnt Specifies the repeat count, that is, the number of times the keystroke is repeated as a result of the user holding down the key.
    //! @param nFlags   Specifies the scan code, key-transition code, previous key state, and context code, (see WM_KEYDOWN)
    virtual bool OnKeyUp([[maybe_unused]] CViewport* view, [[maybe_unused]] uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags) { return false; };

    //! Called when mouse is moved and give oportunity to tool to set it own cursor.
    //! @return true if cursor changed. or false otherwise.
    virtual bool OnSetCursor([[maybe_unused]] CViewport* vp) { return false; };

    // Return objects affected by this edit tool. The returned objects usually will be the selected objects.
    virtual void GetAffectedObjects(DynArray<CBaseObject*>& outAffectedObjects);

    // Called in response to the dragging of the manipulator in the view.
    // Allow edit tool to handle manipulator dragging the way it wants.
    virtual void OnManipulatorDrag([[maybe_unused]] CViewport* view, [[maybe_unused]] ITransformManipulator* pManipulator, [[maybe_unused]] QPoint& p0, [[maybe_unused]] QPoint& p1, [[maybe_unused]] const Vec3& value) {}

    virtual void OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, const Vec3& value)
    {
        // Overload with less boiler-plate
        QPoint p0, p1;
        OnManipulatorDrag(view, pManipulator, p0, p1, value);
    }

    // Called in response to mouse event of the manipulator in the view
    virtual void OnManipulatorMouseEvent([[maybe_unused]] CViewport* view, [[maybe_unused]] ITransformManipulator* pManipulator, [[maybe_unused]] EMouseEvent event, [[maybe_unused]] QPoint& point, [[maybe_unused]] int flags, [[maybe_unused]] bool bHitGizmo = false) {}

    virtual bool IsNeedMoveTool() { return false; }
    virtual bool IsNeedSpecificBehaviorForSpaceAcce() { return false; }
    virtual bool IsNeedToSkipPivotBoxForObjects() { return false; }
    virtual bool IsDisplayGrid() { return true; }
    virtual bool IsUpdateUIPanel() { return false; }
    virtual bool IsMoveToObjectModeAfterEnd() { return true; }
    virtual bool IsCircleTypeRotateGizmo() { return false; }

    // Draws object specific helpers for this tool
    virtual void DrawObjectHelpers([[maybe_unused]] CBaseObject* pObject, [[maybe_unused]] DisplayContext& dc) {}

    // Hit test against edit tool
    virtual bool HitTest([[maybe_unused]] CBaseObject* pObject, [[maybe_unused]] HitContext& hc) { return false; }

protected:
    virtual ~CEditTool() {};
    //////////////////////////////////////////////////////////////////////////
    // Delete edit tool.
    //////////////////////////////////////////////////////////////////////////
    virtual void DeleteThis() = 0;

protected:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    _smart_ptr<CEditTool> m_pParentTool; // Pointer to parent edit tool.
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QString m_statusText;
    IClassDesc* m_pClassDesc;
    int m_nRefCount;
};

#endif // CRYINCLUDE_EDITOR_EDITTOOL_H
