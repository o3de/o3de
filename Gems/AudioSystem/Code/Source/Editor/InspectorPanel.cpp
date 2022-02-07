/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <InspectorPanel.h>

#include <ACETypes.h>
#include <AudioControlsEditorPlugin.h>
#include <IAudioSystemControl.h>
#include <QAudioControlEditorIcons.h>

#include <QMessageBox>
#include <QMimeData>
#include <QDropEvent>
#include <QKeyEvent>


namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    CInspectorPanel::CInspectorPanel(CATLControlsModel* atlControlsModel)
        : m_atlControlsModel(atlControlsModel)
        , m_selectedType(eACET_NUM_TYPES)
        , m_notFoundColor(QColor(255, 128, 128))
        , m_allControlsSameType(true)
    {
        AZ_Assert(m_atlControlsModel, "CInspectorPanel - The ATL Controls model is null!");

        setupUi(this);

        connect(m_nameLineEditor, SIGNAL(editingFinished()), this, SLOT(FinishedEditingName()));
        connect(m_scopeDropDown, SIGNAL(activated(QString)), this, SLOT(SetControlScope(QString)));
        connect(m_autoLoadCheckBox, SIGNAL(clicked(bool)), this, SLOT(SetAutoLoadForCurrentControl(bool)));

        // data validators
        m_nameLineEditor->setValidator(new QRegExpValidator(QRegExp("^[a-zA-Z0-9_]*$"), m_nameLineEditor));

        m_atlControlsModel->AddListener(this);

        Reload();
    }

    //-------------------------------------------------------------------------------------------//
    CInspectorPanel::~CInspectorPanel()
    {
        m_atlControlsModel->RemoveListener(this);
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::Reload()
    {
        UpdateScopeData();
        UpdateInspector();
        UpdateConnectionData();
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::SetSelectedControls(const ControlList& selectedControls)
    {
        m_selectedType = eACET_NUM_TYPES;
        m_selectedControls.clear();
        m_allControlsSameType = true;
        const size_t size = selectedControls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* control = m_atlControlsModel->GetControlByID(selectedControls[i]);
            if (control)
            {
                m_selectedControls.push_back(control);
                if (m_selectedType == eACET_NUM_TYPES)
                {
                    m_selectedType = control->GetType();
                }
                else if (m_allControlsSameType && (m_selectedType != control->GetType()))
                {
                    m_allControlsSameType = false;
                }
            }
        }

        UpdateInspector();
        UpdateConnectionData();
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateConnectionListControl()
    {
        if ((m_selectedControls.size() == 1) && m_allControlsSameType && m_selectedType != eACET_SWITCH)
        {
            if (m_selectedType == eACET_PRELOAD)
            {
                m_connectedControlsLabel->setText(QString(tr("Sound Banks:")));
            }
            else
            {
                m_connectedControlsLabel->setText(QString(tr("Connected Controls:")));
            }

            m_connectedControlsLabel->setHidden(false);
            m_connectionList->setHidden(false);
        }
        else
        {
            m_connectedControlsLabel->setHidden(true);
            m_connectionList->setHidden(true);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateScopeControl()
    {
        const size_t size = m_selectedControls.size();
        if (m_allControlsSameType)
        {
            if (size == 1)
            {
                CATLControl* control = m_selectedControls[0];
                if (control)
                {
                    if (m_selectedType == eACET_SWITCH_STATE)
                    {
                        HideScope(true);
                    }
                    else
                    {
                        QString scope = QString(control->GetScope().c_str());
                        if (scope.isEmpty())
                        {
                            m_scopeDropDown->setCurrentIndex(0);
                        }
                        else
                        {
                            int index = m_scopeDropDown->findText(scope);
                            m_scopeDropDown->setCurrentIndex(index);
                        }
                        HideScope(false);
                    }
                }
            }
            else
            {
                bool isSameScope = true;
                AZStd::string scope;
                for (int i = 0; i < size; ++i)
                {
                    CATLControl* control = m_selectedControls[i];
                    if (control)
                    {
                        AZStd::string controlScope = control->GetScope();
                        if (controlScope.empty())
                        {
                            controlScope = "Global";
                        }

                        if (!scope.empty() && controlScope != scope)
                        {
                            isSameScope = false;
                            break;
                        }
                        scope = controlScope;
                    }
                }

                if (isSameScope)
                {
                    int index = m_scopeDropDown->findText(QString(scope.c_str()));
                    m_scopeDropDown->setCurrentIndex(index);
                }
                else
                {
                    m_scopeDropDown->setCurrentIndex(-1);
                }
            }
        }
        else
        {
            HideScope(true);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateNameControl()
    {
        const size_t size = m_selectedControls.size();
        if (m_allControlsSameType)
        {
            if (size == 1)
            {
                CATLControl* control = m_selectedControls[0];
                if (control)
                {
                    m_nameLineEditor->setText(QString(control->GetName().c_str()));
                    m_nameLineEditor->setEnabled(true);
                }
            }
            else
            {
                m_nameLineEditor->setText(" <" +  QString::number(size) + tr(" items selected>"));
                m_nameLineEditor->setEnabled(false);
            }
        }
        else
        {
            m_nameLineEditor->setText(" <" +  QString::number(size) + tr(" items selected>"));
            m_nameLineEditor->setEnabled(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateInspector()
    {
        if (!m_selectedControls.empty())
        {
            m_propertiesPanel->setHidden(false);
            m_emptyInspectorLabel->setHidden(true);
            UpdateNameControl();
            UpdateScopeControl();
            UpdateAutoLoadControl();
            UpdateConnectionListControl();
        }
        else
        {
            m_propertiesPanel->setHidden(true);
            m_emptyInspectorLabel->setHidden(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateConnectionData()
    {
        if ((m_selectedControls.size() == 1) && m_selectedType != eACET_SWITCH)
        {
            m_connectionList->SetControl(m_selectedControls[0]);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateAutoLoadControl()
    {
        if (!m_selectedControls.empty() && m_allControlsSameType && m_selectedType == eACET_PRELOAD)
        {
            HideAutoLoad(false);
            bool hasAutoLoad = false;
            bool hasNonAutoLoad = false;
            const size_t size = m_selectedControls.size();
            for (int i = 0; i < size; ++i)
            {
                CATLControl* control = m_selectedControls[i];
                if (control)
                {
                    if (control->IsAutoLoad())
                    {
                        hasAutoLoad = true;
                    }
                    else
                    {
                        hasNonAutoLoad = true;
                    }
                }
            }

            if (hasAutoLoad && hasNonAutoLoad)
            {
                m_autoLoadCheckBox->setTristate(true);
                m_autoLoadCheckBox->setCheckState(Qt::PartiallyChecked);
            }
            else
            {
                if (hasAutoLoad)
                {
                    m_autoLoadCheckBox->setChecked(true);
                }
                else
                {
                    m_autoLoadCheckBox->setChecked(false);
                }
                m_autoLoadCheckBox->setTristate(false);
            }
        }
        else
        {
            HideAutoLoad(true);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateScopeData()
    {
        m_scopeDropDown->clear();
        for (int j = 0; j < m_atlControlsModel->GetScopeCount(); ++j)
        {
            SControlScope scope = m_atlControlsModel->GetScopeAt(j);
            m_scopeDropDown->insertItem(0, QString(scope.name.c_str()));
            if (scope.bOnlyLocal)
            {
                m_scopeDropDown->setItemData(0, m_notFoundColor, Qt::ForegroundRole);
                m_scopeDropDown->setItemData(0, "Level not found but it is referenced by some audio controls", Qt::ToolTipRole);
            }
            else
            {
                m_scopeDropDown->setItemData(0, "", Qt::ToolTipRole);
            }
        }
        m_scopeDropDown->insertItem(0, tr("Global"));
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::SetControlName(QString name)
    {
        if (m_selectedControls.size() == 1)
        {
            if (!name.isEmpty())
            {
                CUndo undo("Audio Control Name Changed");
                AZStd::string newName = name.toUtf8().data();
                CATLControl* control = m_selectedControls[0];
                if (control && control->GetName() != newName)
                {
                    if (!m_atlControlsModel->IsNameValid(newName, control->GetType(), control->GetScope(), control->GetParent()))
                    {
                        newName = m_atlControlsModel->GenerateUniqueName(newName, control->GetType(), control->GetScope(), control->GetParent());
                    }
                    control->SetName(newName);
                }
            }
            else
            {
                UpdateNameControl();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::SetControlScope(QString scope)
    {
        CUndo undo("Audio Control Scope Changed");
        size_t size = m_selectedControls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* control = m_selectedControls[i];
            if (control)
            {
                QString currentScope = QString(control->GetScope().c_str());
                if (currentScope != scope && (scope != tr("Global") || currentScope != ""))
                {
                    if (scope == tr("Global"))
                    {
                        control->SetScope("");
                    }
                    else
                    {
                        control->SetScope(scope.toUtf8().data());
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::SetAutoLoadForCurrentControl(bool isAutoLoad)
    {
        CUndo undo("Audio Control Auto-Load Property Changed");
        size_t size = m_selectedControls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* control = m_selectedControls[i];
            if (control)
            {
                control->SetAutoLoad(isAutoLoad);
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::FinishedEditingName()
    {
        SetControlName(m_nameLineEditor->text());
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::OnControlModified(AudioControls::CATLControl* control)
    {
        size_t size = m_selectedControls.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (m_selectedControls[i] == control)
            {
                UpdateInspector();
                UpdateConnectionData();
                break;
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::HideScope(bool hide)
    {
        m_scopeLabel->setHidden(hide);
        m_scopeDropDown->setHidden(hide);
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::HideAutoLoad(bool hide)
    {
        m_autoLoadLabel->setHidden(hide);
        m_autoLoadCheckBox->setHidden(hide);
    }

} // namespace AudioControls

#include <Source/Editor/moc_InspectorPanel.cpp>
