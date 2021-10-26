/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <CryCommon/Mocks/ISystemMock.h>
#include <LyShineBuilder/UiCanvasBuilderWorker.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <LyShineSystemComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <World/UiCanvasAssetRefComponent.h>
#include <World/UiCanvasProxyRefComponent.h>
#include <World/UiCanvasOnMeshComponent.h>
#include <UiCanvasComponent.h>
#include <UiElementComponent.h>
#include <UiTransform2dComponent.h>
#include <UiImageComponent.h>
#include <UiTextComponent.h>
#include <UiButtonComponent.h>
#include <UiCheckboxComponent.h>
#include <UiDraggableComponent.h>
#include <UiDropTargetComponent.h>
#include <UiDropdownOptionComponent.h>
#include <UiTextInputComponent.h>
#include <UiImageSequenceComponent.h>
#include <UiMarkupButtonComponent.h>
#include <UiScrollBoxComponent.h>
#include <UiDropdownComponent.h>
#include <UiSliderComponent.h>
#include <UiScrollBarComponent.h>
#include <UiFaderComponent.h>
#include <UiFlipbookAnimationComponent.h>
#include <UiLayoutFitterComponent.h>
#include <UiMaskComponent.h>
#include <UiLayoutCellComponent.h>
#include <UiLayoutRowComponent.h>
#include <UiLayoutGridComponent.h>
#include <UiTooltipComponent.h>
#include <UiTooltipDisplayComponent.h>
#include <UiDynamicLayoutComponent.h>
#include <UiDynamicScrollBoxComponent.h>
#include <UiSpawnerComponent.h>
#include <UiRadioButtonComponent.h>
#include <UiLayoutColumnComponent.h>
#include <UiRadioButtonGroupComponent.h>
#include <UiParticleEmitterComponent.h>
#include <UiCanvasManager.h>

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

using namespace AZ;
using ::testing::NiceMock;

class LyShineSystemTestComponent
    : public LyShine::LyShineSystemComponent
{
    friend class LyShineEditorTest;
};

class LyShineEditorTest
    : public ::testing::Test
{
protected:

    void SetUp() override
    {
        using namespace LyShine;

        m_priorEnv = gEnv;

        m_data = AZStd::make_unique<DataMembers>();
        memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
        m_data->m_stubEnv.pSystem = &m_data->m_mockSystem;
        gEnv = &m_data->m_stubEnv;

        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        auto projectPathKey =
            AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        registry->Set(projectPathKey, "AutomatedTesting");
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

        m_app.Start(m_descriptor);
        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        const AZStd::string engineRoot = AZ::Test::GetEngineRootPath();

        AZ::IO::FileIOBase::GetInstance()->SetAlias("@engroot@", engineRoot.c_str());

        AZ::IO::Path assetRoot(AZ::Utils::GetProjectPath());
        assetRoot /= "Cache";

        AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", assetRoot.c_str());

        AZ::SerializeContext* context = nullptr;
        ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);

        ASSERT_NE(context, nullptr);

        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(LyShineSystemComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCanvasAssetRefComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCanvasProxyRefComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCanvasOnMeshComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCanvasComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiElementComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTransform2dComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiImageComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiImageSequenceComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTextComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiButtonComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiMarkupButtonComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCheckboxComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDraggableComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDropTargetComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDropdownComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDropdownOptionComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiSliderComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTextInputComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiScrollBoxComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiScrollBarComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiFaderComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiFlipbookAnimationComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutFitterComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiMaskComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutCellComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutColumnComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutRowComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutGridComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTooltipComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTooltipDisplayComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDynamicLayoutComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDynamicScrollBoxComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiSpawnerComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiRadioButtonComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiRadioButtonGroupComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiParticleEmitterComponent::CreateDescriptor()));

        context->ClassDeprecate("SimpleAssetReference_MaterialAsset", "{B7B8ECC7-FF89-4A76-A50E-4C6CA2B6E6B4}",
            [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
        {
            AZStd::vector<AZ::SerializeContext::DataElementNode> childNodeElements;
            for (int index = 0; index < rootElement.GetNumSubElements(); ++index)
            {
                childNodeElements.push_back(rootElement.GetSubElement(index));
            }
            // Convert the rootElement now, the existing child DataElmentNodes are now removed
            rootElement.Convert<AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>>(context);
            for (AZ::SerializeContext::DataElementNode& childNodeElement : childNodeElements)
            {
                rootElement.AddElement(AZStd::move(childNodeElement));
            }
            return true;
        });
        context->ClassDeprecate("SimpleAssetReference_TextureAsset", "{68E92460-5C0C-4031-9620-6F1A08763243}",
            [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
        {
            AZStd::vector<AZ::SerializeContext::DataElementNode> childNodeElements;
            for (int index = 0; index < rootElement.GetNumSubElements(); ++index)
            {
                childNodeElements.push_back(rootElement.GetSubElement(index));
            }
            // Convert the rootElement now, the existing child DataElmentNodes are now removed
            rootElement.Convert<AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>>(context);
            for (AZ::SerializeContext::DataElementNode& childNodeElement : childNodeElements)
            {
                rootElement.AddElement(AZStd::move(childNodeElement));
            }
            return true;
        });
        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>::Register(*context);
        AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>::Register(*context);

        for(const auto& descriptor : m_componentDescriptors)
        {
            descriptor->Reflect(context);
        }

        m_sysComponent = AZStd::make_unique<LyShineSystemTestComponent>();
        m_sysComponent->Activate();
    }

    void TearDown() override
    {
        m_componentDescriptors = {};

        m_sysComponent->Deactivate();
        m_sysComponent = nullptr;
        m_app.Stop();

        m_data.reset();
        gEnv = m_priorEnv;
    }

    AZStd::unique_ptr<LyShineSystemTestComponent> m_sysComponent;
    AzToolsFramework::ToolsApplication m_app;
    AZ::ComponentApplication::Descriptor m_descriptor;
    AZStd::vector<AZStd::unique_ptr<AZ::ComponentDescriptor>> m_componentDescriptors;

    struct DataMembers
    {
        SSystemGlobalEnvironment m_stubEnv;
        NiceMock<SystemMock> m_mockSystem;
    };

    AZStd::unique_ptr<DataMembers> m_data;
    SSystemGlobalEnvironment* m_priorEnv = nullptr;
};

