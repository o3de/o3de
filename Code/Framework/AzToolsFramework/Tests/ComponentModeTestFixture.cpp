/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentModeTestFixture.h"
#include "ComponentModeTestDoubles.h"

#include <AzCore/UserSettings/UserSettingsComponent.h>

namespace UnitTest
{
    void ComponentModeTestFixture::SetUpEditorFixtureImpl()
    {
        namespace AztfCmf = AzToolsFramework::ComponentModeFramework;

        auto* app = GetApplication();

        app->RegisterComponentDescriptor(AztfCmf::PlaceholderEditorComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(AztfCmf::AnotherPlaceholderEditorComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(AztfCmf::DependentPlaceholderEditorComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(
            AztfCmf::TestComponentModeComponent<AztfCmf::OverrideMouseInteractionComponentMode>::CreateDescriptor());
        app->RegisterComponentDescriptor(AztfCmf::IncompatiblePlaceholderEditorComponent::CreateDescriptor());
    }
} // namespace UnitTest
