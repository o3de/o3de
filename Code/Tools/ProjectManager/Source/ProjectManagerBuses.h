/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace O3DE::ProjectManager
{
    class ProjectManagerUtilityRequests : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<ProjectManagerUtilityRequests>;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void CanCloseProjectManager(bool& result) const = 0;
    };

    using ProjectManagerUtilityRequestsBus = AZ::EBus<ProjectManagerUtilityRequests>;
} // namespace O3DE::ProjectManager
