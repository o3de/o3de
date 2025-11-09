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

        // Put your public request methods here.
        
        // Put notification events here. Examples:
        // void RegisterEvent(AZ::EventHandler<...> notifyHandler);
        // AZ::Event<...> m_notifyEvent1;
        
    };

    using ${SanitizedCppName}RequestBus = AZ::EBus<${SanitizedCppName}Requests>;

} // namespace ${GemName}
