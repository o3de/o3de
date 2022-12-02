/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/DocumentPropertyEditor/BasicAdapter.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystem.h>

namespace AZ::DocumentPropertyEditor::Tests
{
    class DocumentPropertyEditorTestFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override;
        void TearDown() override;

        AZStd::unique_ptr<BasicAdapter> m_adapter;
        AZStd::unique_ptr<PropertyEditorSystem> m_system;
    };
}
