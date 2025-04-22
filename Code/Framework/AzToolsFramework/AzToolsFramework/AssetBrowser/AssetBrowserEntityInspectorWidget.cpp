/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/SegmentBar.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntityInspectorWidget.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <QAction>
#include <QDesktopServices>
#include <QDir>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QUrl>

static constexpr int MinimumPopulatedWidth = 328;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class DependentAssetTreeWidgetItem : public QTreeWidgetItem
        {
        public:
            DependentAssetTreeWidgetItem(const AssetBrowserEntry* assetBrowserEntry, QTreeWidgetItem* parent)
                : m_assetBrowserEntry(assetBrowserEntry)
                , QTreeWidgetItem(parent)
            {}

            explicit DependentAssetTreeWidgetItem(QTreeWidget* treeView)
                : m_assetBrowserEntry(nullptr)
                , QTreeWidgetItem(treeView)
            {}

            const AssetBrowserEntry* m_assetBrowserEntry;
        };

        class DependentAssetTreeWidget : public QTreeWidget
        {
        public:
            explicit DependentAssetTreeWidget(QWidget* parent = nullptr)
                : QTreeWidget(parent)
            {
                setContextMenuPolicy(Qt::CustomContextMenu);

                connect(this, &QTreeWidget::customContextMenuRequested, this, &DependentAssetTreeWidget::OnContextMenu);
            }

            ~DependentAssetTreeWidget() override = default;

        private:
            void mouseDoubleClickEvent([[maybe_unused]] QMouseEvent* ev) override
            {
                if (const AssetBrowserEntry* assetBrowserEntry = GetSelectedAssetBrowserEntry())
                {
                    BrowseToAsset(*assetBrowserEntry);
                }
            }

            void OnContextMenu(const QPoint& point)
            {
                const AssetBrowserEntry* assetBrowserEntry = GetSelectedAssetBrowserEntry();
                if (!assetBrowserEntry)
                {
                    return;
                }

                QMenu menu(this);
                menu.addAction(
                    QObject::tr("Browse to asset"),
                    [assetBrowserEntry]()
                    {
                        BrowseToAsset(*assetBrowserEntry);
                    });
                menu.exec(mapToGlobal(point));
            }

            const AssetBrowserEntry* GetSelectedAssetBrowserEntry() const
            {
                const QModelIndexList selection = selectedIndexes();
                if (selection.isEmpty())
                {
                    return nullptr;
                }

                const auto* item = static_cast<const DependentAssetTreeWidgetItem*>(selection.first().internalPointer());
                return item->m_assetBrowserEntry;
            }

            static void BrowseToAsset(const AssetBrowserEntry& assetBrowserEntry)
            {
                // Product asset path is located in the cached folder not discoverable by asset browser
                // so we resolve the source path instead
                auto path = assetBrowserEntry.GetFullPath();
                if (assetBrowserEntry.GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
                {
                    const auto* productItem = static_cast<const ProductAssetBrowserEntry*>(&assetBrowserEntry);
                    if (const SourceAssetBrowserEntry* source = SourceAssetBrowserEntry::GetSourceByUuid(productItem->GetAssetId().m_guid))
                    {
                        path = source->GetFullPath();
                    }
                }

                AssetBrowserInteractionNotificationBus::Broadcast(
                        &AssetBrowserInteractionNotificationBus::Events::SelectAsset,
                        nullptr,
                        path);
            }
        };

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
            populatedLayout->setContentsMargins(0, 0, 0, 0);

            // Create the buttons to switch between asset details and scene settings
            auto buttonLayout = new QHBoxLayout(m_populatedLayoutWidget);
            buttonLayout->setSpacing(0);
            m_detailsButton = new QPushButton(m_populatedLayoutWidget);
            m_detailsButton->setText(QObject::tr("Details"));
            m_detailsButton->setCheckable(true);
            m_detailsButton->setFlat(true);
            m_detailsButton->setStyleSheet(
                "QPushButton {background-color: #333333; border-color #222222; border-style: solid; border-width: 1px;"
                "border-top-left-radius: 3px; border-bottom-left-radius: 3px; border-top-right-radius: 0px;"
                "border-bottom-right-radius: 0px; font-size: 12px; height: 28px;}"
                "QPushButton:checked {background-color: #1E70EB;}"
                "QPushButton:hover:!checked {background-color: #444444;}"
                "QPushButton:pressed:!checked: {background-color: #444444;}");
            m_sceneSettingsButton = new QPushButton(m_populatedLayoutWidget);
            m_sceneSettingsButton->setText("Scene Settings");
            m_sceneSettingsButton->setCheckable(true);
            m_sceneSettingsButton->setFlat(true);
            m_sceneSettingsButton->setStyleSheet(
                "QPushButton {background-color: #333333; border-color #222222; border-style: solid; border-width: 1px;"
                "border-top-left-radius: 0px; border-bottom-left-radius: 0px; border-top-right-radius: 3px;"
                "border-bottom-right-radius: 0px; margin-left: -1px; font-size: 12px; height: 28px;}"
                "QPushButton:checked {background-color: #1E70EB;}"
                "QPushButton:hover:!checked {background-color: #444444;}"
                "QPushButton:pressed:!checked: {background-color: #444444;}");
            buttonLayout->addWidget(m_detailsButton);
            buttonLayout->addWidget(m_sceneSettingsButton);

            // Create the layout for the asset icon preview
            m_previewImage = new AzQtComponents::ExtendedLabel(m_populatedLayoutWidget);
            m_previewImage->setStyleSheet("QLabel {background-color: #333333;}");
            m_previewImage->setAlignment(Qt::AlignCenter);
            m_previewImage->setWordWrap(true);

            // Create the layout for the asset details card
            auto cardLayout = new QVBoxLayout(m_populatedLayoutWidget);
            m_detailsCard = new AzQtComponents::Card();
            m_detailsCard->setTitle(QObject::tr("Asset Details"));
            m_detailsCard->header()->setHasContextMenu(false);
            m_detailsCard->hideFrame();
            cardLayout->addWidget(m_detailsCard);

            // Create a two column layout for the data inside of the asset details card
            m_assetDetailWidget = new QWidget(m_detailsCard);
            m_assetDetailLayout = new QFormLayout(m_assetDetailWidget);
            m_assetDetailLayout->setLabelAlignment(Qt::AlignRight);
            m_assetDetailWidget->setLayout(m_assetDetailLayout);
            m_detailsCard->setContentWidget(m_assetDetailWidget);

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
            m_dependentProducts = new DependentAssetTreeWidget(m_dependentAssetsCard);
            m_dependentProducts->setColumnCount(1);
            m_dependentProducts->setHeaderHidden(true);
            m_dependentProducts->setMinimumHeight(0);
            m_dependentAssetsCard->setContentWidget(m_dependentProducts);

            // If opening the Scene Settings is successful, enable switching between the asset details and scene settings
            m_settingsSwitcher = new QStackedWidget(m_populatedLayoutWidget);
            m_detailsWidget = new QWidget(m_settingsSwitcher);
            m_detailsWidget->setLayout(cardLayout);
            m_settingsSwitcher->addWidget(m_detailsWidget);
            AssetBrowserPreviewRequestBus::BroadcastResult(m_sceneSettings, &AssetBrowserPreviewRequest::GetSceneSettings);
            if (m_sceneSettings)
            {
                m_settingsSwitcher->addWidget(m_sceneSettings);
                connect(
                    m_detailsButton,
                    &QPushButton::clicked,
                    this,
                    [this]
                    {
                        m_settingsSwitcher->setCurrentIndex(0);
                        m_detailsButton->setChecked(true);
                        m_sceneSettingsButton->setChecked(false);
                    });

                connect(
                    m_sceneSettingsButton,
                    &QPushButton::clicked,
                    this,
                    [this]
                    {
                        AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::PreviewSceneSettings, m_currentEntry);
                        m_settingsSwitcher->setCurrentIndex(1);
                        m_sceneSettingsButton->setChecked(true);
                        m_detailsButton->setChecked(false);
                    });
            }

            // Create the QSplitter to resize the image preview and the details/settings
            QSplitter* splitter = new QSplitter(m_populatedLayoutWidget);
            splitter->setHandleWidth(2);
            splitter->setOrientation(Qt::Vertical);
            splitter->addWidget(m_previewImage);
            splitter->addWidget(m_settingsSwitcher);

            // Add the buttons to switch settings, and the QSplitter to the main layout
            populatedLayout->addLayout(buttonLayout);
            populatedLayout->addWidget(splitter);
            m_populatedLayoutWidget->setLayout(populatedLayout);

            // Add the widgets containing both empty and populated layouts to a stacked widget
            m_layoutSwitcher = new QStackedLayout(this);
            m_layoutSwitcher->setContentsMargins(0, 0, 0, 0);
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

            // currently, we only preview sources or products.
            // however, in an effort to make it forward compatible, I assume we will get more than this and extend in the future
            // for example, if someone wants to implement a "folder" or "root" or "gem folder" previewer, at least we call this function
            // and give it the option to return and clear.
            const auto entryType = selectedEntry->GetEntryType();
            bool canBePreviewed = (entryType == AssetBrowserEntry::AssetEntryType::Source) ||
                                  (entryType == AssetBrowserEntry::AssetEntryType::Product);

            if (!canBePreviewed)
            {
                ClearPreview();
                return;
            }

            if (m_layoutSwitcher->currentWidget() != m_populatedLayoutWidget)
            {
                m_layoutSwitcher->setCurrentWidget(m_populatedLayoutWidget);
                setMinimumWidth(MinimumPopulatedWidth);
            }

            if (m_settingsSwitcher->currentWidget() != m_detailsWidget)
            {
                m_settingsSwitcher->setCurrentWidget(m_detailsWidget);
            }

            m_previewImage->show();
            m_detailsButton->setChecked(true);
            m_sceneSettingsButton->setChecked(false);
            if (selectedEntry == m_currentEntry)
            {
                return;
            }
            m_currentEntry = selectedEntry;

            int assetDetailIndex = m_assetDetailLayout->rowCount() - 1;
            while (assetDetailIndex >= 0)
            {
                m_assetDetailLayout->removeRow(assetDetailIndex);
                assetDetailIndex--;
            }
            m_dependentProducts->clear();
            m_detailsCard->setExpanded(true);
            m_detailsButton->hide();
            m_sceneSettingsButton->hide();

            const auto& qVariant = AssetBrowserViewUtils::GetThumbnail(selectedEntry);
            QPixmap pixmap = qVariant.value<QPixmap>();
            {
                if (!pixmap.isNull())
                {
                    m_previewImage->setPixmapKeepAspectRatio(pixmap);
                }
                else
                {
                    m_previewImage->setText(QObject::tr("No preview image avaialble."));
                }
            }

            QString name = selectedEntry->GetDisplayName();
            if (!name.isEmpty())
            {
                const auto extensionIndex = name.lastIndexOf(QLatin1Char{ '.' });
                if (extensionIndex != -1)
                {
                    name = name.left(extensionIndex);
                }
                const auto nameLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                nameLabel->setText(name);
                m_assetDetailLayout->addRow(QObject::tr("<b>Name:</b>"), nameLabel);
            }

            const AZ::IO::Path fullPath = selectedEntry->GetFullPath();
            if (!fullPath.empty())
            {
                const QString location = AZ::IO::Path(fullPath.ParentPath()).c_str();
                QString nativePath(QString("%1%2").arg(QDir::toNativeSeparators(location)).arg(QDir::separator()));
                const auto locationLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                locationLabel->setText(nativePath);

                QIcon explorerIcon;
                explorerIcon.addFile(":/SceneUI/Manifest/show_in_explorer.svg", QSize(18,18), QIcon::Normal, QIcon::Off);
                auto openExplorer = new QPushButton(m_assetDetailWidget);
                openExplorer->setMaximumSize(20, 20);
                openExplorer->setIcon(explorerIcon);
                openExplorer->setFlat(true);

                connect(
                    openExplorer,
                    &QPushButton::clicked,
                    this,
                    [nativePath]()
                    {
                        QDesktopServices::openUrl(QUrl::fromLocalFile(nativePath));
                    });

                auto clickableURL = new QHBoxLayout();
                clickableURL->addWidget(openExplorer);
                clickableURL->addWidget(locationLabel);

                m_assetDetailLayout->addRow(QObject::tr("<b>Location:</b>"), clickableURL);
            }

            if (const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(selectedEntry))
            {
                HandleSourceAsset(selectedEntry, sourceEntry);
            }
            else if (const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(selectedEntry))
            {
                HandleProductAsset(productEntry);
            }

            const float diskSize = static_cast<float>(selectedEntry->GetDiskSize());
            if (diskSize != 0)
            {
                const auto diskSizeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                diskSizeLabel->setText(QString::number(diskSize / 1000));
                m_assetDetailLayout->addRow(QObject::tr("<b>Disk Size (KB):</b>"), diskSizeLabel);
            }

            const AZ::Vector3 dimension = selectedEntry->GetDimension();
            if (!AZStd::isnan(dimension.GetX()) && !AZStd::isnan(dimension.GetY()) && !AZStd::isnan(dimension.GetZ()))
            {
                const auto dimensionString = QString::number(dimension.GetX()) + QObject::tr(" x ") + QString::number(dimension.GetY()) +
                    QObject::tr(" x ") + QString::number(dimension.GetZ());
                const auto dimensionLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                dimensionLabel->setText(dimensionString);
                m_assetDetailLayout->addRow(QObject::tr("<b>Dimensions:</b>"), dimensionLabel);
            }

            const auto vertices = selectedEntry->GetNumVertices();
            if (vertices != 0)
            {
                const auto verticesLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                verticesLabel->setText(QString::number(vertices));
                m_assetDetailLayout->addRow(QObject::tr("<b>Vertices:</b>"), verticesLabel);
            }
        }

        void AssetBrowserEntityInspectorWidget::HandleSourceAsset(const AssetBrowserEntry* selectedEntry, const SourceAssetBrowserEntry* sourceEntry)
        {
            const auto extension = sourceEntry->GetExtension();
            auto fileType = QString::fromUtf8(extension.c_str(), static_cast<int>(extension.size()));
            if (!fileType.isEmpty())
            {
                if (fileType.startsWith(QLatin1Char{ '.' }))
                {
                    fileType.remove(0, 1);
                }
                const auto fileTypeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                fileTypeLabel->setText(fileType);
                m_assetDetailLayout->addRow(QObject::tr("<b>File Type:</b>"), fileTypeLabel);
            }

            const auto assetEntryType = QObject::tr("Source");
            const auto assetTypeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
            assetTypeLabel->setText(assetEntryType);
            m_assetDetailLayout->addRow(QObject::tr("<b>Asset Type:</b>"), assetTypeLabel);

            AZStd::vector<const ProductAssetBrowserEntry*> productChildren;
            sourceEntry->GetChildren(productChildren);
            if (!productChildren.empty())
            {
                m_dependentAssetsCard->show();
                m_dependentAssetsCard->setExpanded(true);
                PopulateSourceDependencies(sourceEntry, productChildren);
                const auto headerItem = new DependentAssetTreeWidgetItem(m_dependentProducts);
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

            if (m_sceneSettings)
            {
                bool validSceneSettings = false;
                AssetBrowserPreviewRequestBus::BroadcastResult(
                    validSceneSettings, &AssetBrowserPreviewRequest::HandleSource, selectedEntry);
                if (validSceneSettings)
                {
                    QString defaultSettings = fileType.isEmpty() ? "Scene" : fileType;
                    m_sceneSettingsButton->setText(QObject::tr("%1 Settings").arg(fileType.toUpper()));
                    m_detailsButton->show();
                    m_detailsButton->setChecked(true);
                    m_sceneSettingsButton->show();
                    m_sceneSettingsButton->setChecked(false);
                }
            }
        }

        void AssetBrowserEntityInspectorWidget::HandleProductAsset(const ProductAssetBrowserEntry* productEntry)
        {
            QString fileType;
            AZ::AssetTypeInfoBus::EventResult(fileType, productEntry->GetAssetType(), &AZ::AssetTypeInfo::GetGroup);
            if (!fileType.isEmpty())
            {
                const auto fileTypeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
                fileTypeLabel->setText(fileType);
                m_assetDetailLayout->addRow(QObject::tr("<b>File Type:</b>"), fileTypeLabel);
            }

            const auto assetEntryType = QObject::tr("Product");
            const auto assetTypeLabel = new AzQtComponents::ElidingLabel(m_assetDetailWidget);
            assetTypeLabel->setText(assetEntryType);
            m_assetDetailLayout->addRow(QObject::tr("<b>Asset Type:</b>"), assetTypeLabel);

            if (PopulateProductDependencies(productEntry))
            {
                m_dependentAssetsCard->show();
                m_dependentAssetsCard->setExpanded(true);
            }
            else
            {
                m_dependentAssetsCard->hide();
            }
        }

        void AssetBrowserEntityInspectorWidget::ClearPreview()
        {
            if (m_layoutSwitcher->currentWidget() != m_emptyLayoutWidget)
            {
                setMinimumWidth(0);
                m_previewImage->hide();
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
                const auto headerItem = new DependentAssetTreeWidgetItem(m_dependentProducts);
                headerItem->setFont(0, m_headerFont);
                headerItem->setExpanded(false);
                if (isOutgoing)
                {
                    headerItem->setText(0, QObject::tr("Outgoing Source Dependencies"));
                    headerItem->setToolTip(0, QObject::tr("Lists all source assets that this asset depends on"));
                }
                else
                {
                    headerItem->setText(0, QObject::tr("Incoming Source Dependencies"));
                    headerItem->setToolTip(0, QObject::tr("Lists all source assets that depend on this asset"));
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
                const auto headerItem = new DependentAssetTreeWidgetItem(m_dependentProducts);
                headerItem->setFont(0, m_headerFont);
                headerItem->setExpanded(true);
                if (isOutgoing)
                {
                    headerItem->setText(0, QObject::tr("Outgoing Product Dependencies"));
                    headerItem->setToolTip(0, QObject::tr("Lists all product assets that this product depends on"));
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

        void AssetBrowserEntityInspectorWidget::AddAssetBrowserEntryToTree(
            const AssetBrowserEntry* entry, DependentAssetTreeWidgetItem* headerItem)
        {
            if (entry)
            {
                const auto item = new DependentAssetTreeWidgetItem(entry, headerItem);
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

        bool AssetBrowserEntityInspectorWidget::HasUnsavedChanges()
        {
            if (m_sceneSettings)
            {
                bool hasUnsavedChanges = false;
                AssetBrowserPreviewRequestBus::BroadcastResult(hasUnsavedChanges, &AssetBrowserPreviewRequest::SaveBeforeClosing);
                return hasUnsavedChanges;
            }
            return false;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
