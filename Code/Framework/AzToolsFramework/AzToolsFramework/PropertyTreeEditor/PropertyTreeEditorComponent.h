/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
