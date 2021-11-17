/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <AzCore/std/functional.h>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <TagWidget.h>
#include <QMenu>
#include <QLocale>
#include <QMovie>

namespace O3DE::ProjectManager
{
    CartOverlayWidget::CartOverlayWidget(GemModel* gemModel, DownloadController* downloadController, QWidget* parent)
        : QWidget(parent)
        , m_gemModel(gemModel)
        , m_downloadController(downloadController)
    {
        setObjectName("GemCatalogCart");

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

        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    }

    CartOverlayWidget::~CartOverlayWidget()
    {
        // disconnect from all download controller signals
        disconnect(m_downloadController, nullptr, this, nullptr);
    }

    void CartOverlayWidget::CreateGemSection(const QString& singularTitle, const QString& pluralTitle, GetTagIndicesCallback getTagIndices)
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

    void CartOverlayWidget::OnCancelDownloadActivated(const QString& gemName)
    {
        m_downloadController->CancelGemDownload(gemName);
    }

    void CartOverlayWidget::CreateDownloadSection()
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
            const AZStd::vector<QString>& downloadQueue = m_downloadController->GetDownloadQueue();

            for (const QString& gemName : downloadQueue)
            {
                GemDownloadAdded(gemName);
            }
        }

        // connect to download controller data changed
        connect(m_downloadController, &DownloadController::GemDownloadAdded, this, &CartOverlayWidget::GemDownloadAdded);
        connect(m_downloadController, &DownloadController::GemDownloadRemoved, this, &CartOverlayWidget::GemDownloadRemoved);
        connect(m_downloadController, &DownloadController::GemDownloadProgress, this, &CartOverlayWidget::GemDownloadProgress);
    }

    void CartOverlayWidget::GemDownloadAdded(const QString& gemName)
    {
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
        connect(cancelText, &QLabel::linkActivated, this, &CartOverlayWidget::OnCancelDownloadActivated);
        nameProgressLayout->addWidget(cancelText);
        downloadingGemLayout->addLayout(nameProgressLayout);

        // Progress bar
        QProgressBar* downloadProgessBar = new QProgressBar(newGemDownloadWidget);
        downloadProgessBar->setObjectName("DownloadProgressBar");
        downloadingGemLayout->addWidget(downloadProgessBar);
        downloadProgessBar->setValue(0);

        m_downloadingListWidget->layout()->addWidget(newGemDownloadWidget);

        const AZStd::vector<QString>& downloadQueue = m_downloadController->GetDownloadQueue();
        QLabel* numDownloads = m_downloadingListWidget->findChild<QLabel*>("NumDownloadsInProgressLabel");
        numDownloads->setText(QString("%1 %2")
                                  .arg(downloadQueue.size())
                                  .arg(downloadQueue.size() == 1 ? tr("download in progress...") : tr("downloads in progress...")));

        m_downloadingListWidget->show();
    }

    void CartOverlayWidget::GemDownloadRemoved(const QString& gemName)
    {
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

    void CartOverlayWidget::GemDownloadProgress(const QString& gemName, int bytesDownloaded, int totalBytes)
    {
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

    QVector<Tag> CartOverlayWidget::GetTagsFromModelIndices(const QVector<QModelIndex>& gems) const
    {
        QVector<Tag> tags;
        tags.reserve(gems.size());
        for (const QModelIndex& modelIndex : gems)
        {
            tags.push_back({ GemModel::GetDisplayName(modelIndex), GemModel::GetName(modelIndex) });
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
        connect(iconButton, &QPushButton::clicked, this, &CartButton::ShowOverlay);
        m_layout->addWidget(iconButton);

        m_countLabel = new QLabel();
        m_countLabel->setObjectName("GemCatalogCartCountLabel");
        m_countLabel->setFixedHeight(s_iconSize - 1); // Compensate for the empty icon space by using a slightly smaller label height.
        m_layout->addWidget(m_countLabel);

        m_dropDownButton = new QPushButton();
        m_dropDownButton->setFlat(true);
        m_dropDownButton->setFocusPolicy(Qt::NoFocus);
        m_dropDownButton->setIcon(QIcon(":/CarrotArrowDown.svg"));
        m_dropDownButton->setFixedSize(s_arrowDownIconSize, s_arrowDownIconSize);
        connect(m_dropDownButton, &QPushButton::clicked, this, &CartButton::ShowOverlay);
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
                if (m_cartOverlay && toBeAdded.isEmpty() && toBeRemoved.isEmpty())
                {
                    m_cartOverlay->deleteLater();
                    m_cartOverlay = nullptr;
                }
            });
    }

    void CartButton::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        ShowOverlay();
    }

    void CartButton::hideEvent(QHideEvent*)
    {
        if (m_cartOverlay)
        {
            m_cartOverlay->hide();
        }
    }

    void CartButton::ShowOverlay()
    {
        const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded(/*includeDependencies=*/true);
        const QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved(/*includeDependencies=*/true);
        if (toBeAdded.isEmpty() && toBeRemoved.isEmpty() && m_downloadController->IsDownloadQueueEmpty())
        {
            return;
        }

        if (m_cartOverlay)
        {
            // Directly delete the former overlay before creating the new one.
            // Don't use deleteLater() here. This might overwrite the new overlay pointer
            // depending on the event queue.
            delete m_cartOverlay;
        }

        m_cartOverlay = new CartOverlayWidget(m_gemModel, m_downloadController, this);
        connect(m_cartOverlay, &QWidget::destroyed, this, [=]
            {
                // Reset the overlay pointer on destruction to prevent dangling pointers.
                m_cartOverlay = nullptr;
            });
        m_cartOverlay->show();

        const QPoint parentPos = m_dropDownButton->mapToParent(m_dropDownButton->pos());
        const QPoint globalPos = m_dropDownButton->mapToGlobal(m_dropDownButton->pos());
        const QPoint offset(-4, 10);
        m_cartOverlay->setGeometry(globalPos.x() - parentPos.x() - m_cartOverlay->width() + width() + offset.x(),
            globalPos.y() + offset.y(),
            m_cartOverlay->width(),
            m_cartOverlay->height());
    }

    CartButton::~CartButton()
    {
        // Make sure the overlay window is automatically closed in case the gem catalog is destroyed.
        if (m_cartOverlay)
        {
            m_cartOverlay->deleteLater();
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
        m_filterLineEdit->setStyleSheet("background-color: #DDDDDD;");
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
        gemMenu->addAction( tr("Refresh"), [this]() { emit RefreshGems(); });
        gemMenu->addAction( tr("Show Gem Repos"), [this]() { emit OpenGemsRepo(); });
        gemMenu->addSeparator();
        gemMenu->addAction( tr("Add Existing Gem"), [this]() { emit AddGem(); });

        QPushButton* gemMenuButton = new QPushButton(this);
        gemMenuButton->setObjectName("gemCatalogMenuButton");
        gemMenuButton->setMenu(gemMenu);
        gemMenuButton->setIcon(QIcon(":/menu.svg"));
        gemMenuButton->setIconSize(QSize(36, 24));
        hLayout->addWidget(gemMenuButton);

        connect(m_downloadController, &DownloadController::GemDownloadAdded, this, &GemCatalogHeaderWidget::GemDownloadAdded);
        connect(m_downloadController, &DownloadController::GemDownloadRemoved, this, &GemCatalogHeaderWidget::GemDownloadRemoved);
    }

    void GemCatalogHeaderWidget::GemDownloadAdded(const QString& /*gemName*/)
    {
        m_downloadSpinner->show();
        m_downloadLabel->show();
        m_downloadSpinnerMovie->start();
        m_cartButton->ShowOverlay();
    }

    void GemCatalogHeaderWidget::GemDownloadRemoved(const QString& /*gemName*/)
    {
        if (m_downloadController->IsDownloadQueueEmpty())
        {
            m_downloadSpinner->hide();
            m_downloadLabel->hide();
            m_downloadSpinnerMovie->stop();
        }
    }

    void GemCatalogHeaderWidget::ReinitForProject()
    {
        m_filterLineEdit->setText({});
    }
} // namespace O3DE::ProjectManager
