/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>

namespace MaterialCanvas
{
    //! MaterialGraphCompiler results that bypass the generated files and the Asset Processor.
    class MaterialGraphCompilerNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        //! Addressed by tool ID, matching the other Material Canvas buses.
        using BusIdType = AZ::Crc32;

        using PropertyValueList = AZStd::vector<AZStd::pair<AZ::Name, AZ::RPI::MaterialPropertyValue>>;

        //! Full set of the graph's property values after each successful compile, applied as live overrides; raised on the job thread.
        virtual void OnMaterialPropertyValuesChanged(
            [[maybe_unused]] const AZStd::string& graphPath, [[maybe_unused]] const PropertyValueList& propertyValues)
        {
        }
    };

    using MaterialGraphCompilerNotificationBus = AZ::EBus<MaterialGraphCompilerNotifications>;
} // namespace MaterialCanvas
