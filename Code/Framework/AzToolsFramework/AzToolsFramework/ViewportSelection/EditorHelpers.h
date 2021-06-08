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

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/functional.h>

namespace AzFramework
{
    class DebugDisplayRequests;
    struct ViewportInfo;
    struct CameraState;
} // namespace AzFramework

namespace AzToolsFramework
{
    class EditorVisibleEntityDataCache;

    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }

    //! EditorHelpers are the visualizations that appear for entities
    //! when 'Display Helpers' is toggled on inside the editor.
    //! These include but are not limited to entity icons and shape visualizations.
    class EditorHelpers
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        //! An EditorVisibleEntityDataCache must be passed to EditorHelpers to allow it to
        //! efficiently read entity data without resorting to EBus calls.
        explicit EditorHelpers(const EditorVisibleEntityDataCache* entityDataCache)
            : m_entityDataCache(entityDataCache)
        {
        }
        EditorHelpers(const EditorHelpers&) = delete;
        EditorHelpers& operator=(const EditorHelpers&) = delete;
        ~EditorHelpers() = default;

        //! Handle any mouse interaction with the EditorHelpers.
        //! Used to check if a particular entity was selected.
        AZ::EntityId HandleMouseInteraction(
            const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        //! Do the drawing responsible for the EditorHelpers.
        //! @param showIconCheck Provide a custom callback to filter certain entities from
        //! displaying an icon under certain conditions.
        void DisplayHelpers(
            const AzFramework::ViewportInfo& viewportInfo,
            const AzFramework::CameraState& cameraState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZStd::function<bool(AZ::EntityId)>& showIconCheck);

    private:
        const EditorVisibleEntityDataCache* m_entityDataCache = nullptr; //!< Entity Data queried by the EditorHelpers.
    };
} // namespace AzToolsFramework
