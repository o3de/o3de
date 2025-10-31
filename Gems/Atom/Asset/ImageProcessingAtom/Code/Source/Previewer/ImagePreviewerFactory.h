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

#include <QString>

namespace ImageProcessingAtom
{
    class ImagePreviewerFactory final
        : public AzToolsFramework::AssetBrowser::PreviewerFactory
    {
    public:
        AZ_CLASS_ALLOCATOR(ImagePreviewerFactory, AZ::SystemAllocator);

        ImagePreviewerFactory() = default;
        ~ImagePreviewerFactory() = default;

        //! AzToolsFramework::AssetBrowser::PreviewerFactory overrides
        AzToolsFramework::AssetBrowser::Previewer* CreatePreviewer(QWidget* parent = nullptr) const override;
        bool IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;
        const QString& GetName() const override;

    private:
        QString m_name = "ImagePreviewer";
    };
} //namespace ImageProcessingAtom
