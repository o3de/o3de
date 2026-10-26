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
    /*
     * ${SanitizedCppName}Requests - EBus interface for ${SanitizedCppName}Component.
     *
     * This class defines the public API surface that other systems use to communicate
     * with ${SanitizedCppName}Component at runtime. It is an AZ::EBus interface, meaning
     * callers do not hold a direct pointer to the component - they send messages through
     * the bus and the component handles them by connecting as a Handler.
     *
     * AZ_RTTI is required here so that the serialization and behavior contexts can
     * identify this type at runtime. The UUID must be unique across the entire project.
     *
     * Usage - sending a request to the component on a given entity:
     *   ${SanitizedCppName}RequestBus::Event(entityId, &${SanitizedCppName}Requests::YourMethod);
     *
     * Usage - broadcasting to all connected handlers:
     *   ${SanitizedCppName}RequestBus::Broadcast(&${SanitizedCppName}Requests::YourMethod);
     *
     * To expose events that other systems can listen to (notifications), consider creating
     * a separate ${SanitizedCppName}NotificationBus in this file following the same pattern.
     */
    class ${SanitizedCppName}Requests
        : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(${GemName}::${SanitizedCppName}Requests, "{${Random_Uuid}}");

        /*
         * Add your public request methods here.
         * Each method declared here must be implemented in ${SanitizedCppName}Component.
         * Example:
         *   virtual void SetEnabled(bool enabled) = 0;
         *   virtual bool GetEnabled() const = 0;
         */

        /*
         * To expose AZ::Event-based notifications that callers can register handlers on,
         * declare them here and fire them from the component implementation.
         * Example:
         *   void RegisterOnEnabledChanged(AZ::EventHandler<bool>& handler)
         *   {
         *       m_onEnabledChanged.Connect(handler);
         *   }
         *   AZ::Event<bool> m_onEnabledChanged;
         */

    };

    /*
     * ${SanitizedCppName}RequestBus - typed EBus alias for ${SanitizedCppName}Requests.
     * Use this alias everywhere you send or handle requests for this component.
     */
    using ${SanitizedCppName}RequestBus = AZ::EBus<${SanitizedCppName}Requests>;

} // namespace ${GemName}
