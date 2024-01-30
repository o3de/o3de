/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Component/ComponentApplication.h>

#include <System/PhysXSystem.h>


namespace PhysX
{
    // We can't load the PhysX gem the same way we do LmbrCentral, because that would lead to the AZ::Environment
    // being create twice.  This is used to initialize the PhysX system component and create the descriptors for all
    // the PhysX components.
    class PhysXApplication
        : public AZ::ComponentApplication
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXApplication, AZ::SystemAllocator)
        PhysXApplication();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateReflectionManager() override; 
        void Destroy() override;

    private:
        PhysXSystem m_physXSystem;
    };

    class Environment
    {
    public:
        virtual ~Environment() = default;

    protected:
        void SetupInternal();
        void TeardownInternal();


        // Flag to enable pvd in tests
        static bool s_enablePvd;

        PhysXApplication* m_application;
        AZ::Entity* m_systemEntity;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIo;

        using PhysXLibraryModules = AZStd::vector<AZStd::unique_ptr<AZ::DynamicModuleHandle>>;
        AZStd::unique_ptr<PhysXLibraryModules> m_physXLibraryModules;
    };

    class TestEnvironment
        : public AZ::Test::ITestEnvironment
        , public Environment
    {
    public:
        void SetupEnvironment() override
        {
            SetupInternal();
        }
        void TeardownEnvironment() override
        {
            TeardownInternal();
        }
    };
}
