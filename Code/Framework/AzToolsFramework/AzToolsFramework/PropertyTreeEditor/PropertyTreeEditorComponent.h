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

namespace AzToolsFramework
{
    namespace Components
    {

        class PropertyTreeEditorComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(PropertyTreeEditorComponent, "{8CE26D19-2469-4E8D-A806-AC63E276B72E}")

            static void Reflect(AZ::ReflectContext* context);

            // AZ::Component ...
            void Activate() override {}
            void Deactivate() override {}
        };

    } // namespace AzToolsFramework::Components

} // namespace AzToolsFramework