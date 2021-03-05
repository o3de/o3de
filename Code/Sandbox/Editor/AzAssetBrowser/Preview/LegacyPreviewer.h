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
#include <Editor/Util/Image.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/AssetBrowser/Previewer/Previewer.h>

#include <QWidget>
#include <QScopedPointer>
#endif

namespace Ui
{
    class LegacyPreviewerClass;
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

class LegacyPreviewer
    : public AzToolsFramework::AssetBrowser::Previewer
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(LegacyPreviewer, AZ::SystemAllocator, 0);

    explicit LegacyPreviewer(QWidget* parent = nullptr);
    ~LegacyPreviewer();

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::AssetBrowser::Previewer
    //////////////////////////////////////////////////////////////////////////
    void Clear() const override;
    void Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
    const QString& GetName() const override;

    static const QString Name;

protected:
    void resizeEvent(QResizeEvent * event) override;

private:
    struct FileInfo
    {
        QString filename;
        unsigned attrib;
        time_t time_create;    /* -1 for FAT file systems */
        time_t time_access;    /* -1 for FAT file systems */
        time_t time_write;
        _fsize_t size;
    };

    enum class TextureType
    {
        RGB,
        RGBA,
        Alpha
    };

    QScopedPointer<Ui::LegacyPreviewerClass> m_ui;
    CImageEx m_previewImageSource;
    CImageEx m_previewImageUpdated;
    TextureType m_textureType;
    QString m_fileinfo;
    QString m_fileinfoAlphaTexture;

    bool DisplayProduct(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product);
    void DisplaySource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* source);

    QString GetFileSize(const char* path);

    bool DisplayTextureLegacy(const char* fullImagePath);
    bool DisplayTextureProductModern(const char* fullProductImagePath);

    void UpdateTextureType();

    static bool FileInfoCompare(const FileInfo& f1, const FileInfo& f2);
    //! QLabel word wrap does not break long words such as filenames, so manual word wrap needed
    static QString WordWrap(const QString& string, int maxLength);
};
