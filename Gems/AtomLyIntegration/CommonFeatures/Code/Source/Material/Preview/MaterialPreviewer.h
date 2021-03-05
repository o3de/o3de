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

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/AssetBrowser/Previewer/Previewer.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QWidget>
#include <QScopedPointer>
AZ_POP_DISABLE_WARNING
#endif

namespace Ui
{
    class MaterialPreviewerClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
        class SourceAssetBrowserEntry;
        class AssetBrowserEntry;
    }
}

class QResizeEvent;

namespace AZ
{
    namespace LyIntegration
    {
        class MaterialPreviewer final
            : public AzToolsFramework::AssetBrowser::Previewer
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(MaterialPreviewer, AZ::SystemAllocator, 0);

            explicit MaterialPreviewer(QWidget* parent = nullptr);
            ~MaterialPreviewer();

            // AzToolsFramework::AssetBrowser::Previewer overrides...
            void Clear() const override {}
            void Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
            const QString& GetName() const override;

        protected:
            void resizeEvent(QResizeEvent* event) override;

        private:
            void DisplayInternal(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product);
            void UpdateFileInfo() const;

            // QLabel word wrap does not break long words such as filenames, so manual word wrap needed
            static QString WordWrap(const QString& string, int maxLength);

            QScopedPointer<Ui::MaterialPreviewerClass> m_ui;
            Data::AssetId m_assetId;
            QString m_fileInfo;
            QString m_name = "MaterialPreviewer";
        };
    } // namespace LyIntegration
} // namespace AZ
