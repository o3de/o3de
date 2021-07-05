/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

namespace Ui
{
    class ArticleViewWidget;
    class PinnedArticleViewWidget;
}

namespace AzQtComponents
{
    class ExtendedLabel;
}

class QFrame;

namespace News
{
    class ArticleDescriptor;
    class ResourceManifest;

    class ArticleView
        : public QWidget
    {
        Q_OBJECT
    public:
        ArticleView(QWidget* parent,
            const ArticleDescriptor& article,
            const ResourceManifest& manifest);
        ~ArticleView() = default;

        void Update();
        QSharedPointer<const ArticleDescriptor> GetArticle() const;

    Q_SIGNALS:
        void articleSelectedSignal(QString resourceId);
        void linkActivatedSignal(const QString& link);

    protected:
        void SetupViewWidget(QFrame* widgetImageFrame, AzQtComponents::ExtendedLabel* widgetTitle, AzQtComponents::ExtendedLabel* widgetBody);
        void mousePressEvent(QMouseEvent* event);

    private:

        QFrame* m_widgetImageFrame = nullptr;
        AzQtComponents::ExtendedLabel* m_widgetTitle = nullptr;
        AzQtComponents::ExtendedLabel* m_widgetBody = nullptr;
        AzQtComponents::ExtendedLabel* m_icon = nullptr;

        QSharedPointer<const ArticleDescriptor> m_pArticle;
        const ResourceManifest& m_manifest;

        void RemoveIcon();

    private Q_SLOTS:
        void linkActivatedSlot(const QString& link);
        void articleSelectedSlot();
    };

    class ArticleViewDefaultWidget : public ArticleView
    {
    public:
        ArticleViewDefaultWidget(QWidget* parent,
            const ArticleDescriptor& article,
            const ResourceManifest& manifest);
        ~ArticleViewDefaultWidget() = default;

    private:
        Ui::ArticleViewWidget* m_ui = nullptr;
    };

    class ArticleViewPinnedWidget : public ArticleView
    {
    public:
        ArticleViewPinnedWidget(QWidget* parent,
            const ArticleDescriptor& article,
            const ResourceManifest& manifest);
        ~ArticleViewPinnedWidget() = default;

    private:
        Ui::PinnedArticleViewWidget* m_ui = nullptr;
    };
}
