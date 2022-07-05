/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class ReflectContext;
    class EntityId;
    class Vector3;
}

namespace ScriptCanvas
{
    namespace Entity
    {
        class RotateMethod
        {
        public:
            AZ_TYPE_INFO(RotateMethod, "{4BC6D515-214A-4DCE-8FCB-A6389B66A1B9}");

            static void Reflect(AZ::ReflectContext* context);

            static void Rotate(const AZ::EntityId&, const AZ::Vector3&);
        };
    }
}
