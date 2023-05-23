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
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>
//#include <C:/o3de/Code/Tools/AssetProcessor/native/ui/ProductAssetDetailsPanel.h>

#include <QPixmap>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserEntityInspectorWidget::AssetBrowserEntityInspectorWidget(QWidget* parent)
            : QWidget(parent)
            , m_databaseConnection(aznew AssetDatabase::AssetDatabaseConnection)
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
            m_assetDetailLayout = new QFormLayout(assetDetailWidget);
            m_assetDetailLayout->setLabelAlignment(Qt::AlignRight);
            m_nameLabel = new AzQtComponents::ElidingLabel(assetDetailWidget);
            m_locationLabel = new AzQtComponents::ElidingLabel(assetDetailWidget);
            m_fileTypeLabel = new AzQtComponents::ElidingLabel(assetDetailWidget);
            m_assetTypeLabel = new AzQtComponents::ElidingLabel(assetDetailWidget);
            m_diskSizeLabel = new AzQtComponents::ElidingLabel(assetDetailWidget);
            m_dimensionLabel = new AzQtComponents::ElidingLabel(assetDetailWidget);
            m_verticesLabel = new AzQtComponents::ElidingLabel(assetDetailWidget);
            m_lastModified = new AzQtComponents::ElidingLabel(assetDetailWidget);
            m_assetDetailLayout->addRow(QObject::tr("Name:"), m_nameLabel);
            m_assetDetailLayout->addRow(QObject::tr("Location:"), m_locationLabel);
            m_assetDetailLayout->addRow(QObject::tr("File Type:"), m_fileTypeLabel);
            m_assetDetailLayout->addRow(QObject::tr("Asset Type:"), m_assetTypeLabel);
            m_assetDetailLayout->addRow(QObject::tr("Disk Size (KB):"), m_diskSizeLabel);
            m_assetDetailLayout->addRow(QObject::tr("Dimensions:"), m_dimensionLabel);
            m_assetDetailLayout->addRow(QObject::tr("Vertices:"), m_verticesLabel);
            m_assetDetailLayout->addRow(QObject::tr("Last Modified:"), m_lastModified);
            assetDetailWidget->setLayout(m_assetDetailLayout);
            assetDetails->setContentWidget(assetDetailWidget);

            // Create the layout for the dependent assets card
            auto dependent = new AzQtComponents::Card();
            dependent->setTitle(QObject::tr("Dependent Assets"));
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

            m_dependentProducts = new QTreeWidget(dependent);
            m_dependentProducts->setColumnCount(1);
            m_dependentProducts->setHeaderHidden(true);
            dependent->setContentWidget(m_dependentProducts);
            /*m_dependentProducts->setRootIsDecorated(false);
            m_dependentProducts->setUniformRowHeights(true);
            m_dependentProducts->setSortingEnabled(false);
            m_dependentProducts->setHeaderHidden(true);
            m_dependentProducts->header()->setDefaultSectionSize(42);*/

            AssetDatabaseLocationNotificationBus::Handler::BusConnect();
            AssetBrowserPreviewRequestBus::Handler::BusConnect();
            m_databaseConnection->OpenDatabase();
        }

        AssetBrowserEntityInspectorWidget::~AssetBrowserEntityInspectorWidget()
        {
            AssetBrowserPreviewRequestBus::Handler::BusDisconnect();
            AssetDatabaseLocationNotificationBus::Handler::BusDisconnect();
        }

        void AssetBrowserEntityInspectorWidget::OnDatabaseInitialized()
        {

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

            m_dependentProducts->clear();

            const auto& qVariant = AssetBrowserViewUtils::GetThumbnail(selectedEntry);
            QPixmap pixmap = qVariant.value<QPixmap>();
            {
                !pixmap.isNull() ? 
                m_previewImage->setPixmap(pixmap) :
                m_previewImage->setText(QObject::tr("No preview image avaialble."));
            }

            const QString name = selectedEntry->GetDisplayName();
            m_nameLabel->setText(name);

            const auto fullPath = selectedEntry->GetFullPath();
            const auto location = QString::fromUtf8(fullPath.c_str(), static_cast<int>(fullPath.size()));
            m_locationLabel->setText(location);

            QString fileType;
            QString assetEntryType;
            if (const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(selectedEntry))
            {
                const auto extension = sourceEntry->GetExtension();
                fileType = QString::fromUtf8(extension.c_str(), static_cast<int>(extension.size()));
                assetEntryType = QObject::tr("Source");
            }
            else if (const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(selectedEntry))
            {
                AZ::AssetTypeInfoBus::EventResult(fileType, productEntry->GetAssetType(), &AZ::AssetTypeInfo::GetGroup);
                assetEntryType = QObject::tr("Product");

                QFont font;
                font.setBold(true);

                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer container;
                bool found = false;
                bool succeeded = m_databaseConnection->QueryDirectProductDependencies(
                    productEntry->GetProductID(),
                    [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
                    {
                        found = true;
                        container.emplace_back() = AZStd::move(entry);
                        return true; // return true to keep iterating over further rows.
                    });
                if (found && succeeded)
                {
                    auto headerItem = new QTreeWidgetItem(m_dependentProducts);
                    headerItem->setText(0, QObject::tr("Outgoing Product Dependencies"));
                    headerItem->setFont(0, font);
                    for (AzToolsFramework::AssetDatabase::ProductDatabaseEntry& dependency : container)
                    {
                        AZ::Uuid sourceGuid;
                        m_databaseConnection->QuerySourceByProductID(
                            dependency.m_productID,
                            [&sourceGuid](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                            {
                                sourceGuid = entry.m_sourceGuid;
                                return false;
                            });
                        AZ::Data::AssetId productID(sourceGuid, dependency.m_subID);
                        auto product = ProductAssetBrowserEntry::GetProductByAssetId(productID);
                        if (product)
                        {
                            auto item = new QTreeWidgetItem(headerItem);
                            const auto namer = QString::fromUtf8(product->GetName().c_str(), static_cast<int>(product->GetName().size()));
                            item->setText(0, namer);
                            item->setToolTip(0, namer);
                            const auto& qvar =
                                AssetBrowserViewUtils::GetThumbnail(azrtti_cast<const AssetBrowserEntry*>(selectedEntry), true);
                            if (const auto& path = qvar.value<QString>(); !path.isEmpty())
                            {
                                QIcon icon;
                                QSize iconSize(item->sizeHint(0).height(), item->sizeHint(0).height());
                                icon.addFile(path, iconSize, QIcon::Normal, QIcon::Off);
                                item->setIcon(0, icon);
                            }
                        }
                    }
                    headerItem->setExpanded(true);
                }

                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer incomingContainer;
                found = false;
                succeeded = m_databaseConnection->QueryDirectReverseProductDependenciesBySourceGuidSubId(
                    productEntry->GetAssetId().m_guid,
                    productEntry->GetAssetId().m_subId,
                    [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& incomingDependency)
                    {
                        found = true;
                        incomingContainer.emplace_back() = AZStd::move(incomingDependency);
                        return true; // return true to keep iterating over further rows.
                    });
                if (found && succeeded)
                {
                    auto secondheaderItem = new QTreeWidgetItem(m_dependentProducts);
                    secondheaderItem->setText(0, QObject::tr("Incoming Product Dependencies"));
                    secondheaderItem->setFont(0, font);
                    for (AzToolsFramework::AssetDatabase::ProductDatabaseEntry& dependency : incomingContainer)
                    {
                        AZ::Uuid sourceGuid;
                        m_databaseConnection->QuerySourceByProductID(
                            dependency.m_productID,
                            [&sourceGuid](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                            {
                                sourceGuid = entry.m_sourceGuid;
                                return false;
                            });
                        AZ::Data::AssetId productID(sourceGuid, dependency.m_subID);
                        auto product = ProductAssetBrowserEntry::GetProductByAssetId(productID);
                        if (product)
                        {
                            auto item = new QTreeWidgetItem(secondheaderItem);
                            const auto namer2 = QString::fromUtf8(product->GetName().c_str(), static_cast<int>(product->GetName().size()));
                            item->setText(0, namer2);
                            item->setToolTip(0, namer2);
                            const auto& qvar =
                                AssetBrowserViewUtils::GetThumbnail(azrtti_cast<const AssetBrowserEntry*>(selectedEntry), true);
                            if (const auto& path = qvar.value<QString>(); !path.isEmpty())
                            {
                                QIcon icon;
                                QSize iconSize(item->sizeHint(0).height(), item->sizeHint(0).height());
                                icon.addFile(path, iconSize, QIcon::Normal, QIcon::Off);
                                item->setIcon(0, icon);
                            }
                        }
                    }
                    secondheaderItem->setExpanded(true);
                }
            }
            m_fileTypeLabel->setText(fileType);
            m_assetTypeLabel->setText(assetEntryType);

            const float diskSize = static_cast<float>(selectedEntry->GetDiskSize());
            m_diskSizeLabel->setText(QString::number(diskSize / 1000));

            const AZ::Vector3 dimension = selectedEntry->GetDimension();
            QString dimensionString;
            if (AZStd::isnan(dimension.GetX()) || AZStd::isnan(dimension.GetY()) || AZStd::isnan(dimension.GetZ()))
            {
                dimensionString = QObject::tr("0 x 0 x 0");
            }
            else
            {
                dimensionString = QString::number(dimension.GetX()) +
                    QObject::tr(" x ") + QString::number(dimension.GetY()) +
                    QObject::tr(" x ") + QString::number(dimension.GetZ());
            }
            m_dimensionLabel->setText(dimensionString);

            const auto vertices = selectedEntry->GetNumVertices();
            m_verticesLabel->setText(QString::number(vertices));

            const auto modificationTime = selectedEntry->GetModificationTime();
            m_lastModified->setText(QString::number(modificationTime));
        }

        void AssetBrowserEntityInspectorWidget::ClearPreview()
        {
            m_previewImage->clear();
            m_layoutSwitcher->setCurrentWidget(m_emptyLayoutWidget);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
