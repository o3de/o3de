/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <QByteArray>
#include <QtGlobal>

// Environments subclass from AZ::Test::ITestEnvironment
class LuaIDETestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    LuaIDETestEnvironment()
    {
        // The flattened GUI (stylesheets disabled) is what the elements and the palette hand-over of LuaIDE are for. The switch is read
        // once for the whole process (GuiPerfFloor), so it is set here, before any test reads it; the legacy twins are made at the
        // legacy rung by the twin harness, which does not read it.
        qputenv("O3DE_DISABLE_STYLESHEETS", "1");
    }

protected:
    void SetupEnvironment() override
    {
    }

    void TeardownEnvironment() override
    {
    }
};

AZ_UNIT_TEST_HOOK(new LuaIDETestEnvironment);
