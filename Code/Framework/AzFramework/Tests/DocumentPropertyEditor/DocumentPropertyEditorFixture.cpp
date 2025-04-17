/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/DocumentPropertyEditor/DocumentPropertyEditorFixture.h>
#include <AzCore/Name/NameDictionary.h>

namespace AZ::DocumentPropertyEditor::Tests
{
    void DocumentPropertyEditorTestFixture::SetUp()
    {
        UnitTest::LeakDetectionFixture::SetUp();
        NameDictionary::Create();
        DocumentAdapter::SetDebugModeEnabled(true);
        m_system = AZStd::make_unique<PropertyEditorSystem>();
        m_adapter = AZStd::make_unique<BasicAdapter>();
    }

    void DocumentPropertyEditorTestFixture::TearDown()
    {
        m_adapter.reset();
        m_system.reset();
        DocumentAdapter::SetDebugModeEnabled(false);
        NameDictionary::Destroy();
        UnitTest::LeakDetectionFixture::TearDown();
    }
}
