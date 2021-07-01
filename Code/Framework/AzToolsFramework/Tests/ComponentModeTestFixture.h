/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class ComponentModeTestFixture
        : public ToolsApplicationFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;
    };
} // namespace UnitTest
