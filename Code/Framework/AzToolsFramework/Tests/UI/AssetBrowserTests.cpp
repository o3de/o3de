/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <Prefab/PrefabTestFixture.h>
#include <QAbstractItemModelTester>

namespace UnitTest
{

    // Test fixture for the AssetBrowser model that uses a QAbstractItemModelTester to validate the state of the model
    // when QAbstractItemModel signals fire. Tests will exit with a fatal error if an invalid state is detected.
    class AssetBrowserTest : public ToolsApplicationFixture
    {
    protected:
        enum class AssetEntryType
        {
            Root,
            Folder,
            Source,
            Product
        };

        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

    protected:

        void CreateScanFolder(AZ::s64 folderID, AZStd::string folderPath, AZStd::string displayName, bool isRoot = false);
        AZ::Uuid CreateSourceEntry(AZ::s64 fileID, AZ::s64 parentFolderID, AZStd::string filename, bool isFolder = false);
        void SetupAssetBrowser();
        void PrintModel(const QAbstractItemModel* model);
        QModelIndex GetModelIndex(const QAbstractItemModel* model, int targetDepth, int row = 0);
        void CreateProduct(AZ::s64 productID, AZ::Uuid sourceUuid, AZStd::string productName);

        AZStd::shared_ptr<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry> GetRootEntry()
        {
            return m_assetBrowserComponent->GetAssetBrowserModel()->GetRootEntry();
        }

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::SearchWidget> m_searchWidget;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserComponent> m_assetBrowserComponent;

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserTableModel> m_tableModel;

        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTesterAssetBrowser;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTesterFilterModel;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTesterTableModel;
    };

    void AssetBrowserTest::SetUpEditorFixtureImpl()
    {
        GetApplication()->RegisterComponentDescriptor(AzToolsFramework::EditorEntityContextComponent::CreateDescriptor());

        m_assetBrowserComponent = AZStd::make_unique<AzToolsFramework::AssetBrowser::AssetBrowserComponent>();
        m_assetBrowserComponent->Activate();

        m_filterModel = AZStd::make_unique<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel>();
        m_tableModel = AZStd::make_unique<AzToolsFramework::AssetBrowser::AssetBrowserTableModel>();

        m_filterModel->setSourceModel(m_assetBrowserComponent->GetAssetBrowserModel());
        m_tableModel->setSourceModel(m_filterModel.get());

        m_modelTesterAssetBrowser = AZStd::make_unique<QAbstractItemModelTester>(m_assetBrowserComponent->GetAssetBrowserModel());
        m_modelTesterFilterModel = AZStd::make_unique<QAbstractItemModelTester>(m_filterModel.get());
        m_modelTesterTableModel = AZStd::make_unique<QAbstractItemModelTester>(m_tableModel.get());
        m_searchWidget = AZStd::make_unique<AzToolsFramework::AssetBrowser::SearchWidget>();

        // Setup String filters
        m_searchWidget->Setup(true, true);
        m_filterModel->SetFilter(m_searchWidget->GetFilter());

        SetupAssetBrowser();
    }

    void AssetBrowserTest::TearDownEditorFixtureImpl()
    {
        m_modelTesterAssetBrowser.reset();
        m_modelTesterFilterModel.reset();
        m_modelTesterTableModel.reset();

        m_assetBrowserComponent->GetAssetBrowserModel()->GetRootEntry().reset();

        m_tableModel.reset();
        m_filterModel.reset();
        delete m_assetBrowserComponent->GetAssetBrowserModel();

        m_assetBrowserComponent->Deactivate();
        m_assetBrowserComponent.reset();
        m_searchWidget.reset();
    }

    void AssetBrowserTest::CreateScanFolder(
        AZ::s64 folderID, AZStd::string folderPath, AZStd::string displayName, bool isRoot /*= false*/)
    {
        AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolder = AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry();
        scanFolder.m_scanFolderID = folderID;
        scanFolder.m_scanFolder = folderPath;
        scanFolder.m_displayName = displayName;
        scanFolder.m_isRoot = isRoot;
        GetRootEntry()->AddScanFolder(scanFolder);
    }

