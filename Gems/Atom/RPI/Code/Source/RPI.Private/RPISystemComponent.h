/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * @file RPISystemComponent.h
 * @brief Contains the definition of the RPISystemComponent that will actually have ownership of
 *        most RPI constructs and will be responsible for propagation to them as necessary
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <Atom/RPI.Public/RPISystem.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialFunctorSourceDataRegistration;

        /**
         * @brief The system level component of managing the RPI systems
         * @detail This class is mainly in charge of wrapping the RPISystem and
         *         providing access to other objects that live a the same execution level.
         *         This is the main entry point for adding GPU work to the RPI and for
         *         controlling RPI execution.
         */
        class RPISystemComponent final
            : public AZ::Component
            , public AZ::SystemTickBus::Handler
        {
        public:
            AZ_COMPONENT(RPISystemComponent, "{83E301F3-7A0C-4099-B530-9342B91B1BC0}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

            RPISystemComponent();
            ~RPISystemComponent() override;

            void Activate() override;
            void Deactivate() override;

        private:
            RPISystemComponent(const RPISystemComponent&) = delete;

            // SystemTickBus overrides...
            void OnSystemTick() override;

            RPISystem m_rpiSystem;

            RPISystemDescriptor m_rpiDescriptor;

            MaterialFunctorSourceDataRegistration* m_materialFunctorRegistration = nullptr;
        };
    } // namespace RPI
} // namespace AZ
