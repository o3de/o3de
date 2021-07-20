/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/SystemBus.h>
#include <QBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QScrollArea>

#include <Editor/CollisionGroupsWidget.h>

namespace PhysX
{
    namespace Editor
    {
        static const int s_columnWidthBuffer = 15;
        static const int s_rowHeight = 25;
        static const int s_rowHeaderWidth = 100;
        static const int s_rowHeaderWidthBuffer = 15;
        static const int s_buttonWidth = 100;

        Cell::Cell(QWidget* parent, const Data& cell)
            : QWidget(parent)
            , m_cell(cell)
        {
            bool isEnabled = cell.row.m_group.IsSet(cell.column.m_layer);
            m_checkBox = new QCheckBox();
            m_checkBox->setChecked(isEnabled);
            m_checkBox->setContentsMargins(0, 0, 0, 0);
            m_checkBox->setEnabled(!cell.row.m_readOnly);

            connect(m_checkBox, &QCheckBox::stateChanged, this, &Cell::OnCheckboxChanged);

            QHBoxLayout* layout = new QHBoxLayout();
            layout->setAlignment(Qt::AlignHCenter);
            layout->addWidget(m_checkBox);

            setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            setLayout(layout);
        }
        
        QSize Cell::sizeHint() const
        {
            return m_checkBox->sizeHint();
        }

        void Cell::OnCheckboxChanged(int state)
        {
            bool enabled = state == Qt::CheckState::Checked;
            emit OnLayerChanged(m_cell.row.m_groupId, m_cell.column.m_layer, enabled);
        }

        ColumnHeader::ColumnHeader(QWidget* parent, const Data& column)
            : QWidget(parent)
            , m_col(column)
        {
            m_label = new QLabel();
            m_label->setText(m_col.m_name.c_str());
            m_label->setAlignment(Qt::AlignHCenter);

            QHBoxLayout* layout = new QHBoxLayout();
            layout->setAlignment(Qt::AlignHCenter);
            layout->addWidget(m_label);

            setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            setLayout(layout);
        }

        QSize ColumnHeader::sizeHint() const
        {
            QSize size = m_label->sizeHint();
            size.setWidth(size.width() + s_columnWidthBuffer);
            return size;
        }

        QSize ColumnHeader::minimumSizeHint() const
        {
            QSize size = m_label->sizeHint();
            size.setWidth(size.width() + s_columnWidthBuffer);
            size.setHeight(s_rowHeight);
            return size;
        }

        const AZStd::string RowHeader::defaultGroupName = "NewGroup";

        bool RowHeader::ForceUniqueGroupName(AZStd::string& groupNameOut)
        {
            if (m_nameSet->find(groupNameOut) == m_nameSet->end())
            {
                return false;
            }

            Physics::Utils::MakeUniqueString(*m_nameSet
                , groupNameOut
                , s_maxCollisionGroupNameLength);
            return true;
        }

        bool RowHeader::SanitizeGroupName(AZStd::string& groupNameOut)
        {
            bool nameModified = false;
            if (groupNameOut.length() == 0)
            {
                groupNameOut = defaultGroupName;
                nameModified = true;
            }
            else if (groupNameOut.length() > s_maxCollisionGroupNameLength)
            {
                groupNameOut = groupNameOut.substr(0, s_maxCollisionGroupNameLength);
                nameModified = true;
            }

            nameModified = nameModified || ForceUniqueGroupName(groupNameOut);

            if (nameModified)
            {
                AZ_Warning("PhysX Collision Groups"
                    , false
                    , "Invalid collision group name used. Collision group automatically renamed to: %s"
                    , groupNameOut.c_str());
            }

            return nameModified;
        }

        RowHeader::RowHeader(QWidget* parent
            , const Data& row
            , Physics::Utils::NameSet& nameSet)
            : QWidget(parent)
            , m_row(row)
            , m_nameSet(&nameSet)
        {
            m_text = new QLineEdit(this);
            SanitizeGroupName(m_row.m_groupName);
            m_nameSet->insert(m_row.m_groupName);
            m_text->setText(m_row.m_groupName.c_str());
            m_text->setEnabled(!m_row.m_readOnly);
            m_text->setMaxLength(s_maxCollisionGroupNameLength);

            connect(m_text, &QLineEdit::textEdited, this, &RowHeader::OnTextChanged);
            QWidget::connect(m_text, &QLineEdit::editingFinished, this, &RowHeader::OnEditingFinished);

            setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        }

