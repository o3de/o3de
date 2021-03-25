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
