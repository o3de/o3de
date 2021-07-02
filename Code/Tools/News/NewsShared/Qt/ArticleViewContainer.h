/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "NewsShared/LogType.h"

#include <QWidget>
#include <QList>
#endif

class QLabel;

namespace Ui
{
    class ArticleViewContainerWidget;
}

namespace News
{
    class Resource;
    class ArticleView;
    class ArticleDescriptor;
    class ResourceManifest;
    class ArticleErrorView;
    class KeepInTouchView;

    class ArticleViewContainer
        : public QWidget
    {
        Q_OBJECT

        enum ArticleStyle
        {
            Default,
            Pinned
        };

    public:
        explicit ArticleViewContainer(QWidget* parent, ResourceManifest& manifest);
        ~ArticleViewContainer();

        virtual void PopulateArticles();
        ArticleView* FindById(const QString& id);
        void AddArticleView(const ArticleDescriptor& articleDesc, int articlePosition = -1);
        void DeleteArticleView(ArticleView* view);
        void ForceRefreshArticleView(ArticleView* articleView);
        void ScrollToView(ArticleView* view) const;
        void UpdateArticleOrder(ArticleView* view, bool direction) const;
        void AddLoadingMessage();
        void AddErrorMessage();
        void Clear();

    Q_SIGNALS:
        void articleSelectedSignal(QString resourceId);
        void addArticle(Resource* article);
        void logSignal(QString text, LogType logType = LogInfo);
        void scrolled(); 
        void linkActivatedSignal(const QString& link);

    private:
        QScopedPointer<Ui::ArticleViewContainerWidget> m_ui;
        QList<ArticleView*> m_articles;
        ResourceManifest& m_manifest;
        QLabel* m_loadingLabel;
        ArticleErrorView* m_errorMessage;
        KeepInTouchView* m_keepInTouchViewWidget = nullptr;

        void ClearError();
        ArticleStyle GetArticleStyleEnumFromString(const QString& articleStyleStr) const;
        ArticleView* CreateArticleView(const ArticleDescriptor& articleDesc);

    private Q_SLOTS:
        virtual void articleSelectedSlot(QString id);
    };
}
