/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AzCore/Name/Name.h>
#include <AzQtComponents/Components/StyledSpinBox.h>
#include <Window/ShaderManagementConsoleTableView.h>

#include <QComboBox>
#include <QHeaderView>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleTableView::ShaderManagementConsoleTableView(
        const AZ::Crc32& toolId, const AZ::Uuid& documentId, QWidget* parent)
        : QTableWidget(parent)
        , m_toolId(toolId)
        , m_documentId(documentId)
    {
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setSelectionMode(QAbstractItemView::SingleSelection);
        horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

        RebuildTable();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
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

    void ShaderManagementConsoleTableView::RebuildTable()
    {
        QSignalBlocker blocker(this);

        // Delete any active edit widget from the current selection
        setCellWidget(currentRow(), currentColumn(), nullptr);

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
        if (rowCount() != m_shaderVariantCount || columnCount() != m_shaderOptionCount)
        {
            clear();
            setRowCount(static_cast<int>(m_shaderVariantCount));
            setColumnCount(static_cast<int>(m_shaderOptionCount));
        }

        // Get a list of all of the shader option descriptors from the shader asset that will be used for the columns in the table
        m_shaderOptionDescriptors = {};
        m_shaderOptionDescriptors.reserve(columnCount());
        for (int column = 0; column < columnCount(); ++column)
        {
            AZ::RPI::ShaderOptionDescriptor shaderOptionDescriptor;
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                shaderOptionDescriptor,
                m_documentId,
                &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor,
                column);
            m_shaderOptionDescriptors.push_back(shaderOptionDescriptor);
        }

        // Sort all of the descriptors by name so that the columns are arranged in alphabetical order
        AZStd::sort(
            m_shaderOptionDescriptors.begin(),
            m_shaderOptionDescriptors.end(),
            [](const auto& a, const auto& b)
            {
                return a.GetName().GetStringView() < b.GetName().GetStringView();
            });

        // Fill in the header of each column with the descriptor name
        for (int column = 0; column < columnCount(); ++column)
        {
            const auto& shaderOptionDescriptor = m_shaderOptionDescriptors[column];
            setHorizontalHeaderItem(column, new QTableWidgetItem(shaderOptionDescriptor.GetName().GetCStr()));
        }

        // Fill all the rows with values from each variant
        for (int row = 0; row < rowCount(); ++row)
        {
            const auto& shaderVariant = m_shaderVariantListSourceData.m_shaderVariants[row];
            setVerticalHeaderItem(row, new QTableWidgetItem(QString::number(row)));

            for (int column = 0; column < columnCount(); ++column)
            {
                const auto& shaderOptionDescriptor = m_shaderOptionDescriptors[column];
                const auto optionIt = shaderVariant.m_options.find(shaderOptionDescriptor.GetName());
                const AZ::Name valueName = optionIt != shaderVariant.m_options.end() ? AZ::Name(optionIt->second) : AZ::Name();
                setItem(row, column, new QTableWidgetItem(valueName.GetCStr()));
            }
        }

        // Connect to the data changed signal to listen for and apply table edits back to the document
        connect(this, &QTableWidget::currentCellChanged, this, &ShaderManagementConsoleTableView::OnCellSelected);
        connect(this, &QTableWidget::cellChanged, this, &ShaderManagementConsoleTableView::OnCellChanged);
    }

    void ShaderManagementConsoleTableView::OnCellSelected(int row, int column, int previousRow, int previousColumn)
    {
        setCellWidget(row, column, nullptr);
        setCellWidget(previousRow, previousColumn, nullptr);

        if (row < 0 || row >= m_shaderVariantListSourceData.m_shaderVariants.size())
        {
            return;
        }

        if (column < 0 || column >= m_shaderOptionDescriptors.size())
        {
            return;
        }

        const auto& shaderOptionDescriptor = m_shaderOptionDescriptors[column];
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
                QComboBox* comboBox = new QComboBox(this);
                comboBox->addItem("");
                for (uint32_t valueIndex = valueMin.GetIndex(); valueIndex <= valueMax.GetIndex(); ++valueIndex)
                {
                    comboBox->addItem(shaderOptionDescriptor.GetValueName(AZ::RPI::ShaderOptionValue{ valueIndex }).GetCStr());
                }
                comboBox->setCurrentText(valueName.GetCStr());
                setCellWidget(row, column, comboBox);
                connect(comboBox, &QComboBox::currentTextChanged, this, [this, row, column](const QString& text) {
                    item(row, column)->setText(text);
                });
                break;
            }
        case AZ::RPI::ShaderOptionType::IntegerRange:
            {
                AzQtComponents::StyledSpinBox* spinBox = new AzQtComponents::StyledSpinBox(this);
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

    void ShaderManagementConsoleTableView::OnCellChanged(int row, int column)
    {
        if (row < 0 || row >= m_shaderVariantListSourceData.m_shaderVariants.size())
        {
            return;
        }

        if (column < 0 || column >= m_shaderOptionDescriptors.size())
        {
            return;
        }

        // Update the shader variant list from the table data
        auto& shaderVariant = m_shaderVariantListSourceData.m_shaderVariants[row];

        const auto optionItem = horizontalHeaderItem(column);
        if (optionItem && !optionItem->text().isEmpty())
        {
            if (const auto variantItem = item(row, column))
            {
                // Set or clear the option based on the item text
                if (variantItem->text().isEmpty())
                {
                    shaderVariant.m_options.erase(AZ::Name{optionItem->text().toUtf8().constData()});
                }
                else
                {
                    shaderVariant.m_options[AZ::Name{optionItem->text().toUtf8().constData()}] = variantItem->text().toUtf8().constData();
                }
            }
        }

        // Temporarily disconnect the document notification bus to prevent recursive notification handling as changes are applied
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();

        // Send the begin edit notification to signify the beginning of an undoable change
        AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
            m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::BeginEdit);

        // Set the shader variant list source data built from the table onto the document
        ShaderManagementConsoleDocumentRequestBus::Event(
            m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::SetShaderVariantListSourceData, m_shaderVariantListSourceData);

        // Signify the end of the undoable change
        AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
            m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);

        // Reconnect to the notification bus now that all changes have been applied
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleTableView.cpp>
