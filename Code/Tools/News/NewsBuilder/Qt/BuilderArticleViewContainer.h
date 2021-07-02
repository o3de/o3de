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
    class BuilderArticleViewContainerWidget;
}

namespace News
{
    class ArticleViewContainer;
    class BuilderResourceManifest;

    //! BuilderArticleViewContainer is a Builder container for ArticleViewContainer.
    /*!
        It is a wrapper with additional builder functionality for displaying articles
    */
    class BuilderArticleViewContainer
        : public QWidget
    {
        Q_OBJECT
    public:
        BuilderArticleViewContainer(QWidget* parent, BuilderResourceManifest& manifest);
        ~BuilderArticleViewContainer();

        void UpdateArticle(QString& id) const;
        void DeleteArticle(QString& id) const;
        void CloseArticle(QString& id);
        void UpdateArticleOrder(QString& id, bool direction) const;
        void PopulateArticles();
        void AddErrorMessage() const;
        void AddArticle();

Q_SIGNALS:
        void logSignal(QString text, LogType logType = LogInfo);
        void articleSelectedSignal(QString id);

    private:
        QScopedPointer<Ui::BuilderArticleViewContainerWidget> m_ui;
        ArticleViewContainer* m_container;
        QString m_selectedArticleId;
        QString m_selectedArticleClass = "SelectedArticle";
        BuilderResourceManifest& m_manifest;

        void Sync();
        void SelectArticle(QString id);
        void UnselectArticle();


    private Q_SLOTS:
        void articleSelectedSlot(QString id);
    };
} // namespace News
