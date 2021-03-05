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
    namespace Render
    {
        //! This is the editor-counterpart to this gem's main CommonSystemComponent class.
        class EditorCommonSystemComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(EditorCommonSystemComponent, "{D688E7FA-576B-4590-93D5-FEBB7B1D782D}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface overrides...
            void Init() override;
            void Activate() override;
            void Deactivate() override;
        };
    } // namespace Render
} // namespace AZ
