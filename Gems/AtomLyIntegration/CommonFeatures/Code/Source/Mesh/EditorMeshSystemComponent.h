/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        //! System component that sets up necessary logic related to EditorMeshComponent.
        class EditorMeshSystemComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(EditorMeshSystemComponent, "{4D332E3D-C4FC-410B-A915-8E234CBDD4EC}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface overrides...
            void Activate() override;
            void Deactivate() override;
        };
    } // namespace Render
} // namespace AZ
