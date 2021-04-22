#include "AssetBrowserTableView.h"

#pragma optimize("", off)
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTableView::AssetBrowserTableView(QWidget* parent)
            : QTableView(parent)
        {
            AssetBrowserViewRequestBus::Handler::BusConnect();
            AssetBrowserComponentNotificationBus::Handler::BusConnect();
        }
        AssetBrowserTableView::~AssetBrowserTableView()
        {
            AssetBrowserViewRequestBus::Handler::BusDisconnect();
            AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
        }
        void AssetBrowserTableView::setModel(QAbstractItemModel* model)
        {
            //m_assetBrowserSortFilterProxyModel = qobject_cast<AssetBrowserTableFilterModel*>(model);
            //AZ_Assert(m_assetBrowserSortFilterProxyModel, "Expecting AssetBrowserTableFilterModel");
            //m_assetBrowserModel = qobject_cast<AssetBrowserModel*>(m_assetBrowserSortFilterProxyModel->sourceModel());
            QTableView::setModel(model);
        }
        void AssetBrowserTableView::SetName(const QString& name)
        {
            m_name = name;
            bool isAssetBrowserComponentReady = false;
            AssetBrowserComponentRequestBus::BroadcastResult(isAssetBrowserComponentReady, &AssetBrowserComponentRequests::AreEntriesReady);
            if (isAssetBrowserComponentReady)
            {
                OnAssetBrowserComponentReady();
            }
        }
        AZStd::vector<AssetBrowserEntry*> AssetBrowserTableView::GetSelectedAssets() const
        {
            return AZStd::vector<AssetBrowserEntry*>();
        }

        void AssetBrowserTableView::SelectProduct(AZ::Data::AssetId assetID)
        {
            AZ_UNUSED(assetID);
        }

        void AssetBrowserTableView::SelectFileAtPath(const AZStd::string& assetPath)
        {
            AZ_UNUSED(assetPath);
        }

        void AssetBrowserTableView::ClearFilter()
        {
        }

        void AssetBrowserTableView::Update()
        {
        }

        //void AssetBrowserTableView::OnAssetBrowserComponentReady()
        //{
        //}
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#pragma optimize("", on)
#include "AssetBrowser/Views/moc_AssetBrowserTableView.cpp"
