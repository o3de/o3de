/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ViewportInteraction.h>
#include <AZTestShared/Utils/Utils.h>

namespace LmbrCentral
{
    void DragMouse(
        const AzFramework::CameraState& cameraState,
        AzManipulatorTestFramework::ImmediateModeActionDispatcher* actionDispatcher,
        const AZ::Vector3& worldStart,
        const AZ::Vector3& worldEnd,
        const AzToolsFramework::ViewportInteraction::KeyboardModifier keyboardModifier =
        AzToolsFramework::ViewportInteraction::KeyboardModifier::None);

    void EnterComponentMode(AZ::Entity* entity, const AZ::Uuid& componentType);

    void ExpectBoxDimensions(AZ::Entity* entity, const AZ::Vector3& expectedBoxDimensions);

    void ExpectTranslationOffset(AZ::Entity* entity, const AZ::Vector3& expectedTranslationOffset);
} // namespace LmbrCentral