AZStd::string GetTestFileAliasedPath(AZStd::string_view fileName)
{
    constexpr char testFileFolder[] = "@engroot@/Gems/LyShine/Code/Tests/";
    return AZStd::string::format("%s%.*s", testFileFolder, aznumeric_cast<int>(fileName.size()), fileName.data());
}

AZStd::string GetTestFileFullPath(AZStd::string_view fileName)
{
    AZStd::string aliasedPath = GetTestFileAliasedPath(fileName);
    char resolvedPath[AZ_MAX_PATH_LEN];
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(aliasedPath.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
    return AZStd::string(resolvedPath);
}

bool OpenTestFile(AZStd::string_view fileName, AZ::IO::FileIOStream& fileStream)
{
    AZStd::string aliasedPath = GetTestFileAliasedPath(fileName);
    return fileStream.Open(aliasedPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
}

TEST_F(LyShineEditorTest, ProcessUiCanvas_ReturnsDependencyOnSpriteAndTexture)
{
    using namespace LyShine;
    using namespace AssetBuilderSDK;

    UiCanvasBuilderWorker worker;

    AZStd::vector<ProductDependency> productDependencies;
    ProductPathDependencySet productPathDependencySet;
    UiSystemToolsInterface::CanvasAssetHandle* canvasAsset = nullptr;
    AZ::Entity* sourceCanvasEntity = nullptr;
    AZ::Entity exportCanvasEntity;

    AZ::IO::FileIOStream stream;

    ASSERT_TRUE(OpenTestFile("1ImageReference.uicanvas", stream));
    ASSERT_TRUE(worker.ProcessUiCanvasAndGetDependencies(stream, productDependencies, productPathDependencySet, canvasAsset, sourceCanvasEntity, exportCanvasEntity));
    ASSERT_EQ(productDependencies.size(), 0);
    ASSERT_THAT(productPathDependencySet, testing::UnorderedElementsAre(
        ProductPathDependency{ "textures/defaults/grey.dds", ProductPathDependencyType::ProductFile },
        ProductPathDependency{ "textures/defaults/grey.sprite", ProductPathDependencyType::ProductFile }
    ));
}

TEST_F(LyShineEditorTest, FindLoadedCanvasByPathName_FT)
{
    UiCanvasManager canvasManager;

    //find loaded canvas, should return invalid id
    AZ::EntityId entityId = canvasManager.FindLoadedCanvasByPathName("@engroot@/Gems/LyShine/Code/Tests/TestAssets/Canvases/empty.uicanvas", false);
    EXPECT_FALSE(entityId.IsValid());

    //load a new canvas
    entityId = canvasManager.FindLoadedCanvasByPathName("@engroot@/Gems/LyShine/Code/Tests/TestAssets/Canvases/empty.uicanvas", true);
    EXPECT_TRUE(entityId.IsValid());
}
