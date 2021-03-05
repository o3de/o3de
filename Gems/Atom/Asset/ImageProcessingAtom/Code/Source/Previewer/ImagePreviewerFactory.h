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

#include <QString>

namespace ImageProcessingAtom
{
    class ImagePreviewerFactory final
        : public AzToolsFramework::AssetBrowser::PreviewerFactory
    {
    public:
        AZ_CLASS_ALLOCATOR(ImagePreviewerFactory, AZ::SystemAllocator, 0);

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