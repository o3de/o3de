/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Name/Name.h>
#include <AzQtComponents/Components/StyledSpinBox.h>
#include <Window/ShaderManagementConsoleTableView.h>

#include <QComboBox>
#include <QHeaderView>
#include <QMenu>
#include <QKeyEvent>
#include <QPushButton>
#include <QFont>

#include <functional>

namespace ShaderManagementConsole
{
    template<typename BaseWidget>
    struct FocusOutConfigurable : public BaseWidget
    {
        template<typename... Objects>
        FocusOutConfigurable(Objects&&... args)
            : BaseWidget(std::forward<Objects>(args)...)
        {
        }

        void hidePopup() override
        {
            BaseWidget::hidePopup();
            if (m_onExit)
            {
                m_onExit();
            }
        }

        std::function<void()> m_onExit;
    };

    ShaderManagementConsoleTableView::ShaderManagementConsoleTableView(
        const AZ::Crc32& toolId, const AZ::Uuid& documentId, QWidget* parent)
        : QTableWidget(parent)
        , m_toolId(toolId)
        , m_documentId(documentId)
        , m_emptyOptionIcon(":/Icons/emptyoption.svg")
    {
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setSelectionMode(QAbstractItemView::SingleSelection);
        verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        setAlternatingRowColors(true);
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QTableWidget::customContextMenuRequested, this, &ShaderManagementConsoleTableView::ShowContextMenu);

        RebuildTable();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    void ShaderManagementConsoleTableView::mousePressEvent(QMouseEvent* e)
    {
        QTableWidget::mousePressEvent(e);
        if (e->button() == Qt::RightButton)
        {
            ShowContextMenu(e->pos());
        }
    }

