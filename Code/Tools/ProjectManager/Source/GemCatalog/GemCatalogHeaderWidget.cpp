/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_layout->setMargin(0);
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

        // enabled
        {
            m_enabledWidget = new QWidget();
            m_enabledWidget->setFixedWidth(s_width);
            m_layout->addWidget(m_enabledWidget);

            QVBoxLayout* layout = new QVBoxLayout();
            layout->setAlignment(Qt::AlignTop);
            m_enabledWidget->setLayout(layout);

            m_enabledLabel = new QLabel();
            m_enabledLabel->setObjectName("GemCatalogCartOverlaySectionLabel");
            layout->addWidget(m_enabledLabel);
            m_enabledTagContainer = new TagContainerWidget();
            layout->addWidget(m_enabledTagContainer);
        }

        // disabled
        {
            m_disabledWidget = new QWidget();
            m_disabledWidget->setFixedWidth(s_width);
            m_layout->addWidget(m_disabledWidget);

            QVBoxLayout* layout = new QVBoxLayout();
            layout->setAlignment(Qt::AlignTop);
            m_disabledWidget->setLayout(layout);

            m_disabledLabel = new QLabel();
            m_disabledLabel->setObjectName("GemCatalogCartOverlaySectionLabel");
            layout->addWidget(m_disabledLabel);
            m_disabledTagContainer = new TagContainerWidget();
            layout->addWidget(m_disabledTagContainer);
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
        const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded();
        if (toBeAdded.isEmpty())
        {
            m_enabledWidget->hide();
        }
        else
        {
            m_enabledTagContainer->Update(ConvertFromModelIndices(toBeAdded));
            m_enabledLabel->setText(QString("%1 %2").arg(QString::number(toBeAdded.size()), tr("Gems to be enabled")));
            m_enabledWidget->show();
        }

        const QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved();
        if (toBeRemoved.isEmpty())
        {
            m_disabledWidget->hide();
        }
        else
        {
            m_disabledTagContainer->Update(ConvertFromModelIndices(toBeRemoved));
            m_disabledLabel->setText(QString("%1 %2").arg(QString::number(toBeRemoved.size()), tr("Gems to be disabled")));
            m_disabledWidget->show();
        }
    }

    QStringList CartOverlayWidget::ConvertFromModelIndices(const QVector<QModelIndex>& gems) const
    {
        QStringList gemNames;
        gemNames.reserve(gems.size());
        for (const QModelIndex& modelIndex : gems)
        {
            gemNames.push_back(GemModel::GetName(modelIndex));
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
                const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded();
                const QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved();

                const int count = toBeAdded.size() + toBeRemoved.size();
                m_countLabel->setText(QString::number(count));

                m_dropDownButton->setVisible(!toBeAdded.isEmpty() || !toBeRemoved.isEmpty());

                // Automatically close the overlay window in case there are no gems to be enabled or disabled anymore.
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
        const QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded();
        const QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved();
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
