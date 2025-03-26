/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "TrackViewDopeSheetBase.h"
#include "TrackViewNode.h"
#include "TrackViewSequence.h"

#include <QDockWidget>
#include <QScopedPointer>
#include "Util/Variable.h"
#endif

#include <AzCore/std/containers/vector.h>

namespace Ui {
    class CTrackViewTrackPropsDlg;
    class CTrackViewKeyPropertiesDlg;
}

class ReflectedPropertyControl;
class CTrackViewKeyPropertiesDlg;

//////////////////////////////////////////////////////////////////////////
class CTrackViewKeyUIControls
    : public QObject
    , public _i_reference_target_t
{
    Q_OBJECT
public:
    CTrackViewKeyUIControls()
    {
        m_pVarBlock = new CVarBlock;
        m_onSetCallback = AZStd::bind(&CTrackViewKeyUIControls::OnInternalVariableChange, this, AZStd::placeholders::_1);
    };

    void SetKeyPropertiesDlg(CTrackViewKeyPropertiesDlg* pDlg) { m_pKeyPropertiesDlg = pDlg; }

    // Return internal variable block.
    CVarBlock* GetVarBlock() const { return m_pVarBlock; }

    //////////////////////////////////////////////////////////////////////////
    // Callbacks that must be implemented in derived class
    //////////////////////////////////////////////////////////////////////////
    // Returns true if specified animation track type is supported by this UI.
    virtual bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType) const = 0;

    // Called when UI variable changes.
    virtual void OnCreateVars() = 0;

    // Called when user changes selected keys.
    // Return true if control update UI values
    virtual bool OnKeySelectionChange(const CTrackViewKeyBundle& keys) = 0;

    // Called when UI variable changes.
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& keys) = 0;

    // Get priority of key UI control, so that specializations can have precedence
    virtual unsigned int GetPriority() const = 0;

protected:
    //////////////////////////////////////////////////////////////////////////
    // Helper functions.
    //////////////////////////////////////////////////////////////////////////
    template <class T>
    void SyncValue(CSmartVariable<T>& var, T& value, bool bCopyToUI, IVariable* pSrcVar = nullptr)
    {
        if (bCopyToUI)
        {
            var = value;
        }
        else
        {
            if (!pSrcVar || pSrcVar == var.GetVar())
            {
                value = var;
            }
        }
    }
    void AddVariable(CVariableBase& varArray, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        var.AddOnSetCallback(&m_onSetCallback);
        varArray.AddVariable(&var);
        m_registeredVariables.push_back(&var);
    }
    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        var.AddOnSetCallback(&m_onSetCallback);
        m_pVarBlock->AddVariable(&var);
        m_registeredVariables.push_back(&var);
    }
    void OnInternalVariableChange(IVariable* pVar);

protected:
    _smart_ptr<CVarBlock> m_pVarBlock;
    AZStd::vector<_smart_ptr<IVariable> > m_registeredVariables;
    CTrackViewKeyPropertiesDlg* m_pKeyPropertiesDlg;
    IVariable::OnSetCallback m_onSetCallback;
};

//////////////////////////////////////////////////////////////////////////
class CTrackViewTrackPropsDlg
    : public QWidget
{
    Q_OBJECT
public:
    CTrackViewTrackPropsDlg(QWidget* parent = 0);
    ~CTrackViewTrackPropsDlg();

    void OnSequenceChanged();
    bool OnKeySelectionChange(const CTrackViewKeyBundle& keys);

protected slots:
    void OnUpdateTime();

protected:
    CTrackViewKeyHandle m_keyHandle;
    QScopedPointer<Ui::CTrackViewTrackPropsDlg> ui;
};


//////////////////////////////////////////////////////////////////////////
class TrackViewKeys;
class CTrackViewKeyPropertiesDlg
    : public QWidget
    , public ITrackViewSequenceListener
{
public:
    CTrackViewKeyPropertiesDlg(QWidget* hParentWnd);

    void SetKeysCtrl(CTrackViewDopeSheetBase* pKeysCtrl)
    {
        m_keysCtrl = pKeysCtrl;
        if (m_keysCtrl)
        {
            m_keysCtrl->SetKeyPropertiesDlg(this);
        }
    }

    void OnSequenceChanged(CTrackViewSequence* sequence);

    void PopulateVariables();
    void PopulateVariables(ReflectedPropertyControl* propCtrl);

    // ITrackViewSequenceListener
    void OnKeysChanged(CTrackViewSequence* pSequence) override;
    void OnKeySelectionChanged(CTrackViewSequence* pSequence) override;

protected:
    //////////////////////////////////////////////////////////////////////////
    void OnVarChange(IVariable* pVar);
    void CreateAllVars();
    void AddVars(CTrackViewKeyUIControls* pUI);
    void ReloadValues();

protected:
    AZStd::vector< _smart_ptr<CTrackViewKeyUIControls> > m_keyControls;

    _smart_ptr<CVarBlock> m_pVarBlock;

    ReflectedPropertyControl* m_wndProps;
    CTrackViewTrackPropsDlg* m_wndTrackProps;

    CTrackViewDopeSheetBase* m_keysCtrl;

    CTrackViewTrack* m_pLastTrackSelected;
    CTrackViewSequence* m_sequence;
};
