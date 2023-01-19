/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShaderBuilderTestFixture.h"

#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Name/NameDictionary.h>

#include <AzslShaderBuilderSystemComponent.h>

namespace UnitTest
{
    void ShaderBuilderTestFixture::SetUp()
    {
        LeakDetectionFixture::SetUp();

        AZ::NameDictionary::Create();
    }

    void ShaderBuilderTestFixture::TearDown()
    {
        AZ::NameDictionary::Destroy();

        LeakDetectionFixture::TearDown();
    }

}

