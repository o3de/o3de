/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Utils.h>
#include <QWidget>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#endif

#pragma once

namespace PhysX
{
    namespace Editor
    {
        /// Represents a column header in the table view.
        ///
        class ColumnHeader
            : public QWidget
        {
            Q_OBJECT
        public:
            struct Data
            {
                AZStd::string m_name;
                AzPhysics::CollisionLayer m_layer;
            };

            ColumnHeader(QWidget* parent, const Data& column);
            QSize sizeHint() const override;
            QSize minimumSizeHint() const override;

        private:
            QLabel* m_label;
            Data m_col;
        };

        /// Represents a row header in the table view which can be edited.
        ///
        class RowHeader
            : public QWidget
        {
            Q_OBJECT
        public:
            static const AZ::u32 s_maxCollisionGroupNameLength = 32;

            /// Default collision group name when a new collision group is added.
            static const AZStd::string defaultGroupName;

            struct Data
            {
                AzPhysics::CollisionGroups::Id m_groupId;
                AZStd::string m_groupName;
                AzPhysics::CollisionGroup m_group;
                bool m_readOnly;
            };

            RowHeader(QWidget* parent, const Data& data, Physics::Utils::NameSet& nameSet);
            QSize sizeHint() const override;
            QSize minimumSizeHint() const override;
            AzPhysics::CollisionGroups::Id GetGroupId() const;
            const AZStd::string& GetGroupName() const;

        signals:
            void OnGroupRenamed(AzPhysics::CollisionGroups::Id groupId, const AZStd::string& newName);

        private:
            void OnEditingFinished();
            void OnTextChanged(const QString& newText);

            /**
             * Corrects group name if it is not unique in the set of names.
             * @return true function modified group name, false otherwise.
             */
            bool ForceUniqueGroupName(AZStd::string& groupNameOut);

            /**
             * Checks group name's length, validity, and uniqueness.
             * @return true function modified group name, false otherwise.
             */
            bool SanitizeGroupName(AZStd::string& groupNameOut);
            
            AZStd::string m_nameBeforeEdit;
            QLineEdit* m_text;
            Data m_row;
            Physics::Utils::NameSet* m_nameSet;
        };

        /// Represents a checkbox in the table view.
        ///
        class Cell
            : public QWidget
        {
            Q_OBJECT
        public:
            struct Data
            {
                ColumnHeader::Data column;
                RowHeader::Data row;
            };

            Cell(QWidget* parent, const Data& cell);
            QSize sizeHint() const override;

        signals:
            void OnLayerChanged(AzPhysics::CollisionGroups::Id groupId, const AzPhysics::CollisionLayer& layer, bool enabled);

        private:
            void OnCheckboxChanged(int state);

            QCheckBox * m_checkBox;
            Data m_cell;
        };

        /// Widget for editing collision groups.
        ///
        class CollisionGroupsWidget
            : public QWidget
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CollisionGroupsWidget, AZ::SystemAllocator);

            explicit CollisionGroupsWidget(QWidget* parent = nullptr);

            void SetValue(const AzPhysics::CollisionGroups& groups, const AzPhysics::CollisionLayers& layers);
            const AzPhysics::CollisionGroups& GetValue() const;

        signals:
            void onValueChanged(const AzPhysics::CollisionGroups& newValue);

        private:
            void CreateLayout();
            void RecreateGridLayout();
            void ClearWidgets();
            void PopulateTableView();

            /// Add a single collision group (a row) to the UI.
            void AddGroupTableView();

            /// Remove a single collision group (a row) from the UI.
            void RemoveGroupTableView(AzPhysics::CollisionGroups::Id groupId);

            void AddWidgetRemoveButton(const RowHeader::Data& rowData
                , int row
                , int column);

            void AddWidgetColumnHeader(const ColumnHeader::Data& columnData
                , int row
                , int column);

            void AddWidgetRowHeader(RowHeader::Data& rowData
                , int row
                , int column);

            void AddWidgetCell(const RowHeader::Data& rowData
                , const ColumnHeader::Data& columnData
                , int row
                , int column);

            // Slots
            void AddGroup();
            void RemoveGroup(AzPhysics::CollisionGroups::Id groupId);
            void RenameGroup(AzPhysics::CollisionGroups::Id groupId, const AZStd::string& newName);
            void EnableLayer(AzPhysics::CollisionGroups::Id groupId, const AzPhysics::CollisionLayer& layer, bool enabled);

            AZStd::vector<RowHeader::Data> GetRows();
            AZStd::vector<ColumnHeader::Data> GetColumns();
            AZ::u64 GetColumnCount() const;

            QList<QWidget*> m_widgets;
            QGridLayout* m_gridLayout;
            QVBoxLayout* m_mainLayout;
            AzPhysics::CollisionGroups m_groups;
            AzPhysics::CollisionLayers m_layers;
            Physics::Utils::NameSet m_nameSet;
        };

    } // namespace Editor
} // namespace PhysX