    AZ::Uuid AssetBrowserTest::CreateSourceEntry(AZ::s64 fileID, AZ::s64 parentFolderID, AZStd::string filename, bool isFolder /* = false */)
    {
        AzToolsFramework::AssetDatabase::FileDatabaseEntry entry = AzToolsFramework::AssetDatabase::FileDatabaseEntry();
        entry.m_scanFolderPK = parentFolderID;
        entry.m_fileID = fileID;
        entry.m_fileName = filename;
        entry.m_isFolder = isFolder;
        GetRootEntry()->AddFile(entry);

        if (!isFolder)
        {
            AzToolsFramework::AssetBrowser::SourceWithFileID entrySource = AzToolsFramework::AssetBrowser::SourceWithFileID();
            entrySource.first = entry.m_fileID;
            entrySource.second = AzToolsFramework::AssetDatabase::SourceDatabaseEntry();
            entrySource.second.m_scanFolderPK = parentFolderID;
            entrySource.second.m_sourceName = filename;
            entrySource.second.m_sourceID = fileID;
            entrySource.second.m_sourceGuid = AZ::Uuid::CreateRandom();

            GetRootEntry()->AddSource(entrySource);

            return entrySource.second.m_sourceGuid;
        }

        return AZ::Uuid::CreateNull();
    }

    void AssetBrowserTest::CreateProduct(AZ::s64 productID, AZ::Uuid sourceUuid, AZStd::string productName)
    {
        AzToolsFramework::AssetBrowser::ProductWithUuid product = AzToolsFramework::AssetBrowser::ProductWithUuid();
        product.first = sourceUuid;
        product.second = AzToolsFramework::AssetDatabase::ProductDatabaseEntry();
        product.second.m_productID = productID;
        product.second.m_subID = productID;
        product.second.m_productName = productName;

        GetRootEntry()->AddProduct(product);
    }

    void AssetBrowserTest::SetupAssetBrowser()
    {
        /*
         * RootEntries : 1
         * Folders : 4
         * SourceEntries : 5
         * ProductEntries : 9
         *
         *
         *RootEntry
         *|
         *|-D:/dev/o3de/GameProject
         *| |
         *| |-Assets (folder)
         *| | |
         *| | |-Source_0
         *| | | |
         *| | | |-product_0_0
         *| | | |-product_0_1
         *| | | |-product_0_2
         *| | | |-product_0_3
         *| | |
         *| | |-Source_1
         *| | | |
         *| | | |-product_1_0
         *| | | |-product_1_1
         *| |
         *| |
         *| |-Scripts (Folder)
         *| | | |
         *| | | |-Source_2
         *| | | | |
         *| | | | |-Product_2_0
         *| | | | |-Product_2_1
         *| | | | |-Product_2_2
         *| | | |
         *| | | |-Source_3
         *| | | |
         *| |
         *| |-Misc (folder)
         *| | | |-Source_4
         *| | | | |
         *| | | | |-Product_4_0
         *| | | | |-Product_4_1
         *| | | | |-Product_4_2
         */

        namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

        CreateScanFolder(15, "D:/dev/o3de/GameProject/Misc", "Misc");
        AZ::Uuid sourceUuid_4 = CreateSourceEntry(5, 15, "Source_4");
        CreateProduct(1, sourceUuid_4, "Product_4_0");
        CreateProduct(2, sourceUuid_4, "Product_4_1");
        CreateProduct(3, sourceUuid_4, "Product_4_2");

        CreateScanFolder(14, "D:/dev/o3de/GameProject/Scripts", "Scripts");

        AZ::Uuid sourceUuid_2 = CreateSourceEntry(3, 14, "Source_2");
        CreateProduct(1, sourceUuid_2, "Product_2_0");
        CreateProduct(2, sourceUuid_2, "Product_2_1");
        CreateProduct(3, sourceUuid_2, "Product_2_2");

        CreateSourceEntry(4, 14, "Source_3");

        CreateScanFolder(13, "D:/dev/o3de/GameProject/Assets", "Assets");

        AZ::Uuid sourceUuid_0 = CreateSourceEntry(1, 13, "Source_0");
        CreateProduct(1, sourceUuid_0, "Product_0_0");
        CreateProduct(2, sourceUuid_0, "Product_0_1");
        CreateProduct(3, sourceUuid_0, "Product_0_2");
        CreateProduct(4, sourceUuid_0, "Product_0_3");

        AZ::Uuid sourceUuid_1 = CreateSourceEntry(2, 13, "Source_1");
        CreateProduct(1, sourceUuid_1, "Product_1_0");
        CreateProduct(2, sourceUuid_1, "Product_1_1");

        qDebug() << "\n-------------Asset Browser Model------------\n";
        PrintModel(m_assetBrowserComponent->GetAssetBrowserModel());

        qDebug() << "\n---------Asset Browser Filter Model---------\n";
        PrintModel(m_filterModel.get());

        qDebug() << "\n-------------Asset Table Model--------------\n";
        PrintModel(m_tableModel.get());
    }

