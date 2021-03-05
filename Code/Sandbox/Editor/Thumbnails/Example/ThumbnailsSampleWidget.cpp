/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "EditorDefs.h"

#include "ThumbnailsSampleWidget.h"

// Qt
#include <QLabel>

// AzToolsFramework
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/Thumbnails/ThumbnailWidget.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/AssetBrowserProductThumbnail.h>

// Editor
#include "QtViewPaneManager.h"                                      // for RegisterQtViewPane


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <Thumbnails/Example/ui_ThumbnailsSampleWidget.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

const int MAX_PRODUCTS_TO_DISPLAY = 20;

ThumbnailsSampleWidget::ThumbnailsSampleWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::ThumbnailsSampleWidgetClass())
    , m_filterModel(new AzToolsFramework::AssetBrowser::AssetBrowserFilterModel(parent))
{
    m_ui->setupUi(this);
    m_ui->m_searchWidget->Setup(true, true);

    using namespace AzToolsFramework::AssetBrowser;
    AssetBrowserComponentRequestBus::BroadcastResult(m_assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
    AZ_Assert(m_assetBrowserModel, "Failed to get filebrowser model");
    m_filterModel->setSourceModel(m_assetBrowserModel);
    m_filterModel->SetFilter(m_ui->m_searchWidget->GetFilter());

    m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel.data());

    auto layout = static_cast<QVBoxLayout*>(m_ui->m_thumbnailScrollAreaRoot->layout());
    layout->addStretch(1);

    connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::selectionChangedSignal,
        this, &ThumbnailsSampleWidget::SelectionChangedSlot);
    connect(m_ui->m_searchWidget->GetFilter().data(), &AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::updatedSignal,
        m_filterModel.data(), &AzToolsFramework::AssetBrowser::AssetBrowserFilterModel::filterUpdatedSlot);
}

ThumbnailsSampleWidget::~ThumbnailsSampleWidget() = default;

void ThumbnailsSampleWidget::RegisterViewClass()
{
    QtViewOptions options;
    options.preferedDockingArea = Qt::NoDockWidgetArea;
    options.canHaveMultipleInstances = true;
    RegisterQtViewPane<ThumbnailsSampleWidget>(GetIEditor(), "Thumbnails Demo", LyViewPane::CategoryTools, options);
}

void ThumbnailsSampleWidget::SelectionChangedSlot(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/) const
{
    UpdateThumbnail();
}

void ThumbnailsSampleWidget::UpdateThumbnail() const
{
    auto layout = static_cast<QVBoxLayout*>(m_ui->m_thumbnailScrollAreaRoot->layout());
    // delete any previous thumbnails
    qDeleteAll(m_ui->m_thumbnailScrollAreaRoot->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));

    auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
    if (selectedAssets.size() > 0)
    {
        using namespace AzToolsFramework::AssetBrowser;
        //get all products from selected entry (it can be a folder, source or product asset and can contain 0 or more products)
        AZStd::vector<const ProductAssetBrowserEntry*> products;
        selectedAssets.front()->GetChildrenRecursively<ProductAssetBrowserEntry>(products);

        // because thumbnails are displayed via individual widgets, limit to 10 otherwise it can take ages
       
        int productsLeft = MAX_PRODUCTS_TO_DISPLAY;
        for (const auto* product : products)
        {
            // create thumbnail widget
            auto thumbnailWidget = new AzToolsFramework::Thumbnailer::ThumbnailWidget(m_ui->m_thumbnailScrollArea);
            thumbnailWidget->SetThumbnailKey(MAKE_TKEY(AzToolsFramework::AssetBrowser::ProductThumbnailKey, product->GetAssetId()));
            thumbnailWidget->setMinimumSize(100, 100);
            thumbnailWidget->setMaximumSize(100, 100);
            // insert it before space to align on top
            layout->insertWidget(layout->count() - 1, thumbnailWidget);
            // add label indicating name of the asset
            auto label = new QLabel(product->GetName().c_str(), m_ui->m_thumbnailScrollArea);
            layout->insertWidget(layout->count() - 1, label);
            // do not render more than 10 thumbnails at a time
            productsLeft--;
            if (productsLeft <= 0)
            {
                break;
            }
        }
    }
    return;
}
#include <Thumbnails/Example/moc_ThumbnailsSampleWidget.cpp>
