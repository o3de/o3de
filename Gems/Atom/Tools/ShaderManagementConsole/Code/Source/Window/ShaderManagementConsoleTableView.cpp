/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

    void ShaderManagementConsole::ShaderManagementConsoleTableView::RebuildTable()
    {
        AZStd::unordered_set<AZStd::string> optionNames;

        size_t shaderOptionCount = 0;
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderOptionCount, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionCount);

        for (size_t optionIndex = 0; optionIndex < shaderOptionCount; ++optionIndex)
        {
            AZ::RPI::ShaderOptionDescriptor shaderOptionDesc;
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                shaderOptionDesc, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor, optionIndex);
            optionNames.insert(shaderOptionDesc.GetName().GetCStr());
        }

        size_t shaderVariantCount = 0;
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderVariantCount, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantCount);

        m_model->clear();
        m_model->setRowCount(static_cast<int>(shaderVariantCount));
        m_model->setColumnCount(static_cast<int>(optionNames.size()));

        int nameIndex = 0;
        for (const auto& optionName : optionNames)
        {
            m_model->setHeaderData(nameIndex++, Qt::Horizontal, optionName.c_str());
        }

        for (int variantIndex = 0; variantIndex < shaderVariantCount; ++variantIndex)
        {
            AZ::RPI::ShaderVariantListSourceData::VariantInfo shaderVariantInfo;
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                shaderVariantInfo, m_documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantInfo, variantIndex);

            m_model->setHeaderData(variantIndex, Qt::Vertical, QString::number(variantIndex));

            for (const auto& shaderOption : shaderVariantInfo.m_options)
            {
                AZ::Name optionName{ shaderOption.first };
                AZ::Name optionValue{ shaderOption.second };

                auto optionIt = optionNames.find(optionName.GetCStr());
                int optionIndex = static_cast<int>(AZStd::distance(optionNames.begin(), optionIt));

                QStandardItem* item = new QStandardItem(optionValue.GetCStr());
                m_model->setItem(variantIndex, optionIndex, item);
            }
        }
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleTableView.cpp>
#include "ShaderManagementConsoleTableView.h"
