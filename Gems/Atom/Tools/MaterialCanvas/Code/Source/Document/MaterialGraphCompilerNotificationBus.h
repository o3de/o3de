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
    //! Notifications raised by MaterialGraphCompiler for results that do not travel through the generated files, and therefore do not
    //! travel through the Asset Processor either.
    class MaterialGraphCompilerNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        //! Addressed by tool ID, matching the other Material Canvas buses.
        using BusIdType = AZ::Crc32;

        using PropertyValueList = AZStd::vector<AZStd::pair<AZ::Name, AZ::RPI::MaterialPropertyValue>>;

        //! Raised at the end of every successful compile with the complete set of material property values the graph currently
        //! describes, addressed by the absolute path of the graph they came from.
        //!
        //! Material Canvas writes material input node values as default values in the generated material type rather than as overrides
        //! in the generated material, so the only way to see a changed value is to rebuild the material type asset, and every shader
        //! built from it. Sending the values directly lets a listener apply them as property overrides on the live material instance,
        //! which is both immediate and independent of whether the Asset Processor has finished, or has finished at all.
        //!
        //! The full set is sent rather than a delta because listeners have to reapply the values after anything that recreates the
        //! material instance, and because a listener that missed an earlier compile would otherwise be permanently out of date.
        //!
        //! Raised from the graph compilation job thread. Handlers must marshal to whatever thread they need.
        virtual void OnMaterialPropertyValuesChanged(
            [[maybe_unused]] const AZStd::string& graphPath, [[maybe_unused]] const PropertyValueList& propertyValues)
        {
        }
    };

    using MaterialGraphCompilerNotificationBus = AZ::EBus<MaterialGraphCompilerNotifications>;
} // namespace MaterialCanvas
