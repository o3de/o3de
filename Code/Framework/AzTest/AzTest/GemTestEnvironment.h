/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>

namespace AZ
{
    namespace Test
    {

        /// A test environment which is intended to facilitate writing unit tests which require components from a gem.
        class GemTestEnvironment
            : public UnitTest::TraceBusHook
        {
        public:
            GemTestEnvironment();

            /// Adds paths for the dynamic modules which should be loaded.
            /// These modules will be loaded prior to the setup of the gem which is the focus of the GemTestEnvironment,
            /// so any other gems etc. which that gem depends on should be added here.  The gem to be tested should not
            /// be added here because it cannot be loaded using the usual module loading process, as that would result
            /// in attempting to create the gem's environment a second time.
            /// @param dynamicModulePaths Dynamic module paths to be added to the existing collection.
            void AddDynamicModulePaths(const AZStd::vector<AZStd::string>& dynamicModulePaths);

            /// Adds to the collection of component descriptors which should be registered during the environment setup.
            /// Generally this will be the same as the descriptors which are registered in the gem's Module function,
            /// or a subset of those if only certain components are required during testing.
            /// @param componentDescriptors Component descriptors to be added to the existing collection.
            void AddComponentDescriptors(const AZStd::vector<AZ::ComponentDescriptor*>& componentDescriptors);

            /// Adds to the sorted list of components which should be activated during the environment setup.
            /// Any required components, for example the gem's system component, should be added here.  Dependency
            /// sorting is not performed, so it is up to the caller to ensure that all dependencies are met and the
            /// components are provided in a valid activation order.
            /// @param requiredComponents Components to be appended to the existing collection of required components.
            void AddRequiredComponents(const AZStd::vector<AZ::TypeId>& requiredComponents);

            /// Allows derived environments to set up which gems, components etc the environment should load.
            virtual void AddGemsAndComponents() {}

            /// Allows derived environments to perform additional steps prior to creating the application.
            virtual void PreCreateApplication() {}

            /// Allows derived environments to perform additional steps after creating the application.
            virtual void PostCreateApplication() {}

            /// Allows derived environments to perform additional steps after activating the system entity.
            virtual void PostSystemEntityActivate() {}

            /// Allows derived environments to override to perform additional steps prior to destroying the application.
            virtual void PreDestroyApplication() {}

            /// Allows derived environments to override to perform additional steps after destroying the application.
            virtual void PostDestroyApplication() {}

            /// Allows derived environments to create a desired instance of the application (for example ToolsApplication).
            virtual AZ::ComponentApplication* CreateApplicationInstance();
        protected:
            class Parameters
            {
            public:
                ~Parameters();

                AZStd::vector<AZ::ComponentDescriptor*> m_componentDescriptors;
                AZStd::vector<AZStd::string> m_dynamicModulePaths;
                AZStd::vector<AZ::TypeId> m_requiredComponents;
            };

            class GemTestEntity :
                public AZ::Entity
            {
                friend class GemTestEnvironment;
            };

            // ITestEnvironment
            void SetupEnvironment() override;
            void TeardownEnvironment() override;

        private:
            AZ::ComponentApplication* m_application;
            AZ::Entity* m_systemEntity;
            GemTestEntity* m_gemEntity;
            Parameters* m_parameters;
        };
    } // namespace Test
} // namespace AZ
