/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Woodpecker_precompiled.h"

#include "ProfilerApplication.h"

#include <Woodpecker/Driller/DrillerContext.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>

namespace Driller
{
    void Application::RegisterCoreComponents()
    {
        Woodpecker::BaseApplication::RegisterCoreComponents();

        Driller::Context::CreateDescriptor();
        RegisterComponentDescriptor(AzFramework::TargetManagementComponent::CreateDescriptor());
    }

    void Application::CreateApplicationComponents()
    {
        Woodpecker::BaseApplication::CreateApplicationComponents();
        EnsureComponentCreated(Driller::Context::RTTI_Type());
        EnsureComponentCreated(AzFramework::TargetManagementComponent::RTTI_Type());
    }

    void Application::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
    {
        Woodpecker::BaseApplication::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("driller");
    }
}
