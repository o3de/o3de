/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/AssetBrowser/Previewer/Previewer.h>

#include <Atom/ImageProcessing/ImageObject.h>

#include <QWidget>
#include <QScopedPointer>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
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
    namespace Utils
    {
        class AsyncImageAssetLoader;
    }

    class ImagePreviewer
        : public AzToolsFramework::AssetBrowser::Previewer
        , private AZ::SystemTickBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ImagePreviewer, AZ::SystemAllocator);

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
        template<class CreateFn>
        void CreateAndDisplayTextureItemAsync(CreateFn create);

        void PreviewSubImage(uint32_t mip);

        // QLabel word wrap does not break long words such as filenames, so manual word wrap needed
        static QString WordWrap(const QString& string, int maxLength);

        // SystemTickBus
        void OnSystemTick() override;

        QScopedPointer<Ui::ImagePreviewerClass> m_ui;
        QString m_fileinfo;
        QString m_name = "ImagePreviewer";

        // Decompressed image in preview. Cache it so we can preview its sub images
        IImageObjectPtr m_previewImageObject;

        // Properties for tracking the status of an asynchronous request to display an asset browser entry
        using CreateDisplayTextureResult = AZStd::pair<IImageObjectPtr, QString>;

        QFuture<CreateDisplayTextureResult> m_createDisplayTextureResult;
        AZStd::shared_ptr<Utils::AsyncImageAssetLoader> m_imageAssetLoader;
    };
}//namespace ImageProcessingAtom
