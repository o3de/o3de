/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserListModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesManager.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <QAbstractItemModelTester>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>

namespace UnitTest
{
    // Test fixture for the AssetBrowser model that uses a QAbstractItemModelTester to validate the state of the model
    // when QAbstractItemModel signals fire. Tests will exit with a fatal error if an invalid state is detected.
    class AssetBrowserTest
        : public ToolsApplicationFixture<>
        , public testing::WithParamInterface<const char*>
    {
    protected:
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
            AZ::s64 fileID,
            AZ::s64 parentFolderID,
            AZStd::string filename,
            AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType sourceType =
                AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source);

        //! Creates a product from a given sourceEntry
        void CreateProduct(AZ::s64 productID, AZ::Uuid sourceUuid, AZStd::string productName);

        void SetupAssetBrowser();
        void PrintModel(const QAbstractItemModel* model, AZStd::function<void(const QString&)> printer);
        QModelIndex GetModelIndex(const QAbstractItemModel* model, int targetDepth, int row = 0);
        AZStd::vector<QString> GetVectorFromFormattedString(const QString& formattedString);

    protected:
        QString m_assetBrowserHierarchy = QString();

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::SearchWidget> m_searchWidget;

        AZStd::shared_ptr<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry> m_rootEntry;

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserModel> m_assetBrowserModel;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserListModel> m_tableModel;

        AZStd::unique_ptr<::testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
        AZ::IO::FileIOBase* m_prevFileIO = nullptr;

