/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignmentId.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <QPixmap>

namespace AZ
{
    namespace Render
    {
        //! EditorMaterialSystemComponentRequests provides an interface for interacting with EditorMaterialSystemComponent, performing
        //! different operations like opening the material editor, the material instance inspector, and managing material preview images
        class EditorMaterialSystemComponentRequests : public AZ::EBusTraits
        {
        public:
            // Only a single handler is allowed
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Open document in material editor
            virtual void OpenMaterialEditor(const AZStd::string& sourcePath) = 0;

            //! Open document in material canvas
            virtual void OpenMaterialCanvas(const AZStd::string& sourcePath) = 0;

            //! Open the material instance editor to preview and edit material property overrides for the primary entity while applying
            //! changes to all entities in the editable set
            virtual void OpenMaterialInspector(
                const AZ::EntityId& primaryEntityId,
                const AzToolsFramework::EntityIdSet& entityIdsToEdit,
                const AZ::Render::MaterialAssignmentId& materialAssignmentId) = 0;

            //! Generate a material preview image for a specific entity and material slot with material and property overrides applied 
            virtual void RenderMaterialPreview(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId) = 0;

            //! Get the most recently rendered material preview image for the entity and material slot if one is available
            virtual QPixmap GetRenderedMaterialPreview(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId) const = 0;
        };
        using EditorMaterialSystemComponentRequestBus = AZ::EBus<EditorMaterialSystemComponentRequests>;
    } // namespace Render
} // namespace AZ
