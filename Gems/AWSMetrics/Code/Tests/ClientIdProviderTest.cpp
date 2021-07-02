/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsGemMock.h>
#include <DefaultClientIdProvider.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace AWSMetrics
{
    class ClientIdProviderTest
        : public AWSMetricsGemAllocatorFixture
    {
    public:
        void SetUp() override
        {
            AWSMetricsGemAllocatorFixture::SetUp();

            m_defaultClientIdProvider = IdentityProvider::CreateIdentityProvider();
        }

        void TearDown() override
        {
            m_defaultClientIdProvider.reset();

            AWSMetricsGemAllocatorFixture::TearDown();
        }

        AZStd::unique_ptr<IdentityProvider> m_defaultClientIdProvider;
    };

    TEST_F(ClientIdProviderTest, CreateClientId_DefaultProvider_RandomUuid)
    {
        //! Default Client ID is engine version plus a ramdom UUID. Its size should be 32 Uuid + 4 dashes + 2 brackets + size of the engine version.
        ASSERT_GT(m_defaultClientIdProvider->GetIdentifier().size(), 38);
    }
}