        void RowHeader::OnEditingFinished()
        {
            if (m_nameBeforeEdit == "")
            {
                return;
            }

            m_nameSet->erase(m_nameBeforeEdit);

            AZStd::string groupName = m_text->text().toUtf8().data();
            if (SanitizeGroupName(groupName))
            {
                m_text->setText(QString(groupName.c_str()));
            }
            m_row.m_groupName = groupName;
            m_nameSet->insert(m_row.m_groupName);

            m_nameBeforeEdit = "";

            emit OnGroupRenamed(m_row.m_groupId, m_row.m_groupName.c_str());
        }

        void RowHeader::OnTextChanged(const QString& newText)
        {
            if (m_nameBeforeEdit == "")
            {
                m_nameBeforeEdit = m_row.m_groupName;
            }
            m_row.m_groupName = newText.toUtf8().data();
            emit OnGroupRenamed(m_row.m_groupId, m_row.m_groupName.c_str());
        }

        QSize RowHeader::sizeHint() const
        {
            return QSize(s_rowHeaderWidth + s_rowHeaderWidthBuffer, s_rowHeight);
        }

        QSize RowHeader::minimumSizeHint() const
        {
            return QSize(s_rowHeaderWidth + s_rowHeaderWidthBuffer, s_rowHeight);
        }

        AzPhysics::CollisionGroups::Id RowHeader::GetGroupId() const
        {
            return m_row.m_groupId;
        }

        const AZStd::string& RowHeader::GetGroupName() const
        {
            return m_row.m_groupName;
        }

        CollisionGroupsWidget::CollisionGroupsWidget(QWidget* parent)
            : QWidget(parent)
        {
            CreateLayout();
        }

        void CollisionGroupsWidget::SetValue(const AzPhysics::CollisionGroups& groups, const AzPhysics::CollisionLayers& layers)
        {
            blockSignals(true);
            m_groups = groups;
            m_layers = layers;
            PopulateTableView();
            blockSignals(false);
        }

        const AzPhysics::CollisionGroups& CollisionGroupsWidget::GetValue() const
        {
            return m_groups;
        }

        void CollisionGroupsWidget::CreateLayout()
        {
            // Hierarchy goes like this:
            // -ThisWidget (CollisionGroupsWidget)
            //   -ScrollLayout (VBox)
            //     -ScrollArea (Widget)
            //       -ScrollContainer (Widget)
            //         -MainLayout (VBox)
            //           -GridLayout
            //           -PushButton

            QVBoxLayout* scrollLayout = new QVBoxLayout();
            scrollLayout->setContentsMargins(0, 0, 0, 0);

            QScrollArea* scrollArea = new QScrollArea();

            QWidget* scrollContainer = new QWidget();

            // Grid layout
            m_gridLayout = new QGridLayout();

            // New group button
            QPushButton* addNewGroup = new QPushButton();
            addNewGroup->setText("Add");
            addNewGroup->setFixedSize(s_buttonWidth + s_rowHeaderWidthBuffer, s_rowHeight);

            m_mainLayout = new QVBoxLayout();
            m_mainLayout->setMargin(0);
            m_mainLayout->setSpacing(0);
            m_mainLayout->setContentsMargins(0, 0, 0, 0);
            m_mainLayout->addLayout(m_gridLayout);
            m_mainLayout->addWidget(addNewGroup,Qt::AlignTop);
            m_mainLayout->addStretch();
            m_mainLayout->setSizeConstraint(QLayout::SizeConstraint::SetMinimumSize);

            scrollLayout->addWidget(scrollArea);

            scrollArea->setWidget(scrollContainer);

            scrollContainer->setLayout(m_mainLayout);

            setLayout(scrollLayout);
            setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            setContentsMargins(0, 0, 0, 0);

            // Connect signals
            connect(addNewGroup, &QPushButton::clicked, this, &CollisionGroupsWidget::AddGroup);
        }

        void CollisionGroupsWidget::RecreateGridLayout()
        {
            m_mainLayout->removeItem(m_gridLayout);
            delete m_gridLayout;
            m_gridLayout = new QGridLayout();
            m_mainLayout->insertLayout(0, m_gridLayout);
        }

        void CollisionGroupsWidget::ClearWidgets()
        {
            qDeleteAll(m_widgets);
            m_widgets.clear();
            m_nameSet.clear();
        }

        void CollisionGroupsWidget::PopulateTableView()

