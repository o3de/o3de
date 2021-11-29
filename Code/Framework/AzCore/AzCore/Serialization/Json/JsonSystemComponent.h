/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class JsonSystemComponent
        : public Component
    {
    public:
        AZ_COMPONENT(JsonSystemComponent, "{3C2C7234-9512-4E24-86F0-C40865D7EECE}", Component);

        void Activate() override;
        void Deactivate() override;

        static void Reflect(ReflectContext* reflectContext);
    };
} // namespace AZ
