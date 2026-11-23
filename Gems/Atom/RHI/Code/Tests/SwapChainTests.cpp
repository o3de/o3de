/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "RHITestFixture.h"
#include <Atom/RHI/DeviceSwapChain.h>
#include <Atom/RHI/SwapChain.h>

namespace UnitTest
{
    class PresentationSwapChain final : public AZ::RHI::DeviceSwapChain
    {
    public:
        uint32_t m_maximumInterval = 4;
        uint32_t m_changes = 0;
        uint32_t m_previousInterval = 0;
        bool m_shutdown = false;
        uint32_t GetMaximumVerticalSyncInterval() const override
        {
            return m_maximumInterval;
        }

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::SwapChainDescriptor&, AZ::RHI::SwapChainDimensions*) override
        {
            return AZ::RHI::ResultCode::Success;
        }
        AZ::RHI::ResultCode InitImageInternal(const InitImageRequest&) override
        {
            return AZ::RHI::ResultCode::Success;
        }
        AZ::RHI::ResultCode ResizeInternal(const AZ::RHI::SwapChainDimensions&, AZ::RHI::SwapChainDimensions*) override
        {
            return AZ::RHI::ResultCode::Success;
        }
        void ShutdownResourceInternal(AZ::RHI::DeviceResource&) override
        {
        }
        void ShutdownInternal() override
        {
            m_shutdown = true;
            AZ::RHI::DeviceSwapChain::ShutdownInternal();
        }
        uint32_t PresentInternal() override
        {
            return 0;
        }
        void SetVerticalSyncIntervalInternal(uint32_t previousInterval) override
        {
            ++m_changes;
            m_previousInterval = previousInterval;
        }
    };

    using SwapChainTests = RHITestFixture;

    TEST_F(SwapChainTests, ExistingVoidSetterAppliesSupportedIntervals)
    {
        PresentationSwapChain swapChain;
        swapChain.SetVerticalSyncInterval(2);
        EXPECT_EQ(swapChain.GetDescriptor().m_verticalSyncInterval, 2u);
        EXPECT_EQ(swapChain.m_changes, 1u);
    }

    TEST_F(SwapChainTests, UninitializedSwapChainRejectsIntervalChanges)
    {
        AZ::RHI::SwapChain swapChain;
        EXPECT_EQ(swapChain.TrySetVerticalSyncInterval(1), AZ::RHI::ResultCode::InvalidOperation);
        EXPECT_EQ(swapChain.GetMaximumVerticalSyncInterval(), 0u);
    }

    TEST_F(SwapChainTests, UnsupportedIntervalPreservesNativeAndReadableState)
    {
        PresentationSwapChain swapChain;
        EXPECT_EQ(swapChain.TrySetVerticalSyncInterval(2), AZ::RHI::ResultCode::Success);
        EXPECT_EQ(swapChain.TrySetVerticalSyncInterval(8), AZ::RHI::ResultCode::Unimplemented);
        EXPECT_EQ(swapChain.GetDescriptor().m_verticalSyncInterval, 2u);
        EXPECT_EQ(swapChain.m_changes, 1u);
        EXPECT_EQ(swapChain.TrySetVerticalSyncInterval(0), AZ::RHI::ResultCode::Success);
        EXPECT_EQ(swapChain.m_previousInterval, 2u);
        EXPECT_EQ(swapChain.GetDescriptor().m_verticalSyncInterval, 0u);
    }

    TEST_F(SwapChainTests, NativeIntervalsAreNotGloballyClampedToFour)
    {
        PresentationSwapChain swapChain;
        swapChain.m_maximumInterval = 8;
        EXPECT_EQ(swapChain.TrySetVerticalSyncInterval(8), AZ::RHI::ResultCode::Success);
        EXPECT_EQ(swapChain.GetDescriptor().m_verticalSyncInterval, 8u);
        swapChain.m_maximumInterval = 1;
        EXPECT_EQ(swapChain.TrySetVerticalSyncInterval(2), AZ::RHI::ResultCode::Unimplemented);
        EXPECT_EQ(swapChain.GetDescriptor().m_verticalSyncInterval, 8u);
        EXPECT_EQ(swapChain.m_changes, 1u);
    }

    TEST_F(SwapChainTests, UnsupportedInitialIntervalReleasesNativeResources)
    {
        Factory factory;
        auto device = MakeTestDevice();
        PresentationSwapChain swapChain;
        AZ::RHI::SwapChainDescriptor descriptor;
        descriptor.m_dimensions.m_imageWidth = 1;
        descriptor.m_dimensions.m_imageHeight = 1;
        descriptor.m_dimensions.m_imageCount = 2;
        descriptor.m_verticalSyncInterval = 5;
        EXPECT_EQ(swapChain.Init(*device, descriptor), AZ::RHI::ResultCode::Unimplemented);
        EXPECT_TRUE(swapChain.m_shutdown);
        EXPECT_FALSE(swapChain.IsInitialized());
        EXPECT_EQ(swapChain.GetImageCount(), 0u);
    }
} // namespace UnitTest
