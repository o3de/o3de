// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tools/Components/Editor${SanitizedCppName}Component.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}EditorTest
        : public UnitTest::AllocatorsTestFixture
    {
    };

    TEST_F(${SanitizedCppName}EditorTest, EditorComponent_Initialization_Succeeds)
    {
        Editor${SanitizedCppName}Component editorComponent;
        EXPECT_NE(&editorComponent, nullptr);
    }
} // namespace ${SanitizedCppName}
