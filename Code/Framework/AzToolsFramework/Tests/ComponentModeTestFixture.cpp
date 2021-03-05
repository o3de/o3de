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

#include "ComponentModeTestDoubles.h"
#include "ComponentModeTestFixture.h"

#include <AzCore/UserSettings/UserSettingsComponent.h>

namespace UnitTest
{
    void ComponentModeTestFixture::SetUpEditorFixtureImpl()
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::ComponentModeFramework;

        auto* app = GetApplication();
        ASSERT_TRUE(app);

        app->RegisterComponentDescriptor(PlaceholderEditorComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(AnotherPlaceholderEditorComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(DependentPlaceholderEditorComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(
            TestComponentModeComponent<OverrideMouseInteractionComponentMode>::CreateDescriptor());
        app->RegisterComponentDescriptor(IncompatiblePlaceholderEditorComponent::CreateDescriptor());
    }
} // namespace UnitTest