        QVector<int> m_folderIds = { 13, 14, 15, 16};
        QVector<int> m_sourceIDs = { 1, 2, 3, 4, 5};
        QVector<int> m_productIDs = { 1, 2, 3, 4, 5 };
        QVector<AZ::Uuid> m_sourceUUIDs;
    };

    void AssetBrowserTest::SetUpEditorFixtureImpl()
    {
        m_fileIOMock = AZStd::make_unique<testing::NiceMock<AZ::IO::MockFileIOBase>>();
        m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_fileIOMock.get());

        m_assetBrowserModel = AZStd::make_unique<AzToolsFramework::AssetBrowser::AssetBrowserModel>();
        m_filterModel = AZStd::make_unique<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel>();
        m_tableModel = AZStd::make_unique<AzToolsFramework::AssetBrowser::AssetBrowserListModel>();

        m_rootEntry = AZStd::make_shared<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry>();
        m_assetBrowserModel->SetRootEntry(m_rootEntry);

        m_assetBrowserModel->SetFilterModel(m_filterModel.get());
        m_filterModel->setSourceModel(m_assetBrowserModel.get());
        m_tableModel->setSourceModel(m_filterModel.get());

        m_searchWidget = AZStd::make_unique<AzToolsFramework::AssetBrowser::SearchWidget>();

        // Setup String filters
        m_searchWidget->Setup(true, true);
        m_filterModel->SetFilter(m_searchWidget->GetFilter());

        SetupAssetBrowser();
    }

    void AssetBrowserTest::TearDownEditorFixtureImpl()
    {
        AzToolsFramework::AssetBrowser::EntryCache::DestroyInstance();
        AzToolsFramework::AssetBrowser::AssetBrowserFavoritesManager::DestroyInstance();
        EXPECT_EQ(m_fileIOMock.get(), AZ::IO::FileIOBase::GetInstance());
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        m_fileIOMock.reset();
    }

    void AssetBrowserTest::AddScanFolder(
        AZ::s64 folderID, AZStd::string folderPath, AZStd::string displayName, FolderType folderType /*= FolderType::File*/)
    {
        AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolder = AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry();
        scanFolder.m_scanFolderID = folderID;
        scanFolder.m_scanFolder = folderPath;
        scanFolder.m_displayName = displayName;
        scanFolder.m_isRoot = folderType == FolderType::Root;
        m_rootEntry->AddScanFolder(scanFolder);
    }

    AZ::Uuid AssetBrowserTest::CreateSourceEntry(
        AZ::s64 fileID,
        AZ::s64 parentFolderID,
        AZStd::string filename,
        AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType sourceType /*= AssetEntryType::Source*/)
    {
        AzToolsFramework::AssetDatabase::FileDatabaseEntry entry = AzToolsFramework::AssetDatabase::FileDatabaseEntry();
        entry.m_scanFolderPK = parentFolderID;
        entry.m_fileID = fileID;
        entry.m_fileName = filename;
        entry.m_isFolder = sourceType == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Folder;
        m_rootEntry->AddFile(entry);

        if (!entry.m_isFolder)
        {
            AzToolsFramework::AssetBrowser::SourceWithFileID entrySource = AzToolsFramework::AssetBrowser::SourceWithFileID();
            entrySource.first = entry.m_fileID;
            entrySource.second = AzToolsFramework::AssetDatabase::SourceDatabaseEntry();
            entrySource.second.m_scanFolderPK = parentFolderID;
            entrySource.second.m_sourceName = filename;
            entrySource.second.m_sourceID = fileID;
            entrySource.second.m_sourceGuid = AZ::Uuid::CreateRandom();

            m_rootEntry->AddSource(entrySource);

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

        // note:  ProductName in terms of database entries is the relative path to the product
        // example: pc/shaders/diffuseglobalillumination/diffusecomposite-nomsaa_vulkan.srg.json
        // since it comes from the database.  However, the actual path to the product in reality is
        // the cache folder with this appended to it.
        product.second.m_productName = productName;

        m_rootEntry->AddProduct(product);
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
                  Assets                  <--- scan folder "Assets"
                    Source_1
                      Product_1_1
                      Product_1_0
                    Source_0
                      Product_0_3
                      Product_0_2
                      Product_0_1
                      Product_0_0
                  Scripts                  <--- scan folder "Scripts"
                    Source_3
                    Source_2
                      Product_2_2
                      Product_2_1
                      Product_2_0
                  Misc                     <--- scan folder "Misc"
                    SubFolder              <--- not a scan folder!
                        Source_4
                          Product_4_2
                          Product_4_1
                          Product_4_0 )";

        namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;
        static constexpr int s_numScanFolders = 3;
        static constexpr char s_scanFolders[s_numScanFolders][AZ_MAX_PATH_LEN] =
        {
            "D:/dev/o3de/GameProject/Misc",
            "D:/dev/o3de/GameProject/Scripts",
            "D:/dev/o3de/GameProject/Assets"
        };

        ON_CALL(*m_fileIOMock.get(), IsDirectory(::testing::_))
            .WillByDefault([&](const char* folderName)
            {
                AZ::IO::PathView folderNamePath(folderName);
            // forward slashes by default - compare any part of any of the above scan folders
                for (const char* scanFolder : s_scanFolders)
                {
                    AZ::IO::PathView scanFolderPath(scanFolder);
                    if (scanFolderPath == folderNamePath)
                    {
                        return true;
                    }
                }

                return false;
            }
        );

        ON_CALL(*m_fileIOMock, Open(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(
                [&](auto filePath, auto, AZ::IO::HandleType& handle)
                {
                    handle = AZ::u32(AZStd::hash<AZStd::string>{}(filePath));
                    return AZ::IO::Result(AZ::IO::ResultCode::Success);
                });

        ON_CALL(*m_fileIOMock, Close(::testing::_))
            .WillByDefault(
                [&](AZ::IO::HandleType /* handle*/)
                {
                    return AZ::IO::Result(AZ::IO::ResultCode::Success);
                });

        ON_CALL(*m_fileIOMock, Size(testing::An<AZ::IO::HandleType>(), ::testing::_))
            .WillByDefault(
                [&](AZ::IO::HandleType /* handle*/, auto& size)
                {
                    size = 0;
                    return AZ::IO::Result(AZ::IO::ResultCode::Success);
                });

        using namespace testing;

        // a function that "resolves" a path by just copying it from the input to the output
        auto resolveToCopyChars = [](const char* unresolvedPath, char* resolvedPath, AZ::u64 pathLength) -> bool
        {
            if ((!resolvedPath) || (!unresolvedPath) || (pathLength < strlen(unresolvedPath) + 1))
            {
                return false;
            }
            azstrcpy(resolvedPath, pathLength, unresolvedPath);
            return true;
        };

        auto resolveToCopyPaths = [](AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) -> bool
        {
            resolvedPath = path;
            return true;
        };

        // a function that "resolves" a path by just 
        ON_CALL(*m_fileIOMock.get(), ResolvePath(_, _, _)).WillByDefault(resolveToCopyChars);
        ON_CALL(*m_fileIOMock.get(), ResolvePath(_, _)).WillByDefault(resolveToCopyPaths);
                
        AddScanFolder(m_folderIds.at(2), s_scanFolders[0], "Misc");
        CreateSourceEntry(m_folderIds.at(3), m_folderIds.at(2), "SubFolder", AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Folder);
        AZ::Uuid sourceUuid_4 = CreateSourceEntry(m_sourceIDs.at(4), m_folderIds.at(2), "SubFolder/Source_4");

        // note that for maximum realism here, products are emitted as they are in actual AB - lowercase, and in the same
        // relative path as the source.
        // also of note, the database is for several different platforms (e.g. you can run Asset Processor
        // for PC and "android" platforms, and it will have a 'pc' and 'android' subfolder in the cache.  This
        // means that the database of products includes this 'pc' subfolder to disambiguate between the products
        // for the android vs pc platforms.  So the first path element of a "real" database entry is always the platform

        CreateProduct(m_productIDs.at(0), sourceUuid_4, "pc/subfolder/product_4_0");
        CreateProduct(m_productIDs.at(1), sourceUuid_4, "pc/subfolder/product_4_1");
        CreateProduct(m_productIDs.at(2), sourceUuid_4, "pc/subfolder/product_4_2");

        AddScanFolder(m_folderIds.at(1), s_scanFolders[1], "Scripts");
        AZ::Uuid sourceUuid_3 = CreateSourceEntry(m_sourceIDs.at(3), m_folderIds.at(1), "Source_3");

        AZ::Uuid sourceUuid_2 = CreateSourceEntry(m_sourceIDs.at(2), m_folderIds.at(1), "Source_2");
        CreateProduct(m_productIDs.at(0), sourceUuid_2, "pc/product_2_0");
        CreateProduct(m_productIDs.at(1), sourceUuid_2, "pc/product_2_1");
        CreateProduct(m_productIDs.at(2), sourceUuid_2, "pc/product_2_2");

        AddScanFolder(m_folderIds.at(0), s_scanFolders[2], "Assets");

        AZ::Uuid sourceUuid_0 = CreateSourceEntry(m_sourceIDs.at(0), m_folderIds.at(0), "Source_0");
        CreateProduct(m_productIDs.at(0), sourceUuid_0, "pc/product_0_0");
        CreateProduct(m_productIDs.at(1), sourceUuid_0, "pc/product_0_1");
        CreateProduct(m_productIDs.at(2), sourceUuid_0, "pc/product_0_2");
        CreateProduct(m_productIDs.at(3), sourceUuid_0, "pc/product_0_3");

        AZ::Uuid sourceUuid_1 = CreateSourceEntry(m_sourceIDs.at(1), m_folderIds.at(0), "Source_1");
        CreateProduct(m_productIDs.at(0), sourceUuid_1, "pc/product_1_0");
        CreateProduct(m_productIDs.at(1), sourceUuid_1, "pc/product_1_1");

        m_sourceUUIDs = {
            sourceUuid_0, sourceUuid_1, sourceUuid_2, sourceUuid_3, sourceUuid_4
        };

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

    // This test just ensures that the data entered into the mock model returns the correct data for
    // each type of entry (root, folder, source, product) and that the various fields like "full path",
    // "display path", "display name", "name", and "relative path" are operating as expected, given that
    // reasonable data is fed to it.  
    TEST_F(AssetBrowserTest, ValidateBasicData_Sanity)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;
        // validates that the data sent to the asset browser makes sense in the first place.
        const RootAssetBrowserEntry* rootEntry = m_rootEntry.get();
        ASSERT_NE(nullptr, rootEntry);
        EXPECT_STREQ("", rootEntry->GetFullPath().c_str());
        ASSERT_EQ(1, rootEntry->GetChildCount());
        // the misc source is "D:/dev/o3de/GameProject/Misc/SubFolder/Source_4":
        const SourceAssetBrowserEntry* miscSource = SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs[4]);
        ASSERT_NE(nullptr, miscSource);

        AZ::IO::Path expectedPath("D:/dev/o3de/GameProject/Misc/SubFolder/Source_4");
        AZ::IO::Path actualPath(miscSource->GetFullPath().c_str());
        EXPECT_EQ(expectedPath.AsPosix(), actualPath.AsPosix());

        EXPECT_STREQ(miscSource->GetName().c_str(), "Source_4");
        EXPECT_STREQ(miscSource->GetDisplayName().toUtf8().constData(), "Source_4");

        // the display path does not include the file's name.
        EXPECT_STREQ(miscSource->GetDisplayPath().toUtf8().constData(), "SubFolder");

        // the "relative" path does include the file's name:
        expectedPath = AZ::IO::Path("SubFolder/Source_4");
        actualPath = AZ::IO::Path(miscSource->GetRelativePath().c_str());
        EXPECT_EQ(expectedPath.AsPosix(), actualPath.AsPosix());

        // the parent folder of this source should be the SubFolder.
        const FolderAssetBrowserEntry* subFolder = azrtti_cast<const FolderAssetBrowserEntry*>(miscSource->GetParent());
        ASSERT_NE(nullptr, subFolder);

        // note that the PARENT folder of the subfolder is a scan folder, and folder paths are relative to the scan)
        EXPECT_STREQ("SubFolder", subFolder->GetName().c_str());
        EXPECT_STREQ("SubFolder", subFolder->GetDisplayName().toUtf8().constData());
        EXPECT_STREQ("", subFolder->GetDisplayPath().toUtf8().constData());
        EXPECT_STREQ("SubFolder", subFolder->GetRelativePath().c_str());

        // the parent of this folder is a scan folder.  Scanfolder's relative and full paths are both full paths.
        const FolderAssetBrowserEntry* scanFolder = azrtti_cast<const FolderAssetBrowserEntry*>(subFolder->GetParent());
        ASSERT_NE(nullptr, scanFolder);
        ASSERT_TRUE(scanFolder->IsScanFolder());
        expectedPath = AZ::IO::Path("D:/dev/o3de/GameProject/Misc");
        actualPath = AZ::IO::Path(scanFolder->GetRelativePath().c_str());
        EXPECT_EQ(expectedPath.AsPosix(), actualPath.AsPosix());
        actualPath = AZ::IO::Path(scanFolder->GetFullPath().c_str());
        EXPECT_EQ(expectedPath.AsPosix(), actualPath.AsPosix());
        EXPECT_STREQ("Misc", scanFolder->GetDisplayName().toUtf8().constData());
        EXPECT_STREQ("Misc", scanFolder->GetName().c_str());

        // products should make sense too:
        ASSERT_EQ(3, miscSource->GetChildCount());
        const ProductAssetBrowserEntry* product_4_0 = azrtti_cast<const ProductAssetBrowserEntry*>(miscSource->GetChild(0));
        ASSERT_NE(nullptr, product_4_0);

        // product paths are relative to the actual cache...
        expectedPath = AZ::IO::Path("@products@/subfolder/product_4_0");
        actualPath = AZ::IO::Path(product_4_0->GetFullPath().c_str());
        EXPECT_STREQ(expectedPath.AsPosix().c_str(), actualPath.AsPosix().c_str());

        expectedPath = AZ::IO::Path("subfolder/product_4_0");
        actualPath = AZ::IO::Path(product_4_0->GetRelativePath().c_str());
        EXPECT_STREQ(expectedPath.AsPosix().c_str(), actualPath.AsPosix().c_str());

        EXPECT_STREQ("product_4_0", product_4_0->GetName().c_str());
        EXPECT_STREQ("product_4_0", product_4_0->GetDisplayName().toUtf8().constData());
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
        EXPECT_EQ(m_assetBrowserModel->rowCount(), 1);
        const int newFolderId = 20;
        AddScanFolder(newFolderId, "E:/TestFolder/TestFolder2", "TestFolder");

        // Since the folder is empty it shouldn't be added to the model.
        EXPECT_EQ(m_assetBrowserModel->rowCount(), 1);

        CreateSourceEntry(123, newFolderId, "DummyFile");

        // When we add a file to the folder it should be added to the model
        EXPECT_EQ(m_assetBrowserModel->rowCount(), 2);
    }

    // this test exercises the functions on nullptr to ensure it does not crash
    TEST_F(AssetBrowserTest, EnsureEncodingAndDecodingWorks_NullPointer)
    {
        namespace AB = AzToolsFramework::AssetBrowser;

        AZStd::vector<const AB::AssetBrowserEntry*> selection;
        AB::Utils::ToMimeData(nullptr, selection);

        AZStd::vector<const AB::AssetBrowserEntry*> decoded;
        EXPECT_FALSE(AB::Utils::FromMimeData(nullptr, decoded));
        EXPECT_TRUE(decoded.empty());
    }

    // this test exercises the functions on empty data to ensure its not going to crash.
    TEST_F(AssetBrowserTest, EnsureEncodingAndDecodingWorks_Empty)
    {
        namespace AB = AzToolsFramework::AssetBrowser;

        AZStd::vector<const AB::AssetBrowserEntry*> selection;
        QMimeData md;
        AB::Utils::ToMimeData(&md, selection);

        AZStd::vector<const AB::AssetBrowserEntry*> decoded;
        EXPECT_FALSE(AB::Utils::FromMimeData(&md, decoded));
        EXPECT_TRUE(decoded.empty());
    }

    TEST_F(AssetBrowserTest, EnsureEncodingAndDecodingWorks_BadData)
    {
        namespace AB = AzToolsFramework::AssetBrowser;
        // encode the selection in mimedata
        QMimeData md;
        md.setData(AB::AssetBrowserEntry::GetMimeType(), "21312638127631|28796321asdkjhakjhfasda:21321#:!@312#:!@\n\n12312312");
        // decode the selection
        AZStd::vector<const AB::AssetBrowserEntry*> decoded;
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(AB::Utils::FromMimeData(&md, decoded));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        // more plausable but still garbage data
        AZ_TEST_START_TRACE_SUPPRESSION;
        md.setData(AB::AssetBrowserEntry::GetMimeType(), "1|2|3|4\n|5|4");
        EXPECT_FALSE(AB::Utils::FromMimeData(&md, decoded));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        // valid data but non-existent assets. This should not trigger an error.
        md.setData(AB::AssetBrowserEntry::GetMimeType(), "Source|{D7C08FE3-D762-4E92-A530-8A42D828B81E}\n"
                                                         "Product|{D7C08FE3-D762-4E92-A530-8A42D828B81E}:1\n");
        EXPECT_FALSE(AB::Utils::FromMimeData(&md, decoded));

        // it should also not make a difference if there's extra data after a comment token
        md.setData(
            AB::AssetBrowserEntry::GetMimeType(),
            "Source|{D7C08FE3-D762-4E92-A530-8A42D828B81E}//comment goes here\n"
            "Product|{D7C08FE3-D762-4E92-A530-8A42D828B81E}:1// an example of a comment\n");
        EXPECT_FALSE(AB::Utils::FromMimeData(&md, decoded));
    }

    TEST_F(AssetBrowserTest, EnsureEncodingAndDecodingWorks_DuplicatesRemoved)
    {
        namespace AB = AzToolsFramework::AssetBrowser;
        // see the heirarchy that the fixture sets up to understand this...
        // we will select source1 and source3.

        AZStd::vector<const AB::AssetBrowserEntry*> selection;
        selection.push_back(AB::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs.at(3)));
        selection.push_back(AB::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs.at(3))); // <-- duplicate!
        selection.push_back(AB::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs.at(1)));

        // encode the selection in mimedata
        QMimeData md;
        AB::Utils::ToMimeData(&md, selection);
        EXPECT_TRUE(md.hasFormat("text/plain"));
        EXPECT_TRUE(md.hasFormat(AB::AssetBrowserEntry::GetMimeType()));

        // decode the selection
        AZStd::vector<const AB::AssetBrowserEntry*> decoded;
        EXPECT_TRUE(AB::Utils::FromMimeData(&md, decoded));
        ASSERT_EQ(decoded.size(), 2);
        EXPECT_EQ(decoded[0], selection[0]);
        // skip selection[1]
        EXPECT_EQ(decoded[1], selection[2]);
    }

    TEST_F(AssetBrowserTest, EnsureEncodingAndDecodingWorks_MissingData)
    {
        namespace AB = AzToolsFramework::AssetBrowser;
        // encode the selection in mimedata
        QMimeData md;
        AZStd::vector<const AB::AssetBrowserEntry*> decoded;
        EXPECT_FALSE(AB::Utils::FromMimeData(&md, decoded));
    }

    TEST_F(AssetBrowserTest, EnsureEncodingAndDecodingWorks_MultipleSources)
    {
        namespace AB = AzToolsFramework::AssetBrowser;
        // see the heirarchy that the fixture sets up to understand this...
        // we will select source1 and source3.

        AZStd::vector<const AB::AssetBrowserEntry*> selection;
        selection.push_back(AB::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs.at(3)));
        selection.push_back(AB::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs.at(1)));

        // encode the selection in mimedata
        QMimeData md;
        AB::Utils::ToMimeData(&md, selection);
        EXPECT_TRUE(md.hasFormat("text/plain"));
        EXPECT_TRUE(md.hasFormat(AB::AssetBrowserEntry::GetMimeType()));

        // decode the selection
        AZStd::vector<const AB::AssetBrowserEntry*> decoded;
        EXPECT_TRUE(AB::Utils::FromMimeData(&md, decoded));
        ASSERT_EQ(decoded.size(), 2);
        EXPECT_EQ(decoded[0], selection[0]);
        EXPECT_EQ(decoded[1], selection[1]);
    }

    TEST_F(AssetBrowserTest, EnsureEncodingAndDecodingWorks_MixedProductsAndSources)
    {
        namespace AB = AzToolsFramework::AssetBrowser;
        // see the heirarchy that the fixture sets up to understand this...
        // we will select source1 and source3.

        AZStd::vector<const AB::AssetBrowserEntry*> selection;
        selection.push_back(AB::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs.at(3)));
        selection.push_back(AB::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs.at(1)));

        AZ::Data::AssetId product11id = AZ::Data::AssetId(m_sourceUUIDs.at(1), m_productIDs.at(1));
        AZ::Data::AssetId product02id = AZ::Data::AssetId(m_sourceUUIDs.at(0), m_productIDs.at(2));

        selection.push_back(AB::ProductAssetBrowserEntry::GetProductByAssetId(product11id));
        selection.push_back(AB::ProductAssetBrowserEntry::GetProductByAssetId(product02id));

        // encode the selection in mimedata
        QMimeData md;
        AB::Utils::ToMimeData(&md, selection);
        EXPECT_TRUE(md.hasFormat("text/plain"));
        EXPECT_TRUE(md.hasFormat(AB::AssetBrowserEntry::GetMimeType()));

        // decode the selection
        AZStd::vector<const AB::AssetBrowserEntry*> decoded;
        EXPECT_TRUE(AB::Utils::FromMimeData(&md, decoded));
        ASSERT_EQ(decoded.size(), 4); // we don't pack nullptrs, so this also ensures all the products were found.
        EXPECT_EQ(decoded[0], selection[0]);
        EXPECT_EQ(decoded[1], selection[1]);
        EXPECT_EQ(decoded[2], selection[2]);
        EXPECT_EQ(decoded[3], selection[3]);
    }

    // its possible for the data in the model to change between being written and read.
    // this test removes an element after encoding and ensures no crash happens.
    TEST_F(AssetBrowserTest, EnsureEncodingAndDecodingWorks_RemovedSourceNoCrash)
    {
        namespace AB = AzToolsFramework::AssetBrowser;
        // see the heirarchy that the fixture sets up to understand this...
        // we will select source1 and source3.

        AZStd::vector<const AB::AssetBrowserEntry*> selection;
        selection.push_back(AB::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs.at(3)));
        selection.push_back(AB::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceUUIDs.at(1)));

        // encode the selection in mimedata
        QMimeData md;
        AB::Utils::ToMimeData(&md, selection);
        EXPECT_TRUE(md.hasFormat("text/plain"));
        EXPECT_TRUE(md.hasFormat(AB::AssetBrowserEntry::GetMimeType()));

        // remove the source!
        m_rootEntry->RemoveFile(m_sourceIDs.at(3));

        // decode the selection
        AZStd::vector<const AB::AssetBrowserEntry*> decoded;
        EXPECT_TRUE(AB::Utils::FromMimeData(&md, decoded));
        ASSERT_EQ(decoded.size(), 1); // we don't pack nullptrs, so this also ensures all the products were found.
        EXPECT_EQ(decoded[0], selection[1]);
    }

} // namespace UnitTest
