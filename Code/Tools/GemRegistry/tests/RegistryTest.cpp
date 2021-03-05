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
#include <AzTest/AzTest.h>
#include "GemRegistry.h"

using Gems::GemRegistry;

class RegistryTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_errCreationFailed         = "Failed to create a GemRegistry instance.";
        m_errInvalidErrorMessage    = "Error message is not what it was set to.";
    }

    const char* m_errCreationFailed;
    const char* m_errInvalidErrorMessage;
};

TEST_F(RegistryTest, CreateAndDestroyTest)
{
    Gems::IGemRegistry* gr = CreateGemRegistry();
    ASSERT_NE(gr, nullptr)                                                  << m_errCreationFailed;
    DestroyGemRegistry(gr);
}