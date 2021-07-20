/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <windows.h>
#include <QApplication>

#include "Qt/NewsBuilder.h"
#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/IO/Path/Path.h>


int main(int argc, char *argv[])
{
    // Must be set before QApplication is initialized, so that we support HighDpi monitors, like the Retina displays
    // on Windows 10
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication a(argc, argv);
    AZ::IO::FixedMaxPath engineRootPath;
    {
        AZ::ComponentApplication componentApplication;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }
    News::NewsBuilder w(nullptr, engineRootPath);
    w.show();
    return a.exec();
}
