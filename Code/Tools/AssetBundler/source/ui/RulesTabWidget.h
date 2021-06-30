/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <source/ui/AssetBundlerTabWidget.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <QAction>
#include <QModelIndex>
#include <QSharedPointer>
#include <QString>
#include <QWidget>
#endif

namespace Ui
{
    class RulesTabWidget;
}

class QItemSelection;

namespace AzToolsFramework
{
    class AssetFileInfoListComparison;
}

namespace AssetBundler
{
    class GUIApplicationManager;

    class RulesFileTableModel;

    class ComparisonDataCard;

    class ComparisonDataWidget;

    class RulesTabWidget
        : public AssetBundlerTabWidget
    {
        Q_OBJECT

    public:
        explicit RulesTabWidget(QWidget* parent, GUIApplicationManager* guiApplicationManager);
        virtual ~RulesTabWidget() {}

        QString GetTabTitle() override { return tr("Rules"); }

        QString GetFileTypeDisplayName() override { return tr("Rules file"); }

        AssetBundlingFileType GetFileType() override { return AssetBundlingFileType::RulesFileType; }

        bool HasUnsavedChanges() override;

        void Reload() override;

        bool SaveCurrentSelection() override;

        bool SaveAll() override;

        void SetModelDataSource() override;

        AzQtComponents::TableView* GetFileTableView() override;

        QModelIndex GetSelectedFileTableIndex() override;

        AssetBundlerAbstractFileTableModel* GetFileTableModel() override;

        void SetActiveProjectLabel(const QString& labelText) override;

        void ApplyConfig() override;

        void FileSelectionChanged(
            const QItemSelection& /*selected*/ = QItemSelection(),
            const QItemSelection& /*deselected*/ = QItemSelection()) override;

    private:
        void OnNewFileButtonPressed();

        void OnRunRuleButtonPressed();

        void MarkFileChanged();

        void RebuildComparisonDataCardList();

        void PopulateComparisonDataCardList();

        void CreateComparisonDataCard(
            AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> comparisonList,
            size_t comparisonDataIndex);

        void RemoveAllComparisonDataCards();

        void AddNewComparisonStep();

        void RemoveComparisonStep(size_t comparisonDataIndex);

        void MoveComparisonStep(size_t startingIndex, size_t destinationIndex);

        void OnAnyTokenNameChanged(size_t comparisonDataIndex);

        QSharedPointer<Ui::RulesTabWidget> m_ui;

        QSharedPointer<RulesFileTableModel> m_fileTableModel;
        QModelIndex m_selectedFileTableIndex;
        AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> m_selectedComparisonRules;

        AZStd::vector<ComparisonDataCard*> m_comparisonDataCardList;

    private slots:
        void OnComparisonDataCardContextMenuRequested(size_t comparisonDataIndex, const QPoint& position);
    };
} // namespace AssetBundler
