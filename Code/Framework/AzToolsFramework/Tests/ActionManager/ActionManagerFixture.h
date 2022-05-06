/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzTest/AzTest.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <AzToolsFramework/ActionManager/Action/ActionManager.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>

#include <QWidget>

class QWidget;

namespace UnitTest
{
    class ActionManagerFixture : public AllocatorsTestFixture
    {
    protected:
        void SetUp() override;
        void TearDown() override;

    public:
        AZStd::unique_ptr<AzToolsFramework::ActionManager> m_actionManager;
        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
        QWidget* m_widget = nullptr;
    };

} // namespace UnitTest
