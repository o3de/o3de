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
#include <QWidget>
#include <NewsShared/LogType.h>
#endif

namespace Ui
{
    class ArticleDetailsContainerWidget;
}

namespace News 
{
    class ArticleDetails;
    class BuilderResourceManifest;
    class SelectImage;
    class ResourceManifest;
    class Resource;

    //! Control that manages articleDetails control
    class ArticleDetailsContainer
        : public QWidget
    {
        Q_OBJECT

    public:
        ArticleDetailsContainer(QWidget* parent, BuilderResourceManifest& manifest);
        ~ArticleDetailsContainer();

        void SelectArticle(const QString& id);
        void Reset();

Q_SIGNALS:
        void logSignal(QString text, LogType logType = LogInfo);
        void updateArticleSignal(QString id);
        void deleteArticleSignal(QString id);
        void closeArticleSignal(QString id);
        void orderChangedSignal(QString id, bool direction);

    private:
        QScopedPointer<Ui::ArticleDetailsContainerWidget> m_ui;
        BuilderResourceManifest& m_manifest;
        ArticleDetails* m_articleDetails = nullptr;
        QString m_selectedId;

    private Q_SLOTS:
        void updateArticleSlot();
        void deleteArticleSlot();
        void closeArticleSlot();
    };
} // namespace News