    void ShaderManagementConsoleTableView::ShowContextMenu(const QPoint& pos)
    {
        QMenu contextMenu(tr("Context menu"), this);
        contextMenu.addAction(
            tr("Add Variant"),
            [this]()
            {
                AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                    m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::BeginEdit);

                ShaderManagementConsoleDocumentRequestBus::Event(
                    m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::AddOneVariantRow);

                AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                    m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);
            });

        QMenu* scriptsMenu = contextMenu.addMenu(QObject::tr("Python Scripts"));
        const AZStd::vector<AZStd::string> arguments{ m_documentId.ToString<AZStd::string>(false, true) };
        AtomToolsFramework::AddRegisteredScriptToMenu(scriptsMenu, "/O3DE/ShaderManagementConsole/DocumentTableView/ContextMenuScripts", arguments);

        contextMenu.exec(mapToGlobal(pos));
    }

    ShaderManagementConsoleTableView::~ShaderManagementConsoleTableView()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    void ShaderManagementConsoleTableView::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            RebuildTable();
        }
    }

    void ShaderManagementConsoleTableView::OnDocumentModified(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            RebuildTable();
        }
    }

    int ShaderManagementConsoleTableView::UiColumnToOption(int uiColumnIndex) const
    {
        return uiColumnIndex - 1;  // because column #0 is the "X" (deleters)
    }

    int ShaderManagementConsoleTableView::GetColumnsCount(CountQuery query) const
    {
        return query == CountQuery::ForUi ? columnCount() : UiColumnToOption(columnCount());
    }

    void ShaderManagementConsoleTableView::RebuildTable()
    {
        QSignalBlocker blocker(this);

        // Delete any active edit widget from the current selection
        if (currentColumn() != 0)
        {
            removeCellWidget(currentRow(), currentColumn());
        }

        // Disconnect data change signal while populating the table
        disconnect();

        // Get the shader variant list source data whose options will be used to populate the table
        m_shaderVariantListSourceData = {};
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            m_shaderVariantListSourceData, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData);

        // The number of variants corresponds to the number of rows in the table
        m_shaderVariantCount = m_shaderVariantListSourceData.m_shaderVariants.size();

        // The number of options corresponds to the number of columns in the table. This data is being pulled from the asset instead of the
        // shader variant list source data. The asset may contain more options that are listed in the source data. This will result in
        // several columns with no values.
        m_shaderOptionCount = {};
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            m_shaderOptionCount, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptorCount);

        // Only clear the table if the number of columns or rows have changed
        if (rowCount() != m_shaderVariantCount || GetColumnsCount(CountQuery::Options) != m_shaderOptionCount)
        {
            clear();
            setRowCount(static_cast<int>(m_shaderVariantCount));
            setColumnCount(static_cast<int>(m_shaderOptionCount) + 1);  // 1 for "delete row" widgets
        }

        // Get a list of all of the shader option descriptors from the shader asset that will be used for the columns in the table
        m_shaderOptionDescriptors = {};
        m_shaderOptionDescriptors.reserve(GetColumnsCount(CountQuery::Options));
        for (int column = 0; column < GetColumnsCount(CountQuery::Options); ++column)
        {
            AZ::RPI::ShaderOptionDescriptor shaderOptionDescriptor;
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                shaderOptionDescriptor,
                m_documentId,
                &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor,
                column);
            m_shaderOptionDescriptors.push_back(shaderOptionDescriptor);
        }

        switch (m_columnSortMode)
        {
        case Alpha:
            {
                // Sort descriptors by name and ascending order
                AZStd::sort(
                    m_shaderOptionDescriptors.begin(),
                    m_shaderOptionDescriptors.end(),
                    [](const auto& a, const auto& b)
                    {
                        return a.GetName().GetStringView() < b.GetName().GetStringView();
                    });
            }
            break;
        case Rank:
            {
                // Sort descriptors by ascending declaration order
                AZStd::sort(
                    m_shaderOptionDescriptors.begin(),
                    m_shaderOptionDescriptors.end(),
                    [](const auto& a, const auto& b)
                    {
                        return a.GetOrder() < b.GetOrder();
                    });
            }
            break;
        case Cost:
            {
                // Sort by cost estimate score in descending order
                AZStd::sort(
                    m_shaderOptionDescriptors.begin(),
                    m_shaderOptionDescriptors.end(),
                    [](const auto& a, const auto& b)
                    {
                        return a.GetCostEstimate() > b.GetCostEstimate();
                    });
            }
            break;
        }

        // Fill in the header of each column with the descriptor name
        for (int column = 1; column < GetColumnsCount(CountQuery::ForUi); ++column)
        {
            const auto& shaderOptionDescriptor = m_shaderOptionDescriptors[UiColumnToOption(column)];
            auto* tableItem = new QTableWidgetItem(shaderOptionDescriptor.GetName().GetCStr());
            tableItem->setToolTip(tr("cost %1").arg(shaderOptionDescriptor.GetCostEstimate()));
            // color material options in yellow
            if (m_shaderVariantListSourceData.m_materialOptionsHint.find(shaderOptionDescriptor.GetName()) !=
                m_shaderVariantListSourceData.m_materialOptionsHint.end())
            {
                tableItem->setForeground(QColorConstants::Yellow);
            }
            setHorizontalHeaderItem(column, tableItem);
        }
        setHorizontalHeaderItem(0, new QTableWidgetItem(""));

        // Fill all the rows with values from each variant
        for (int row = 0; row < rowCount(); ++row)
        {
            const auto& shaderVariant = m_shaderVariantListSourceData.m_shaderVariants[row];
            setVerticalHeaderItem(row, new QTableWidgetItem(QString::number(shaderVariant.m_stableId)));

            for (int column = 1; column < GetColumnsCount(CountQuery::ForUi); ++column)
            {
                const auto& shaderOptionDescriptor = m_shaderOptionDescriptors[UiColumnToOption(column)];
                const auto optionIt = shaderVariant.m_options.find(shaderOptionDescriptor.GetName());
                const AZ::Name valueName = optionIt != shaderVariant.m_options.end() ? AZ::Name(optionIt->second) : AZ::Name();
                auto* newItem = new QTableWidgetItem(valueName.GetCStr());
                if (valueName.IsEmpty())
                {
                    newItem->setIcon(m_emptyOptionIcon);
                    newItem->setToolTip(tr("runtime variable"));
                }
                setItem(row, column, newItem);
            }
            auto* deleterButton = new QPushButton;
            deleterButton->setText(u8"\u274C");  // cross sign
            deleterButton->setToolTip(tr("delete row"));
            connect(deleterButton, &QPushButton::clicked, this, [this, row](){
                auto& vec = m_shaderVariantListSourceData.m_shaderVariants;
                vec.erase(vec.begin() + row);
                TransferViewModelToModel(CallOnModified);
            });
            setCellWidget(row, 0, deleterButton);
        }
        horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);

        // Connect to the data changed signal to listen for and apply table edits back to the document
        connect(this, &QTableWidget::currentCellChanged, this, &ShaderManagementConsoleTableView::OnCellSelected);
        connect(this, &QTableWidget::cellChanged, this, &ShaderManagementConsoleTableView::OnCellChanged);
    }

    void ShaderManagementConsoleTableView::OnCellSelected(int row, int column, int previousRow, int previousColumn)
    {
        if (column == 0)
        {
            return;
        }

        removeCellWidget(row, column);
        removeCellWidget(previousRow, previousColumn);

        if (row < 0 || row >= m_shaderVariantListSourceData.m_shaderVariants.size())
        {
            return;
        }

        if (column < 0 || UiColumnToOption(column) >= m_shaderOptionDescriptors.size())
        {
            return;
        }

        const auto& shaderOptionDescriptor = m_shaderOptionDescriptors[UiColumnToOption(column)];
        const auto& shaderVariant = m_shaderVariantListSourceData.m_shaderVariants[row];
        const auto optionIt = shaderVariant.m_options.find(shaderOptionDescriptor.GetName());

        const AZ::Name valueName = optionIt != shaderVariant.m_options.end() ? AZ::Name(optionIt->second) : AZ::Name();
        const AZ::RPI::ShaderOptionValue value = shaderOptionDescriptor.FindValue(valueName);
        const AZ::RPI::ShaderOptionValue valueMin = shaderOptionDescriptor.GetMinValue();
        const AZ::RPI::ShaderOptionValue valueMax = shaderOptionDescriptor.GetMaxValue();

        switch (shaderOptionDescriptor.GetType())
        {
        case AZ::RPI::ShaderOptionType::Boolean:
        case AZ::RPI::ShaderOptionType::Enumeration:
            {
                auto* comboBox = new FocusOutConfigurable<QComboBox>(this);
                static auto italicFont = QFont(comboBox->fontInfo().family(), comboBox->fontInfo().pointSize(), comboBox->fontInfo().weight(), true);
                comboBox->addItem("<dynamic>");
                comboBox->setItemData(0, italicFont, Qt::FontRole);
                comboBox->setItemIcon(0, m_emptyOptionIcon);
                for (uint32_t valueIndex = valueMin.GetIndex(); valueIndex <= valueMax.GetIndex(); ++valueIndex)
                {
                    comboBox->addItem(shaderOptionDescriptor.GetValueName(AZ::RPI::ShaderOptionValue{ valueIndex }).GetCStr());
                }
                comboBox->setCurrentText(valueName.GetCStr());
                setCellWidget(row, column, comboBox);
                connect(comboBox, &QComboBox::currentTextChanged, this, [this, row, column](const QString& text) {
                    item(row, column)->setText(text == "<dynamic>" ? "" : text);
                });
                comboBox->m_onExit = [this, row, column]() { removeCellWidget(row, column); };
                break;
            }
        case AZ::RPI::ShaderOptionType::IntegerRange:
            {
                auto* spinBox = new AzQtComponents::StyledSpinBox(this);
                spinBox->setRange(valueMin.GetIndex(), valueMax.GetIndex());
                spinBox->setValue(value.GetIndex());
                setCellWidget(row, column, spinBox);
                connect(spinBox, &AzQtComponents::StyledSpinBox::textChanged, this, [this, row, column](const QString& text) {
                    item(row, column)->setText(text);
                });
                break;
            }
        }
    }

    void ShaderManagementConsoleTableView::keyPressEvent(QKeyEvent* e)
    {
        if (e->key() == Qt::Key_Escape)
        {
            setCurrentCell(-1, -1);
            clearFocus();
        }
        else if (e->key() == Qt::Key_Menu)
        {
            ShowContextMenu(mapFromGlobal(QCursor::pos()));
        }
    }

    void ShaderManagementConsoleTableView::OnCellChanged(int row, int column)
    {
        if (row < 0 || row >= m_shaderVariantListSourceData.m_shaderVariants.size())
        {
            return;
        }

        if (column < 0 || column > m_shaderOptionDescriptors.size())
        {
            return;
        }

        // Update the shader variant list from the table data
        auto& shaderVariant = m_shaderVariantListSourceData.m_shaderVariants[row];

        const auto optionItem = horizontalHeaderItem(column);
        if (optionItem && !optionItem->text().isEmpty())
        {
            if (auto variantItem = item(row, column))
            {
                QSignalBlocker blocker(this);
                // Set or clear the option based on the item text
                if (variantItem->text().isEmpty())
                {
                    shaderVariant.m_options.erase(AZ::Name{optionItem->text().toUtf8().constData()});
                    variantItem->setIcon(m_emptyOptionIcon);
                    variantItem->setToolTip(tr("runtime variable"));
                }
                else
                {
                    shaderVariant.m_options[AZ::Name{optionItem->text().toUtf8().constData()}] = variantItem->text().toUtf8().constData();
                    variantItem->setIcon({});
                    variantItem->setToolTip("");
                }
            }
        }

        TransferViewModelToModel(KeepAsIs/*because we know the change is already reflected*/);
    }

    void ShaderManagementConsoleTableView::TransferViewModelToModel(RebuildMode mode)
    {
        // Temporarily disconnect the document notification bus to prevent recursive notification handling as changes are applied
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();

        // Send the begin edit notification to signify the beginning of an undoable change
        AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
            m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::BeginEdit);

        // Set the shader variant list source data built from the table onto the document
        ShaderManagementConsoleDocumentRequestBus::Event(
            m_documentId,
            &ShaderManagementConsoleDocumentRequestBus::Events::SetShaderVariantListSourceData,
            m_shaderVariantListSourceData);

        // Signify the end of the undoable change
        AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
            m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);

        // Reconnect to the notification bus now that all changes have been applied
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);

        if (mode == CallOnModified)
        {
            // manual call to the modified handler because when the bus is disconnected, events to this goes to naught.
            OnDocumentModified(m_documentId);
        }
    }

    void ShaderManagementConsoleTableView::SetColumnSortMode(ColumnSortMode m)
    {
        m_columnSortMode = m;
        RebuildTable();
    }

    ShaderManagementConsoleContainer::ShaderManagementConsoleContainer(QWidget* container, const AZ::Crc32& toolId, const AZ::Uuid& documentId, QWidget* parent)
        :
        QVBoxLayout(container)
        , m_tableView(toolId, documentId, parent)
    {
        m_sortLabel.setText(tr("Option sort mode:"));
        m_sortComboBox.addItem(tr("Alphabetical"));
        m_sortComboBox.addItem(tr("Rank (shader declaration order)"));
        m_sortComboBox.addItem(tr("Cost impact (likely-performance weight, by static-analysis)"));
        m_sortComboBox.setCurrentIndex(2);
        m_defragVariants.setIcon(QIcon(":/Icons/defrag.svg"));
        m_defragVariants.setToolTip(tr("Merge duplicated variants, and recompact stable IDs"));
        connect(&m_defragVariants,
                &QPushButton::clicked,
                this,
                [documentId]() {
                    AtomToolsFramework::AtomToolsDocumentRequestBus::Event(documentId,
                        &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::BeginEdit);

                    ShaderManagementConsoleDocumentRequestBus::Event(documentId,
                        &ShaderManagementConsoleDocumentRequestBus::Events::DefragmentVariantList);

                    AtomToolsFramework::AtomToolsDocumentRequestBus::Event(documentId,
                        &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);
                });
        m_subLayout.addWidget(&m_sortLabel);
        m_subLayout.addWidget(&m_sortComboBox);
        m_subLayout.addWidget(&m_defragVariants);
        m_subLayout.addStretch();

        addLayout(&m_subLayout);
        addWidget(&m_tableView);
        connect(&m_sortComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int index) {
            m_tableView.SetColumnSortMode(ColumnSortMode(index));
        });

    }

} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleTableView.cpp>
