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
#include <AzToolsFramework/AssetBrowser/Previewer/Previewer.h>

#include <Atom/ImageProcessing/ImageObject.h>

#include <QWidget>
#include <QScopedPointer>
#endif

namespace Ui
{
    class ImagePreviewerClass;
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

namespace ImageProcessingAtom
{
    class ImagePreviewer
        : public AzToolsFramework::AssetBrowser::Previewer
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ImagePreviewer, AZ::SystemAllocator, 0);

        explicit ImagePreviewer(QWidget* parent = nullptr);
        ~ImagePreviewer();

        //! AzToolsFramework::AssetBrowser::Previewer overrides
        void Clear() const override;
        void Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
        const QString& GetName() const override;

    protected:
        void resizeEvent(QResizeEvent * event) override;

    private:
        void DisplayProduct(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product);
        void DisplaySource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* source);
        
        QString GetFileSize(const char* path);

        void DisplayTextureItem();
        void PreviewSubImage(uint32_t mip);

        // QLabel word wrap does not break long words such as filenames, so manual word wrap needed
        static QString WordWrap(const QString& string, int maxLength);

        QScopedPointer<Ui::ImagePreviewerClass> m_ui;
        QString m_fileinfo;
        QString m_name = "ImagePreviewer";

        // Decompressed image in preview. Cache it so we can preview its sub images
        IImageObjectPtr m_previewImageObject;
    };
}//namespace ImageProcessingAtom
