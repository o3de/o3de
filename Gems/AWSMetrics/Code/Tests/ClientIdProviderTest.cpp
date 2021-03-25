/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
