// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace ${GemName}
{
    class ${SanitizedCppName}Requests
        : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(${GemName}::${SanitizedCppName}Requests, "{${Random_Uuid}}");

        // Put your public request methods here
    };
    using ${SanitizedCppName}RequestBus = AZ::EBus<${SanitizedCppName}Requests>;

    inline namespace ${SanitizedCppName}Interface
    {
        inline constexpr auto Get = [](AZ::ComponentBus::BusIdType busId) { return ${SanitizedCppName}RequestBus::FindFirstHandler(busId); };
    }

} // namespace ${GemName}
