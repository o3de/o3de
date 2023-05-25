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
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>

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
            m_assetDetailWidget = new QWidget(assetDetails);
            m_assetDetailLayout = new QFormLayout(m_assetDetailWidget);
            m_assetDetailLayout->setLabelAlignment(Qt::AlignRight);
            m_assetDetailWidget->setLayout(m_assetDetailLayout);
            assetDetails->setContentWidget(m_assetDetailWidget);

            // Create the layout for the dependent assets card
            m_dependentAssetsCard = new AzQtComponents::Card();
            m_dependentProducts = new QTreeWidget(m_dependentAssetsCard);
            m_dependentProducts->setColumnCount(1);
            m_dependentProducts->setHeaderHidden(true);
            m_dependentAssetsCard->setContentWidget(m_dependentProducts);
            m_dependentAssetsCard->setTitle(QObject::tr("Dependent Assets"));
            m_dependentAssetsCard->header()->setHasContextMenu(false);
            m_dependentAssetsCard->hideFrame();
            cardLayout->addWidget(m_dependentAssetsCard);
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

            int assetDetailIndex = m_assetDetailLayout->rowCount() - 1;
            for (assetDetailIndex; assetDetailIndex >= 0; --assetDetailIndex)
            {
                m_assetDetailLayout->removeRow(assetDetailIndex);
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
            if (!name.isEmpty())
            {
                auto nameLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                nameLabel->setText(name);
                m_assetDetailLayout->addRow(QObject::tr("Name:"), nameLabel);
            }

            const auto fullPath = selectedEntry->GetFullPath();
            const auto location = QString::fromUtf8(fullPath.c_str(), static_cast<int>(fullPath.size()));
            if (!location.isEmpty())
            {
                auto locationLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                locationLabel->setText(location);
                m_assetDetailLayout->addRow(QObject::tr("Location:"), locationLabel);
            }

            QString fileType;
            QString assetEntryType;
            QFont font;
            font.setBold(true);
            if (const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(selectedEntry))
            {
                const auto extension = sourceEntry->GetExtension();
                fileType = QString::fromUtf8(extension.c_str(), static_cast<int>(extension.size()));
                assetEntryType = QObject::tr("Source");

                AZStd::vector<const ProductAssetBrowserEntry*> productChildren;
                sourceEntry->GetChildren(productChildren);

                if (!productChildren.empty())
                {
                    m_dependentAssetsCard->show();
                    PopulateSourceDependencies(sourceEntry, productChildren);
                    auto headerItem = new QTreeWidgetItem(m_dependentProducts);
                    headerItem->setText(0, QObject::tr("Product Assets"));
                    headerItem->setFont(0, font);
                    headerItem->setExpanded(true);
                    for (const auto productChild : productChildren)
                    {
                        auto assetEntry = azrtti_cast<const AssetBrowserEntry*>(productChild);
                        AddAssetBrowserEntryToTree(assetEntry, headerItem);
                    }
                }
                else
                {
                    m_dependentAssetsCard->hide();
                }
            }
            else if (const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(selectedEntry))
            {
                AZStd::set<AZ::Data::AssetId> incomingDependencyIds;
                AZStd::set<AZ::Data::AssetId> outgoingDependencyIds;
                AZ::AssetTypeInfoBus::EventResult(fileType, productEntry->GetAssetType(), &AZ::AssetTypeInfo::GetGroup);
                assetEntryType = QObject::tr("Product");

                m_databaseConnection->QueryDirectProductDependencies(
                    productEntry->GetProductID(),
                    [&, productEntry](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& ProductDatabaseEntry)
                    {
                        if (ProductDatabaseEntry.m_productID != productEntry->GetProductID())
                        {
                            m_databaseConnection->QuerySourceByProductID(
                                ProductDatabaseEntry.m_productID,
                                [&outgoingDependencyIds,
                                 &ProductDatabaseEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& SourceDatabaseEntry)
                                {
                                    AZ::Data::AssetId assetId(SourceDatabaseEntry.m_sourceGuid, ProductDatabaseEntry.m_subID);
                                    outgoingDependencyIds.insert(assetId);
                                    return false;
                                });
                        }
                        return true;
                    });

                m_databaseConnection->QueryDirectReverseProductDependenciesBySourceGuidSubId(
                    productEntry->GetAssetId().m_guid,
                    productEntry->GetAssetId().m_subId,
                    [&, productEntry](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& ProductDatabaseEntry)
                    {
                        if (ProductDatabaseEntry.m_productID != productEntry->GetProductID())
                        {
                            m_databaseConnection->QuerySourceByProductID(
                                ProductDatabaseEntry.m_productID,
                                [&incomingDependencyIds,
                                 ProductDatabaseEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& SourceDatabaseEntry)
                                {
                                    AZ::Data::AssetId assetId(SourceDatabaseEntry.m_sourceGuid, ProductDatabaseEntry.m_subID);
                                    incomingDependencyIds.insert(assetId);
                                    return false;
                                });
                        }
                        return true;
                    });

                if (!outgoingDependencyIds.empty())
                {
                    auto headerItem = new QTreeWidgetItem(m_dependentProducts);
                    headerItem->setText(0, QObject::tr("Outgoing Product Dependencies"));
                    headerItem->setFont(0, font);
                    for (AZ::Data::AssetId productId : outgoingDependencyIds)
                    {
                        auto product = ProductAssetBrowserEntry::GetProductByAssetId(productId);
                        if (product)
                        {
                            auto item = new QTreeWidgetItem(headerItem);
                            const auto namer = QString::fromUtf8(product->GetName().c_str(), static_cast<int>(product->GetName().size()));
                            item->setText(0, namer);
                            item->setToolTip(0, namer);
                            const auto& qvar =
                                AssetBrowserViewUtils::GetThumbnail(azrtti_cast<const AssetBrowserEntry*>(product), true);
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
                
                if (!incomingDependencyIds.empty())
                {
                    auto secondheaderItem = new QTreeWidgetItem(m_dependentProducts);
                    secondheaderItem->setText(0, QObject::tr("Incoming Product Dependencies"));
                    secondheaderItem->setFont(0, font);
                    for (AZ::Data::AssetId productId : incomingDependencyIds)
                    {
                        auto product = ProductAssetBrowserEntry::GetProductByAssetId(productId);
                        if (product)
                        {
                            auto item = new QTreeWidgetItem(secondheaderItem);
                            const auto namer2 = QString::fromUtf8(product->GetName().c_str(), static_cast<int>(product->GetName().size()));
                            item->setText(0, namer2);
                            item->setToolTip(0, namer2);
                            const auto& qvar =
                                AssetBrowserViewUtils::GetThumbnail(azrtti_cast<const AssetBrowserEntry*>(product), true);
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
            if (!fileType.isEmpty())
            {
                auto fileTypeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                fileTypeLabel->setText(fileType);
                m_assetDetailLayout->addRow(QObject::tr("File Type:"), fileTypeLabel);
            }

            if (!assetEntryType.isEmpty())
            {
                auto assetTypeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                assetTypeLabel->setText(assetEntryType);
                m_assetDetailLayout->addRow(QObject::tr("Asset Type:"), assetTypeLabel);
            }

            const float diskSize = static_cast<float>(selectedEntry->GetDiskSize());
            if (diskSize != 0)
            {
                auto diskSizeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                diskSizeLabel->setText(QString::number(diskSize / 1000));
                m_assetDetailLayout->addRow(QObject::tr("Disk Size (KB):"), diskSizeLabel);
            }

            const AZ::Vector3 dimension = selectedEntry->GetDimension();
            if (!AZStd::isnan(dimension.GetX()) && !AZStd::isnan(dimension.GetY()) && !AZStd::isnan(dimension.GetZ()))
            {
                const auto dimensionString = QString::number(dimension.GetX()) + QObject::tr(" x ") + QString::number(dimension.GetY()) +
                    QObject::tr(" x ") + QString::number(dimension.GetZ());
                auto dimensionLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                dimensionLabel->setText(dimensionString);
                m_assetDetailLayout->addRow(QObject::tr("Dimensions:"), dimensionLabel);
            }
           

            const auto vertices = selectedEntry->GetNumVertices();
            if (vertices != 0)
            {
                auto verticesLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                verticesLabel->setText(QString::number(vertices));
                m_assetDetailLayout->addRow(QObject::tr("Vertices:"), verticesLabel);
            }

            const auto modificationTime = selectedEntry->GetModificationTime();
            if (modificationTime != 0)
            {
                auto lastModified = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                lastModified->setText(QString::number(modificationTime));
                m_assetDetailLayout->addRow(QObject::tr("Last Modified:"), lastModified);
            }
        }

        void AssetBrowserEntityInspectorWidget::ClearPreview()
        {
            m_previewImage->clear();
            m_layoutSwitcher->setCurrentWidget(m_emptyLayoutWidget);
        }

        void AssetBrowserEntityInspectorWidget::PopulateSourceDependencies(
            const SourceAssetBrowserEntry* sourceEntry,AZStd::vector<const ProductAssetBrowserEntry*> productList)
        {
            AZStd::set<AZ::Uuid> outgoingDependencyUuids;
            AZStd::set<AZ::Uuid> incomingDependencyUuids;
            for (const ProductAssetBrowserEntry* productChild : productList)
            {
                // Create a set of all sources whose products are depended on by any of the products of the specified source
                m_databaseConnection->QueryDirectProductDependencies(
                    productChild->GetProductID(),
                    [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
                    {
                        m_databaseConnection->QuerySourceByProductID(
                            entry.m_productID,
                            [&outgoingDependencyUuids, sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                            {
                                if (entry.m_sourceGuid != sourceEntry->GetSourceUuid())
                                {
                                    outgoingDependencyUuids.insert(entry.m_sourceGuid);
                                }
                                return false;
                            });
                        return true;
                    });

                // Create a set of all sources whose products depend on any of the products of the specified source
                m_databaseConnection->QueryDirectReverseProductDependenciesBySourceGuidSubId(
                    productChild->GetAssetId().m_guid,
                    productChild->GetAssetId().m_subId,
                    [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
                    {
                        m_databaseConnection->QuerySourceByProductID(
                            entry.m_productID,
                            [&incomingDependencyUuids, sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                            {
                                if (entry.m_sourceGuid != sourceEntry->GetSourceUuid())
                                {
                                    incomingDependencyUuids.insert(entry.m_sourceGuid);
                                }
                                return false;
                            });
                        return true;
                    });
            }
            CreateSourceDependencyTree(outgoingDependencyUuids, true);
            CreateSourceDependencyTree(incomingDependencyUuids, false);
        }

        void AssetBrowserEntityInspectorWidget::CreateSourceDependencyTree(AZStd::set<AZ::Uuid> sourceUuids, bool isOutgoing)
        {
            if (!sourceUuids.empty())
            {
                auto headerItem = new QTreeWidgetItem(m_dependentProducts);
                QFont font;
                font.setBold(true);
                headerItem->setFont(0, font);
                headerItem->setExpanded(false);
                if (isOutgoing)
                {
                    headerItem->setText(0, QObject::tr("Outgoing Source Dependencies"));
                    headerItem->setToolTip(
                        0, QObject::tr("Outgoing Source Dependencies lists all source assets that this asset depends on"));
                }
                else
                {
                    headerItem->setText(0, QObject::tr("Incoming Source Dependencies"));
                    headerItem->setToolTip(
                        0, QObject::tr("Incoming Source Dependencies lists all source assets that depend on this asset"));
                }
                for (AZ::Uuid sourceUuid : sourceUuids)
                {
                    auto sourceDependency = azrtti_cast<const AssetBrowserEntry*>(SourceAssetBrowserEntry::GetSourceByUuid(sourceUuid));
                    AddAssetBrowserEntryToTree(sourceDependency, headerItem);
                }
            }
        }

        void AssetBrowserEntityInspectorWidget::AddAssetBrowserEntryToTree(const AssetBrowserEntry* entry, QTreeWidgetItem* headerItem)
        {
            if (entry)
            {
                auto item = new QTreeWidgetItem(headerItem);
                const auto entryName = QString::fromUtf8(entry->GetName().c_str(), static_cast<int>(entry->GetName().size()));
                item->setText(0, entryName);
                item->setToolTip(0, entryName);
                const auto& qvar = AssetBrowserViewUtils::GetThumbnail(entry, true);
                if (const auto& path = qvar.value<QString>(); !path.isEmpty())
                {
                    QIcon icon;
                    QSize iconSize(item->sizeHint(0).height(), item->sizeHint(0).height());
                    icon.addFile(path, iconSize, QIcon::Normal, QIcon::Off);
                    item->setIcon(0, icon);
                }
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
