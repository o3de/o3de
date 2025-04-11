/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/AzTest.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

// Test that editor-components wrapped within a GenericComponentWrapper
// are moved out of the wrapper when a slice is loaded.
const char kWrappedEditorComponent[] =
R"DELIMITER(<ObjectStream version="1">
    <Class name="SliceComponent" field="element" version="1" type="{AFD304E4-1773-47C8-855A-8B622398934F}">
        <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
            <Class name="AZ::u64" field="Id" value="7737200995084371546" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        </Class>
        <Class name="AZStd::vector" field="Entities" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}">
            <Class name="AZ::Entity" field="element" version="2" type="{75651658-8663-478D-9090-2432DFCAFA44}">
                <Class name="EntityId" field="Id" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                    <Class name="AZ::u64" field="id" value="16119032733109672753" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
                <Class name="AZStd::string" field="Name" value="RigidPhysicsMesh" type="{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}"/>
                <Class name="bool" field="IsDependencyReady" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                <Class name="AZStd::vector" field="Components" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}">
                    <Class name="GenericComponentWrapper" field="element" type="{68D358CA-89B9-4730-8BA6-E181DEA28FDE}">
                        <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                            <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                                <Class name="AZ::u64" field="Id" value="11874523501682509824" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                            </Class>
                        </Class>
                        <Class name="SelectionComponent" field="m_template" type="{A7CBE7BC-9B4A-47DC-962F-1BFAE85DBF3A}">
                            <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                                <Class name="AZ::u64" field="Id" value="0" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                            </Class>
                        </Class>
                    </Class>
                </Class>
            </Class>
        </Class>
        <Class name="AZStd::list" field="Prefabs" type="{B845AD64-B5A0-4CCD-A86B-3477A36779BE}"/>
        <Class name="bool" field="IsDynamic" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
    </Class>
</ObjectStream>)DELIMITER";

class WrappedEditorComponentTest
    : public UnitTest::LeakDetectionFixture
{
protected:
    void SetUp() override
    {
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        auto projectPathKey =
            AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        AZ::IO::FixedMaxPath enginePath;
        registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        m_app.Start(AZ::ComponentApplication::Descriptor(), startupParameters);

        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_slice.reset(AZ::Utils::LoadObjectFromBuffer<AZ::SliceComponent>(kWrappedEditorComponent, strlen(kWrappedEditorComponent) + 1));
        if (m_slice)
        {
            if (m_slice->GetNewEntities().size() > 0)
            {
                m_entityFromSlice = m_slice->GetNewEntities()[0];
                if (m_entityFromSlice)
                {
                    if (m_entityFromSlice->GetComponents().size() > 0)
                    {
                        m_componentFromSlice = m_entityFromSlice->GetComponents()[0];
                    }
                }
            }
        }
    }

    void TearDown() override
    {
        m_slice.reset();

        m_app.Stop();
    }

    AzToolsFramework::ToolsApplication m_app;
    AZStd::unique_ptr<AZ::SliceComponent> m_slice;
    AZ::Entity* m_entityFromSlice = nullptr;
    AZ::Component* m_componentFromSlice = nullptr;
};

TEST_F(WrappedEditorComponentTest, Slice_Loaded)
{
    EXPECT_NE(m_slice.get(), nullptr);
}

TEST_F(WrappedEditorComponentTest, EntityFromSlice_Exists)
{
    EXPECT_NE(m_entityFromSlice, nullptr);
}

TEST_F(WrappedEditorComponentTest, ComponentFromSlice_Exists)
{
    EXPECT_NE(m_componentFromSlice, nullptr);
}

TEST_F(WrappedEditorComponentTest, Component_IsNotGenericComponentWrapper)
{
    EXPECT_EQ(azrtti_cast<AzToolsFramework::Components::GenericComponentWrapper*>(m_componentFromSlice), nullptr);
}

// The swapped component should have adopted the GenericComponentWrapper's ComponentId.
TEST_F(WrappedEditorComponentTest, ComponentId_MatchesWrapperId)
{
    EXPECT_EQ(m_componentFromSlice->GetId(), 11874523501682509824u);
}

static constexpr AZ::TypeId InGameOnlyComponentTypeId{ "{1D538623-2052-464F-B0DA-D000E1520333}" };
class InGameOnlyComponent
    : public AZ::Component
{
public:
    AZ_COMPONENT(InGameOnlyComponent, InGameOnlyComponentTypeId);

    void Activate() override {}
    void Deactivate() override {}

    static void Reflect(AZ::ReflectContext* reflection)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<InGameOnlyComponent, AZ::Component>();
            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<InGameOnlyComponent>("InGame Only", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
            }
        }
    }
};

static constexpr AZ::Uuid NoneEditorComponentTypeId{ "{AE3454BA-D785-4EE2-A55B-A089F2B2916A}" };
class NoneEditorComponent
    : public AZ::Component
{
public:
    AZ_COMPONENT(NoneEditorComponent, NoneEditorComponentTypeId);

    void Activate() override {}
    void Deactivate() override {}

    static void Reflect(AZ::ReflectContext* reflection)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<NoneEditorComponent, AZ::Component>();
            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NoneEditorComponent>("None Editor", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
            }
        }
    }
};

class FindWrappedComponentsTest
    : public UnitTest::LeakDetectionFixture
{
public:
    void SetUp() override
    {
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        auto projectPathKey =
            AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        AZ::IO::FixedMaxPath enginePath;
        registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        m_app.Start(AzFramework::Application::Descriptor(), startupParameters);

        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_app.RegisterComponentDescriptor(InGameOnlyComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(NoneEditorComponent::CreateDescriptor());

        m_entity = new AZ::Entity("Entity1");

        AZ::Component* inGameOnlyComponent = nullptr;
        AZ::ComponentDescriptorBus::EventResult(inGameOnlyComponent, InGameOnlyComponentTypeId, &AZ::ComponentDescriptorBus::Events::CreateComponent);
        AZ::Component* genericComponent0 = aznew AzToolsFramework::Components::GenericComponentWrapper(inGameOnlyComponent);
        m_entity->AddComponent(genericComponent0);

        AZ::Component* noneEditorComponent = nullptr;
        AZ::ComponentDescriptorBus::EventResult(noneEditorComponent, NoneEditorComponentTypeId, &AZ::ComponentDescriptorBus::Events::CreateComponent);
        AZ::Component* genericComponent1 = aznew AzToolsFramework::Components::GenericComponentWrapper(noneEditorComponent);
        m_entity->AddComponent(genericComponent1);

        m_entity->Init();
    }

    void TearDown() override
    {
        m_app.Stop();
    }

    AzToolsFramework::ToolsApplication m_app;

    AZ::Entity* m_entity = nullptr;
};

TEST_F(FindWrappedComponentsTest, found)
{
    InGameOnlyComponent* ingameOnlyComponent = AzToolsFramework::FindWrappedComponentForEntity<InGameOnlyComponent>(m_entity);
    EXPECT_NE(ingameOnlyComponent, nullptr);

    NoneEditorComponent* noneEditorComponent = AzToolsFramework::FindWrappedComponentForEntity<NoneEditorComponent>(m_entity);
    EXPECT_NE(noneEditorComponent, nullptr);
}

