#pragma once
#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableModel.h>

#include <QModelIndex>
#include <QPointer>
#include <QTableView>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class AssetBrowserTableModel;
        class AssetBrowserFilterModel;
        class EntryDelegate;

        class AssetBrowserTableView
            : public QTableView
            , public AssetBrowserViewRequestBus::Handler
            , public AssetBrowserComponentNotificationBus::Handler
        {
            Q_OBJECT
        public:
            explicit AssetBrowserTableView(QWidget* parent = nullptr);
            ~AssetBrowserTableView() override;

            void setModel(QAbstractItemModel *model) override;
            void SetName(const QString& name);

            AZStd::vector<AssetBrowserEntry*> GetSelectedAssets() const;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserViewRequestBus
            virtual void SelectProduct(AZ::Data::AssetId assetID) override;
            virtual void SelectFileAtPath(const AZStd::string& assetPath) override;
            virtual void ClearFilter() override;
            virtual void Update() override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentNotificationBus
            void OnAssetBrowserComponentReady() override;
            //////////////////////////////////////////////////////////////////////////

        private Q_SLOTS:
            void OnContextMenu(const QPoint& point);

            //! Get all visible source entries and place them in a queue to update their source control status
            //void OnUpdateSCThumbnailsList();

        private:
            QString m_name;
            QPointer<AssetBrowserFilterModel> m_sourceFilterModel = nullptr;
            QPointer<AssetBrowserTableModel> m_sourceModel = nullptr;
            EntryDelegate* m_delegate = nullptr;

        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
