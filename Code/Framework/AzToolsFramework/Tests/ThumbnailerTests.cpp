/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/std/optional.h>
#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/StyledBusyLabel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Thumbnailer;

    class ThumbnailerTests
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_jobManager.emplace(AZ::JobManagerDesc{});
            m_jobContext.emplace(*m_jobManager);
            AZ::JobContext::SetGlobalContext(&*m_jobContext);
            m_thumbnailer.Activate();
        }

        void TearDown() override
        {
            m_thumbnailer.Deactivate();
            AZ::JobContext::SetGlobalContext(nullptr);
            m_jobContext.reset();
            m_jobManager.reset();
            // Release cached job allocations before leak-checking fixtures take their baseline.
            AZ::AllocatorManager::Instance().GarbageCollect();
        }

        AZStd::optional<AZ::JobManager> m_jobManager;
        AZStd::optional<AZ::JobContext> m_jobContext;
        ThumbnailerComponent m_thumbnailer;
    };

    TEST_F(ThumbnailerTests, BusyLabelRepaintsUntilAllPendingLoadsComplete)
    {
        using namespace AzToolsFramework::AssetBrowser;

        class BusyLabelProvider
            : public AssetBrowserComponentRequestBus::Handler
            , public AssetBrowserViewRequestBus::Handler
        {
        public:
            AssetBrowserModel* GetAssetBrowserModel() override { return nullptr; }
            bool AreEntriesReady() override { return false; }
            void PickAssets(AssetSelectionModel&, QWidget*) override {}
            AzQtComponents::StyledBusyLabel* GetStyledBusyLabel() override { return &m_label; }
            void SelectProduct(AZ::Data::AssetId) override {}
            void SelectFileAtPath(const AZStd::string&) override {}
            void ClearFilter() override {}
            void Update() override { ++m_updates; }

            AzQtComponents::StyledBusyLabel m_label;
            int m_updates = 0;
        } busyLabelProvider;

        class TestThumbnail : public Thumbnail
        {
        public:
            using Thumbnail::Thumbnail;

            void FinishLoad()
            {
                m_state = State::Ready;
                Q_EMIT ThumbnailUpdated();
            }

            void ResetLoad()
            {
                m_state = State::Unloaded;
            }

            void Update() override
            {
                ++m_updateRequests;
            }

            int m_updateRequests = 0;
        };

        class Provider : public ThumbnailProvider
        {
        public:
            bool GetThumbnail(SharedThumbnailKey key, SharedThumbnail& thumbnail) override
            {
                thumbnail = key == m_first->GetKey() ? m_first : m_second;
                return true;
            }
            const char* GetProviderName() const override { return "Test"; }

            QSharedPointer<TestThumbnail> m_first = QSharedPointer<TestThumbnail>::create(AZStd::make_shared<ThumbnailKey>());
            QSharedPointer<TestThumbnail> m_second = QSharedPointer<TestThumbnail>::create(AZStd::make_shared<ThumbnailKey>());
        };

        auto provider = AZStd::make_shared<Provider>();
        m_thumbnailer.RegisterThumbnailProvider(provider);

        m_thumbnailer.GetThumbnail(provider->m_first->GetKey());
        provider->m_first->FinishLoad();
        EXPECT_TRUE(provider->m_first->GetKey()->IsReady());
        provider->m_first = QSharedPointer<TestThumbnail>::create(AZStd::make_shared<ThumbnailKey>());

        busyLabelProvider.AssetBrowserComponentRequestBus::Handler::BusConnect();
        busyLabelProvider.AssetBrowserViewRequestBus::Handler::BusConnect();
        m_thumbnailer.GetThumbnail(provider->m_first->GetKey());
        m_thumbnailer.GetThumbnail(provider->m_second->GetKey());

        Q_EMIT busyLabelProvider.m_label.repaintNeeded();
        EXPECT_EQ(busyLabelProvider.m_updates, 1);

        provider->m_first->FinishLoad();
        busyLabelProvider.m_updates = 0;
        Q_EMIT busyLabelProvider.m_label.repaintNeeded();
        EXPECT_EQ(busyLabelProvider.m_updates, 1);

        provider->m_second->FinishLoad();
        busyLabelProvider.m_updates = 0;
        Q_EMIT busyLabelProvider.m_label.repaintNeeded();
        EXPECT_EQ(busyLabelProvider.m_updates, 0);

        Q_EMIT provider->m_first->ThumbnailUpdated();
        EXPECT_EQ(busyLabelProvider.m_updates, 1);

        provider->m_first->ResetLoad();
        m_thumbnailer.GetThumbnail(provider->m_first->GetKey());
        busyLabelProvider.m_updates = 0;
        provider->m_first->FinishLoad();
        EXPECT_EQ(busyLabelProvider.m_updates, 1);
        EXPECT_TRUE(provider->m_first->GetKey()->UpdateThumbnail());
        EXPECT_EQ(provider->m_first->m_updateRequests, 1);
    }
} // namespace UnitTest
