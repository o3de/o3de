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
}

namespace AZ::RHI
{
    class ReflectSystemComponent
        : public AZ::Component
    {
    public:
        ReflectSystemComponent() = default;
        virtual ~ReflectSystemComponent() = default;
        AZ_COMPONENT(ReflectSystemComponent, "{5FC0BA8B-C6B9-4E7A-9C47-589373291F6A}");

        static void Reflect(AZ::ReflectContext* context);

        // Optional reflection to support save/load named enums using json serializer
        static void ReflectNamedEnums(AZ::ReflectContext* context);

    private:
        void Activate() override {}
        void Deactivate() override {}
    };
}
