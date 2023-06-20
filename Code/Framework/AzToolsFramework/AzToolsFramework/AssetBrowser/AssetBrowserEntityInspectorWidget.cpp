/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntityInspectorWidget.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>

static constexpr int MinimumPopulatedWidth = 328;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserEntityInspectorWidget::AssetBrowserEntityInspectorWidget(QWidget* parent)
            : QWidget(parent)
            , m_databaseConnection(aznew AssetDatabase::AssetDatabaseConnection)
        {
            // Create the empty previewer
            m_emptyLayoutWidget = new QWidget(this);
            auto emptyLayout = new QVBoxLayout(m_emptyLayoutWidget);
            auto emptyLabel = new QLabel(m_emptyLayoutWidget);
            emptyLabel->setWordWrap(true);
            emptyLabel->setMargin(4);
            emptyLabel->setText(QObject::tr("Select an asset to view its properties in the inspector."));
            emptyLayout->addWidget(emptyLabel);
            emptyLayout->addStretch(1);
            m_emptyLayoutWidget->setLayout(emptyLayout);

            // Instantiate the populated previewer
            m_populatedLayoutWidget = new QWidget(this);
            auto populatedLayout = new QVBoxLayout(m_populatedLayoutWidget);

            // Create the layout for the asset icon preview
            auto iconLayout = new QVBoxLayout(m_populatedLayoutWidget);
            iconLayout->setSizeConstraint(QLayout::SetFixedSize);
            m_previewImage = new QLabel(m_populatedLayoutWidget);
            m_previewImage->setStyleSheet("QLabel{background-color: #222222; }");
            m_previewImage->setAlignment(Qt::AlignCenter);
            m_previewImage->setWordWrap(true);
            iconLayout->addWidget(m_previewImage, Qt::AlignCenter);

            // Create the layout for the asset details card
            auto cardLayout = new QVBoxLayout(m_populatedLayoutWidget);
            auto assetDetails = new AzQtComponents::Card();
            assetDetails->setTitle("Asset Details");
            assetDetails->header()->setHasContextMenu(false);
            assetDetails->hideFrame();
            cardLayout->addWidget(assetDetails);

            // Create a two column layout for the data inside of the asset details card
            m_assetDetailWidget = new QWidget(assetDetails);
            m_assetDetailLayout = new QFormLayout(m_assetDetailWidget);
            m_assetDetailLayout->setLabelAlignment(Qt::AlignRight);
            m_assetDetailWidget->setLayout(m_assetDetailLayout);
            assetDetails->setContentWidget(m_assetDetailWidget);

            // Create the layout for the dependent assets card
            m_dependentAssetsCard = new AzQtComponents::Card();
            QSizePolicy sizePolicyExpand = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            sizePolicyExpand.setRetainSizeWhenHidden(true);
            m_dependentAssetsCard->setSizePolicy(sizePolicyExpand);
            m_dependentAssetsCard->setTitle(QObject::tr("Dependent Assets"));
            m_dependentAssetsCard->header()->setHasContextMenu(false);
            m_dependentAssetsCard->hideFrame();
            cardLayout->addWidget(m_dependentAssetsCard);
            QSpacerItem* bottomSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Maximum);
            cardLayout->addItem(bottomSpacer);

            // Create a tree view for the data inside the depends assets card
            m_dependentProducts = new QTreeWidget(m_dependentAssetsCard);
            m_dependentProducts->setColumnCount(1);
            m_dependentProducts->setHeaderHidden(true);
            m_dependentProducts->setMinimumHeight(0);
            m_dependentAssetsCard->setContentWidget(m_dependentProducts);

            // Add the image preview and the card layouts to a single layout, spacing them appropriately
            populatedLayout->addLayout(iconLayout, 3);
            populatedLayout->addLayout(cardLayout, 7);
            m_populatedLayoutWidget->setLayout(populatedLayout);

            // Add the widgets containing both empty and populated layouts to a stacked widget
            m_layoutSwitcher = new QStackedLayout(this);
            m_layoutSwitcher->addWidget(m_populatedLayoutWidget);
            m_layoutSwitcher->addWidget(m_emptyLayoutWidget);
            setLayout(m_layoutSwitcher);

            m_headerFont.setBold(true);

            connect(
                m_dependentAssetsCard,
                &AzQtComponents::Card::expandStateChanged,
                this,
                [this, sizePolicyExpand, bottomSpacer](bool expanded)
                {
                    if (expanded)
                    {
                        m_dependentAssetsCard->setSizePolicy(sizePolicyExpand);
                        bottomSpacer->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Maximum);
                    }
                    else
                    {
                        m_dependentAssetsCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
                        bottomSpacer->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
                    }
                });

            AssetBrowserPreviewRequestBus::Handler::BusConnect();
            InitializeDatabase();
            ClearPreview();
        }

        AssetBrowserEntityInspectorWidget::~AssetBrowserEntityInspectorWidget()
        {
            AssetBrowserPreviewRequestBus::Handler::BusDisconnect();
        }

        void AssetBrowserEntityInspectorWidget::InitializeDatabase()
        {
            AZStd::string databaseLocation;
            AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Broadcast(
                &AzToolsFramework::AssetDatabase::AssetDatabaseRequests::GetAssetDatabaseLocation, databaseLocation);

            if (!databaseLocation.empty())
            {
                m_databaseConnection->OpenDatabase();
                m_databaseReady = true;
            }

        }

        void AssetBrowserEntityInspectorWidget::PreviewAsset(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry)
        {
            if (selectedEntry == nullptr || !m_databaseReady)
            {
                ClearPreview();
                return;
            }

            if (m_layoutSwitcher->currentWidget() != m_populatedLayoutWidget)
            {
                m_layoutSwitcher->setCurrentWidget(m_populatedLayoutWidget);
                setMinimumWidth(MinimumPopulatedWidth);
            }

            int assetDetailIndex = m_assetDetailLayout->rowCount() - 1;
            while (assetDetailIndex >= 0)
            {
                m_assetDetailLayout->removeRow(assetDetailIndex);
                assetDetailIndex--;
            }
            m_dependentProducts->clear();

            const auto& qVariant = AssetBrowserViewUtils::GetThumbnail(selectedEntry);
            QPixmap pixmap = qVariant.value<QPixmap>();
            {
                if (!pixmap.isNull())
                {
                    m_previewImage->setPixmap(pixmap);
                }
                else
                {
                    m_previewImage->setText(QObject::tr("No preview image avaialble."));
                }
            }

            const QString name = selectedEntry->GetDisplayName();
            if (!name.isEmpty())
            {
                const auto nameLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                nameLabel->setText(name);
                m_assetDetailLayout->addRow(QObject::tr("Name:"), nameLabel);
            }

            const auto fullPath = selectedEntry->GetFullPath();
            const auto location = QString::fromUtf8(fullPath.c_str(), static_cast<int>(fullPath.size()));
            if (!location.isEmpty())
            {
                const auto locationLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                locationLabel->setText(location);
                m_assetDetailLayout->addRow(QObject::tr("Location:"), locationLabel);
            }

            QString fileType;
            QString assetEntryType;
            if (const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(selectedEntry))
            {
                const auto extension = sourceEntry->GetExtension();
                fileType = QString::fromUtf8(extension.c_str(), static_cast<int>(extension.size()));
                if (fileType.startsWith(QLatin1Char{ '.' }))
                {
                    fileType.remove(0, 1);
                }
                assetEntryType = QObject::tr("Source");

                AZStd::vector<const ProductAssetBrowserEntry*> productChildren;
                sourceEntry->GetChildren(productChildren);
                if (!productChildren.empty())
                {
                    m_dependentAssetsCard->show();
                    PopulateSourceDependencies(sourceEntry, productChildren);
                    const auto headerItem = new QTreeWidgetItem(m_dependentProducts);
                    headerItem->setText(0, QObject::tr("Product Assets"));
                    headerItem->setFont(0, m_headerFont);
                    headerItem->setExpanded(true);
                    for (const auto productChild : productChildren)
                    {
                        const auto assetEntry = azrtti_cast<const AssetBrowserEntry*>(productChild);
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
                AZ::AssetTypeInfoBus::EventResult(fileType, productEntry->GetAssetType(), &AZ::AssetTypeInfo::GetGroup);
                assetEntryType = QObject::tr("Product");

                if (PopulateProductDependencies(productEntry))
                {
                    m_dependentAssetsCard->show();
                }
                else
                {
                    m_dependentAssetsCard->hide();
                }
            }

            if (!fileType.isEmpty())
            {
                const auto fileTypeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                fileTypeLabel->setText(fileType);
                m_assetDetailLayout->addRow(QObject::tr("File Type:"), fileTypeLabel);
            }

            if (!assetEntryType.isEmpty())
            {
                const auto assetTypeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                assetTypeLabel->setText(assetEntryType);
                m_assetDetailLayout->addRow(QObject::tr("Asset Type:"), assetTypeLabel);
            }

            const float diskSize = static_cast<float>(selectedEntry->GetDiskSize());
            if (diskSize != 0)
            {
                const auto diskSizeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                diskSizeLabel->setText(QString::number(diskSize / 1000));
                m_assetDetailLayout->addRow(QObject::tr("Disk Size (KB):"), diskSizeLabel);
            }

            const AZ::Vector3 dimension = selectedEntry->GetDimension();
            if (!AZStd::isnan(dimension.GetX()) && !AZStd::isnan(dimension.GetY()) && !AZStd::isnan(dimension.GetZ()))
            {
                const auto dimensionString = QString::number(dimension.GetX()) + QObject::tr(" x ") + QString::number(dimension.GetY()) +
                    QObject::tr(" x ") + QString::number(dimension.GetZ());
                const auto dimensionLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                dimensionLabel->setText(dimensionString);
                m_assetDetailLayout->addRow(QObject::tr("Dimensions:"), dimensionLabel);
            }

            const auto vertices = selectedEntry->GetNumVertices();
            if (vertices != 0)
            {
                const auto verticesLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                verticesLabel->setText(QString::number(vertices));
                m_assetDetailLayout->addRow(QObject::tr("Vertices:"), verticesLabel);
            }
        }

        void AssetBrowserEntityInspectorWidget::ClearPreview()
        {
            if (m_layoutSwitcher->currentWidget() != m_emptyLayoutWidget)
            {
                setMinimumWidth(0);
                m_previewImage->clear();
                m_dependentProducts->clear();
                int assetDetailIndex = m_assetDetailLayout->rowCount() - 1;
                while (assetDetailIndex >= 0)
                {
                    m_assetDetailLayout->removeRow(assetDetailIndex);
                    assetDetailIndex--;
                }
                m_populatedLayoutWidget->setMinimumWidth(m_previewImage->minimumSizeHint().width());
                m_layoutSwitcher->setCurrentWidget(m_emptyLayoutWidget);
            }
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

        bool AssetBrowserEntityInspectorWidget::PopulateProductDependencies(const ProductAssetBrowserEntry* productEntry)
        {
            AZStd::set<AZ::Data::AssetId> outgoingDependencyIds;
            AZStd::set<AZ::Data::AssetId> incomingDependencyIds;
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

            CreateProductDependencyTree(outgoingDependencyIds, true);
            CreateProductDependencyTree(incomingDependencyIds, false);
            return (!outgoingDependencyIds.empty() || !incomingDependencyIds.empty());
        }

        void AssetBrowserEntityInspectorWidget::CreateSourceDependencyTree(const AZStd::set<AZ::Uuid> sourceUuids, bool isOutgoing)
        {
            if (!sourceUuids.empty())
            {
                const auto headerItem = new QTreeWidgetItem(m_dependentProducts);
                headerItem->setFont(0, m_headerFont);
                headerItem->setExpanded(true);
                if (isOutgoing)
                {
                    headerItem->setText(0, QObject::tr("Outgoing Source Dependencies"));
                    headerItem->setToolTip(
                        0, QObject::tr("Lists all source assets that this asset depends on"));
                }
                else
                {
                    headerItem->setText(0, QObject::tr("Incoming Source Dependencies"));
                    headerItem->setToolTip(
                        0, QObject::tr("Lists all source assets that depend on this asset"));
                }
                for (AZ::Uuid sourceUuid : sourceUuids)
                {
                    auto sourceDependency = azrtti_cast<const AssetBrowserEntry*>(SourceAssetBrowserEntry::GetSourceByUuid(sourceUuid));
                    AddAssetBrowserEntryToTree(sourceDependency, headerItem);
                }
            }
        }

        void AssetBrowserEntityInspectorWidget::CreateProductDependencyTree(const AZStd::set<AZ::Data::AssetId> dependencyUuids, bool isOutgoing)
        {
            if (!dependencyUuids.empty())
            {
                const auto headerItem = new QTreeWidgetItem(m_dependentProducts);
                headerItem->setFont(0, m_headerFont);
                headerItem->setExpanded(true);
                if (isOutgoing)
                {
                    headerItem->setText(0, QObject::tr("Outgoing Product Dependencies"));
                    headerItem->setToolTip(
                        0, QObject::tr("Lists all product assets that this product depends on"));
                }
                else
                {
                    headerItem->setText(0, QObject::tr("Incoming Product Dependencies"));
                    headerItem->setToolTip(0, QObject::tr("Lists all product assets that depend on this product"));

                }
                for (AZ::Data::AssetId productId : dependencyUuids)
                {
                    auto productDependency = azrtti_cast<const AssetBrowserEntry*>(ProductAssetBrowserEntry::GetProductByAssetId(productId));
                    AddAssetBrowserEntryToTree(productDependency, headerItem);
                }
            }
        }

        void AssetBrowserEntityInspectorWidget::AddAssetBrowserEntryToTree(const AssetBrowserEntry* entry, QTreeWidgetItem* headerItem)
        {
            if (entry)
            {
                const auto item = new QTreeWidgetItem(headerItem);
                const auto entryName = QString::fromUtf8(entry->GetName().c_str(), static_cast<int>(entry->GetName().size()));
                item->setText(0, entryName);
                item->setToolTip(0, entryName);
                const auto& qvar = AssetBrowserViewUtils::GetThumbnail(entry, true);
                if (const auto& path = qvar.value<QString>(); !path.isEmpty())
                {
                    QIcon icon;
                    const QSize iconSize(item->sizeHint(0).height(), item->sizeHint(0).height());
                    icon.addFile(path, iconSize, QIcon::Normal, QIcon::Off);
                    item->setIcon(0, icon);
                }
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
