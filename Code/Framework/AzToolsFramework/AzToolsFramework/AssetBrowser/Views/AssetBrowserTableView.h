#pragma once
#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
//#include <AzToolsFramework/AssetBrowser/AssetBrowserTableFilterModel.h>

#include <QModelIndex>
#include <QPointer>
#include <QTableView>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class AssetBrowserModel;
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
            //void OnAssetBrowserComponentReady() override;
            //////////////////////////////////////////////////////////////////////////
        private:
            QString m_name;


        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
