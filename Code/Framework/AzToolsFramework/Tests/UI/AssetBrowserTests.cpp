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
#include <Prefab/PrefabTestFixture.h>
#include <QAbstractItemModelTester>

namespace UnitTest
{
    using uniqueAssetBrowserEntry = AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserEntry>;

    class PublicAssetBrowserEntry : public AzToolsFramework::AssetBrowser::AssetBrowserEntry
    {
    public:
        void AddChild(AssetBrowserEntry* child) override
        {
            AzToolsFramework::AssetBrowser::AssetBrowserEntry::AddChild(child);
        }

        void setDisplayName(QString name)
        {
            m_displayName = name;
        }
    };

    class PublicAssetBrowserTableMode : public AzToolsFramework::AssetBrowser::AssetBrowserTableModel
    {
    public:
        int rowCount(const QModelIndex& parent = QModelIndex()) const override
        {
            AzToolsFramework::AssetBrowser::AssetBrowserTableModel::rowCount(parent);
        }
    };

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

        void SetUpEditorFixtureImpl() override
        {
            ToolsApplicationFixture::SetUpEditorFixtureImpl();
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

            SetupAssetBrowser();
        }

        void TearDownEditorFixtureImpl() override
        {
            m_modelTesterAssetBrowser.reset();
            m_modelTesterFilterModel.reset();
            m_modelTesterTableModel.reset();
            m_tableModel.reset();
            m_filterModel.reset();
            m_assetBrowserComponent->Deactivate();
            m_assetBrowserComponent.reset();
            m_searchWidget.reset();

            ToolsApplicationFixture::TearDownEditorFixtureImpl();
        }

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserEntry> CreateAssetBrowserEntry(
            AssetEntryType entryType, AzToolsFramework::AssetBrowser::AssetBrowserEntry* parent = nullptr, QString name = QString())
        {
            AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserEntry> newEntry;
            switch (entryType)
            {
            case AssetEntryType::Root:
                newEntry = AZStd::make_unique<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry>();
                break;
            case AssetEntryType::Folder:
                {
                    newEntry = AZStd::make_unique<AzToolsFramework::AssetBrowser::FolderAssetBrowserEntry>();
                    if (parent)
                    {
                        reinterpret_cast<PublicAssetBrowserEntry*>(parent)->AddChild(newEntry.get());
                    }
                }
                break;
            case AssetEntryType::Source:
                {
                    newEntry = AZStd::make_unique<AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry>();
                    if (parent)
                    {
                        reinterpret_cast<PublicAssetBrowserEntry*>(parent)->AddChild(newEntry.get());
                    }
                }
                break;
            case AssetEntryType::Product:
                {
                    newEntry = AZStd::make_unique<AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry>();
                    if (parent)
                    {
                        reinterpret_cast<PublicAssetBrowserEntry*>(parent)->AddChild(newEntry.get());
                    }
                }
                break;
            }

            reinterpret_cast<PublicAssetBrowserEntry*>(newEntry.get())->setDisplayName(name);
            return newEntry;
        }

        void SetupAssetBrowser()
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
             *|-ProjectFolder
             *| |
             *| |-Folder_0
             *| | |
             *| | |-Source_0_0
             *| | | |
             *| | | |-product_0_0_0
             *| | | |-product_0_0_1
             *| | | |-product_0_0_2
             *| | | |-product_0_0_3
             *| | |-Source_0_1
             *| | | |
             *| | | |-product_0_1_0
             *| | | |-product_0_1_1
             *| |
             *| |
             *| |-Folder_1
             *| | | |
             *| | | |-Source_1_0
             *| | | | |
             *| | | | |-product_1_1_0
             *| | | | |-product_1_1_1
             *| | | | |-product_1_1_2
             *| | | |
             *| | | |-Source_1_1
             *| | | |
             *| | | |-Source_1_2
             *| |
             *| |-Folder_2
             */

            namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

            AzAssetBrowser::RootAssetBrowserEntry* rootEntry = m_assetBrowserComponent->GetAssetBrowserModel()->GetRootEntry().get();

            uniqueAssetBrowserEntry projectFolder = CreateAssetBrowserEntry(AssetEntryType::Folder, rootEntry, QString("projectFolder"));

