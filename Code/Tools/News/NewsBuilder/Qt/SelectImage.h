/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class SelectImageDialog;
}

namespace News {
    class ImageItem;
    class Resource;
    class ResourceManifest;

    //! Allows to select existing images for multiple messages without re-uploading same one
    class SelectImage
        : public QDialog
    {
        Q_OBJECT
    public:
        explicit SelectImage(const ResourceManifest& manifest);
        ~SelectImage();

        void Select();
        void Close();
        Resource* GetSelected() const;

    private:
        const static int MAX_COLS;
        QScopedPointer<Ui::SelectImageDialog> m_ui;
        const ResourceManifest& m_manifest;
        QList<ImageItem*> m_images;
        Resource* m_pSelected = nullptr;

        void ImageSelected(ImageItem* imageItem);
    };
} // namespace News
