/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>


#include <Prefab/PrefabTestFixture.h>

#include <QAbstractItemModelTester>

namespace UnitTest
{

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
            ToolsApplicationFixture::TearDownEditorFixtureImpl();
        }

         AzToolsFramework::AssetBrowser::AssetBrowserEntry* CreateAssetBrowserEntry(
            AssetEntryType entryType, AzToolsFramework::AssetBrowser::AssetBrowserEntry* parent = nullptr, QString name = QString())
        {

             AzToolsFramework::AssetBrowser::AssetBrowserEntry* newEntry = nullptr;
             switch (entryType)
             {
             case AssetEntryType::Root:
                 newEntry = aznew AzToolsFramework::AssetBrowser::RootAssetBrowserEntry();
                break;
             case AssetEntryType::Folder:
                 {
                     newEntry = aznew AzToolsFramework::AssetBrowser::FolderAssetBrowserEntry();
                     if (parent)
                     {
                         reinterpret_cast<PublicAssetBrowserEntry*>(parent)->AddChild(newEntry);
                     }
                 }
                 break;
             case AssetEntryType::Source:
                 {
                     newEntry = aznew AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry();
                     if (parent)
                     {
                         reinterpret_cast<PublicAssetBrowserEntry*>(parent)->AddChild(newEntry);
                     }
                 }
                 break;
             case AssetEntryType::Product:
                 {
                     newEntry = aznew AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry();
                     if (parent)
                     {
                         reinterpret_cast<PublicAssetBrowserEntry*>(parent)->AddChild(newEntry);
                     }
                 }
                 break;
             }

              reinterpret_cast<PublicAssetBrowserEntry*>(newEntry)->setDisplayName(name);

             return newEntry;
        }

        void SetupAssetBrowser()
        {
            namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;
            AzAssetBrowser::RootAssetBrowserEntry* rootEntry = m_assetBrowserComponent->GetAssetBrowserModel()->GetRootEntry().get();

            AzAssetBrowser::AssetBrowserEntry* folder_0 = CreateAssetBrowserEntry(AssetEntryType::Folder, rootEntry, QString("folder_0"));
                AzAssetBrowser::AssetBrowserEntry* source_0_0 = CreateAssetBrowserEntry(AssetEntryType::Source, folder_0, QString("source_0_0"));
                   [[maybe_unused]] AzAssetBrowser::AssetBrowserEntry* product_0_0_0 = CreateAssetBrowserEntry(AssetEntryType::Source, source_0_0, QString("product_0_0_0"));
                   [[maybe_unused]] AzAssetBrowser::AssetBrowserEntry* product_0_0_1 = CreateAssetBrowserEntry(AssetEntryType::Source, source_0_0, QString("product_0_0_1"));
                   [[maybe_unused]] AzAssetBrowser::AssetBrowserEntry* product_0_0_2 = CreateAssetBrowserEntry(AssetEntryType::Source, source_0_0, QString("product_0_0_2"));
                   [[maybe_unused]] AzAssetBrowser::AssetBrowserEntry* product_0_0_3 = CreateAssetBrowserEntry(AssetEntryType::Source, source_0_0, QString("product_0_0_3"));

                AzAssetBrowser::AssetBrowserEntry* source_0_1 = CreateAssetBrowserEntry(AssetEntryType::Source, folder_0, QString("source_0_1"));
                   [[maybe_unused]] AzAssetBrowser::AssetBrowserEntry* product_0_1_0 = CreateAssetBrowserEntry(AssetEntryType::Source, source_0_1, QString("product_0_1_0"));
                   [[maybe_unused]] AzAssetBrowser::AssetBrowserEntry* product_0_1_1 = CreateAssetBrowserEntry(AssetEntryType::Source, source_0_1,  QString("product_0_1_1"));


            [[maybe_unused]] AzAssetBrowser::AssetBrowserEntry* folder_1 = CreateAssetBrowserEntry(AssetEntryType::Folder, rootEntry, QString("Folder_1"));
            [[maybe_unused]] AzAssetBrowser::AssetBrowserEntry* folder_2 = CreateAssetBrowserEntry(AssetEntryType::Folder, rootEntry, QString("Folder_2"));
            m_tableModel->UpdateTableModelMaps();
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
                    indices.emplace_back(model->index(i, 0, index), depth + 1);
                }
            }

        }

        // Gets the index of the root prefab, i.e. the "New Level" container entity
        QModelIndex GetRootIndex() const
        {
            return m_assetBrowserComponent->GetAssetBrowserModel()->index(0, 0);
        }

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserComponent> m_assetBrowserComponent;

        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserTableModel> m_tableModel;

        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTesterAssetBrowser;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTesterFilterModel;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTesterTableModel;

    };

    TEST_F(AssetBrowserTest, AssetBrowserTestingTest)
    {
        constexpr size_t size = 10;
        int count = 0;
        for (int i = 0; i < size; ++i)
        {
            count++;
        }
        SetupAssetBrowser();
        qDebug() << "-------------Asset Browser Model------------";
        PrintModel(m_assetBrowserComponent->GetAssetBrowserModel());

        qDebug() << "---------Asset Browser Filter Model---------";
        PrintModel(m_filterModel.get());

        qDebug() << "-------------Asset Table Model--------------";
        PrintModel(m_tableModel.get());

        EXPECT_EQ(count, 10);
    }
} // namespace UnitTest
