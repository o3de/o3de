/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerFactory.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QString>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace LyIntegration
    {
        class SharedPreviewerFactory final : public AzToolsFramework::AssetBrowser::PreviewerFactory
        {
        public:
            AZ_CLASS_ALLOCATOR(SharedPreviewerFactory, AZ::SystemAllocator, 0);

            SharedPreviewerFactory() = default;
            ~SharedPreviewerFactory() = default;

            // AzToolsFramework::AssetBrowser::PreviewerFactory overrides...
            AzToolsFramework::AssetBrowser::Previewer* CreatePreviewer(QWidget* parent = nullptr) const override;
            bool IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;
            const QString& GetName() const override;

        private:
            QString m_name = "SharedPreviewer";
        };
    } // namespace LyIntegration
} // namespace AZ
