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

#include <QStandardItemModel>
#include <QTableView>
#endif

namespace ShaderManagementConsole
{
    class ShaderManagementConsoleTableView
        : public QTableView
        , public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleTableView, AZ::SystemAllocator, 0);
        ShaderManagementConsoleTableView(const AZ::Crc32& toolId, const AZ::Uuid& documentId, QWidget* parent);
        ~ShaderManagementConsoleTableView();

    protected:
        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;

        void RebuildTable();

        const AZ::Crc32 m_toolId = {};
        const AZ::Uuid m_documentId = AZ::Uuid::CreateNull();
        QStandardItemModel* m_model = {};
    };
} // namespace ShaderManagementConsole