    void AssetBrowserTest::PrintModel(const QAbstractItemModel* model)
    {
        AZStd::deque<AZStd::pair<QModelIndex, int>> indices;
        indices.push_back({ model->index(0, 0), 0 });
        while (!indices.empty())
        {
            auto [index, depth] = indices.front();
            indices.pop_front();

            QString indentString;
            for (int i = 0; i < depth; ++i)
            {
                indentString += "  ";
            }
            qDebug() << (indentString + index.data(Qt::DisplayRole).toString()) << index.internalId();
            for (int i = 0; i < model->rowCount(index); ++i)
            {
                indices.emplace_front(model->index(i, 0, index), depth + 1);
            }
        }
    }

    QModelIndex AssetBrowserTest::GetModelIndex(const QAbstractItemModel* model, int targetDepth, int row)
    {
        AZStd::deque<AZStd::pair<QModelIndex, int>> indices;
        indices.push_back({ model->index(0, 0), 0 });
        while (!indices.empty())
        {
            auto [index, depth] = indices.front();
            indices.pop_front();

            for (int i = 0; i < model->rowCount(index); ++i)
            {
                if (depth + 1 == targetDepth && row == i)
                {
                    return model->index(i, 0, index);
                }
                indices.emplace_front(model->index(i, 0, index), depth + 1);
            }
        }
        return QModelIndex();
    }


    TEST_F(AssetBrowserTest, CheckScanFolderAddition)
    {
        
        EXPECT_EQ(m_assetBrowserComponent->GetAssetBrowserModel()->rowCount(), 1);

        CreateScanFolder(20, "E:/TestFolder/TestFolder2", "TestFolder");

        //Since the folder is empty it shouldn't be added to the model.
        EXPECT_EQ(m_assetBrowserComponent->GetAssetBrowserModel()->rowCount(), 1);

        AZ::Uuid sourceUuid = CreateSourceEntry(123, 20, "DummyFile");

        //When we add a file to the folder it should be added to the model
        EXPECT_EQ(m_assetBrowserComponent->GetAssetBrowserModel()->rowCount(), 2);
    }

    TEST_F(AssetBrowserTest, CheckSourceAddition)
    {
        //Get the asset Folder which should have 2 child entries
        QModelIndex AssetFolderIndex = GetModelIndex(m_assetBrowserComponent->GetAssetBrowserModel(), 5, 2);

        EXPECT_EQ(m_assetBrowserComponent->GetAssetBrowserModel()->rowCount(AssetFolderIndex), 2);

        AZ::Uuid addedEntryUuid_1 = CreateSourceEntry(123, 13, "DummyFle_1");
        AZ::Uuid addedEntryUuid_2 = CreateSourceEntry(124, 13, "DummyFle_2");
        AZ::Uuid addedEntryUuid_3 = CreateSourceEntry(125, 13, "DummyFle_3");
        AZ::Uuid addedEntryUuid_4 = CreateSourceEntry(126, 13, "DummyFle_4");
        AZ::Uuid addedEntryUuid_5 = CreateSourceEntry(127, 13, "DummyFle_5");

        //After Adding 5 entries it should have 7 child entries
        EXPECT_EQ(m_assetBrowserComponent->GetAssetBrowserModel()->rowCount(AssetFolderIndex),7);
    }

    TEST_F(AssetBrowserTest, CheckCorrectNumberOfEntriesInTableView)
    {
        m_filterModel->FilterUpdatedSlotImmediate();
        int tableViewRowcount = m_tableModel->rowCount();

        // RowCount should be 17 -> 5 SourceEntries + 12 ProductEntries)
        EXPECT_EQ(tableViewRowcount, 17);
    }

    TEST_F(AssetBrowserTest, CheckCorrectNumberOfEntriesInTableViewAfterStringFilter)
    {
        /*
         *-Source_1
         * |
         * |-product_1_0
         * |-product_1_1
         *
         *
         * Matching entries = 3
         */

        // Apply string filter
        m_searchWidget->SetTextFilter(QString("source_1"));
        m_filterModel->FilterUpdatedSlotImmediate();

        int tableViewRowcount = m_tableModel->rowCount();
        EXPECT_EQ(tableViewRowcount, 3);
    }

} // namespace UnitTest
