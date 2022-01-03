/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

#include <Credential/AWSCVarCredentialHandler.h>

using namespace AWSCore;

class AWSCVarCredentialHandlerTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:
    AWSCVarCredentialHandlerTest() = default;
    ~AWSCVarCredentialHandlerTest() override = default;

    void SetUp() override
    {
        m_credentialHandler = AZStd::make_unique<AWSCVarCredentialHandler>();
    }

    AZStd::unique_ptr<AWSCVarCredentialHandler> m_credentialHandler;
};

TEST_F(AWSCVarCredentialHandlerTest, GetCredentialsProvider_WhenCVarValuesAreEmpty_GetNullptr)
{
    auto actualCredentialsProvider = m_credentialHandler->GetCredentialsProvider();
    EXPECT_FALSE(actualCredentialsProvider);
}

TEST_F(AWSCVarCredentialHandlerTest, GetCredentialHandlerOrder_Call_AlwaysGetExpectedValue)
{
    auto actualOrder = m_credentialHandler->GetCredentialHandlerOrder();
    EXPECT_EQ(actualOrder, CredentialHandlerOrder::CVAR_CREDENTIAL_HANDLER);
}
