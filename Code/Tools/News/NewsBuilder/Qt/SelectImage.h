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