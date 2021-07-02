/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/TableView.h>
#endif

namespace AzQtComponents
{
    class Style;
    class TreeViewWatcher;

    class AZ_QT_COMPONENTS_API TreeView
    {
    public:
        struct Config
        {
            int branchLineWidth;
            QColor branchLineColor;
        };

        static Config loadConfig(QSettings& settings);

        static Config defaultConfig();

        static void setBranchLinesEnabled(QTreeView* widget, bool enabled);
        static bool isBranchLinesEnabled(const QTreeView* widget);

    private:
        static constexpr const auto IndentationWidth = 20;

        friend class Style;

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
        static QPointer<TreeViewWatcher> s_treeViewWatcher;
        AZ_POP_DISABLE_WARNING

        static unsigned int s_watcherReferenceCount;

        static void initializeWatcher();
        static void uninitializeWatcher();

        static bool drawBranchIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static bool polish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config);
    };

    // If branch lines are displayed we need to shift the row contents to the right to
    // make room for the arrow, thus this delegate.
    class AZ_QT_COMPONENTS_API BranchDelegate
        : public QStyledItemDelegate
    {
        Q_OBJECT

        using QStyledItemDelegate::QStyledItemDelegate;

    protected:
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex &index) override;
        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    };

} // namespace AzQtComponents
