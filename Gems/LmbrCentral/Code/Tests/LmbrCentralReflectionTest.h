/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Application/Application.h>
#ifdef LMBR_CENTRAL_EDITOR
#include <AzToolsFramework/Application/ToolsApplication.h>
#include "LmbrCentralEditor.h"
#endif

/**
 * Fixture class for tests that require a module to have been reflected.
 * An AZ application is created to handle the reflection. The application
 * starts up and shuts down only once for all tests using this fixture.
 * \tparam ApplicationT Type of AZ::ComponentApplication to use.
 * \tparam ModuleT Type of AZ::Module to reflect. It must be defined within this library.
 */
template<class ApplicationT, class ModuleT>
class ModuleReflectionTest
    : public UnitTest::LeakDetectionFixture
{
public:
    static ApplicationT* GetApplication() { return s_application.get(); }

protected:
    void SetUp() override;
    void TearDown() override;

private:
    // We need reflection from ApplicationT and nothing more.
    // This class lets us simplify the application that we run for tests.
    class InternalApplication : public ApplicationT
    {
    public:
        // Unhide these core startup/shutdown functions (They're 'protected' in AzFramework::Application)
        using ApplicationT::Create;
        using ApplicationT::Destroy;

        // Don't create any system components.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override { return AZ::ComponentTypeList(); }
    };

    static AZStd::unique_ptr<InternalApplication> s_application;
    static AZ::Entity* s_systemEntity;
};

/**
 * Fixture class for tests that load an object whose class is reflected within a module.
 * Upon setup, the object is loaded from a source data buffer.
 * \tparam ApplicationT Type of AZ::ComponentApplication to start.
 * \tparam ModuleT Type of AZ::Module to reflect. It must be defined within this library.
 * \tparam ObjectT Type of object to load from buffer.
 */
template<class ApplicationT, class ModuleT, class ObjectT>
class LoadReflectedObjectTest
    : public ModuleReflectionTest<ApplicationT, ModuleT>
{
    typedef ModuleReflectionTest<ApplicationT, ModuleT> BaseType;

protected:
    void SetUp() override;
    void TearDown() override;

    virtual const char* GetSourceDataBuffer() const = 0;

    AZStd::unique_ptr<ObjectT> m_object;
};

#ifdef LMBR_CENTRAL_EDITOR
/**
*  Creates, registers a dummy transform component for Editor Component Tests
*  Manages an entity for the editor component
*/
template<class ComponentT>
class LoadEditorComponentTest
    : public LoadReflectedObjectTest<AzToolsFramework::ToolsApplication, LmbrCentral::LmbrCentralEditorModule, ComponentT>
{
public:
    using LoadReflectedObjectTestBase = LoadReflectedObjectTest<AzToolsFramework::ToolsApplication, LmbrCentral::LmbrCentralEditorModule, ComponentT>;

    void SetUp() override;
    void TearDown() override;

protected:
    // simply fulfills the transform component dependency on editor components
    class DummyTransformComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(DummyTransformComponent, "{971C64A3-C9FB-4ADB-B122-BC579A889CD4}", AZ::Component);
        void Activate() override {};
        void Deactivate() override {};
    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("TransformService"));
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ_UNUSED(reflection);
        }
    };

    AZStd::unique_ptr<AZ::Entity> m_entity;
    AZ::ComponentDescriptor* m_transformComponentDescriptor{};
};
#endif

template<class ApplicationT, class ModuleT>
void ModuleReflectionTest<ApplicationT, ModuleT>::SetUp()
{
    s_application.reset(new ModuleReflectionTest::InternalApplication);

    AZ::ComponentApplication::Descriptor appDescriptor;
    appDescriptor.m_useExistingAllocator = true;
    appDescriptor.m_recordingMode = AZ::Debug::AllocationRecords::Mode::RECORD_FULL;

    AZ::ComponentApplication::StartupParameters appStartup;

    // ModuleT is declared within this dll.
    // Therefore, we can treat it like a statically linked module.
    appStartup.m_createStaticModulesCallback =
        [](AZStd::vector<AZ::Module*>& modules)
    {
        modules.emplace_back(new ModuleT);
    };

    // Create() starts the application and returns the system entity.
    s_systemEntity = s_application->Create(appDescriptor, appStartup);
}

template<class ApplicationT, class ModuleT>
void ModuleReflectionTest<ApplicationT, ModuleT>::TearDown()
{
    s_application->GetSerializeContext()->DestroyEditContext();
    s_systemEntity = nullptr;
    s_application->Destroy();
    s_application.reset();
}

template<class ApplicationT, class ModuleT>
AZStd::unique_ptr<typename ModuleReflectionTest<ApplicationT, ModuleT>::InternalApplication> ModuleReflectionTest<ApplicationT, ModuleT>::s_application;

template<class ApplicationT, class ModuleT>
AZ::Entity* ModuleReflectionTest<ApplicationT, ModuleT>::s_systemEntity = nullptr;

template<class ApplicationT, class ModuleT, class ObjectT>
void LoadReflectedObjectTest<ApplicationT, ModuleT, ObjectT>::SetUp()
{
    BaseType::SetUp();
    const char* buffer = GetSourceDataBuffer();
    if (buffer)
    {
        // don't load any assets referenced from the data
        AZ::ObjectStream::FilterDescriptor filter;
        filter.m_assetCB = AZ::Data::AssetFilterNoAssetLoading;

        m_object.reset(AZ::Utils::LoadObjectFromBuffer<ObjectT>(buffer, strlen(buffer) + 1, this->GetApplication()->GetSerializeContext(), filter));
    }
}

template<class ApplicationT, class ModuleT, class ObjectT>
void LoadReflectedObjectTest<ApplicationT, ModuleT, ObjectT>::TearDown()
{
    m_object.reset();
    BaseType::TearDown();
}

#ifdef LMBR_CENTRAL_EDITOR
template<class ComponentT>
void LoadEditorComponentTest<ComponentT>::SetUp()
{
    this->m_transformComponentDescriptor = DummyTransformComponent::CreateDescriptor();
    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::RegisterComponentDescriptor, this->m_transformComponentDescriptor);

    m_entity = AZStd::make_unique<AZ::Entity>("LoadEditorComponentTestEntity");
    m_entity->Init();

    LoadReflectedObjectTestBase::SetUp();
    m_entity->AddComponent(aznew DummyTransformComponent());
    if (this->m_object)
    {
        m_entity->AddComponent(this->m_object.get());
    }
    m_entity->Activate();
}

template<class ComponentT>
void LoadEditorComponentTest<ComponentT>::TearDown()
{
    m_entity->Deactivate();
    if (this->m_object)
    {
        m_entity->RemoveComponent(this->m_object.get());
    }
    LoadReflectedObjectTestBase::TearDown();
    m_entity.reset();

    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::UnregisterComponentDescriptor, this->m_transformComponentDescriptor);
    this->m_transformComponentDescriptor = nullptr;
}
#endif
