/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AzCore/Name/Name.h>
#include <Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <Window/ShaderManagementConsoleTableView.h>

#include <QHeaderView>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleTableView::ShaderManagementConsoleTableView(
        const AZ::Crc32& toolId, const AZ::Uuid& documentId, QWidget* parent)
        : QTableView(parent)
        , m_toolId(toolId)
        , m_documentId(documentId)
        , m_model(new QStandardItemModel(this))
    {
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setModel(m_model);

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
        // Disconnect data change signal while populating the table
        disconnect(m_model, &QAbstractItemModel::dataChanged, this, &ShaderManagementConsoleTableView::RebuildDocument);

        // Get the shader variant list source data whose options will be used to populate the table
        AZ::RPI::ShaderVariantListSourceData shaderVariantListSourceData;
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderVariantListSourceData, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData);

        // The number of variants corresponds to the number of rows in the table
        size_t shaderVariantCount = shaderVariantListSourceData.m_shaderVariants.size();

        // The number of options corresponds to the number of columns in the table. This data is being pulled from the asset instead of the
        // shader variant list source data. The asset may contain more options that are listed in the source data. This will result in
        // several columns with no values.
        size_t shaderOptionCount = 0;
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderOptionCount, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptorCount);

        // Only clear the table if the number of columns or rows have changed
        if (m_model->rowCount() != shaderVariantCount || m_model->columnCount() != shaderOptionCount)
        {
            m_model->clear();
            m_model->setRowCount(static_cast<int>(shaderVariantCount));
            m_model->setColumnCount(static_cast<int>(shaderOptionCount));
        }

        // Get a list of all of the shader option descriptors from the shader asset that will be used for the columns in the table
        AZStd::vector<AZ::RPI::ShaderOptionDescriptor> shaderOptionDescriptors;
        for (size_t shaderOptionIndex = 0; shaderOptionIndex < shaderOptionCount; ++shaderOptionIndex)
        {
            AZ::RPI::ShaderOptionDescriptor shaderOptionDescriptor;
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                shaderOptionDescriptor, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor,
                shaderOptionIndex);
            shaderOptionDescriptors.push_back(shaderOptionDescriptor);
        }

        // Sort all of the descriptors by name so that the columns are arranged in alphabetical order
        AZStd::sort(shaderOptionDescriptors.begin(), shaderOptionDescriptors.end(), [](const auto& a, const auto& b) {
            return a.GetName().GetStringView() < b.GetName().GetStringView();
        });

        // Do a first pass iteration over all of the variants to set the numeric headers for each variant
        size_t shaderVariantIndex = 0;
        for (const auto& shaderVariant : shaderVariantListSourceData.m_shaderVariants)
        {
            m_model->setHeaderData(
                static_cast<int>(shaderVariantIndex), Qt::Vertical, QString::number(static_cast<int>(shaderVariant.m_stableId)));
            ++shaderVariantIndex;
        }

        for (size_t shaderOptionIndex = 0; shaderOptionIndex < shaderOptionCount; ++shaderOptionIndex)
        {
            const auto& shaderOptionDescriptor = shaderOptionDescriptors[shaderOptionIndex];

            // Create a horizontal header at the top of the table using the descriptor name
            m_model->setHeaderData(static_cast<int>(shaderOptionIndex), Qt::Horizontal, shaderOptionDescriptor.GetName().GetCStr());

            // Fill in the rows of this column with the shader option values stored in the variant
            shaderVariantIndex = 0;
            for (const auto& shaderVariant : shaderVariantListSourceData.m_shaderVariants)
            {
                const auto optionIt = shaderVariant.m_options.find(shaderOptionDescriptor.GetName().GetStringView());
                m_model->setItem(
                    static_cast<int>(shaderVariantIndex), static_cast<int>(shaderOptionIndex),
                    new QStandardItem(optionIt != shaderVariant.m_options.end() ? optionIt->second.c_str() : ""));
                ++shaderVariantIndex;
            }
        }

        // Connect to the data changed signal to listen for and apply table edits back to the document
        connect(m_model, &QAbstractItemModel::dataChanged, this, &ShaderManagementConsoleTableView::RebuildDocument);
    }

    void ShaderManagementConsoleTableView::RebuildDocument()
    {
        // Get a copy of the document's current shader variant list source data so that changes can be applied to it
        AZ::RPI::ShaderVariantListSourceData shaderVariantListSourceData;
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderVariantListSourceData, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData);

        // Reset the shader variant list because it will be rebuilt from table data
        shaderVariantListSourceData.m_shaderVariants.clear();
        shaderVariantListSourceData.m_shaderVariants.resize(m_model->rowCount());
        for (int row = 0; row < m_model->rowCount(); ++row)
        {
            auto& shaderVariant = shaderVariantListSourceData.m_shaderVariants[row];
            shaderVariant.m_stableId = row;

            for (int column = 0; column < m_model->columnCount(); ++column)
            {
                auto header = m_model->horizontalHeaderItem(column);
                if (header && !header->text().isEmpty())
                {
                    auto item = m_model->item(row, column);
                    if (item && !item->text().isEmpty())
                    {
                        // If the header and item text are not empty assign them as a shader option
                        shaderVariant.m_options.emplace(header->text().toUtf8().constData(), item->text().toUtf8().constData());
                    }
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
            m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::SetShaderVariantListSourceData, shaderVariantListSourceData);

        // Signify the end of the undoable change
        AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
            m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);

        // Reconnect to the notification bus now that all changes have been applied
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleTableView.cpp>
