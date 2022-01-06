/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplication.h>

#include "EditorFrameworkAPI.h"

#include <QtCore/QString>

namespace LegacyFramework
{
    const char* appName()
    {
        const char* result = nullptr;
        EBUS_EVENT_RESULT(result, FrameworkApplicationMessages::Bus, GetApplicationName);
        return result;
    }

    const char* appModule()
    {
        const char* result = nullptr;
        EBUS_EVENT_RESULT(result, FrameworkApplicationMessages::Bus, GetApplicationModule);
        return result;
    }

    const char* appDir()
    {
        const char* result = nullptr;
        EBUS_EVENT_RESULT(result, FrameworkApplicationMessages::Bus, GetApplicationDirectory);
        return result;
    }

    bool RequiresGameProject()
    {
        bool result = false;
        EBUS_EVENT_RESULT(result, FrameworkApplicationMessages::Bus, RequiresGameProject);
        return result;
    }

    bool IsGUIMode()
    {
        bool result = false;
        EBUS_EVENT_RESULT(result, FrameworkApplicationMessages::Bus, IsRunningInGUIMode);
        return result;
    }

    bool appAbortRequested()
    {
        bool result = false;
        EBUS_EVENT_RESULT(result, FrameworkApplicationMessages::Bus, GetAbortRequested);
        return result;
    }

    bool isPrimary()
    {
        bool result = false;
        EBUS_EVENT_RESULT(result, FrameworkApplicationMessages::Bus, IsPrimary);
        return result;
    }

    bool IsAppConfigWritable()
    {
        bool result = false;
        EBUS_EVENT_RESULT(result, FrameworkApplicationMessages::Bus, IsAppConfigWritable);
        return result;
    }

    // helper function which retrieves the serialize context and asserts if its not found.
    AZ::SerializeContext* GetSerializeContext()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "No serialize context");
        return serializeContext;
    }

    void AddToPATH(const char* folder)
    {
        QString currentPath = QString::fromUtf8(qgetenv("PATH"));
        currentPath.append(";");
        currentPath.append(folder);
        qputenv("PATH", currentPath.toUtf8());
    }

    bool ShouldRunAssetProcessor()
    {
        bool result = false;
        EBUS_EVENT_RESULT(result, FrameworkApplicationMessages::Bus, ShouldRunAssetProcessor);
        return result;
    }
}
