/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/AssetBrowser/Previewer/Previewer.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QScopedPointer>
#include <QWidget>
AZ_POP_DISABLE_WARNING
#endif

namespace Ui
{
    class SharedPreviewerClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
        class SourceAssetBrowserEntry;
        class AssetBrowserEntry;
    } // namespace AssetBrowser
} // namespace AzToolsFramework

class QResizeEvent;

namespace AZ
{
    namespace LyIntegration
    {
        class SharedPreviewer final : public AzToolsFramework::AssetBrowser::Previewer
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(SharedPreviewer, AZ::SystemAllocator, 0);

            explicit SharedPreviewer(QWidget* parent = nullptr);
            ~SharedPreviewer();

            // AzToolsFramework::AssetBrowser::Previewer overrides...
            void Clear() const override;
            void Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
            const QString& GetName() const override;

        protected:
            void resizeEvent(QResizeEvent* event) override;

        private:
            void UpdateFileInfo() const;

            QScopedPointer<Ui::SharedPreviewerClass> m_ui;
            QString m_fileInfo;
            QString m_name = "SharedPreviewer";
        };
    } // namespace LyIntegration
} // namespace AZ
