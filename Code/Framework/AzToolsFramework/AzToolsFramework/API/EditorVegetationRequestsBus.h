/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>

class CVegetationMap;
struct CVegetationInstance;

namespace AzToolsFramework
{
    namespace EditorVegetation
    {
        /**
         * Bus used to talk to VegetationMap across the application
         */
        class EditorVegetationRequests
            : public AZ::EBusTraits
        {
        public:
            using Bus = AZ::EBus<EditorVegetationRequests>;

            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            typedef CVegetationMap* BusIdType;

            virtual ~EditorVegetationRequests() {}

            virtual AZStd::vector<CVegetationInstance*> GetObjectInstances(const AZ::Vector2& min, const AZ::Vector2& max) = 0;
            virtual void DeleteObjectInstance(CVegetationInstance* instance) = 0;
        };

        using EditorVegetationRequestsBus = AZ::EBus<EditorVegetationRequests>;
    }
}
