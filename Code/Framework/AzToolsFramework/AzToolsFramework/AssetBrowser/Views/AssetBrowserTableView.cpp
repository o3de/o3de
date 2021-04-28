#include <API/EditorAssetSystemAPI.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTableView.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

AZ_PUSH_DISABLE_WARNING(
    4244 4251 4800, "-Wunknown-warning-option") // conversion from 'int' to 'float', possible loss of data, needs to have dll-interface to
                                                // be used by clients of class 'QFlags<QPainter::RenderHint>::Int': forcing value to bool
                                                // 'true' or 'false' (performance warning)
#include <QCoreApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QTimer>
AZ_POP_DISABLE_WARNING
#pragma optimize("", off)
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTableView::AssetBrowserTableView(QWidget* parent)
            : QTableView(parent)
            , m_delegate(new EntryDelegate(this))

        {
            setSortingEnabled(true);
            setItemDelegate(m_delegate);
           // header()->hide();
            setContextMenuPolicy(Qt::CustomContextMenu);

            setMouseTracking(true);

            connect(this, &QTableView::customContextMenuRequested, this, &AssetBrowserTableView::OnContextMenu);
            //connect(m_scTimer, &QTimer::timeout, this, &AssetBrowserTreeView::OnUpdateSCThumbnailsList);

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
            m_sourceModel = qobject_cast<AssetBrowserTableModel*>(model);
            AZ_Assert(m_sourceModel, "Expecting AssetBrowserTableModel");
            m_sourceFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_sourceModel->sourceModel());
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
            update();
        }

        void AssetBrowserTableView::OnAssetBrowserComponentReady()
        {
        }

        void AssetBrowserTableView::OnContextMenu(const QPoint& point)
        {
            AZ_UNUSED(point);

            auto selectedAssets = GetSelectedAssets();
            if (selectedAssets.size() != 1)
            {
                return;
            }

            QMenu menu(this);
            AssetBrowserInteractionNotificationBus::Broadcast(&AssetBrowserInteractionNotificationBus::Events::AddContextMenuActions, this, &menu, selectedAssets);
            if (!menu.isEmpty())
            {
                menu.exec(QCursor::pos());
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#pragma optimize("", on)
#include "AssetBrowser/Views/moc_AssetBrowserTableView.cpp"
