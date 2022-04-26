/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        //! The interface for the visual feedback component of the central editor mode tracker for all viewports.
        class EditorModeFeedbackInterface
        {
        public:
            AZ_RTTI(EditorModeFeedbackInterface, "{3B36BE06-52B1-439C-AA8E-027AF80298AA}");

            virtual ~EditorModeFeedbackInterface() = default;

            //! Returns true if the editor mode feedback effect is enabled, otherwise false.
            virtual bool IsEnabled() const = 0;

            //! Registers a new (or updates an existing) drawable component to be associated with the provided entity id.
            //! @param entityComponentId The id of the entity and drawable component to be registered.
            //! @param meshHandle The handle of this drawable component's mesh.
            virtual void RegisterOrUpdateDrawableComponent(
                EntityComponentIdPair entityComponentId, const MeshFeatureProcessorInterface::MeshHandle& meshHandle) = 0;
        };
    } // namespace Render
} // namespace AZ

