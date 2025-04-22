/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemsGemRepoScreen.h>
#include <GemRepo/GemRepoScreen.h>
#include <GemCatalog/GemCatalogScreen.h>
#include <ScreenHeaderWidget.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPushButton>

namespace O3DE::ProjectManager
{
    GemsGemRepoScreen::GemsGemRepoScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);

        m_header = new ScreenHeader(this);
        m_header->setTitle(tr(""));
        m_header->setSubTitle(tr("Remote Sources"));
        connect(m_header->backButton(), &QPushButton::clicked, this, &GemsGemRepoScreen::HandleBackButton);
        vLayout->addWidget(m_header);

        m_gemRepoScreen = new GemRepoScreen(this);
        m_gemRepoScreen->setObjectName("body");
        m_gemRepoScreen->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        vLayout->addWidget(m_gemRepoScreen);

        setLayout(vLayout);
    }

    ProjectManagerScreen GemsGemRepoScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::GemsGemRepos;
    }

    void GemsGemRepoScreen::HandleBackButton()
    {
        emit GoToPreviousScreenRequest();
    }

} // namespace O3DE::ProjectManager
