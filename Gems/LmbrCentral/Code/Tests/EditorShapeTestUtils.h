/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AZTestShared/Utils/Utils.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ViewportInteraction.h>
#include <AzToolsFramework/ComponentModes/ShapeComponentModeBus.h>

namespace LmbrCentral
{
    void EnterComponentMode(AZ::EntityId entityId, const AZ::Uuid& componentType);

    void SetComponentSubMode(
        AZ::EntityComponentIdPair entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode subMode);

    void ExpectBoxDimensions(AZ::EntityId entityId, const AZ::Vector3& expectedBoxDimensions);

    void ExpectCapsuleRadius(AZ::EntityId entityId, float expectedRadius);

    void ExpectCapsuleHeight(AZ::EntityId entityId, float expectedHeight);

    void ExpectSphereRadius(AZ::EntityId, float expectedRadius);

    void ExpectTranslationOffset(AZ::EntityId entityId, const AZ::Vector3& expectedTranslationOffset);

    void ExpectSubMode(
        AZ::EntityComponentIdPair entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode expectedSubMode);

    AzToolsFramework::ViewportInteraction::MouseInteractionResult CtrlScroll(float wheelDelta);
} // namespace LmbrCentral
