/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>
#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        // Ebus for script-friendly APIs for the prefab loader
        struct PrefabLoaderScriptingTraits : AZ::EBusTraits
        {
            AZ_TYPE_INFO(PrefabLoaderScriptingTraits, "{C344B7D8-8299-48C9-8450-26E1332EA011}");

            virtual ~PrefabLoaderScriptingTraits() = default;

            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            /**
             * Saves a Prefab Template into the provided output string.
             * Converts Prefab Template form into .prefab form by collapsing nested Template info
             * into a source path and patches.
             * @param templateId Id of the template to be saved
             * @return Will contain the serialized template json on success
             */
            virtual AZ::Outcome<AZStd::string, void> SaveTemplateToString(TemplateId templateId) = 0;
        };

        using PrefabLoaderScriptingBus = AZ::EBus<PrefabLoaderScriptingTraits>;

    } // namespace Prefab
} // namespace AzToolsFramework

