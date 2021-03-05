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

#ifndef CRYINCLUDE_EDITOR_TIMEOFDAYDIALOG_H
#define CRYINCLUDE_EDITOR_TIMEOFDAYDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "Controls/TimelineCtrl.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>
#include "Undo/IUndoManagerListener.h"
#include "LyViewPaneNames.h"
#include <QMainWindow>
#endif

//////////////////////////////////////////////////////////////////////////

class QResizeEvent;
class CCurveEditorCtrl;
class CHDRPane;

namespace Ui {
    class TimeOfDayDialog;
}

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
//////////////////////////////////////////////////////////////////////////
// Window that holds effector info.
//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CTimeOfDayDialog
    : public QMainWindow
    , public IEditorNotifyListener
    , public ISystemEventListener
    , public IUndoManagerListener
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    Q_OBJECT
public:
    static const char* ClassName() { return LyViewPane::TimeOfDayEditor; }
    static const GUID& GetClassID();

    CTimeOfDayDialog(QWidget* parent = nullptr);
    ~CTimeOfDayDialog();

    static void RegisterViewClass();
    void UpdateValues();

    // overrides from ISystemEventListener
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

protected:
    void OnBeforeSplineChange();
    void OnSplineChange(const QWidget* source);
    void OnPlayAnimFrom0();
    void OnChangeTimeAnimSpeed(double speed);

    void OnImport();
    void OnExport();
    void OnExpandAll();
    void OnResetToDefaultValues();
    void OnCollapseAll();

    void OnHold();
    void OnFetch();
    void OnUndo();
    void OnRedo();

    void OnPropertySelected(IVariable* node);
    void OnSplineCtrlScrollZoom();
    void OnTimelineCtrlChange();

    void Init();

    void OnUpdateProperties(IVariable* var);

    void CreateProperties();

    void SetTime(float time);
    void SetTimeRange(float fTimeStart, float fTimeEnd, float fSpeed);
    float GetTime() const;

    void RefreshPropertiesValues();
    void ResetSpline(IVariable* var);

    IVariable* FindVariable(const char* name) const;

    void CopyAllProperties();
    void PasteAllProperties();

    void HdrPropertySelected(IVariable* v);
    void StartTimeChanged(const QTime& time);
    void EndTimeChanged(const QTime& time);

    //////////////////////////////////////////////////////////////////////////
    // IEditorNotifyListener
    //////////////////////////////////////////////////////////////////////////
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    //////////////////////////////////////////////////////////////////////////

    // IUndoManagerListener
    void SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo) override;

    void resizeEvent(QResizeEvent* event) override;

private:
    void UpdateUI(bool updateProperties=true);
    void SetTimeFromActiveKey(bool useColorGradient = false);

    bool m_alive = true;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<Ui::TimeOfDayDialog> m_ui;

    CHDRPane* m_pHDRPane;
    CVarBlockPtr m_pVars;

    TimelineWidget* m_timelineCtrl;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    float m_maxTime;
};

class CHDRPane
    : public QWidget
{
    Q_OBJECT
public:
    CHDRPane(CTimeOfDayDialog* pTODDlg);

    ReflectedPropertyControl& properties() { return *m_propsCtrl; }
    CVarBlockPtr variables() { return m_pVars; }

    void UpdateFilmCurve();

signals:
    void propertySelected(IVariable* variable);

protected:
    bool Init();
    void OnPropertySelected(IVariable*);

    bool GetFilmCurveParams(float& shoulderScale, float& midScale, float& toeScale, float& whitePoint) const;

    CTimeOfDayDialog* m_pTODDlg;
    CCurveEditorCtrl* m_filmCurveCtrl;
    ReflectedPropertyControl* m_propsCtrl;
    CVarBlockPtr m_pVars;
};

/** Undo object stored when track is modified.
*/
class CUndoTimeOfDayObject
    : public IUndoObject
{
public:
    CUndoTimeOfDayObject();

protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "Time of Day"; };

    virtual void Undo(bool bUndo);
    virtual void Redo();

private:
    void UpdateTimeOfDayDialog();

    XmlNodeRef m_undo;
    XmlNodeRef m_redo;
};

#endif // CRYINCLUDE_EDITOR_TIMEOFDAYDIALOG_H
