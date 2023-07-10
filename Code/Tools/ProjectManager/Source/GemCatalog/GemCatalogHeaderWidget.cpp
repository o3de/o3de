/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <TagWidget.h>

#include <AzCore/std/functional.h>

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QMenu>
#include <QLocale>
#include <QMovie>
#include <QPainter>
#include <QPainterPath>

namespace O3DE::ProjectManager
{
    GemCartWidget::GemCartWidget(GemModel* gemModel, DownloadController* downloadController, QWidget* parent)
        : QScrollArea(parent)
        , m_gemModel(gemModel)
        , m_downloadController(downloadController)
    {
        setObjectName("GemCatalogCart");
        setWidgetResizable(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        m_layout = new QVBoxLayout();
        m_layout->setSpacing(0);
        m_layout->setMargin(5);
        m_layout->setAlignment(Qt::AlignTop);
        setLayout(m_layout);
        setMinimumHeight(400);

        QHBoxLayout* hLayout = new QHBoxLayout();

        QPushButton* closeButton = new QPushButton();
        closeButton->setFlat(true);
        closeButton->setFocusPolicy(Qt::NoFocus);
        closeButton->setIcon(QIcon(":/WindowClose.svg"));
        connect(closeButton, &QPushButton::clicked, this, [=]
            {
                deleteLater();
            });
        hLayout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::Expanding));
        hLayout->addWidget(closeButton);
        m_layout->addLayout(hLayout);

        // downloading gems
        CreateDownloadSection();

        // added
        CreateGemSection( tr("Gem to be activated"), tr("Gems to be activated"), [=]
            {
                QVector<QModelIndex> gems;
                const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded(/*includeDependencies=*/false);

                // don't include gems that were already active because they were dependencies
                for (const QModelIndex& modelIndex : toBeAdded)
                {
                    if (!GemModel::WasPreviouslyAddedDependency(modelIndex))
                    {
                        gems.push_back(modelIndex);
                    }
                }
                return gems;
            });

        // removed
        CreateGemSection( tr("Gem to be deactivated"), tr("Gems to be deactivated"), [=]
            {
                QVector<QModelIndex> gems;
                const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeRemoved(/*includeDependencies=*/false);

                // don't include gems that are still active because they are dependencies
                for (const QModelIndex& modelIndex : toBeAdded)
                {
                    if (!GemModel::IsAddedDependency(modelIndex))
                    {
                        gems.push_back(modelIndex);
                    }
                }
                return gems;
            });

        // added dependencies 
        CreateGemSection( tr("Dependency to be activated"), tr("Dependencies to be activated"), [=]
            {
                QVector<QModelIndex> dependencies;
                const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded(/*includeDependencies=*/true);

                // only include gems that are dependencies and not explicitly added 
                for (const QModelIndex& modelIndex : toBeAdded)
                {
                    if (GemModel::IsAddedDependency(modelIndex) && !GemModel::IsAdded(modelIndex))
                    {
                        dependencies.push_back(modelIndex);
                    }
                }
                return dependencies;
            });

