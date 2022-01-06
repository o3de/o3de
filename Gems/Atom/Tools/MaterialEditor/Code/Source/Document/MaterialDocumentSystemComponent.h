/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace MaterialEditor
{
    //! MaterialDocumentSystemComponent
    class MaterialDocumentSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MaterialDocumentSystemComponent, "{E011DA51-855D-45FA-87A3-1C1CD6379091}");

        MaterialDocumentSystemComponent() = default;
        ~MaterialDocumentSystemComponent() = default;
        MaterialDocumentSystemComponent(const MaterialDocumentSystemComponent&) = delete;
        MaterialDocumentSystemComponent& operator=(const MaterialDocumentSystemComponent&) = delete;

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
} // namespace MaterialEditor
