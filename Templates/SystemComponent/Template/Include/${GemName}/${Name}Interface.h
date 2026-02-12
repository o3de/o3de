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

#include <AzCore/EBus/EBus.h>

namespace ${GemName}
{
    /*
     * ${SanitizedCppName}Requests - Global EBus interface for ${SanitizedCppName}SystemComponent.
     *
     * This is a single-address (global) bus. There is only one address on the bus,
     * meaning exactly one handler is expected to connect at a time - the system component
     * itself. Callers do not need an entity ID to reach it.
     *
     * AddressPolicy::Single is declared below to enforce this. It is the correct policy
     * for system-wide services that exist once for the lifetime of the engine.
     * AZ::ComponentBus (used by game/entity components) should NOT be used here because
     * it is keyed by entity ID, which is not meaningful for a global system service.
     *
     * AZ_RTTI is required so that the behavior context can identify this type at runtime.
     * The UUID must be unique across the entire project.
     *
     * Usage - calling a method on the global system:
     *   ${SanitizedCppName}RequestBus::Broadcast(&${SanitizedCppName}Requests::YourMethod);
     *
     * Usage - calling with a return value:
     *   bool result = false;
     *   ${SanitizedCppName}RequestBus::BroadcastResult(result, &${SanitizedCppName}Requests::YourQuery);
     *
     * Do NOT use ${SanitizedCppName}RequestBus::Event(id, ...) - this bus has no address.
     *
     * To expose events that other systems can listen to (notifications), consider creating
     * a separate ${SanitizedCppName}NotificationBus in this file following the same pattern.
     */
    class ${SanitizedCppName}Requests
        : public AZ::EBusTraits
    {
    public:
        /*
         * Single-address policy: only one handler connects to this bus at a time.
         * Attempting to connect a second handler will assert in debug builds.
         */
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        AZ_RTTI(${GemName}::${SanitizedCppName}Requests, "{${Random_Uuid}}");

        /*
         * Add your public request methods here.
         * Each method declared here must be implemented in ${SanitizedCppName}SystemComponent.
         * Example:
         *   virtual void SetEnabled(bool enabled) = 0;
         *   virtual bool GetEnabled() const = 0;
         */

        /*
         * To expose AZ::Event-based notifications that callers can register handlers on,
         * declare them here and fire them from the system component implementation.
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
     * Use Broadcast() and BroadcastResult() to communicate with the system component.
     */
    using ${SanitizedCppName}RequestBus = AZ::EBus<${SanitizedCppName}Requests>;

} // namespace ${GemName}