        {
            RecreateGridLayout();
            ClearWidgets();

            auto rows = GetRows();
            auto columns = GetColumns();

            for (int row = 0; row <= rows.size(); ++row)
            {
                const int columnCount = static_cast<int>(columns.size() + 1); // 2 extra columns for row header and 'Remove' button. This value is only meant for for-loop iteration.
                for (int col = 0; col <= columnCount; ++col)
                {
                    if (row == 0 && col == 0)
                    {
                        // Topleft cell is empty
                    }
                    else if (col == columnCount)
                    {
                        if (row == 0)
                        {
                            continue;
                        }
                        else
                        {
                            AZ_Assert(row > 0, "Unexpected row index.");
                            AddWidgetRemoveButton(rows[row - 1], row, col);
                        }
                    }
                    else if (row == 0)
                    {
                        AZ_Assert(col > 0, "Unexpected column index.");
                        AddWidgetColumnHeader(columns[col - 1], row, col);
                    }
                    else if (col == 0)
                    {
                        AZ_Assert(row > 0, "Unexpected row index.");
                        AddWidgetRowHeader(rows[row - 1], row, col);
                    }
                    else
                    {
                        AZ_Assert(row > 0 && col > 0, "Unexpected row or column index.");
                        AddWidgetCell(rows[row - 1], columns[col - 1], row, col);
                    }
                }
            }
        }

        void CollisionGroupsWidget::AddGroupTableView()
        {
            AZStd::vector<PhysX::Editor::RowHeader::Data> rows = GetRows();
            AZStd::vector<PhysX::Editor::ColumnHeader::Data> columns = GetColumns();

            if (rows.size() < 1 || columns.size() < 1)
            {
                return;
            }

            int row = static_cast<int>(rows.size());

            for (int col = 0; col <= columns.size() + 1; ++col)
            {
                auto& rowData = rows[row - 1];
                if (col == columns.size() + 1) // Column for 'Remove' buttons
                {
                    AddWidgetRemoveButton(rowData, row, col);
                }
                else if (col == 0) // Column for row headers
                {
                    AddWidgetRowHeader(rowData, row, col);
                }
                else
                {
                    AddWidgetCell(rowData, columns[col - 1], row, col);
                }
            }
        }

        void CollisionGroupsWidget::RemoveGroupTableView(AzPhysics::CollisionGroups::Id groupId)
        {
            // Search for index of row to delete
            RowHeader* rowHeader = nullptr;
            for (int row = 1; row < m_gridLayout->rowCount(); ++row)
            {
                QLayoutItem* layoutItem = m_gridLayout->itemAtPosition(row, 0);
                if (layoutItem == nullptr)
                {
                    continue;
                }
                QWidget* widget = layoutItem->widget();
                if (widget == nullptr)
                {
                    continue;
                }
                rowHeader = qobject_cast<RowHeader*>(widget);
                if (rowHeader != nullptr)
                {
                    if (rowHeader->GetGroupId() == groupId)
                    {
                        break;
                    }
                }
            }

            // If row to delete cannot be found, return.
            if (rowHeader == nullptr || rowHeader->GetGroupId() != groupId)
            {
                return;
            }

            blockSignals(true);
            m_nameSet.erase(rowHeader->GetGroupName());
            m_mainLayout->removeItem(m_gridLayout); // Detach grid layout from main layout

            for (AZ::u32 widgetIndex = 0; aznumeric_cast<int>(widgetIndex) < m_widgets.size(); ++widgetIndex)
            {
                if (m_widgets[widgetIndex] == rowHeader)
                {
                    AZ::u64 columnCount = GetColumnCount() + 2; // +2 for RowHeader and last column for 'Remove' button

                    // Delete and remove references to widget pointers in deleted row
                    for (AZ::u32 deleteIndex = 0; deleteIndex < columnCount; ++deleteIndex)
                    {
                        delete m_widgets[widgetIndex + deleteIndex];
                    }
                    m_widgets.erase(m_widgets.begin() + widgetIndex, m_widgets.begin() + widgetIndex + columnCount);
                    break;
                }
            }

            delete m_gridLayout;
            m_gridLayout = new QGridLayout();
            m_mainLayout->insertLayout(0, m_gridLayout);

            // Place widget pointers of undeleted rows in new grid layout, i.e. reuse them.
            auto rows = GetRows();
            auto columns = GetColumns();
            AZ::u32 widgetIndex = 0;
            for (int row = 0; row <= rows.size(); ++row)
            {
                for (int col = 0; col <= columns.size() + 1; ++col)
                {
                    if (row == 0 && col == 0)
                    {
                        // Topleft cell is empty
                    }
                    else if (col == columns.size() + 1)
                    {
                        if (row == 0)
                        {
                            continue;
                        }
                        else
                        {
                            auto& rowData = rows[row - 1];
                            if (!rowData.m_readOnly)
                            {
                                m_gridLayout->addWidget(m_widgets[widgetIndex++], row, col);
                            }
                        }
                    }
                    else
                    {
                        m_gridLayout->addWidget(m_widgets[widgetIndex++], row, col);
                    }
                }
            }
            blockSignals(false);
        }

