/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <AudioControl.h>
#include <AudioControlFilters.h>
#include <IAudioInterfacesCommonData.h>
#include <QTreeWidgetFilter.h>

#include <QWidget>
#include <QMenu>
#include <QLabel>

#include <Source/Editor/ui_ATLControlsPanel.h>
#endif

// Forward declarations
namespace Audio
{
    struct IAudioProxy;
} // namespace Audio

class QStandardItemModel;
class QStandardItem;
class QSortFilterProxyModel;

namespace AudioControls
{
    class CATLControlsModel;
    class QFilterButton;
    class IAudioSystemEditor;
    class QATLTreeModel;

    //-------------------------------------------------------------------------------------------//
    class CATLControlsPanel
        : public QWidget
        , public Ui::ATLControlsPanel
        , public IATLControlModelListener
    {
        Q_OBJECT

    public:
        CATLControlsPanel(CATLControlsModel* pATLModel, QATLTreeModel* pATLControlsModel);
        ~CATLControlsPanel();

        ControlList GetSelectedControls();
        void Reload();

    private:
        // Filtering
        void ResetFilters();
        void ApplyFilter();
        bool ApplyFilter(const QModelIndex parent);
        bool IsValid(const QModelIndex index);
        void ShowControlType(EACEControlType type, bool bShow, bool bExclusive);

        // Helper Functions
        void SelectItem(QStandardItem* pItem);
        void DeselectAll();
        QStandardItem* GetCurrentItem();
        CATLControl* GetControlFromItem(QStandardItem* pItem);
        CATLControl* GetControlFromIndex(QModelIndex index);
        bool IsValidParent(QStandardItem* pParent, const EACEControlType eControlType);

        void HandleExternalDropEvent(QDropEvent* pDropEvent);

        // ------------- IATLControlModelListener ----------------
        void OnControlAdded(CATLControl* pControl) override;
        // -------------------------------------------------------

        // ------------------ QWidget ----------------------------
        bool eventFilter(QObject* pObject, QEvent* pEvent) override;
        // -------------------------------------------------------

    private slots:
        void ItemModified(QStandardItem* pItem);

        QStandardItem* CreateFolder();
        QStandardItem* AddControl(CATLControl* pControl);

        // Create controls / folders
        void CreateRTPCControl();
        void CreateSwitchControl();
        void CreateStateControl();
        void CreateTriggerControl();
        void CreateEnvironmentsControl();
        void CreatePreloadControl();
        void DeleteSelectedControl();

        void ShowControlsContextMenu(const QPoint& pos);

        // Filtering
        void SetFilterString(const QString& text);
        void ShowTriggers(bool bShow);
        void ShowRTPCs(bool bShow);
        void ShowEnvironments(bool bShow);
        void ShowSwitches(bool bShow);
        void ShowPreloads(bool bShow);
        void ShowUnassigned(bool bShow);

        // Audio Preview
        void ExecuteControl();
        void StopControlExecution();

    signals:
        void SelectedControlChanged();
        void ControlTypeFiltered(EACEControlType type, bool bShow);

    private:
        QSortFilterProxyModel* m_pProxyModel;
        QATLTreeModel* m_pTreeModel;
        CATLControlsModel* const m_pATLModel;

        // Context Menu
        QMenu m_addItemMenu;

        // Filtering
        QString m_sFilter;
        QMenu m_filterMenu;
        QFilterButton* m_pControlTypeFilterButtons[eACET_NUM_TYPES];
        QFilterButton* m_unassignedFilterButton;
        bool m_visibleTypes[eACET_NUM_TYPES];
        bool m_showUnassignedControls;
    };

    class QFilterButton
        : public QWidget
    {
        Q_OBJECT
    public:
        QFilterButton(const QIcon& icon, const QString& text, QWidget* parent = 0);

        void SetText(const QString& text){ m_actionText.setText(text); }

        void SetChecked(bool checked);

    signals:
        void clicked(bool checked = false);

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;

        QLabel m_checkIcon;
        QLabel m_filterIcon;
        QLabel m_actionText;
        QWidget* m_background;
        bool m_checked = true;
    };

} // namespace AudioControls
