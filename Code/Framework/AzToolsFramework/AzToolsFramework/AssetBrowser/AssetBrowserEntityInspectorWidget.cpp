/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntityInspectorWidget.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>

#include <QPixmap>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserEntityInspectorWidget::AssetBrowserEntityInspectorWidget(QWidget* parent)
            : QWidget(parent)
        {
            // Create the empty label layout
            m_emptyLayoutWidget = new QWidget(this);
            auto emptyLayout = new QVBoxLayout(m_emptyLayoutWidget);
            auto emptyLabel = new QLabel(m_emptyLayoutWidget);
            emptyLabel->setWordWrap(true);
            emptyLabel->setMargin(4);
            emptyLabel->setText(QObject::tr("Select an asset to view its properties in the inspector."));
            emptyLayout->addWidget(emptyLabel);
            emptyLayout->addStretch(1);
            m_emptyLayoutWidget->setLayout(emptyLayout);

            // Instantiate the populated layout and its parent widget
            m_populatedLayoutWidget = new QWidget(this);
            auto populatedLayout = new QVBoxLayout(m_populatedLayoutWidget);

             // Create the layout for the image preview by centering the image's QLabel
            auto iconLayout = new QHBoxLayout(m_populatedLayoutWidget);
            iconLayout->addStretch(1);
            m_previewImage = new QLabel(m_populatedLayoutWidget);
            m_previewImage->setStyleSheet("QLabel{border: 1px solid #222222; border-radius: 2px;}");
            m_previewImage->setAlignment(Qt::AlignCenter);
            m_previewImage->setWordWrap(true);
            m_previewImage->setScaledContents(true);
            iconLayout->addWidget(m_previewImage, 3);
            iconLayout->addStretch(1);

            // Create the layout for the asset details card
            auto cardLayout = new QVBoxLayout(m_populatedLayoutWidget);
            auto assetDetails = new AzQtComponents::Card();
            assetDetails->setTitle("Asset Details");
            assetDetails->header()->setHasContextMenu(false);
            assetDetails->hideFrame();
            cardLayout->addWidget(assetDetails);

            // Create a two column layout for the labels and data inside of the asset details card
            auto assetDetailWidget = new QWidget(assetDetails);
            m_assetDetailLayout = new QFormLayout;
            m_assetDetailLayout->setLabelAlignment(Qt::AlignRight);
            m_nameLabel = new AzQtComponents::ElidingLabel;
            m_locationLabel = new AzQtComponents::ElidingLabel;
            m_fileTypeLabel = new AzQtComponents::ElidingLabel;
            m_assetTypeLabel = new AzQtComponents::ElidingLabel;
            m_diskSizeLabel = new AzQtComponents::ElidingLabel;
            m_dimensionLabel = new AzQtComponents::ElidingLabel;
            m_verticesLabel = new AzQtComponents::ElidingLabel;
            m_assetDetailLayout->addRow(QObject::tr("Name:"), m_nameLabel);
            m_assetDetailLayout->addRow(QObject::tr("Location:"), m_locationLabel);
            m_assetDetailLayout->addRow(QObject::tr("File Type:"), m_fileTypeLabel);
            m_assetDetailLayout->addRow(QObject::tr("Asset Type:"), m_assetTypeLabel);
            m_assetDetailLayout->addRow(QObject::tr("Disk Size (KB):"), m_diskSizeLabel);
            m_assetDetailLayout->addRow(QObject::tr("Dimensions:"), m_dimensionLabel);
            m_assetDetailLayout->addRow(QObject::tr("Vertices:"), m_verticesLabel);
            assetDetailWidget->setLayout(m_assetDetailLayout);
            assetDetails->setContentWidget(assetDetailWidget);

            // Create the layout for the dependent assets card
            auto dependent = new AzQtComponents::Card();
            dependent->setTitle("Dependent Assets");
            dependent->header()->setHasContextMenu(false);
            dependent->hideFrame();
            cardLayout->addWidget(dependent);
            cardLayout->addStretch(1);

            // Add the image preview and the card layouts to a single layout, spacing them appropriately
            populatedLayout->addLayout(iconLayout, 3);
            populatedLayout->addLayout(cardLayout, 7);
            m_populatedLayoutWidget->setLayout(populatedLayout);

            // Add the widgets containing both empty and populated layouts to a stacked widget
            m_layoutSwitcher = new QStackedLayout(this);
            m_layoutSwitcher->addWidget(m_emptyLayoutWidget);
            m_layoutSwitcher->addWidget(m_populatedLayoutWidget);
            setLayout(m_layoutSwitcher);

            AssetBrowserPreviewRequestBus::Handler::BusConnect();
        }

        AssetBrowserEntityInspectorWidget::~AssetBrowserEntityInspectorWidget()
        {
            AssetBrowserPreviewRequestBus::Handler::BusDisconnect();
        }

        void AssetBrowserEntityInspectorWidget::PreviewAsset(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry)
        {
            if (selectedEntry == nullptr)
            {
                ClearPreview();
                return;
            }
            
            if (m_layoutSwitcher->currentWidget() != m_populatedLayoutWidget)
            {
                m_layoutSwitcher->setCurrentWidget(m_populatedLayoutWidget);
            }

            const auto& qVariant = AssetBrowserViewUtils::GetThumbnail(selectedEntry);
            QPixmap pixmap = qVariant.value<QPixmap>();
            {
                !pixmap.isNull() ? 
                m_previewImage->setPixmap(pixmap) :
                m_previewImage->setText(QObject::tr("No preview image avaialble."));
            }

            if (!m_assetDetailLayout->isEmpty())
            {
                const auto rowCount = m_assetDetailLayout->count();
                for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
                {
                    m_assetDetailLayout->removeRow(rowIndex);
                }
            }

            QString name = selectedEntry->GetDisplayName();
            m_nameLabel->setText(name);

            auto fullPath = selectedEntry->GetFullPath();
            auto location = QString::fromUtf8(fullPath.c_str(), fullPath.size());
            m_locationLabel->setText(location);

            QString fileType;
            QString assetEntryType;
            if (const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(selectedEntry))
            {
                auto extension = sourceEntry->GetExtension();
                fileType = QString::fromUtf8(extension.c_str(), extension.size());
                assetEntryType = QObject::tr("Source");
            }
            else if (const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(selectedEntry))
            {
                auto assetType = productEntry->GetAssetTypeString();
                fileType = QString::fromUtf8(assetType.c_str(), assetType.size());
                assetEntryType = QObject::tr("Product");
            }
            m_fileTypeLabel->setText(fileType);
            m_assetTypeLabel->setText(assetEntryType);

            auto diskSize = selectedEntry->GetDiskSize();
            m_diskSizeLabel->setText(QString::number(diskSize));

            auto dimension = selectedEntry->GetDimension();
            auto dimensionString = QString::number(dimension.GetX()) + QObject::tr(" x ") + QString::number(dimension.GetY()) +
                QObject::tr(" x ") + QString::number(dimension.GetZ());
            m_dimensionLabel->setText(dimensionString);

            auto vertices = selectedEntry->GetNumVertices();
            m_verticesLabel->setText(QString::number(vertices));
        }

        void AssetBrowserEntityInspectorWidget::ClearPreview()
        {
            m_previewImage->clear();
            m_layoutSwitcher->setCurrentWidget(m_emptyLayoutWidget);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
