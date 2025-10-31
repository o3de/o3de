/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RHI/FactoryManagerBus.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/ValidationLayer.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::RHI
{
    //! System component in charge of selecting which Factory to use at runtime in case
    //! multiple ones are active. It also contains reflected data that determines the priorities
    //! between the factory backends.
    class FactoryManagerSystemComponent final
        : public AZ::Component
        , public FactoryManagerBus::Handler
    {
    public:
        AZ_COMPONENT(FactoryManagerSystemComponent, "{7C7AD991-9DD8-49D9-8C5F-6626937378E9}");
        static void Reflect(AZ::ReflectContext* context);

        FactoryManagerSystemComponent() = default;
        ~FactoryManagerSystemComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // FactoryManagerBus overrides ...
        void RegisterFactory(Factory* factory) override;
        void UnregisterFactory(Factory* factory) override;
        void FactoryRegistrationFinalize() override;
        AZ::RHI::ValidationMode DetermineValidationMode() const override;
        void EnumerateFactories(AZStd::function<bool(Factory* factory)> callback) override;

    private:
        FactoryManagerSystemComponent(const FactoryManagerSystemComponent&) = delete;

        /// Check if a factory was specified via command line.
        Factory* GetFactoryFromCommandLine();
        /// Select a factory from the available list using the user generated priorities or the factory default ones.
        Factory* SelectRegisteredFactory();

        void UpdateValidationModeFromCommandline();

        /// List with the factory priorities set by the user.
        AZStd::vector<AZStd::string> m_factoriesPriority;
        /// List of registered factories.
        AZStd::vector<Factory*> m_registeredFactories;
        AZ::RHI::ValidationMode m_validationMode = AZ::RHI::ValidationMode::Disabled;
    };
}
