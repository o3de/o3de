/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

#include <AzFramework/Visibility/OctreeSystemComponent.h>

#include <AzTest/AzTest.h>

#include <Common/RPITestFixture.h>
#include <Common/SerializeTester.h>
#include <Common/ErrorMessageFinder.h>
#include <Common/TestFeatureProcessors.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::RPI;

    class SceneTests
        : public RPITestFixture
    {
    protected:
        AzFramework::OctreeSystemComponent* m_octreeSystemComponent;

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_octreeSystemComponent = new AzFramework::OctreeSystemComponent;

            TestFeatureProcessor1::Reflect(GetSerializeContext());
            TestFeatureProcessor2::Reflect(GetSerializeContext());
            TestFeatureProcessorImplementation::Reflect(GetSerializeContext());
            TestFeatureProcessorImplementation2::Reflect(GetSerializeContext());

            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<TestFeatureProcessor1>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<TestFeatureProcessor2>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<TestFeatureProcessorImplementation, TestFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<TestFeatureProcessorImplementation2, TestFeatureProcessorInterface>();
        }

        void TearDown() override
        {
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessor1>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessor2>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessorImplementation>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessorImplementation2>();

            delete m_octreeSystemComponent;

            RPITestFixture::TearDown();
        }
    };

    /**
    * Unit test to test RPI::Scene's feature processor management functions
    */
    TEST_F(SceneTests, FeatureProcessorManagement)
    {
        AZ::Test::ScopedAutoTempDirectory tempDirectory;
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@user@", tempDirectory.GetDirectory());

        // Create scene with two test feature processors
        SceneDescriptor sceneDesc;
        sceneDesc.m_featureProcessorNames.push_back(AZStd::string(TestFeatureProcessor1::RTTI_TypeName()));
        ScenePtr testScene = Scene::CreateScene(sceneDesc);

        EXPECT_TRUE(testScene->GetFeatureProcessor(FeatureProcessorId{ TestFeatureProcessor1::RTTI_TypeName() }) != nullptr);
        EXPECT_TRUE(testScene->GetFeatureProcessor(FeatureProcessorId{ TestFeatureProcessor2::RTTI_TypeName() }) == nullptr);

        testScene->DisableAllFeatureProcessors();

        EXPECT_TRUE(testScene->GetFeatureProcessor(FeatureProcessorId{ TestFeatureProcessor1::RTTI_TypeName() }) == nullptr);
                
        // Enable feature processors
        FeatureProcessor* testFeature1 = testScene->EnableFeatureProcessor(FeatureProcessorId{ TestFeatureProcessor1::RTTI_TypeName() });
        EXPECT_TRUE(testFeature1 != nullptr);

        TestFeatureProcessor2* testFeature2 = testScene->EnableFeatureProcessor<TestFeatureProcessor2>();
        EXPECT_TRUE(testFeature2 != nullptr);

        testScene->DisableFeatureProcessor< TestFeatureProcessor1>();
        EXPECT_TRUE(testScene->GetFeatureProcessor(FeatureProcessorId{ TestFeatureProcessor1::RTTI_TypeName() }) == nullptr);

        testScene->DisableFeatureProcessor(FeatureProcessorId{ testFeature2->RTTI_GetTypeName() });
        EXPECT_TRUE(testScene->GetFeatureProcessor< TestFeatureProcessor2>() == nullptr);
    }

    /**
     * Unit test to test RPI::Scene's render pipeline management functions
     */
    TEST_F(SceneTests, RenderPipelineManagement)
    {
        RenderPipelineId pipelineId1 = RenderPipelineId("pipeline1");
        RenderPipelineId pipelineId2 = RenderPipelineId("pipeline2");

        RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.m_name = pipelineId1.GetCStr();
        RenderPipelinePtr pipeline1 = RenderPipeline::CreateRenderPipeline(pipelineDesc);
        RenderPipelinePtr pipeline2 = RenderPipeline::CreateRenderPipeline(pipelineDesc);

        SceneDescriptor sceneDesc;
        ScenePtr testScene = Scene::CreateScene(sceneDesc);

        testScene->AddRenderPipeline(pipeline1);
        EXPECT_TRUE(testScene->GetRenderPipeline(pipelineId1) == pipeline1);
        EXPECT_TRUE(testScene->GetRenderPipeline(pipelineId2) == nullptr);

        // Asset false with pipeline have same name. And the pipeline won't be added
        AZ_TEST_START_ASSERTTEST;
        testScene->AddRenderPipeline(pipeline2);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_TRUE(testScene->GetRenderPipeline(pipelineId2) == nullptr);

        pipelineDesc.m_name = pipelineId2.GetCStr();
        pipeline2 = RenderPipeline::CreateRenderPipeline(pipelineDesc);
        testScene->AddRenderPipeline(pipeline2);
        EXPECT_TRUE(testScene->GetRenderPipeline(pipelineId2) != nullptr);

        EXPECT_TRUE(pipeline2->GetId() == pipelineId2);
        EXPECT_TRUE(testScene->GetRenderPipeline(pipelineId2) == pipeline2);
        
        // create another pipeline with same name as pipeline2
        RenderPipelinePtr pipeline3 = RenderPipeline::CreateRenderPipeline(pipelineDesc);

        AZ_TEST_START_ASSERTTEST;
        pipeline3->RemoveFromScene();
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_TRUE(testScene->GetRenderPipeline(pipelineId2) == pipeline2);

        pipeline2->RemoveFromScene();
        EXPECT_TRUE(testScene->GetRenderPipeline(pipelineId2) == nullptr);
    }
 
    TEST_F(SceneTests, SceneNotificationTest)
    {
        // Create scene with feature processor which enables scene notification handler
        SceneDescriptor sceneDesc;
        sceneDesc.m_featureProcessorNames.push_back(AZStd::string(TestFeatureProcessor1::RTTI_TypeName()));
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();
        auto feature = testScene->GetFeatureProcessor< TestFeatureProcessor1>();

        RenderPipelineId pipelineId1 = RenderPipelineId("pipeline1");
        RenderPipelineId pipelineId2 = RenderPipelineId("pipeline2");

        RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.m_name = pipelineId1.GetCStr();
        RenderPipelinePtr pipeline1 = RenderPipeline::CreateRenderPipeline(pipelineDesc);
        pipelineDesc.m_name = pipelineId2.GetCStr();
        RenderPipelinePtr pipeline2 = RenderPipeline::CreateRenderPipeline(pipelineDesc);

        // Add one render pipeline
        testScene->AddRenderPipeline(pipeline1);
        EXPECT_TRUE(feature->m_pipelineCount == 1);
        EXPECT_TRUE(pipeline1.get() == feature->m_lastPipeline);

        // Add another render pipeline
        testScene->AddRenderPipeline(pipeline2);
        EXPECT_TRUE(pipeline2.get() == feature->m_lastPipeline);
        EXPECT_TRUE(feature->m_pipelineCount == 2);

        // Remove the first render pipeline which was added
        testScene->RemoveRenderPipeline(pipelineId1);
        EXPECT_TRUE(feature->m_pipelineCount == 1);
        EXPECT_TRUE(pipeline1.get() == feature->m_lastPipeline);

        // Create a raster pass with viewTag "mainCamera"
        const RPI::PipelineViewTag viewTag{ "mainCamera" };
        PassDescriptor passDesc;
        AZStd::shared_ptr<PassTemplate> passTemplate = AZStd::make_shared<PassTemplate>();
        AZStd::shared_ptr<RasterPassData> passData = AZStd::make_shared<RasterPassData>();
        passData->m_drawListTag = "forward";
        passData->m_pipelineViewTag = viewTag.GetCStr();
        passTemplate->m_passData = passData;
        passDesc.m_passName = "raster";
        passDesc.m_passTemplate = passTemplate;
        Ptr<RasterPass> newPass = RasterPass::Create(passDesc);

        // Add new pass to render pipeline which should trigger render pipeline pass modify notification
        pipeline2->GetRootPass()->AddChild(newPass, true);
        // This function is called in every RPISystem render tick. We call it manually here to trigger the pass change notification
        pipeline2->UpdatePasses();
        EXPECT_TRUE(feature->m_pipelineChangedCount == 1);
        EXPECT_TRUE(pipeline2.get() == feature->m_lastPipeline);

        ViewPtr view = View::CreateView(AZ::Name("TestView"), RPI::View::UsageCamera);
        pipeline2->SetPersistentView(viewTag, view);
        EXPECT_TRUE(feature->m_viewSetCount == 1);
        pipeline2->SetPersistentView(viewTag, nullptr);
        EXPECT_TRUE(feature->m_viewSetCount == 2);

        testScene->Deactivate();
    }

    TEST_F(SceneTests, SceneNotificationTest_ConnectAfterRenderPipelineAdded)
    {
        // Create scene with feature processor which enables scene notification handler
        SceneDescriptor sceneDesc;
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();

        RenderPipelineId pipelineId1 = RenderPipelineId("pipeline1");
        RenderPipelineId pipelineId2 = RenderPipelineId("pipeline2");

        RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.m_name = pipelineId1.GetCStr();
        RenderPipelinePtr pipeline1 = RenderPipeline::CreateRenderPipeline(pipelineDesc);
        pipelineDesc.m_name = pipelineId2.GetCStr();
        RenderPipelinePtr pipeline2 = RenderPipeline::CreateRenderPipeline(pipelineDesc);

        // Do some render pipeline operations and keep render pipeline1 added
        testScene->AddRenderPipeline(pipeline1);
        testScene->AddRenderPipeline(pipeline2);
        testScene->RemoveRenderPipeline(pipelineId2);

        // Create a raster pass with viewTag "mainCamera"
        const RPI::PipelineViewTag viewTag{ "mainCamera" };
        PassDescriptor passDesc;
        AZStd::shared_ptr<PassTemplate> passTemplate = AZStd::make_shared<PassTemplate>();
        AZStd::shared_ptr<RasterPassData> passData = AZStd::make_shared<RasterPassData>();
        passData->m_drawListTag = "forward";
        passData->m_pipelineViewTag = viewTag.GetCStr();
        passTemplate->m_passData = passData;
        passDesc.m_passName = "raster";
        passDesc.m_passTemplate = passTemplate;
        Ptr<RasterPass> newPass = RasterPass::Create(passDesc);

        // Add new pass to render pipeline which should trigger render pipeline pass modify notification
        pipeline1->GetRootPass()->AddChild(newPass, true);
        // This function is called in every RPISystem render tick. We call it manually here to trigger the pass change notification
        pipeline1->UpdatePasses();
        // Set view to pipeline
        ViewPtr view = View::CreateView(AZ::Name("TestView"), RPI::View::UsageCamera);
        pipeline1->SetPersistentView(viewTag, view);

        // Enable the feature processor which has notification handler enabled
        TestFeatureProcessor1* feature = testScene->EnableFeatureProcessor<TestFeatureProcessor1>();
        EXPECT_TRUE(feature->m_pipelineCount == 1);
        EXPECT_TRUE(feature->m_lastPipeline == pipeline1.get());
        EXPECT_TRUE(feature->m_viewSetCount == 1);
        EXPECT_TRUE(feature->m_pipelineChangedCount == 0);

        testScene->Deactivate();
    }


    TEST_F(SceneTests, GetFeatureProcessor_ByInterface_CanModifyFeatureProcessorViaInterface)
    {
        // Create scene with one test feature processor
        SceneDescriptor sceneDesc;
        sceneDesc.m_featureProcessorNames.push_back(AZStd::string(TestFeatureProcessorImplementation::RTTI_TypeName()));
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();
        
        TestFeatureProcessorInterface* featureProcessorInterface = testScene->GetFeatureProcessor<TestFeatureProcessorInterface>();
        EXPECT_TRUE(featureProcessorInterface != nullptr);

        // Check that the pointer is valid
        int testValue = 7;
        featureProcessorInterface->SetValue(testValue);
        EXPECT_EQ(featureProcessorInterface->GetValue(), testValue);
        
        // Check that changes made by the interface apply to the underlying implemented feature processor
        TestFeatureProcessorImplementation* featureProcessorImplementation = testScene->GetFeatureProcessor<TestFeatureProcessorImplementation>();
        EXPECT_TRUE(featureProcessorInterface == featureProcessorImplementation);
        EXPECT_TRUE(featureProcessorImplementation->GetValue() == testValue);

        // Check that changes made by the implementation apply to the interface
        int anotherTestValue = 21;
        featureProcessorImplementation->SetValue(anotherTestValue);
        EXPECT_TRUE(featureProcessorInterface->GetValue() == anotherTestValue);

        // Check that the feature processor can be disabled
        testScene->DisableFeatureProcessor<TestFeatureProcessorImplementation>();
        EXPECT_TRUE(testScene->GetFeatureProcessor<TestFeatureProcessorInterface>() == nullptr);
        EXPECT_TRUE(testScene->GetFeatureProcessor<TestFeatureProcessorImplementation>() == nullptr);

        testScene->Deactivate();
    }

    TEST_F(SceneTests, GetFeatureProcessorByNameId_UsingStringForFeatureProcessorId_ReturnsValidFeatureProcessor)
    {
        // Create scene with one test feature processor
        SceneDescriptor sceneDesc;
        sceneDesc.m_featureProcessorNames.push_back(AZStd::string(TestFeatureProcessor1::RTTI_TypeName()));
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();

        EXPECT_TRUE(testScene->GetFeatureProcessor(FeatureProcessorId{ "TestFeatureProcessor1" }) != nullptr);

        testScene->Deactivate();
    }

    //
    // Two implementations of the same interface
    //
    TEST_F(SceneTests, EnableDisableFeatureProcessorByNameId_MultipleImplmentationsOfTheSameInterface_ReturnsValidFeatureProcessor)
    {
        SceneDescriptor sceneDesc;
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();

        // You can enable either implementation, as long as they are not both active in the same scene
        FeatureProcessor* firstImplementation = testScene->EnableFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation::RTTI_TypeName() });
        EXPECT_TRUE(firstImplementation != nullptr);
        testScene->DisableFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation::RTTI_TypeName() });

        FeatureProcessor* secondImplementation = testScene->EnableFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation2::RTTI_TypeName() });
        EXPECT_TRUE(secondImplementation != nullptr);

        testScene->Deactivate();
    }

    TEST_F(SceneTests, EnableDisableFeatureProcessorByType_MultipleImplmentationsOfTheSameInterface_ReturnsValidFeatureProcessor)
    {
        SceneDescriptor sceneDesc;
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();

        // You can enable either implementation, as long as they are not both active in the same scene
        FeatureProcessor* firstImplementation = testScene->EnableFeatureProcessor<TestFeatureProcessorImplementation>();
        EXPECT_TRUE(firstImplementation != nullptr);
        testScene->DisableFeatureProcessor<TestFeatureProcessorImplementation>();

        FeatureProcessor* secondImplementation = testScene->EnableFeatureProcessor<TestFeatureProcessorImplementation2>();
        EXPECT_TRUE(secondImplementation != nullptr);

        testScene->Deactivate();
    }

    TEST_F(SceneTests, GetFeatureProcessorByNameId_MultipleImplmentationsOfTheSameInterface_ReturnsValidFeatureProcessor)
    {
        SceneDescriptor sceneDesc;
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();

        // You can get a feature processor via its name ID, no matter which implementation is enabled by the scene
        FeatureProcessor* firstImplementation = testScene->EnableFeatureProcessor<TestFeatureProcessorImplementation>();
        FeatureProcessor* featureProcessor = testScene->GetFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation::RTTI_TypeName() });
        EXPECT_TRUE(firstImplementation == featureProcessor);
        testScene->DisableFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation::RTTI_TypeName() });

        FeatureProcessor* secondImplementation = testScene->EnableFeatureProcessor<TestFeatureProcessorImplementation2>();
        featureProcessor = testScene->GetFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation2::RTTI_TypeName() });
        EXPECT_TRUE(secondImplementation == featureProcessor);

        testScene->Deactivate();
    }

    TEST_F(SceneTests, GetFeatureProcessorByInterface_MultipleImplmentationsOfTheSameInterface_ReturnsValidFeatureProcessor)
    {
        SceneDescriptor sceneDesc;
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();

        // You can get a feature processor via its interface, no matter which implementation is enabled by the scene
        // as long as only one is enabled by the scene
        FeatureProcessor* firstImplementation = testScene->EnableFeatureProcessor<TestFeatureProcessorImplementation>();
        TestFeatureProcessorInterface* featureProcessorInterface = testScene->GetFeatureProcessor<TestFeatureProcessorInterface>();
        EXPECT_TRUE(firstImplementation == featureProcessorInterface);
        testScene->DisableFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation::RTTI_TypeName() });

        FeatureProcessor* secondImplementation = testScene->EnableFeatureProcessor<TestFeatureProcessorImplementation2>();

        featureProcessorInterface = testScene->GetFeatureProcessor<TestFeatureProcessorInterface>();
        EXPECT_TRUE(secondImplementation == featureProcessorInterface);

        testScene->Deactivate();
    }    

    //
    // Invalid cases
    //
    TEST_F(SceneTests, EnableFeatureProcessor_ByInterfaceName_FailsToEnable)
    {
        SceneDescriptor sceneDesc;
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();

        EXPECT_TRUE(testScene->GetFeatureProcessor<TestFeatureProcessorImplementation>() == nullptr);
        EXPECT_TRUE(testScene->GetFeatureProcessor<TestFeatureProcessorInterface>() == nullptr);

        testScene->Deactivate();
    }

    TEST_F(SceneTests, DisableFeatureProcessor_ByInterface_FailsToDisable)
    {
        SceneDescriptor sceneDesc;
        sceneDesc.m_featureProcessorNames.push_back(AZStd::string(TestFeatureProcessorImplementation::RTTI_TypeName()));
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();

        testScene->DisableFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorInterface::RTTI_TypeName() });
        EXPECT_TRUE(testScene->GetFeatureProcessor<TestFeatureProcessorImplementation>() != nullptr);
        EXPECT_TRUE(testScene->GetFeatureProcessor<TestFeatureProcessorInterface>() != nullptr);

        testScene->Deactivate();
    }

    TEST_F(SceneTests, EnableFeatureProcessor_MultipleImplmentationsOfTheSameInterface_FailsToEnable)
    {
        SceneDescriptor sceneDesc;
        ScenePtr testScene = Scene::CreateScene(sceneDesc);
        testScene->Activate();

        // You can enable one implementation of a feature processor on a scene
        FeatureProcessor* firstImplementation = testScene->EnableFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation::RTTI_TypeName() });
        EXPECT_TRUE(firstImplementation != nullptr);

        // But you can't enable two implementations of the same interface in one scene at the same time.
        // Otherwise, when you get a feature processor by its interface, the scene wouldn't know which one to return.
        AZ_TEST_START_ASSERTTEST;
        FeatureProcessor* secondImplementation = testScene->EnableFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation2::RTTI_TypeName() });
        AZ_TEST_STOP_ASSERTTEST(1);
        
        // If another implementation that uses the same interface exists, that will be the feature processor that is returned
        EXPECT_TRUE(secondImplementation == firstImplementation);

        testScene->Deactivate();
    }

}  // namespace UnitTest
