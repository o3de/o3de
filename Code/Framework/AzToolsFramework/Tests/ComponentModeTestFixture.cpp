/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
