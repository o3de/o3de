/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    class ReflectContext;

    namespace Null
    {
        class ReflectSystemComponent
            : public AZ::Component
        {
        public:
            ReflectSystemComponent() = default;
            virtual ~ReflectSystemComponent() = default;
            AZ_COMPONENT(ReflectSystemComponent, "{E1870694-B44A-4908-B1A4-F8AD70C6E990}");

            static void Reflect(AZ::ReflectContext* context);

        private:
            void Activate() override {}
            void Deactivate() override {}
        };
    }
}
