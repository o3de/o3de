/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATLControlsResourceDialog.h>

#include <AzCore/StringFunc/StringFunc.h>

#include <ACEEnums.h>
#include <ATLControlsModel.h>
#include <AudioControlsEditorPlugin.h>
#include <QAudioControlTreeWidget.h>

#include <QDialogButtonBox>
#include <QBoxLayout>
#include <QApplication>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QPushButton>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    ATLControlsDialog::ATLControlsDialog(QWidget* pParent, EACEControlType eType)
        : QDialog(pParent)
        , m_eType(eType)
    {
        AZ_Assert(CAudioControlsEditorPlugin::GetATLModel() != nullptr, "ATLControlsDialog - ATL Model is null!");

        setWindowTitle(GetWindowTitle(m_eType));
        setWindowModality(Qt::ApplicationModal);

        QBoxLayout* pLayout = new QBoxLayout(QBoxLayout::TopToBottom);
        setLayout(pLayout);

        m_TextFilterLineEdit = new QLineEdit(this);
        m_TextFilterLineEdit->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
        m_TextFilterLineEdit->setPlaceholderText(QApplication::translate("ATLControlsPanel", "Search", 0));
        connect(m_TextFilterLineEdit, &QLineEdit::textChanged, this, &ATLControlsDialog::SetTextFilter);
        connect(m_TextFilterLineEdit, &QLineEdit::returnPressed, this, &ATLControlsDialog::EnterPressed);
        pLayout->addWidget(m_TextFilterLineEdit, 0);

        m_pControlTree = new QAudioControlsTreeView(this);
        m_pControlTree->header()->setVisible(false);
        m_pControlTree->setEnabled(true);
        m_pControlTree->setAutoScroll(true);
        m_pControlTree->setDragEnabled(false);
        m_pControlTree->setDragDropMode(QAbstractItemView::NoDragDrop);
        m_pControlTree->setDefaultDropAction(Qt::IgnoreAction);
        m_pControlTree->setAlternatingRowColors(false);
        m_pControlTree->setSelectionMode(QAbstractItemView::SingleSelection);
        m_pControlTree->setRootIsDecorated(true);
        m_pControlTree->setSortingEnabled(true);
        m_pControlTree->setAnimated(false);
        m_pControlTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
        pLayout->addWidget(m_pControlTree, 0);

        m_pATLModel = CAudioControlsEditorPlugin::GetATLModel();
        m_pTreeModel = CAudioControlsEditorPlugin::GetControlsTree();
        m_pProxyModel = new QAudioControlSortProxy(this);
        m_pProxyModel->setSourceModel(m_pTreeModel);
        m_pControlTree->setModel(m_pProxyModel);

        pDialogButtons = new QDialogButtonBox(this);
        pDialogButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(pDialogButtons, SIGNAL(accepted()), this, SLOT(accept()));
        connect(pDialogButtons, SIGNAL(rejected()), this, SLOT(reject()));
        pLayout->addWidget(pDialogButtons, 0);

        connect(m_pControlTree->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(UpdateSelectedControl()));
        ApplyFilter();
        UpdateSelectedControl();
        m_pControlTree->setFocus();

        m_pControlTree->installEventFilter(this);
        m_pControlTree->viewport()->installEventFilter(this);
        m_TextFilterLineEdit->installEventFilter(this);

        connect(m_pControlTree->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(StopTrigger()));
    }

    //-------------------------------------------------------------------------------------------//
    ATLControlsDialog::~ATLControlsDialog()
    {
        StopTrigger();
    }

    //-------------------------------------------------------------------------------------------//
    const char* ATLControlsDialog::ChooseItem(const char* currentValue)
    {
        m_sControlName = currentValue;
        if (m_pTreeModel && m_pProxyModel && !m_sControlName.empty())
        {
            QModelIndex index = FindItem(m_sControlName);
            if (index.isValid())
            {
                m_pControlTree->setCurrentIndex(index);
            }
        }

        if (exec() == QDialog::Accepted)
        {
            return m_sControlName.c_str();
        }
        return currentValue;
    }

    //-------------------------------------------------------------------------------------------//
    void ATLControlsDialog::UpdateSelectedControl()
    {
        m_sControlName.clear();
        if (m_pATLModel)
        {
            QModelIndexList indexes = m_pControlTree->selectionModel()->selectedIndexes();
            if (!indexes.empty())
            {
                const CATLControl* control = GetControlFromModelIndex(indexes[0]);
                if (control && IsCriteriaMatch(control))
                {
                    m_sControlName = control->GetName();
                }
            }
        }

        if (QPushButton* pButton = pDialogButtons->button(QDialogButtonBox::Ok))
        {
            pButton->setEnabled(!m_sControlName.empty());
        }
    }

    //-------------------------------------------------------------------------------------------//
    void ATLControlsDialog::SetTextFilter(QString filter)
    {
        m_sFilter = filter;
        ApplyFilter();
    }

    //-------------------------------------------------------------------------------------------//
    void ATLControlsDialog::EnterPressed()
    {
        m_sFilter = m_TextFilterLineEdit->text();
        ApplyFilter();
        m_pControlTree->setFocus();
    }

    //-------------------------------------------------------------------------------------------//
    void ATLControlsDialog::ApplyFilter()
    {
        QModelIndex index = m_pProxyModel->index(0, 0);
        for (int i = 0; index.isValid(); ++i)
        {
            ApplyFilter(index);
            index = index.sibling(i, 0);
        }
        if (!m_sFilter.isEmpty())
        {
            m_pControlTree->expandAll();
        }
    }

    //-------------------------------------------------------------------------------------------//
    bool ATLControlsDialog::ApplyFilter(const QModelIndex parent)
    {
        if (parent.isValid())
        {
            bool bChildValid = false;
            QModelIndex child = parent.model()->index(0, 0, parent);
            for (int i = 1; child.isValid(); ++i)
            {
                if (ApplyFilter(child))
                {
                    bChildValid = true;
                }
                child = parent.model()->index(i, 0, parent);
            }

            if (bChildValid || IsValid(parent))
            {
                m_pControlTree->setRowHidden(parent.row(), parent.parent(), false);
                return true;
            }
            else
            {
                m_pControlTree->setRowHidden(parent.row(), parent.parent(), true);
            }
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    bool ATLControlsDialog::IsValid(const QModelIndex index)
    {
        const QString sName = index.data(Qt::DisplayRole).toString();
        if (m_sFilter.isEmpty() || sName.contains(m_sFilter, Qt::CaseInsensitive))
        {
            if (index.data(eDR_TYPE) == eIT_AUDIO_CONTROL)
            {
                return IsCriteriaMatch(GetControlFromModelIndex(index));
            }
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    bool ATLControlsDialog::IsCriteriaMatch(const CATLControl* control) const
    {
        if (control)
        {
            AZStd::string sControlScope = control->GetScope();
            if (control->GetType() == m_eType && (sControlScope.empty() || AZ::StringFunc::Equal(sControlScope.c_str(), m_sScope.c_str())))
            {
                // LY-22029 - Only allow selecting Preload controls that are non-AutoLoad.
                // This change applies everywhere, to all Preload selectors, so if there's ever a need to select preloads
                // that are AutoLoad, then that option would need to be introduced to the resource selectors.
                // Note, there are no validation mechanisms in place if a user selects a preload, then later switches
                // it to be AutoLoad in the ACEditor.  Any property containing that preload wouldn't know about it.
                // Fortunately, attempting to load one that's already been loaded won't double-dip.
                if (m_eType == eACET_PRELOAD && control->IsAutoLoad())
                {
                    return false;
                }
                return true;
            }
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    const CATLControl* ATLControlsDialog::GetControlFromModelIndex(const QModelIndex index) const
    {
        CID controlId = index.isValid() ? index.data(eDR_ID).toUInt() : ACE_INVALID_CID;
        if (controlId != ACE_INVALID_CID)
        {
            return m_pATLModel->GetControlByID(controlId);
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    QString ATLControlsDialog::GetWindowTitle(EACEControlType type) const
    {
        switch (type)
        {
        case eACET_TRIGGER:
            return QString(tr("Choose Trigger..."));
        case eACET_RTPC:
            return QString(tr("Choose Rtpc..."));
        case eACET_SWITCH:
            return QString(tr("Choose Switch..."));
        case eACET_SWITCH_STATE:
            return QString(tr("Choose Switch State..."));
        case eACET_ENVIRONMENT:
            return QString(tr("Choose Environment..."));
        case eACET_PRELOAD:
            return QString(tr("Choose Preload..."));
        default:
            return QString(tr("Choose..."));
        }
    }

    //-------------------------------------------------------------------------------------------//
    QSize ATLControlsDialog::sizeHint() const
    {
        return QSize(400, 900);
    }

    //-------------------------------------------------------------------------------------------//
    void ATLControlsDialog::SetScope(const AZStd::string& sScope)
    {
        m_sScope = sScope;
        ApplyFilter();
    }

    //-------------------------------------------------------------------------------------------//
    QModelIndex ATLControlsDialog::FindItem(const AZStd::string_view sControlName)
    {
        if (m_pTreeModel && m_pATLModel)
        {
            QModelIndexList indexes = m_pTreeModel->match(m_pTreeModel->index(0, 0, QModelIndex()), Qt::DisplayRole, QString(sControlName.data()), -1, Qt::MatchRecursive);
            if (!indexes.empty())
            {
                const int size = indexes.size();
                for (int i = 0; i < size; ++i)
                {
                    QModelIndex index = indexes[i];
                    if (index.isValid() && (index.data(eDR_TYPE) == eIT_AUDIO_CONTROL))
                    {
                        if (IsCriteriaMatch(GetControlFromModelIndex(index)))
                        {
                            return m_pProxyModel->mapFromSource(index);
                        }
                    }
                }
            }
        }
        return QModelIndex();
    }

    //-------------------------------------------------------------------------------------------//
    bool ATLControlsDialog::eventFilter(QObject* pObject, QEvent* pEvent)
    {
        if (pEvent->type() == QEvent::FocusIn && pObject == m_TextFilterLineEdit)
        {
            // Clear the selection when moving to the line edit, so that the enter key does not select the item.
            m_pControlTree->clearSelection();
            UpdateSelectedControl();
        }

        if (pEvent->type() == QEvent::KeyRelease)
        {
            QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
            if (pKeyEvent && pKeyEvent->key() == Qt::Key_Space)
            {
                QModelIndex index = m_pControlTree->currentIndex();
                if (index.isValid() && index.data(eDR_TYPE) == eIT_AUDIO_CONTROL)
                {
                    CAudioControlsEditorPlugin::ExecuteTrigger(index.data(Qt::DisplayRole).toString().toUtf8().data());
                }
            }
        }

        // double-clicking an item will accept it when valid (not a folder, wrong type)...
        if (pEvent->type() == QEvent::MouseButtonDblClick)
        {
            QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
            if (pMouseEvent && pMouseEvent->button() == Qt::LeftButton)
            {
                QModelIndex index = m_pControlTree->currentIndex();
                if (index.isValid() && IsValid(index) && index.data(eDR_TYPE) != eIT_FOLDER)
                {
                    accept();
                }
            }
        }
        return QWidget::eventFilter(pObject, pEvent);
    }

    //-------------------------------------------------------------------------------------------//
    void ATLControlsDialog::showEvent(QShowEvent* e)
    {
        QDialog::showEvent(e);
        window()->resize(sizeHint()); 
    }

    //-------------------------------------------------------------------------------------------//
    void ATLControlsDialog::StopTrigger()
    {
        CAudioControlsEditorPlugin::StopTriggerExecution();
    }

} // namespace AudioControls

#include <Source/Editor/moc_ATLControlsResourceDialog.cpp>
