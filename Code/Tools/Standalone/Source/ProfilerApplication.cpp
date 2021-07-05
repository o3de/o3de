/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ProfilerApplication.h"

#include <Source/Driller/DrillerContext.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>

namespace Driller
{
    void Application::RegisterCoreComponents()
    {
        StandaloneTools::BaseApplication::RegisterCoreComponents();

        Driller::Context::CreateDescriptor();
        RegisterComponentDescriptor(AzFramework::TargetManagementComponent::CreateDescriptor());
    }

    void Application::CreateApplicationComponents()
    {
        StandaloneTools::BaseApplication::CreateApplicationComponents();
        EnsureComponentCreated(Driller::Context::RTTI_Type());
        EnsureComponentCreated(AzFramework::TargetManagementComponent::RTTI_Type());
    }

    void Application::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
    {
        StandaloneTools::BaseApplication::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("driller");
    }
}
