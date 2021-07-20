/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BuilderArticleViewContainer.h"
#include "ResourceManagement/BuilderResourceManifest.h"
#include "ArticleDetails.h"
#include <NewsShared/ResourceManagement/Resource.h>
#include <NewsShared/Qt/ArticleViewContainer.h>
#include <NewsShared/Qt/ArticleView.h>
#include "Qt/ui_BuilderArticleViewContainer.h"

#include <AzQtComponents/Components/Style.h>

#include "QCustomMessageBox.h"

namespace News
{

    BuilderArticleViewContainer::BuilderArticleViewContainer(
        QWidget* parent,
        BuilderResourceManifest& manifest)
        : QWidget(parent)
        , m_ui(new Ui::BuilderArticleViewContainerWidget)
        , m_container(new ArticleViewContainer(this, manifest))
        , m_manifest(manifest)
    {
        m_ui->setupUi(this);

        m_ui->ArticleViewContainerRoot->layout()->addWidget(m_container);
        connect(m_container, SIGNAL(articleSelectedSignal(QString)),
            this, SLOT(articleSelectedSlot(QString)));
    }

    BuilderArticleViewContainer::~BuilderArticleViewContainer(){}

    void BuilderArticleViewContainer::AddArticle()
    {
        auto pArticleResource = static_cast<BuilderResourceManifest&>(m_manifest).AddArticle();
        if (pArticleResource)
        {
            m_container->AddArticleView(ArticleDescriptor(*pArticleResource));
            SelectArticle(pArticleResource->GetId());
        }
    }

    void BuilderArticleViewContainer::SelectArticle(QString id)
    {
        if (m_selectedArticleId.compare(id) == 0)
        {
            return;
        }

        UnselectArticle();
        m_selectedArticleId = id;
        auto article = m_container->FindById(id);
        if (article)
        {
            m_container->ScrollToView(article);
            emit articleSelectedSignal(id);

            // Add class and refresh style
            AzQtComponents::Style::addClass(article, m_selectedArticleClass);
            article->style()->unpolish(qApp);
            article->style()->polish(qApp);
        }
    }

    void BuilderArticleViewContainer::Sync()
    {
        emit logSignal("Starting sync");

        m_manifest.Sync();
    }

    void BuilderArticleViewContainer::UpdateArticle(QString& id) const
    {
        auto view = m_container->FindById(id);
        if (view)
        {
            view->Update();
            m_container->ForceRefreshArticleView(view);
        }
    }

    void BuilderArticleViewContainer::DeleteArticle(QString& id) const
    {
        auto view = m_container->FindById(id);
        if (view)
        {
            m_container->DeleteArticleView(view);
        }
    }

    //possibly support multiple selection in future?
    void BuilderArticleViewContainer::CloseArticle(QString& id)
    {
        if (m_selectedArticleId == id)
        {
            UnselectArticle();
        }
    }

    void BuilderArticleViewContainer::UpdateArticleOrder(QString& id, bool direction) const
    {
        auto view = m_container->FindById(id);
        if (view)
        {
            m_container->UpdateArticleOrder(view, direction);
        }
    }

    void BuilderArticleViewContainer::PopulateArticles()
    {
        m_container->PopulateArticles();

        if (!m_selectedArticleId.isEmpty())
        {
            SelectArticle(m_selectedArticleId);
        }
    }

    void BuilderArticleViewContainer::AddErrorMessage() const
    {
        m_container->AddErrorMessage();
    }

    void BuilderArticleViewContainer::UnselectArticle()
    {
        if (m_selectedArticleId.isEmpty())
        {
            return;
        }
        auto article = m_container->FindById(m_selectedArticleId);
    
        // Remove class and refresh style
        if (article)
        {
            AzQtComponents::Style::removeClass(article, m_selectedArticleClass);
            article->style()->unpolish(qApp);
            article->style()->polish(qApp);
        }

        m_selectedArticleId = "";
    }

    void BuilderArticleViewContainer::articleSelectedSlot(QString id)
    {
        SelectArticle(id);
    }

#include "Qt/moc_BuilderArticleViewContainer.cpp"

}
