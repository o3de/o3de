/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AtomToolsFramework/Inspector/InspectorRequestBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QVBoxLayout>
#include <QWidget>
AZ_POP_DISABLE_WARNING
#endif

namespace Ui
{
    class InspectorWidget;
}

namespace AtomToolsFramework
{
    class InspectorGroupHeaderWidget;

    //! Provides controls for viewing and editing object settings.
    //! The settings can be divided into groups, with each one showing a subset of properties.
    class InspectorWidget
        : public QWidget
        , public InspectorRequestBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(InspectorWidget, AZ::SystemAllocator, 0);

        explicit InspectorWidget(QWidget* parent = nullptr);
        ~InspectorWidget() override;

        // InspectorRequestBus::Handler overrides...
        void AddHeading(QWidget* headingWidget) override;

        void ClearHeading() override;

        void Reset() override;

        void AddGroupsBegin() override;

        void AddGroupsEnd() override;

        void AddGroup(
            const AZStd::string& groupName,
            const AZStd::string& groupDisplayName,
            const AZStd::string& groupDescription,
            QWidget* groupWidget) override;

        void SetGroupVisible(const AZStd::string& groupName, bool visible) override;
        bool IsGroupVisible(const AZStd::string& groupName) const override;
        bool IsGroupHidden(const AZStd::string& groupName) const override;

        void RefreshGroup(const AZStd::string& groupName) override;
        void RebuildGroup(const AZStd::string& groupName) override;

        void RefreshAll() override;
        void RebuildAll() override;

        void ExpandGroup(const AZStd::string& groupName) override;
        void CollapseGroup(const AZStd::string& groupName) override;
        bool IsGroupExpanded(const AZStd::string& groupName) const override;

        void ExpandAll() override;
        void CollapseAll() override;

    protected:
        virtual bool ShouldGroupAutoExpanded(const AZStd::string& groupName) const;
        virtual void OnGroupExpanded(const AZStd::string& groupName);
        virtual void OnGroupCollapsed(const AZStd::string& groupName);
        virtual void OnHeaderClicked(const AZStd::string& groupName, QMouseEvent* event);

    private:
        QScopedPointer<Ui::InspectorWidget> m_ui;

        struct GroupWidgetPair
        {
            InspectorGroupHeaderWidget* m_header;
            QWidget* m_panel;
        };

        AZStd::unordered_map<AZStd::string, GroupWidgetPair> m_groups;
    };
} // namespace AtomToolsFramework
