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
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class JsonSystemComponent
        : public Component
    {
    public:
        AZ_COMPONENT(JsonSystemComponent, "{3C2C7234-9512-4E24-86F0-C40865D7EECE}", Component);

        void Activate();
        void Deactivate();

        static void Reflect(ReflectContext* reflectContext);
    };
} // namespace AZ
