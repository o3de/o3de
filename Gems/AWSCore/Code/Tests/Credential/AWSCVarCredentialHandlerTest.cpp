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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

#include <Credential/AWSCVarCredentialHandler.h>

using namespace AWSCore;

class AWSCVarCredentialHandlerTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:
    AWSCVarCredentialHandlerTest() = default;
    virtual ~AWSCVarCredentialHandlerTest() = default;

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
