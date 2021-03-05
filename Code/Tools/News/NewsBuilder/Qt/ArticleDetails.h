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
#include <NewsShared/ResourceManagement/ArticleDescriptor.h>
#include "ResourceManagement/BuilderResourceManifest.h"

#include <QWidget>
#endif

namespace Ui
{
    class ArticleDetailsWidget;
}

namespace News {
    class SelectImage;
    class ResourceManifest;
    class Resource;

    //! Control that allows to modify all parameters of a single article
    class ArticleDetails
        : public QWidget
    {
        Q_OBJECT

    public:
        ArticleDetails(QWidget* parent,
            Resource& resource,
            BuilderResourceManifest& manifest);
        ~ArticleDetails();
        ArticleDescriptor& GetArticle();
        QString GetId() const;

    Q_SIGNALS:
        void logSignal(QString text, LogType logType = LogInfo);
        void deleteArticleSignal();
        void updateArticleSignal();
        void closeArticleSignal();
        void orderChangedSignal(QString id, bool direction);

    private:
        QScopedPointer<Ui::ArticleDetailsWidget> m_ui;
        QString m_filename;
        ArticleDescriptor m_article;
        BuilderResourceManifest& m_manifest;
        SelectImage* m_pSelectImage;
        QString m_imageIdOriginal;
        QString m_imageId;
        float m_imageRatio = 184.0f / 430.0f;

        void resizeEvent(QResizeEvent *event) override;
        void resizePreviewImage();

        void OpenImageFromFile();
        void OpenImageFromResource();
        QString LoadImage() const;
        void SetImage(Resource& resource);
        void SetImage(QString& filename);
        void SetNoImage() const;
        void ClearImage();

        void UpdateArticle();
        void DeleteArticle();
        void Close();
        void MoveUp();
        void MoveDown();
    };
} // namespace News
