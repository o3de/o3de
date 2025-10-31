/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <Document/ShaderManagementConsoleDocumentRequestBus.h>

#include <QStandardItemModel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#endif

namespace ShaderManagementConsole
{
    enum ColumnSortMode
    {
        Alpha, Rank, Cost
    };

    class ShaderManagementConsoleTableView
        : public QTableWidget
        , public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleTableView, AZ::SystemAllocator);
        ShaderManagementConsoleTableView(const AZ::Crc32& toolId, const AZ::Uuid& documentId, QWidget* parent);
        ~ShaderManagementConsoleTableView();

        void SetColumnSortMode(ColumnSortMode);

    protected:
        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;

        void RebuildTable();
        void OnCellSelected(int row, int column, int previousRow, int previousColumn);
        void OnCellChanged(int row, int column);

        void ShowContextMenu(const QPoint& pos);

        void keyPressEvent(QKeyEvent*) override;
        void mousePressEvent(QMouseEvent* event) override;

        enum RebuildMode { KeepAsIs, CallOnModified };
        void TransferViewModelToModel(RebuildMode);

        enum class CountQuery { ForUi, Options };
        int GetColumnsCount(CountQuery) const;
        int UiColumnToOption(int uiColumnIndex) const;

        const AZ::Crc32 m_toolId = {};
        const AZ::Uuid m_documentId = AZ::Uuid::CreateNull();
        AZ::RPI::ShaderVariantListSourceData m_shaderVariantListSourceData;
        AZStd::vector<AZ::RPI::ShaderOptionDescriptor> m_shaderOptionDescriptors;
        size_t m_shaderVariantCount = {};
        size_t m_shaderOptionCount = {};
        ColumnSortMode m_columnSortMode = Cost;
        QIcon m_emptyOptionIcon;
    };

    class ShaderManagementConsoleContainer : public QVBoxLayout
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleContainer, AZ::SystemAllocator);

        ShaderManagementConsoleContainer(QWidget* container, const AZ::Crc32& toolId, const AZ::Uuid& documentId, QWidget* parent);

        ShaderManagementConsoleTableView m_tableView;
        QHBoxLayout m_subLayout;
        QLabel m_sortLabel;
        QComboBox m_sortComboBox;
        QPushButton m_defragVariants;
    };
} // namespace ShaderManagementConsole
