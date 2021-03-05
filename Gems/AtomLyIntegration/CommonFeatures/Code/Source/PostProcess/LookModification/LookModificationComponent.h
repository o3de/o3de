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
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/LookModification/LookModificationComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/LookModification/LookModificationComponentConstants.h>
#include <PostProcess/LookModification/LookModificationComponentController.h>

namespace AZ
{
    namespace Render
    {
        class LookModificationComponent final
            : public AzFramework::Components::ComponentAdapter<LookModificationComponentController, LookModificationComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<LookModificationComponentController, LookModificationComponentConfig>;
            AZ_COMPONENT(AZ::Render::LookModificationComponent, LookModificationComponentTypeId , BaseClass);

            LookModificationComponent() = default;
            LookModificationComponent(const LookModificationComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
