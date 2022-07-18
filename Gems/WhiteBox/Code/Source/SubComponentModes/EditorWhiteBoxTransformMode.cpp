/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentModeCommon.h"
#include "EditorWhiteBoxTransformMode.h"
#include <Viewport/WhiteBoxModifierUtil.h>
#include <Viewport/WhiteBoxViewportConstants.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(TransformMode, AZ::SystemAllocator, 0)

    void TransformMode::Refresh()
    {
    }

    AZStd::vector<AzToolsFramework::ActionOverride> TransformMode::PopulateActions([[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return {};
    }

    void TransformMode::Display(
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair,
        [[maybe_unused]] const AZ::Transform& worldFromLocal,
        [[maybe_unused]] const IntersectionAndRenderData& renderData,
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
    }

    bool TransformMode::HandleMouseInteraction(
        [[maybe_unused]] const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair,
        [[maybe_unused]] const AZStd::optional<EdgeIntersection>& edgeIntersection,
        [[maybe_unused]] const AZStd::optional<PolygonIntersection>& polygonIntersection,
        [[maybe_unused]] const AZStd::optional<VertexIntersection>& vertexIntersection)
    {
        return false;
    }

} // namespace WhiteBox