        // removed dependencies 
        CreateGemSection( tr("Dependency to be deactivated"), tr("Dependencies to be deactivated"), [=]
            {
                QVector<QModelIndex> dependencies;
                const QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved(/*includeDependencies=*/true);

                // don't include gems that were explicitly removed - those are listed in a different section
                for (const QModelIndex& modelIndex : toBeRemoved)
                {
                    if (!GemModel::WasPreviouslyAdded(modelIndex))
                    {
                        dependencies.push_back(modelIndex);
                    }
                }
                return dependencies;
            });
    }

    void GemCartWidget::CreateGemSection(const QString& singularTitle, const QString& pluralTitle, GetTagIndicesCallback getTagIndices)
    {
        QWidget* widget = new QWidget();
        widget->setFixedWidth(s_width);
        m_layout->addWidget(widget);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        widget->setLayout(layout);

        QLabel* label = new QLabel();
        label->setObjectName("GemCatalogCartOverlaySectionLabel");
        layout->addWidget(label);

        TagContainerWidget* tagContainer = new TagContainerWidget();
        layout->addWidget(tagContainer);

        auto update = [=]()
        {
            const QVector<QModelIndex> tagIndices = getTagIndices();
            if (tagIndices.isEmpty())
            {
                widget->hide();
            }
            else
            {
                tagContainer->Update(GetTagsFromModelIndices(tagIndices));
                label->setText(QString("%1 %2").arg(tagIndices.size()).arg(tagIndices.size() == 1 ? singularTitle : pluralTitle));
                widget->show();
            }
        };

        connect(m_gemModel, &GemModel::dataChanged, this, update); 
        update();
    }

    void GemCartWidget::OnCancelDownloadActivated(const QString& gemName)
    {
        m_downloadController->CancelObjectDownload(gemName, DownloadController::DownloadObjectType::Gem);
    }

    void GemCartWidget::CreateDownloadSection()
    {
        m_downloadSectionWidget = new QWidget();
        m_downloadSectionWidget->setFixedWidth(s_width);
        m_layout->addWidget(m_downloadSectionWidget);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        m_downloadSectionWidget->setLayout(layout);

        QLabel* titleLabel = new QLabel();
        titleLabel->setObjectName("GemCatalogCartOverlaySectionLabel");
        layout->addWidget(titleLabel);

        titleLabel->setText(tr("Gems to be installed"));

        // Create header section
        QWidget* downloadingGemsWidget = new QWidget();
        downloadingGemsWidget->setObjectName("GemCatalogCartOverlayGemDownloadHeader");
        layout->addWidget(downloadingGemsWidget);
        QVBoxLayout* gemDownloadLayout = new QVBoxLayout();
        gemDownloadLayout->setMargin(0);
        gemDownloadLayout->setAlignment(Qt::AlignTop);
        downloadingGemsWidget->setLayout(gemDownloadLayout);
        QLabel* processingQueueLabel = new QLabel("Processing Queue");
        gemDownloadLayout->addWidget(processingQueueLabel);

        m_downloadingListWidget = new QWidget();
        m_downloadingListWidget->setObjectName("GemCatalogCartOverlayGemDownloadBG");
        gemDownloadLayout->addWidget(m_downloadingListWidget);
        QVBoxLayout* downloadingItemLayout = new QVBoxLayout();
        downloadingItemLayout->setAlignment(Qt::AlignTop);
        m_downloadingListWidget->setLayout(downloadingItemLayout);

        QLabel* downloadsInProgessLabel = new QLabel("");
        downloadsInProgessLabel->setObjectName("NumDownloadsInProgressLabel");
        downloadingItemLayout->addWidget(downloadsInProgessLabel);

        if (m_downloadController->IsDownloadQueueEmpty())
        {
            m_downloadSectionWidget->hide();
        }
        else
        {
            // Setup gem download rows for gems that are already in the queue
            const AZStd::deque<DownloadController::DownloadableObject>& downloadQueue = m_downloadController->GetDownloadQueue();

            for (const DownloadController::DownloadableObject& o3deObject : downloadQueue)
            {
                if (o3deObject.m_objectType == DownloadController::DownloadObjectType::Gem)
                {
                    ObjectDownloadAdded(o3deObject.m_objectName, o3deObject.m_objectType);
                }
            }
        }

        // connect to download controller data changed
        connect(m_downloadController, &DownloadController::ObjectDownloadAdded, this, &GemCartWidget::ObjectDownloadAdded);
        connect(m_downloadController, &DownloadController::ObjectDownloadRemoved, this, &GemCartWidget::ObjectDownloadRemoved);
        connect(m_downloadController, &DownloadController::ObjectDownloadProgress, this, &GemCartWidget::ObjectDownloadProgress);
    }

    void GemCartWidget::ObjectDownloadAdded(const QString& gemName, DownloadController::DownloadObjectType objectType)
    {
        if (objectType != DownloadController::DownloadObjectType::Gem)
        {
            return;
        }

        // Containing widget for the current download item
        QWidget* newGemDownloadWidget = new QWidget();
        newGemDownloadWidget->setObjectName(gemName);
        QVBoxLayout* downloadingGemLayout = new QVBoxLayout(newGemDownloadWidget);
        newGemDownloadWidget->setLayout(downloadingGemLayout);

        // Gem name, progress string, cancel
        QHBoxLayout* nameProgressLayout = new QHBoxLayout(newGemDownloadWidget);
        TagWidget* newTag = new TagWidget({gemName, gemName}, newGemDownloadWidget);
        nameProgressLayout->addWidget(newTag);
        QLabel* progress = new QLabel(tr("Queued"), newGemDownloadWidget);
        progress->setObjectName("DownloadProgressLabel");
        nameProgressLayout->addWidget(progress);
        nameProgressLayout->addStretch();
        QLabel* cancelText = new QLabel(tr("<a href=\"%1\">Cancel</a>").arg(gemName), newGemDownloadWidget);
        cancelText->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        connect(cancelText, &QLabel::linkActivated, this, &GemCartWidget::OnCancelDownloadActivated);
        nameProgressLayout->addWidget(cancelText);
        downloadingGemLayout->addLayout(nameProgressLayout);

        // Progress bar
        QProgressBar* downloadProgessBar = new QProgressBar(newGemDownloadWidget);
        downloadProgessBar->setObjectName("DownloadProgressBar");
        downloadingGemLayout->addWidget(downloadProgessBar);
        downloadProgessBar->setValue(0);

        m_downloadingListWidget->layout()->addWidget(newGemDownloadWidget);

        const AZStd::deque<DownloadController::DownloadableObject>& downloadQueue = m_downloadController->GetDownloadQueue();
        QLabel* numDownloads = m_downloadingListWidget->findChild<QLabel*>("NumDownloadsInProgressLabel");
        numDownloads->setText(QString("%1 %2")
                                  .arg(downloadQueue.size())
                                  .arg(downloadQueue.size() == 1 ? tr("download in progress...") : tr("downloads in progress...")));

        m_downloadingListWidget->show();
    }

    void GemCartWidget::ObjectDownloadRemoved(const QString& gemName, DownloadController::DownloadObjectType objectType)
    {
        if (objectType != DownloadController::DownloadObjectType::Gem)
        {
            return;
        }

        QWidget* gemToRemove = m_downloadingListWidget->findChild<QWidget*>(gemName);
        if (gemToRemove)
        {
            gemToRemove->deleteLater();
        }

        if (m_downloadController->IsDownloadQueueEmpty())
        {
            m_downloadSectionWidget->hide();
        }
        else
        {
            size_t downloadQueueSize = m_downloadController->GetDownloadQueue().size();
            QLabel* numDownloads = m_downloadingListWidget->findChild<QLabel*>("NumDownloadsInProgressLabel");
            numDownloads->setText(QString("%1 %2")
                                      .arg(downloadQueueSize)
                                      .arg(downloadQueueSize == 1 ? tr("download in progress...") : tr("downloads in progress...")));
        }
    }

    void GemCartWidget::ObjectDownloadProgress(const QString& gemName, DownloadController::DownloadObjectType objectType, int bytesDownloaded, int totalBytes)
    {
        if (objectType != DownloadController::DownloadObjectType::Gem)
        {
            return;
        }

        QWidget* gemToUpdate = m_downloadingListWidget->findChild<QWidget*>(gemName);
        if (gemToUpdate)
        {
            QLabel* progressLabel = gemToUpdate->findChild<QLabel*>("DownloadProgressLabel");
            QProgressBar* progressBar = gemToUpdate->findChild<QProgressBar*>("DownloadProgressBar");

            // totalBytes can be 0 if the server does not return a content-length for the object
            if (totalBytes != 0)
            {
                int downloadPercentage = static_cast<int>((bytesDownloaded / static_cast<float>(totalBytes)) * 100);
                if (progressLabel)
                {
                    progressLabel->setText(QString("%1%").arg(downloadPercentage));
                }
                if (progressBar)
                {
                    progressBar->setValue(downloadPercentage);
                }
            }
            else
            {
                if (progressLabel)
                {
                    progressLabel->setText(QLocale::system().formattedDataSize(bytesDownloaded));
                }
                if (progressBar)
                {
                    progressBar->setRange(0, 0);
                }
            }
        }
    }

    QVector<Tag> GemCartWidget::GetTagsFromModelIndices(const QVector<QModelIndex>& gems) const
    {
        QVector<Tag> tags;
        tags.reserve(gems.size());
        for (const QModelIndex& modelIndex : gems)
        {
            const GemInfo& gemInfo = GemModel::GetGemInfo(modelIndex);
            if(gemInfo.m_isEngineGem)
            {
                // don't show engine gem versions
                tags.push_back({ gemInfo.m_displayName, gemInfo.m_name });
            }
            else
            {
                // show non-engine gem versions if available
                QString version =  GemModel::GetNewVersion(modelIndex);
                if (version.isEmpty())
                {
                    version =  gemInfo.m_version;
                }

                if (version.isEmpty() || version.contains("Unknown", Qt::CaseInsensitive) || gemInfo.m_displayName.contains(version))
                {
                    tags.push_back({ gemInfo.m_displayName, gemInfo.m_name });
                }
                else
                {
                    const QString& title = QString("%1 %2").arg(gemInfo.m_displayName, version);
                    tags.push_back({ title, gemInfo.m_name });
                }
            }

        }
        return tags;
    }

    CartButton::CartButton(GemModel* gemModel, DownloadController* downloadController, QWidget* parent)
        : QWidget(parent)
        , m_gemModel(gemModel)
        , m_downloadController(downloadController)
    {
        m_layout = new QHBoxLayout();
        m_layout->setMargin(0);
        setLayout(m_layout);

        QPushButton* iconButton = new QPushButton();
        iconButton->setFlat(true);
        iconButton->setFocusPolicy(Qt::NoFocus);
        iconButton->setIcon(QIcon(":/Summary.svg"));
        iconButton->setFixedSize(s_iconSize, s_iconSize);
        connect(iconButton, &QPushButton::clicked, this, &CartButton::ShowGemCart);
        m_layout->addWidget(iconButton);

        m_countLabel = new QLabel("0");
        m_countLabel->setObjectName("GemCatalogCartCountLabel");
        m_countLabel->setFixedHeight(s_iconSize - 1); // Compensate for the empty icon space by using a slightly smaller label height.
        m_layout->addWidget(m_countLabel);

        m_dropDownButton = new QPushButton();
        m_dropDownButton->setFlat(true);
        m_dropDownButton->setFocusPolicy(Qt::NoFocus);
        m_dropDownButton->setIcon(QIcon(":/CarrotArrowDown.svg"));
        m_dropDownButton->setFixedSize(s_arrowDownIconSize, s_arrowDownIconSize);
        connect(m_dropDownButton, &QPushButton::clicked, this, &CartButton::ShowGemCart);
        m_layout->addWidget(m_dropDownButton);

        // Adjust the label text whenever the model gets updated.
        connect(gemModel, &GemModel::dataChanged, [=]
            {
                const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded(/*includeDependencies=*/true);
                const QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved(/*includeDependencies=*/true);

                const int count = toBeAdded.size() + toBeRemoved.size();
                m_countLabel->setText(QString::number(count));

                m_dropDownButton->setVisible(!toBeAdded.isEmpty() || !toBeRemoved.isEmpty());

                // Automatically close the overlay window in case there are no gems to be activated or deactivated anymore.
                if (m_gemCart && toBeAdded.isEmpty() && toBeRemoved.isEmpty())
                {
                    m_gemCart->deleteLater();
                    m_gemCart = nullptr;
                }
            });
    }

    void CartButton::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        ShowGemCart();
    }

    void CartButton::hideEvent(QHideEvent*)
    {
        if (m_gemCart)
        {
            m_gemCart->hide();
        }
    }

    void CartButton::ShowGemCart()
    {
        const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded(/*includeDependencies=*/true);
        const QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved(/*includeDependencies=*/true);
        if (toBeAdded.isEmpty() && toBeRemoved.isEmpty() && m_downloadController->IsDownloadQueueEmpty())
        {
            return;
        }

        if (m_gemCart)
        {
            // Directly delete the former overlay before creating the new one.
            // Don't use deleteLater() here. This might overwrite the new overlay pointer
            // depending on the event queue.
            delete m_gemCart;
        }

        m_gemCart = new GemCartWidget(m_gemModel, m_downloadController, this);
        connect(m_gemCart, &QWidget::destroyed, this, [=]
            {
                // Reset the overlay pointer on destruction to prevent dangling pointers.
                m_gemCart = nullptr;
                // Tell header gem cart is no longer open
                UpdateGemCart(nullptr);
            });
        m_gemCart->show();

        emit UpdateGemCart(m_gemCart);
    }

    CartButton::~CartButton()
    {
        // Make sure the overlay window is automatically closed in case the gem catalog is destroyed.
        if (m_gemCart)
        {
            m_gemCart->deleteLater();
        }
    }

    GemCatalogHeaderWidget::GemCatalogHeaderWidget(GemModel* gemModel, GemSortFilterProxyModel* filterProxyModel, DownloadController* downloadController, QWidget* parent)
        : QFrame(parent)
        , m_downloadController(downloadController)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setAlignment(Qt::AlignLeft);
        hLayout->setContentsMargins(10, 7, 10, 7);
        setLayout(hLayout);

        setObjectName("GemCatalogHeaderWidget");
        setFixedHeight(s_height);

        QLabel* titleLabel = new QLabel(tr("Gem Catalog"));
        titleLabel->setObjectName("GemCatalogTitle");
        hLayout->addWidget(titleLabel);

        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        m_filterLineEdit = new AzQtComponents::SearchLineEdit();
        connect(m_filterLineEdit, &QLineEdit::textChanged, this, [=](const QString& text)
            {
                filterProxyModel->SetSearchString(text);
            });
        hLayout->addWidget(m_filterLineEdit);

        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        hLayout->addSpacerItem(new QSpacerItem(75, 0, QSizePolicy::Fixed));

        // spinner
        m_downloadSpinnerMovie = new QMovie(":/in_progress.gif");
        m_downloadSpinner = new QLabel(this);
        m_downloadSpinner->setScaledContents(true);
        m_downloadSpinner->setMaximumSize(16, 16);
        m_downloadSpinner->setMovie(m_downloadSpinnerMovie);
        hLayout->addWidget(m_downloadSpinner);
        hLayout->addSpacing(8);

        // downloading label
        m_downloadLabel = new QLabel(tr("Downloading"));
        hLayout->addWidget(m_downloadLabel);
        m_downloadSpinner->hide();
        m_downloadLabel->hide();

        hLayout->addSpacing(16);

        m_cartButton = new CartButton(gemModel, downloadController);
        hLayout->addWidget(m_cartButton);
        hLayout->addSpacing(16);

        // Separating line
        QFrame* vLine = new QFrame();
        vLine->setFrameShape(QFrame::VLine);
        vLine->setObjectName("verticalSeparatingLine");
        hLayout->addWidget(vLine);

        hLayout->addSpacing(16);

        QMenu* gemMenu = new QMenu(this);
        gemMenu->addAction(tr("Refresh"), [this]() { emit RefreshGems(/*refreshRemoteRepos*/true); });
        gemMenu->addAction( tr("Show Gem Repos"), [this]() { emit OpenGemsRepo(); });
        gemMenu->addSeparator();
        gemMenu->addAction( tr("Add Existing Gem"), [this]() { emit AddGem(); });
        gemMenu->addAction( tr("Create New Gem"), [this]() { emit CreateGem(); });

        QPushButton* gemMenuButton = new QPushButton(this);
        gemMenuButton->setObjectName("gemCatalogMenuButton");
        gemMenuButton->setMenu(gemMenu);
        gemMenuButton->setIcon(QIcon(":/menu.svg"));
        gemMenuButton->setIconSize(QSize(36, 24));
        hLayout->addWidget(gemMenuButton);

        connect(m_downloadController, &DownloadController::ObjectDownloadAdded, this, &GemCatalogHeaderWidget::GemDownloadAdded);
        connect(m_downloadController, &DownloadController::ObjectDownloadRemoved, this, &GemCatalogHeaderWidget::GemDownloadRemoved);

        connect(
            m_cartButton, &CartButton::UpdateGemCart, this,
            [this](QWidget* gemCart)
            {
                GemCartShown(gemCart);
                if (gemCart)
                {
                    emit UpdateGemCart(gemCart);
                }
            });
    }

    void GemCatalogHeaderWidget::GemDownloadAdded(const QString& /*gemName*/, DownloadController::DownloadObjectType objectType)
    {
        if (objectType != DownloadController::DownloadObjectType::Gem)
        {
            return;
        }

        m_downloadSpinner->show();
        m_downloadLabel->show();
        m_downloadSpinnerMovie->start();
        m_cartButton->ShowGemCart();
    }

    void GemCatalogHeaderWidget::GemDownloadRemoved(const QString& /*gemName*/, DownloadController::DownloadObjectType objectType)
    {
        if (objectType == DownloadController::DownloadObjectType::Gem && m_downloadController->IsDownloadQueueEmpty())
        {
            m_downloadSpinner->hide();
            m_downloadLabel->hide();
            m_downloadSpinnerMovie->stop();
        }
    }

    void GemCatalogHeaderWidget::GemCartShown(bool state)
    {
        m_showGemCart = state;
        repaint();
    }

    void GemCatalogHeaderWidget::ReinitForProject()
    {
        m_filterLineEdit->setText({});
    }

    void GemCatalogHeaderWidget::paintEvent([[maybe_unused]] QPaintEvent* event)
    {
        // Only show triangle when cart is shown
        if (!m_showGemCart)
        {
            return;
        }

        const QPoint buttonPos = m_cartButton->pos();
        const QSize buttonSize = m_cartButton->size();

        // Draw isosceles triangle with top point touching bottom of cartButton
        // Bottom aligned with header bottom and top of right panel
        const QPoint topPoint(buttonPos.x() + buttonSize.width() / 2, buttonPos.y() + buttonSize.height());
        const QPoint bottomLeftPoint(topPoint.x() - 20, height());
        const QPoint bottomRightPoint(topPoint.x() + 20, height());

        QPainterPath trianglePath;
        trianglePath.moveTo(topPoint);
        trianglePath.lineTo(bottomLeftPoint);
        trianglePath.lineTo(bottomRightPoint);
        trianglePath.lineTo(topPoint);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.fillPath(trianglePath, QBrush(QColor("#555555")));
    }

} // namespace O3DE::ProjectManager
