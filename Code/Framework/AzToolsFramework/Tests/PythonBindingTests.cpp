/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class AzToolsFrameworkPythonBindingsFixture
        : public ToolsApplicationFixture<>
    {
    };

    TEST_F(AzToolsFrameworkPythonBindingsFixture, AzToolsFrameworkToolsApplicationRequestBus_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = GetApplication()->GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        auto toolsApplicationRequestBus = behaviorContext->m_ebuses.find("ToolsApplicationRequestBus");
        EXPECT_TRUE(behaviorContext->m_ebuses.end() != toolsApplicationRequestBus);
    }

    TEST_F(AzToolsFrameworkPythonBindingsFixture, AzToolsFrameworkToolsApplicationNotificationBus_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = GetApplication()->GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        auto toolsApplicationNotificationBus = behaviorContext->m_ebuses.find("ToolsApplicationNotificationBus");
        EXPECT_TRUE(behaviorContext->m_ebuses.end() != toolsApplicationNotificationBus);
    }

    TEST_F(AzToolsFrameworkPythonBindingsFixture, AzToolsFrameworkEditorEntityContextNotificationBus_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = GetApplication()->GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        auto editorEntityContextNotificationBus = behaviorContext->m_ebuses.find("EditorEntityContextNotificationBus");
        EXPECT_TRUE(behaviorContext->m_ebuses.end() != editorEntityContextNotificationBus);
    }

    TEST_F(AzToolsFrameworkPythonBindingsFixture, AzToolsFrameworkSliceRequestBus_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = GetApplication()->GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        auto sliceRequestBus = behaviorContext->m_ebuses.find("SliceRequestBus");
        EXPECT_TRUE(behaviorContext->m_ebuses.end() != sliceRequestBus);
    }
}