        void CollisionGroupsWidget::AddWidgetRemoveButton(const RowHeader::Data& rowData
            , int row
            , int column)
        {
            if (rowData.m_readOnly)
            {
                return;
            }

            QPushButton* deleteRow = new QPushButton("Remove");
            deleteRow->setFixedSize(s_buttonWidth, s_rowHeight);
            m_gridLayout->addWidget(deleteRow, row, column);
            m_widgets.push_back(deleteRow);
            auto groupId = rowData.m_groupId;
            connect(deleteRow, &QPushButton::clicked, this, [this, groupId]() {
                RemoveGroup(groupId);
            });
        }

        void CollisionGroupsWidget::AddWidgetColumnHeader(const ColumnHeader::Data& columnData
            , int row
            , int column)
        {
            ColumnHeader* colHeader = new ColumnHeader(nullptr, columnData);
            m_gridLayout->addWidget(colHeader, row, column);
            m_widgets.push_back(colHeader);
        }

        void CollisionGroupsWidget::AddWidgetRowHeader(RowHeader::Data& rowData
            , int row
            , int column)
        {
            RowHeader* rowHeader = new RowHeader(nullptr, rowData, m_nameSet);
            if (rowHeader->GetGroupName() != rowData.m_groupName)
            {
                RenameGroup(rowData.m_groupId, rowHeader->GetGroupName());
            }
            m_gridLayout->addWidget(rowHeader, row, column);
            m_widgets.push_back(rowHeader);
            connect(rowHeader, &RowHeader::OnGroupRenamed, this, &CollisionGroupsWidget::RenameGroup);
        }

        void CollisionGroupsWidget::AddWidgetCell(const RowHeader::Data& rowData
            , const ColumnHeader::Data& columnData
            , int row
            , int column)
        {
            Cell::Data cellData = {
                columnData,
                rowData
            };

            Cell* cell = new Cell(nullptr, cellData);
            m_gridLayout->addWidget(cell, row, column);
            m_widgets.push_back(cell);
            connect(cell, &Cell::OnLayerChanged, this, &CollisionGroupsWidget::EnableLayer);
        }

        void CollisionGroupsWidget::AddGroup()
        {
            m_groups.CreateGroup("NewGroup", AzPhysics::CollisionGroup::All, AzPhysics::CollisionGroups::Id::Create(), false);
            AddGroupTableView();
            emit onValueChanged(m_groups);
        }

        void CollisionGroupsWidget::RemoveGroup(AzPhysics::CollisionGroups::Id groupId)
        {
            m_groups.DeleteGroup(groupId);
            RemoveGroupTableView(groupId);
            emit onValueChanged(m_groups);
        }

        void CollisionGroupsWidget::RenameGroup(AzPhysics::CollisionGroups::Id groupId, const AZStd::string& newName)
        {
            m_groups.SetGroupName(groupId, newName);
            emit onValueChanged(m_groups);
        }

        void CollisionGroupsWidget::EnableLayer(AzPhysics::CollisionGroups::Id groupId, const AzPhysics::CollisionLayer& layer, bool enabled)
        {
            m_groups.SetLayer(groupId, layer, enabled);
            emit onValueChanged(m_groups);
        }

        AZStd::vector<RowHeader::Data> CollisionGroupsWidget::GetRows()
        {
            AZStd::vector<RowHeader::Data> m_rows;
            for (auto& group : m_groups.GetPresets())
            {
                RowHeader::Data row =
                {
                    group.m_id,
                    group.m_name,
                    group.m_group,
                    group.m_readOnly
                };
                m_rows.push_back(row);
            }
            return m_rows;
        }

        AZStd::vector<ColumnHeader::Data> CollisionGroupsWidget::GetColumns()
        {
            AZStd::vector<ColumnHeader::Data> m_cols;
            for(AZ::u8 i = 0; i < AzPhysics::CollisionLayers::MaxCollisionLayers; ++i)
            {
                auto layerName = m_layers.GetName(i);
                if (layerName.empty())
                {
                    continue;
                }

                ColumnHeader::Data col =
                {
                    layerName,
                    static_cast<AZ::u8>(i)
                };
                m_cols.push_back(col);
            }
            return m_cols;
        }

        AZ::u64 CollisionGroupsWidget::GetColumnCount() const
        {
            AZ::u64 columnCount = 0;
            for (AZ::u8 i = 0; i < AzPhysics::CollisionLayers::MaxCollisionLayers; ++i)
            {
                auto layerName = m_layers.GetName(i);
                if (!layerName.empty())
                {
                    columnCount++;
                }
            }
            return columnCount;
        }
    } // namespace Editor
} // namespace PhysX

#include <Editor/moc_CollisionGroupsWidget.cpp>
