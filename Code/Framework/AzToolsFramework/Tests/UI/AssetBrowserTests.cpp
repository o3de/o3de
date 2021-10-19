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
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>

#include <Prefab/PrefabTestFixture.h>

#include <QAbstractItemModelTester>

namespace UnitTest
{
    // Test fixture for the AssetBrowser model that uses a QAbstractItemModelTester to validate the state of the model
    // when QAbstractItemModel signals fire. Tests will exit with a fatal error if an invalid state is detected.
    class AssetBrowserTest : public ToolsApplicationFixture
    {
    protected:
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

        EXPECT_EQ(count, 11);
    }
} // namespace UnitTest