            uniqueAssetBrowserEntry folder_0 = CreateAssetBrowserEntry(AssetEntryType::Folder, projectFolder.get(), QString("folder_0"));
            uniqueAssetBrowserEntry source_0_0 = CreateAssetBrowserEntry(AssetEntryType::Source, folder_0.get(), QString("source_0_0"));
            [[maybe_unused]] uniqueAssetBrowserEntry product_0_0_0 =
                CreateAssetBrowserEntry(AssetEntryType::Product, source_0_0.get(), QString("product_0_0_0"));
            [[maybe_unused]] uniqueAssetBrowserEntry product_0_0_1 =
                CreateAssetBrowserEntry(AssetEntryType::Product, source_0_0.get(), QString("product_0_0_1"));
            [[maybe_unused]] uniqueAssetBrowserEntry product_0_0_2 =
                CreateAssetBrowserEntry(AssetEntryType::Product, source_0_0.get(), QString("product_0_0_2"));
            [[maybe_unused]] uniqueAssetBrowserEntry product_0_0_3 =
                CreateAssetBrowserEntry(AssetEntryType::Product, source_0_0.get(), QString("product_0_0_3"));

            uniqueAssetBrowserEntry source_0_1 = CreateAssetBrowserEntry(AssetEntryType::Source, folder_0.get(), QString("source_0_1"));
            [[maybe_unused]] uniqueAssetBrowserEntry product_0_1_0 =
                CreateAssetBrowserEntry(AssetEntryType::Product, source_0_1.get(), QString("product_0_1_0"));
            [[maybe_unused]] uniqueAssetBrowserEntry product_0_1_1 =
                CreateAssetBrowserEntry(AssetEntryType::Product, source_0_1.get(), QString("product_0_1_1"));

            uniqueAssetBrowserEntry folder_1 = CreateAssetBrowserEntry(AssetEntryType::Folder, projectFolder.get(), QString("folder_1"));
            uniqueAssetBrowserEntry source_1_0 = CreateAssetBrowserEntry(AssetEntryType::Source, folder_1.get(), QString("source_1_0"));
            CreateAssetBrowserEntry(AssetEntryType::Product, source_1_0.get(), QString("product_1_0_0"));
            CreateAssetBrowserEntry(AssetEntryType::Product, source_1_0.get(), QString("product_1_0_1"));
            CreateAssetBrowserEntry(AssetEntryType::Product, source_1_0.get(), QString("product_1_0_2"));
            [[maybe_unused]] uniqueAssetBrowserEntry source_1_1 =
                CreateAssetBrowserEntry(AssetEntryType::Source, folder_1.get(), QString("source_1_1"));
            [[maybe_unused]] uniqueAssetBrowserEntry source_1_2 =
                CreateAssetBrowserEntry(AssetEntryType::Source, folder_1.get(), QString("source_1_2"));

            [[maybe_unused]] uniqueAssetBrowserEntry folder_2 =
                CreateAssetBrowserEntry(AssetEntryType::Folder, projectFolder.get(), QString("folder_2"));

            // Setup String filters
            m_searchWidget->Setup(true, true);
            m_filterModel->SetFilter(m_searchWidget->GetFilter());

            m_tableModel->UpdateTableModelMaps();

            qDebug() << "\n-------------Asset Browser Model------------\n";
            PrintModel(m_assetBrowserComponent->GetAssetBrowserModel());

            qDebug() << "\n---------Asset Browser Filter Model---------\n";
            PrintModel(m_filterModel.get());

            qDebug() << "\n-------------Asset Table Model--------------\n";
            PrintModel(m_tableModel.get());
        }

        void PrintModel(const QAbstractItemModel* model)
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

        // Gets the index of the root prefab, i.e. the "New Level" container entity
        QModelIndex GetRootIndex() const
        {
            return m_assetBrowserComponent->GetAssetBrowserModel()->index(0, 0);
        }
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::SearchWidget> m_searchWidget;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserComponent> m_assetBrowserComponent;

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserTableModel> m_tableModel;

        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTesterAssetBrowser;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTesterFilterModel;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTesterTableModel;
    };

    TEST_F(AssetBrowserTest, TestCheckCorrectNumberOfEntriesInTableView)
    {
        int tableViewRowcount = reinterpret_cast<PublicAssetBrowserTableMode*>(m_tableModel.get())->rowCount();

        // RowCount should be 14 -> 5 SourceEntries + 9 ProductEntries)
        EXPECT_EQ(tableViewRowcount, 14);
    }

    TEST_F(AssetBrowserTest, TestCheckCorrectNumberOfEntriesInTableViewAfterStringFilter)
    {
        // Apply string filter
        m_searchWidget->SetTextFilter(QString("source_1_"));
        m_filterModel->FilterUpdatedSlotImmediate();
        m_tableModel->UpdateTableModelMaps();

        /*
        |-Source_1_0
        | |
        | |-product_1_1_0
        | |-product_1_1_1
        | |-product_1_1_2
        |
        |-Source_1_1
        |
        |-Source_1_2

        Total matching entries 6.
        */

        int tableViewRowcount = reinterpret_cast<PublicAssetBrowserTableMode*>(m_tableModel.get())->rowCount();
        EXPECT_EQ(tableViewRowcount, 6);
    }
} // namespace UnitTest
