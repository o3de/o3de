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
