/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
