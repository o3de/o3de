/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        class MaterialPreviewerFactory final
            : public AzToolsFramework::AssetBrowser::PreviewerFactory
        {
        public:
            AZ_CLASS_ALLOCATOR(MaterialPreviewerFactory, AZ::SystemAllocator, 0);

            MaterialPreviewerFactory() = default;
            ~MaterialPreviewerFactory() = default;

            // AzToolsFramework::AssetBrowser::PreviewerFactory overrides...
            AzToolsFramework::AssetBrowser::Previewer* CreatePreviewer(QWidget* parent = nullptr) const override;
            bool IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;
            const QString& GetName() const override;

        private:
            QString m_name = "MaterialPreviewer";
        };
    } // namespace LyIntegration
} // namespace AZ
