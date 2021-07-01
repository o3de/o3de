/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Vector3Parameter.h"

namespace EMotionFX
{
    class Vector3GizmoParameter
        : public Vector3Parameter
    {
    public:
        AZ_RTTI(Vector3GizmoParameter, "{67A19A92-14A1-4A45-AE3B-DF5A8AB62E68}", Vector3Parameter)
        AZ_CLASS_ALLOCATOR_DECL

        Vector3GizmoParameter()
            : Vector3Parameter()
        {}

        static void Reflect(AZ::ReflectContext* context);

        const char* GetTypeDisplayName() const override;
    };
}
