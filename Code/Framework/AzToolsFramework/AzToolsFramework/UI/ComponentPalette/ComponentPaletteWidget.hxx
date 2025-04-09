/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/map.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/SearchWidget/SearchWidgetTypes.hxx>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QFrame>
#include <QString>
AZ_POP_DISABLE_WARNING
#endif

class QLineEdit;
class QPushButton;
class QSortFilterProxyModel;
class QTreeView;

namespace AZ
{
    class SerializeContext;
};

namespace AzToolsFramework
{
    class ComponentPaletteModel;

    class ComponentPaletteWidget
        : public QFrame
    {
        Q_OBJECT

    public:

        ComponentPaletteWidget(QWidget* parent, bool enableSearch);

        void Populate(
            AZ::SerializeContext* serializeContext,
            const AzToolsFramework::EntityIdList& selectedEntityIds,
            const AzToolsFramework::ComponentFilter& componentFilter,
            AZStd::span<const AZ::ComponentServiceType> serviceFilter,
            AZStd::span<const AZ::ComponentServiceType> incompatibleServiceFilter);

        void Present();

    Q_SIGNALS:
        void OnAddComponentBegin();
        void OnAddComponentEnd();
        void OnAddComponentCancel();

    protected:
        void focusOutEvent(QFocusEvent* event) override;

    private slots:
        void UpdateContent();
        void QueueUpdateSearch();
        void UpdateSearch();
        void ClearSearch();
        void ActivateSelection(const QModelIndex& index);
        void ExpandCategory(const QModelIndex& index);
        void CollapseCategory(const QModelIndex& index);
        void FocusSearchBox();
        void FocusComponentTree();

    private:
        bool eventFilter(QObject* object, QEvent* event) override;
        bool BranchHasNoChildren(QStandardItem* item);
        void SetExpanded(QModelIndex itemIndex);

        QRegExp m_searchRegExp;
        QFrame* m_searchFrame = nullptr;
        QLineEdit* m_searchText = nullptr;
        QTreeView* m_componentTree = nullptr;
        ComponentPaletteModel* m_componentModel = nullptr;
        AZ::SerializeContext* m_serializeContext = nullptr;
        EntityIdList m_selectedEntityIds;
        ComponentFilter m_componentFilter;
        AZ::ComponentDescriptor::DependencyArrayType m_serviceFilter;
        AZ::ComponentDescriptor::DependencyArrayType m_incompatibleServiceFilter;
        AZStd::map<QString, bool> m_categoryExpandedState;
    };
}
