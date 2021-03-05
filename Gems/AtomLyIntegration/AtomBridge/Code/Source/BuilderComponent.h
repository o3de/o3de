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
#include <AzFramework/API/AtomActiveInterface.h>

namespace AZ
{
    namespace AtomBridge
    {
        class BuilderComponent final
            : public AZ::Component
            , public AzFramework::AtomActiveInterface
        {
        public:
            AZ_COMPONENT(BuilderComponent, "{D1FE015B-8431-4155-8FD0-8197F246901A}");
            static void Reflect(AZ::ReflectContext* context);

            BuilderComponent();
            ~BuilderComponent() override;

            // AZ::Component overrides...
            void Activate() override;
            void Deactivate() override;
        };
    } // namespace RPI
} // namespace AZ
