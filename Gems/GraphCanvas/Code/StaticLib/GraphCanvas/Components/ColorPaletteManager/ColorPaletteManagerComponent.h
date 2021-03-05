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

#include <AzCore/Component/Component.h>

namespace GraphCanvas
{
    namespace Deprecated
    {
        class ColorPaletteManagerComponent
            : public AZ::Component
        {
            typedef AZStd::pair<QColor, QPixmap*> IconDescriptor;
            typedef AZStd::unordered_map< AZStd::string, IconDescriptor> PaletteToIconDescriptorMap;

        public:
            AZ_COMPONENT(ColorPaletteManagerComponent, "{486B009F-632B-44F6-81C2-3838746190AE}");
            static void Reflect(AZ::ReflectContext* reflectContext);

            // Component
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("GraphCanvas_ColorPaletteManagerService", 0x6495addb));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC("GraphCanvas_ColorPaletteManagerService", 0x6495addb));
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                (void)dependent;
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                (void)required;
            }

            void Init() {};
            void Activate() {};
            void Deactivate() {};
            ////
        };
    }
}