/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * @file PassTemplatesAutoLoader.h
 * @brief Contains the definition of the PassTemplatesAutoLoader which provides
 *        a data-driven alternative for Gems and Projects to load custom PassTemplates.
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

namespace AZ
{
    namespace RPI
    {
        /**
         * @brief A data-driven System Component that loads PassTemplates across all Gems and the Game Project.
         * @detail This service provides an opt-in mechanism for Gems and any Game Project to load
         *         custom PassTemplates.azasset WITHOUT having to write C++ code.
                   This system component works as a convenience service because there's already an API
         *         in AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent that helps Gems and Game Projects
         *         load their custom PassTemplates (*.azasset), the problem is, of course, that C++ code
         *         needs to be written to invoque the API. And this is where this System Component comes
         *         to the rescue.
         *         How it works?
         *         This service, at startup time, looks across all active Gems for assets with the following
         *         naming convention:
         *         "Passes/<Gem Name>/AutoLoadPassTemplates.azassset".
         *         or (Applicable to the Game Project)
         *         "Passes/<Project Name>/AutoLoadPassTemplates.azassset".
         *         or (Applicable to the Game Project)
         *         "Assets/Passes/<Project Name>/AutoLoadPassTemplates.azassset".
         *         If any of those asset paths exist, this service will automatically add those
         *         PassTemplates to the PassLibrary.
         */
        class PassTemplatesAutoLoader final
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(PassTemplatesAutoLoader, "{75FEC6CC-ACA7-419C-8A63-4286998CBC0B}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            PassTemplatesAutoLoader() = default;
            ~PassTemplatesAutoLoader() override = default;

            void Activate() override;
            void Deactivate() override;

        private:
            PassTemplatesAutoLoader(const PassTemplatesAutoLoader&) = delete;

            static constexpr char LogWindow[] = "PassTemplatesAutoLoader";

            //! Loads PassTemplates across all Gems and the Game Project according to the following
            //! naming convention:
            //! "Passes/<Gem Name>/AutoLoadPassTemplates.azassset".
            //! Or (Applicable to the Game Project)
            //! "Passes/<Project Name>/AutoLoadPassTemplates.azassset".
            //! Or (Applicable to the Game Project)
            //! "Assets/Passes/<Project Name>/AutoLoadPassTemplates.azassset".
            void LoadPassTemplates();

            //! We use this event handler to register with RPI::PassSystem to be notified
            //! of the right time to load PassTemplates.
            //! The callback will invoke this->LoadPassTemplates()
            AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
        };
    } // namespace RPI
} // namespace AZ
