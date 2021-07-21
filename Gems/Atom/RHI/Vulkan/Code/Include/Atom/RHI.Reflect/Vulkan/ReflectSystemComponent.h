/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    class ReflectContext;

    namespace Vulkan
    {
        class ReflectSystemComponent
            : public AZ::Component
        {
        public:
            ReflectSystemComponent() = default;
            virtual ~ReflectSystemComponent() = default;
            AZ_COMPONENT(ReflectSystemComponent, "{A31C491E-279B-47E4-9E17-3A1598B3FC86}");

            static void Reflect(AZ::ReflectContext* context);

        private:
            void Activate() override {}
            void Deactivate() override {}
        };
    }
}
