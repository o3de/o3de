/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/ActionManager/ActionManagerFixture.h>

namespace UnitTest
{
    void ActionManagerFixture::SetUpEditorFixtureImpl()
    {
        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        ASSERT_TRUE(m_actionManagerInterface != nullptr);

        m_widget = new QWidget();
    }

    void ActionManagerFixture::TearDownEditorFixtureImpl()
    {
    }

} // namespace UnitTest
