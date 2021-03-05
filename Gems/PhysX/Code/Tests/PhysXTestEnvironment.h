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

        void LoadPhysXLibraryModules();
        void UnloadPhysXLibraryModules();

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

            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };
}
