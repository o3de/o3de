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
        FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::GetApplicationName);
        return result;
    }

    const char* appModule()
    {
        const char* result = nullptr;
        FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::GetApplicationModule);
        return result;
    }

    const char* appDir()
    {
        const char* result = nullptr;
        FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::GetApplicationDirectory);
        return result;
    }

    bool RequiresGameProject()
    {
        bool result = false;
        FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::RequiresGameProject);
        return result;
    }

    bool IsGUIMode()
    {
        bool result = false;
        FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::IsRunningInGUIMode);
        return result;
    }

    bool appAbortRequested()
    {
        bool result = false;
        FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::GetAbortRequested);
        return result;
    }

    bool isPrimary()
    {
        bool result = false;
        FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::IsPrimary);
        return result;
    }

    bool IsAppConfigWritable()
    {
        bool result = false;
        FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::IsAppConfigWritable);
        return result;
    }

    // helper function which retrieves the serialize context and asserts if its not found.
    AZ::SerializeContext* GetSerializeContext()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
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
        FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::ShouldRunAssetProcessor);
        return result;
    }
}
