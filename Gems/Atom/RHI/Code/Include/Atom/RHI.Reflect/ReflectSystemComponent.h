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

namespace AZ
{
    class ReflectContext;

    namespace RHI
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
}
