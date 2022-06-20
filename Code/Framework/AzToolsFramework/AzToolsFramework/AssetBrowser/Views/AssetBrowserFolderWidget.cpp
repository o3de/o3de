/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserFolderWidget.h>

#include <AzQtComponents/Components/Widgets/AssetFolderListView.h>
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>

#include <QAction>
#include <QComboBox>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserFolderWidget::AssetBrowserFolderWidget(QWidget* parent)
            : QWidget(parent)
            , m_viewStack(new QStackedWidget(this))
            , m_pathView(new AzQtComponents::BreadCrumbs(this))
            , m_listView(new AzQtComponents::AssetFolderListView(this))
            , m_thumbnailView(new AzQtComponents::AssetFolderThumbnailView(this))
        {
            m_viewStack->addWidget(m_thumbnailView);
            m_viewStack->addWidget(m_listView);
            m_viewStack->setCurrentWidget(m_thumbnailView);

            auto mainLayout = new QVBoxLayout(this);
            auto toolbar = new QToolBar(this);

            toolbar->addWidget(m_pathView->createButton(AzQtComponents::NavigationButton::Back));
            toolbar->addWidget(m_pathView->createButton(AzQtComponents::NavigationButton::Forward));
            toolbar->addWidget(m_pathView);
            mainLayout->addWidget(toolbar, 1);

            auto buttonLayout = new QHBoxLayout;
            mainLayout->addLayout(buttonLayout);

            auto actionGroup = new QActionGroup(this);

            auto thumbnailViewAction = new QAction(actionGroup);
            QIcon thumbnailViewIcon;
            thumbnailViewIcon.addPixmap(QPixmap(":/AssetBrowser/FolderWidget/thumb-unselected.png"), QIcon::Normal, QIcon::Off);
            thumbnailViewIcon.addPixmap(QPixmap(":/AssetBrowser/FolderWidget/thumb-selected.png"), QIcon::Normal, QIcon::On);
            thumbnailViewAction->setIcon(thumbnailViewIcon);
            thumbnailViewAction->setCheckable(true);
            thumbnailViewAction->setChecked(true);

            auto listViewAction = new QAction(actionGroup);
            QIcon listViewIcon;
            listViewIcon.addPixmap(QPixmap(":/AssetBrowser/FolderWidget/list-unselected.png"), QIcon::Normal, QIcon::Off);
            listViewIcon.addPixmap(QPixmap(":/AssetBrowser/FolderWidget/list-selected.png"), QIcon::Normal, QIcon::On);
            listViewAction->setIcon(listViewIcon);
            listViewAction->setCheckable(true);

            auto thumbnailViewButton = new QToolButton(this);
            thumbnailViewButton->setDefaultAction(thumbnailViewAction);
            buttonLayout->addWidget(thumbnailViewButton);

            auto listViewButton = new QToolButton(this);
            listViewButton->setDefaultAction(listViewAction);
            buttonLayout->addWidget(listViewButton);

            buttonLayout->addStretch(1);

            auto sizeComboBox = new QComboBox(this);
            sizeComboBox->addItems({tr("Small"), tr("Medium"), tr("Large")});
            sizeComboBox->setFrame(false);
            sizeComboBox->setCurrentIndex(1);
            buttonLayout->addWidget(sizeComboBox);

            mainLayout->addWidget(m_viewStack);

            connect(actionGroup, &QActionGroup::triggered, this, [this, thumbnailViewAction, sizeComboBox](QAction* action) {
                if (action == thumbnailViewAction)
                {
                    m_viewStack->setCurrentWidget(m_thumbnailView);
                    sizeComboBox->setVisible(true);
                }
                else
                {
                    m_viewStack->setCurrentWidget(m_listView);
                    sizeComboBox->setVisible(false);
                }
            });

            connect(sizeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {
                switch (index)
                {
                    case 0:
                        m_thumbnailView->setThumbnailSize(AzQtComponents::AssetFolderThumbnailView::ThumbnailSize::Small);
                        break;

                    case 1:
                        m_thumbnailView->setThumbnailSize(AzQtComponents::AssetFolderThumbnailView::ThumbnailSize::Medium);
                        break;

                    case 2:
                        m_thumbnailView->setThumbnailSize(AzQtComponents::AssetFolderThumbnailView::ThumbnailSize::Large);
                        break;

                    default:
                        break;
                }
            });

            connect(m_pathView, &AzQtComponents::BreadCrumbs::pathChanged, this, &AssetBrowserFolderWidget::pathChanged);
        }

        AssetBrowserFolderWidget::~AssetBrowserFolderWidget() = default;

        void AssetBrowserFolderWidget::setModel(QAbstractItemModel* model)
        {
            m_listView->setModel(model);
            m_thumbnailView->setModel(model);
        }

        void AssetBrowserFolderWidget::setCurrentPath(const QString& path)
        {
            if (m_pathView->currentPath() != path)
            {
                m_pathView->pushPath(path);
            }
        }

        QString AssetBrowserFolderWidget::currentPath() const
        {
            return m_pathView->currentPath();
        }

   }
}

#include "AssetBrowser/Views/moc_AssetBrowserFolderWidget.cpp"
