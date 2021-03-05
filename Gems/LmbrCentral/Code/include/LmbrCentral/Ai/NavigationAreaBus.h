/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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