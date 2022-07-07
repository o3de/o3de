/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string_view.h>

#include <ACETypes.h>
#include <AudioControlFilters.h>
#include <QTreeWidgetFilter.h>

#include <QWidget>
#include <QDialog>
#include <QLineEdit>
#endif

class QAudioControlsTreeView;
class QDialogButtonBox;
class QAudioControlSortProxy;
class QStandardItemModel;

namespace AudioControls
{
    class CATLControlsModel;
    class CATLControl;

    //-------------------------------------------------------------------------------------------//
    class ATLControlsDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        ATLControlsDialog(QWidget* pParent, EACEControlType eType);
        ~ATLControlsDialog();

    private slots:
        void UpdateSelectedControl();
        void SetTextFilter(QString filter);
        void EnterPressed();
        void StopTrigger();

    public:
        void SetScope(const AZStd::string& sScope);
        const char* ChooseItem(const char* currentValue);
        QSize sizeHint() const override;
        void showEvent(QShowEvent* e) override;

    private:

        QModelIndex FindItem(const AZStd::string_view sControlName);
        void ApplyFilter();
        bool ApplyFilter(const QModelIndex parent);
        bool IsValid(const QModelIndex index);
        bool IsCriteriaMatch(const CATLControl* control) const;
        const CATLControl* GetControlFromModelIndex(const QModelIndex index) const;
        QString GetWindowTitle(EACEControlType type) const;

        // ------------------ QWidget ----------------------------
        bool eventFilter(QObject* pObject, QEvent* pEvent) override;
        // -------------------------------------------------------

        // Filtering
        QString m_sFilter;
        EACEControlType m_eType;
        AZStd::string m_sScope;

        AZStd::string m_sControlName;
        QAudioControlsTreeView* m_pControlTree;
        QDialogButtonBox* pDialogButtons;
        QLineEdit* m_TextFilterLineEdit;

        QStandardItemModel* m_pTreeModel;
        QAudioControlSortProxy* m_pProxyModel;
        CATLControlsModel* m_pATLModel;
    };
} // namespace AudioControls
