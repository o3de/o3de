/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <QAbstractItemModelTester>

namespace UnitTest
{
    // Test fixture for the AssetBrowser model that uses a QAbstractItemModelTester to validate the state of the model
    // when QAbstractItemModel signals fire. Tests will exit with a fatal error if an invalid state is detected.
    class AssetBrowserTest
        : public ToolsApplicationFixture
        , public testing::WithParamInterface<const char*>
    {
    protected:
        enum class AssetEntryType
        {
            Root,
            Folder,
            Source,
            Product
        };

        enum class FolderType
        {
            Root,
            File
        };

        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        //! Creates a Mock Scan Folder
        void AddScanFolder(AZ::s64 folderID, AZStd::string folderPath, AZStd::string displayName, FolderType folderType = FolderType::File);

        //! Creates a Source entry from a mock file
        AZ::Uuid CreateSourceEntry(
            AZ::s64 fileID, AZ::s64 parentFolderID, AZStd::string filename, AssetEntryType sourceType = AssetEntryType::Source);

        //! Creates a product from a given sourceEntry
        void CreateProduct(AZ::s64 productID, AZ::Uuid sourceUuid, AZStd::string productName);

        void SetupAssetBrowser();
        void PrintModel(const QAbstractItemModel* model, AZStd::function<void(const QString&)> printer);
        QModelIndex GetModelIndex(const QAbstractItemModel* model, int targetDepth, int row = 0);
        AZStd::shared_ptr<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry> GetRootEntry();
        AZStd::vector<QString> GetVectorFromFormattedString(const QString& formattedString);

    protected:
        QString m_assetBrowserHierarchy = QString();

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::SearchWidget> m_searchWidget;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserComponent> m_assetBrowserComponent;

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserTableModel> m_tableModel;

        QVector<int> m_folderIds = { 13, 14, 15 };
        QVector<int> m_sourceIDs = { 1, 2, 3, 4, 5 };
        QVector<int> m_productIDs = { 1, 2, 3, 4, 5 };
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

        m_searchWidget = AZStd::make_unique<AzToolsFramework::AssetBrowser::SearchWidget>();

        // Setup String filters
        m_searchWidget->Setup(true, true);
        m_filterModel->SetFilter(m_searchWidget->GetFilter());

        SetupAssetBrowser();
    }

    void AssetBrowserTest::TearDownEditorFixtureImpl()
    {
        m_tableModel.reset();
        m_filterModel.reset();
        m_assetBrowserComponent->Deactivate();

        m_assetBrowserComponent.reset();
        m_searchWidget.reset();
    }

    void AssetBrowserTest::AddScanFolder(
        AZ::s64 folderID, AZStd::string folderPath, AZStd::string displayName, FolderType folderType /*= FolderType::File*/)
    {
        AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolder = AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry();
        scanFolder.m_scanFolderID = folderID;
        scanFolder.m_scanFolder = folderPath;
        scanFolder.m_displayName = displayName;
        scanFolder.m_isRoot = folderType == FolderType::Root;
        GetRootEntry()->AddScanFolder(scanFolder);
    }

    AZ::Uuid AssetBrowserTest::CreateSourceEntry(
        AZ::s64 fileID, AZ::s64 parentFolderID, AZStd::string filename, AssetEntryType sourceType /*= AssetEntryType::Source*/)
    {
        AzToolsFramework::AssetDatabase::FileDatabaseEntry entry = AzToolsFramework::AssetDatabase::FileDatabaseEntry();
        entry.m_scanFolderPK = parentFolderID;
        entry.m_fileID = fileID;
        entry.m_fileName = filename;
        entry.m_isFolder = sourceType == AssetEntryType::Folder;
        GetRootEntry()->AddFile(entry);

        if (!entry.m_isFolder)
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

        product.second.m_subID = aznumeric_cast<AZ::u32>(productID);
        product.second.m_productName = productName;

        GetRootEntry()->AddProduct(product);
    }

    void AssetBrowserTest::SetupAssetBrowser()
    {
        // RootEntries : 1 | Folders : 4 | SourceEntries : 5 | ProductEntries : 9
        m_assetBrowserHierarchy = R"(
        D:
          \
            dev
              o3de
                GameProject
                  Assets
                    Source_1
                      Product_1_1
                      Product_1_0
                    Source_0
                      Product_0_3
                      Product_0_2
                      Product_0_1
                      Product_0_0
                  Scripts
                    Source_3
                    Source_2
                      Product_2_2
                      Product_2_1
                      Product_2_0
                  Misc
                    Source_4
                      Product_4_2
                      Product_4_1
                      Product_4_0 )";

        namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

        AddScanFolder(m_folderIds.at(2), "D:/dev/o3de/GameProject/Misc", "Misc");
        AZ::Uuid sourceUuid_4 = CreateSourceEntry(m_sourceIDs.at(4), m_folderIds.at(2), "Source_4");
        CreateProduct(m_productIDs.at(0), sourceUuid_4, "Product_4_0");
        CreateProduct(m_productIDs.at(1), sourceUuid_4, "Product_4_1");
        CreateProduct(m_productIDs.at(2), sourceUuid_4, "Product_4_2");

        AddScanFolder(m_folderIds.at(1), "D:/dev/o3de/GameProject/Scripts", "Scripts");

        AZ::Uuid sourceUuid_2 = CreateSourceEntry(m_sourceIDs.at(2), m_folderIds.at(1), "Source_2");
        CreateProduct(m_productIDs.at(0), sourceUuid_2, "Product_2_0");
        CreateProduct(m_productIDs.at(1), sourceUuid_2, "Product_2_1");
        CreateProduct(m_productIDs.at(2), sourceUuid_2, "Product_2_2");

        CreateSourceEntry(m_sourceIDs.at(3), m_folderIds.at(1), "Source_3");

        AddScanFolder(m_folderIds.at(0), "D:/dev/o3de/GameProject/Assets", "Assets");

        AZ::Uuid sourceUuid_0 = CreateSourceEntry(m_sourceIDs.at(0), m_folderIds.at(0), "Source_0");
        CreateProduct(m_productIDs.at(0), sourceUuid_0, "Product_0_0");
        CreateProduct(m_productIDs.at(1), sourceUuid_0, "Product_0_1");
        CreateProduct(m_productIDs.at(2), sourceUuid_0, "Product_0_2");
        CreateProduct(m_productIDs.at(3), sourceUuid_0, "Product_0_3");

        AZ::Uuid sourceUuid_1 = CreateSourceEntry(m_sourceIDs.at(1), m_folderIds.at(0), "Source_1");
        CreateProduct(m_productIDs.at(0), sourceUuid_1, "Product_1_0");
        CreateProduct(m_productIDs.at(1), sourceUuid_1, "Product_1_1");
    }

    void AssetBrowserTest::PrintModel(const QAbstractItemModel* model, AZStd::function<void(const QString&)> printer)
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
            const QString message = indentString + index.data(Qt::DisplayRole).toString();
            printer(message);

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

    AZStd::shared_ptr<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry> AssetBrowserTest::GetRootEntry()
    {
        return m_assetBrowserComponent->GetAssetBrowserModel()->GetRootEntry();
    }

    AZStd::vector<QString> AssetBrowserTest::GetVectorFromFormattedString(const QString& formattedString)
    {
        AZStd::vector<QString> hierarchySections;
        QStringList splittedList = formattedString.split('\n', Qt::SkipEmptyParts);

        for (auto& str : splittedList)
        {
            str.replace(" ", "");
            hierarchySections.push_back(str);
        }
        return hierarchySections;
    }

    TEST_F(AssetBrowserTest, CheckCorrectNumberOfEntriesInTableView)
    {
        m_filterModel->FilterUpdatedSlotImmediate();
        const int tableViewRowcount = m_tableModel->rowCount();

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

        const int tableViewRowcount = m_tableModel->rowCount();
        EXPECT_EQ(tableViewRowcount, 3);
    }

    TEST_F(AssetBrowserTest, CheckScanFolderAddition)
    {
        EXPECT_EQ(m_assetBrowserComponent->GetAssetBrowserModel()->rowCount(), 1);
        const int newFolderId = 20;
        AddScanFolder(newFolderId, "E:/TestFolder/TestFolder2", "TestFolder");

        // Since the folder is empty it shouldn't be added to the model.
        EXPECT_EQ(m_assetBrowserComponent->GetAssetBrowserModel()->rowCount(), 1);

        CreateSourceEntry(123, newFolderId, "DummyFile");

        // When we add a file to the folder it should be added to the model
        EXPECT_EQ(m_assetBrowserComponent->GetAssetBrowserModel()->rowCount(), 2);
    }

} // namespace UnitTest
