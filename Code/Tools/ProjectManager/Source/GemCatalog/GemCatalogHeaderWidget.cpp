/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <TagWidget.h>

namespace O3DE::ProjectManager
{
    CartOverlayWidget::CartOverlayWidget(GemModel* gemModel, QWidget* parent)
        : QWidget(parent)
        , m_gemModel(gemModel)
    {
        setObjectName("GemCatalogCart");

        m_layout = new QVBoxLayout();
        m_layout->setSpacing(0);
        m_layout->setMargin(5);
        m_layout->setAlignment(Qt::AlignTop);
        setLayout(m_layout);

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

        // added
        {
            m_addedWidget = new QWidget();
            m_addedWidget->setFixedWidth(s_width);
            m_layout->addWidget(m_addedWidget);

            QVBoxLayout* layout = new QVBoxLayout();
            layout->setAlignment(Qt::AlignTop);
            m_addedWidget->setLayout(layout);

            m_addedLabel = new QLabel();
            m_addedLabel->setObjectName("GemCatalogCartOverlaySectionLabel");
            layout->addWidget(m_addedLabel);
            m_addedTagContainer = new TagContainerWidget();
            layout->addWidget(m_addedTagContainer);
        }

        // removed
        {
            m_removedWidget = new QWidget();
            m_removedWidget->setFixedWidth(s_width);
            m_layout->addWidget(m_removedWidget);

            QVBoxLayout* layout = new QVBoxLayout();
            layout->setAlignment(Qt::AlignTop);
            m_removedWidget->setLayout(layout);

            m_removedLabel = new QLabel();
            m_removedLabel->setObjectName("GemCatalogCartOverlaySectionLabel");
            layout->addWidget(m_removedLabel);
            m_removedTagContainer = new TagContainerWidget();
            layout->addWidget(m_removedTagContainer);
        }

        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);

        Update();
        connect(gemModel, &GemModel::dataChanged, this, [=]
            {
                Update();
            });
    }

    void CartOverlayWidget::Update()
    {
        const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded(/*includeDependencies=*/true);
        if (toBeAdded.isEmpty())
        {
            m_addedWidget->hide();
        }
        else
        {
            m_addedTagContainer->Update(ConvertFromModelIndices(toBeAdded));
            m_addedLabel->setText(QString("%1 %2").arg(QString::number(toBeAdded.size()), tr("Gems to be activated")));
            m_addedWidget->show();
        }

        const QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved(/*includeDependencies=*/true);
        if (toBeRemoved.isEmpty())
        {
            m_removedWidget->hide();
        }
        else
        {
            m_removedTagContainer->Update(ConvertFromModelIndices(toBeRemoved));
            m_removedLabel->setText(QString("%1 %2").arg(QString::number(toBeRemoved.size()), tr("Gems to be deactivated")));
            m_removedWidget->show();
        }
    }

    QStringList CartOverlayWidget::ConvertFromModelIndices(const QVector<QModelIndex>& gems) const
    {
        QStringList gemNames;
        gemNames.reserve(gems.size());
        for (const QModelIndex& modelIndex : gems)
        {
            gemNames.push_back(GemModel::GetDisplayName(modelIndex));
        }
        return gemNames;
    }

    CartButton::CartButton(GemModel* gemModel, QWidget* parent)
        : QWidget(parent)
        , m_gemModel(gemModel)
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
        if (toBeAdded.isEmpty() && toBeRemoved.isEmpty())
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

        m_cartOverlay = new CartOverlayWidget(m_gemModel, this);
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

    GemCatalogHeaderWidget::GemCatalogHeaderWidget(GemModel* gemModel, GemSortFilterProxyModel* filterProxyModel, QWidget* parent)
        : QFrame(parent)
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

        CartButton* cartButton = new CartButton(gemModel);
        hLayout->addWidget(cartButton);
    }

    void GemCatalogHeaderWidget::ReinitForProject()
    {
        m_filterLineEdit->setText({});
    }
} // namespace O3DE::ProjectManager
