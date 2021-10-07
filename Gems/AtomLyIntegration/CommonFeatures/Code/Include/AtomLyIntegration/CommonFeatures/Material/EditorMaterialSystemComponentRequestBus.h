/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/Material/MaterialAssignmentId.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Render
    {
        //! EditorMaterialSystemComponentRequests provides an interface to communicate with MaterialEditor
        class EditorMaterialSystemComponentRequests
            : public AZ::EBusTraits
        {
        public:
            // Only a single handler is allowed
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Open source material in material editor
            virtual void OpenMaterialEditor(const AZStd::string& sourcePath) = 0;

            //! Open material instance editor
            virtual void OpenMaterialInspector(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId) = 0;
        };
        using EditorMaterialSystemComponentRequestBus = AZ::EBus<EditorMaterialSystemComponentRequests>;
    } // namespace Render
} // namespace AZ
