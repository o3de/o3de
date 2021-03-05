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

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>

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
