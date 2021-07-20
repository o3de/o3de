/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace LmbrCentral
{
    /**
     * Messages serviced by the EditorNavigationAreaComponent.
     * The EditorNavigationAreaComponent produces meshes and a volume to create
     * a NavMesh to be used by the runtime.
     */
    class NavigationAreaRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~NavigationAreaRequests() = default;

        /**
         * Update/refresh the navigation area (foe example if changes have occurred to it or its surroundings).
         */
        virtual void RefreshArea() {}
    };

    /**
     * Bus to service requests made to the Navigation Area component.
     */
    using NavigationAreaRequestBus = AZ::EBus<NavigationAreaRequests>;
} // namespace LmbrCentral
