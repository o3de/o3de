/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include "UiAnimViewSequence.h"
#include "UiAnimViewNode.h"
#include "UiAnimViewDopeSheetBase.h"

#include <QScopedPointer>
#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>

#include <QDockWidget>
#endif

namespace Ui {
    class CUiAnimViewTrackPropsDlg;
    class CUiAnimViewKeyPropertiesDlg;
}

class CUiAnimViewKeyPropertiesDlg;

//////////////////////////////////////////////////////////////////////////
class CUiAnimViewKeyUIControls
    : public QObject
    , public _i_reference_target_t
{
    Q_OBJECT
public:
    CUiAnimViewKeyUIControls()
    {
        m_pVarBlock = new CVarBlock;
        m_onSetCallback = AZStd::bind(&CUiAnimViewKeyUIControls::OnInternalVariableChange, this, AZStd::placeholders::_1);
    };

    void SetKeyPropertiesDlg(CUiAnimViewKeyPropertiesDlg* pDlg) { m_pKeyPropertiesDlg = pDlg; }

    // Return internal variable block.
    CVarBlock* GetVarBlock() const { return m_pVarBlock; }

    //////////////////////////////////////////////////////////////////////////
    // Callbacks that must be implemented in derived class
    //////////////////////////////////////////////////////////////////////////
    // Returns true if specified animation track type is supported by this UI.
    virtual bool SupportTrackType(const CUiAnimParamType& paramType, EUiAnimCurveType trackType, EUiAnimValue valueType) const = 0;

    // Called when UI variable changes.
    virtual void OnCreateVars() = 0;

    // Called when user changes selected keys.
    // Return true if control update UI values
    virtual bool OnKeySelectionChange(CUiAnimViewKeyBundle& keys) = 0;

    // Called when UI variable changes.
    virtual void OnUIChange(IVariable* pVar, CUiAnimViewKeyBundle& keys) = 0;

    // Get priority of key UI control, so that specializations can have precedence
    virtual unsigned int GetPriority() const = 0;

protected:
    //////////////////////////////////////////////////////////////////////////
    // Helper functions.
    //////////////////////////////////////////////////////////////////////////
    template <class T>
    void SyncValue(CSmartVariable<T>& var, T& value, bool bCopyToUI, IVariable* pSrcVar = NULL)
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
    std::vector<_smart_ptr<IVariable> > m_registeredVariables;
    CUiAnimViewKeyPropertiesDlg* m_pKeyPropertiesDlg;
    IVariable::OnSetCallback m_onSetCallback;
};

//////////////////////////////////////////////////////////////////////////
class CUiAnimViewTrackPropsDlg
    : public QWidget
{
    Q_OBJECT
public:
    CUiAnimViewTrackPropsDlg(QWidget* parent = 0);
    ~CUiAnimViewTrackPropsDlg();

    void OnSequenceChanged();
    bool OnKeySelectionChange(CUiAnimViewKeyBundle& keys);
    void ReloadKey();

protected:
    void SetCurrKey(CUiAnimViewKeyHandle& keyHandle);

protected slots:
    void OnUpdateTime();

protected:
    CUiAnimViewKeyHandle m_keyHandle;
    QScopedPointer<Ui::CUiAnimViewTrackPropsDlg> ui;
};


//////////////////////////////////////////////////////////////////////////
class UiAnimViewKeys;
class CUiAnimViewKeyPropertiesDlg
    : public QWidget
    , public IUiAnimViewSequenceListener
{
public:
    CUiAnimViewKeyPropertiesDlg(QWidget* hParentWnd);

    void SetKeysCtrl(CUiAnimViewDopeSheetBase* pKeysCtrl)
    {
        m_keysCtrl = pKeysCtrl;
        if (m_keysCtrl)
        {
            m_keysCtrl->SetKeyPropertiesDlg(this);
        }
    }

    void OnSequenceChanged(CUiAnimViewSequence* sequence);

    void PopulateVariables();
    void PopulateVariables(ReflectedPropertyControl& propCtrl);

    // IUiAnimViewSequenceListener
    virtual void OnKeysChanged(CUiAnimViewSequence* pSequence) override;
    virtual void OnKeySelectionChanged(CUiAnimViewSequence* pSequence) override;

protected:
    //////////////////////////////////////////////////////////////////////////
    void OnVarChange(IVariable* pVar);
    void CreateAllVars();
    void AddVars(CUiAnimViewKeyUIControls* pUI);
    void ReloadValues();

protected:
    std::vector< _smart_ptr<CUiAnimViewKeyUIControls> > m_keyControls;

    _smart_ptr<CVarBlock> m_pVarBlock;

    ReflectedPropertyControl* m_wndProps;

    CUiAnimViewTrackPropsDlg* m_wndTrackProps;

    CUiAnimViewDopeSheetBase* m_keysCtrl;

    CUiAnimViewTrack* m_pLastTrackSelected;
};
