/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>

#include <AzTest/AzTest.h>

#include <Common/RPITestFixture.h>
#include <Common/SerializeTester.h>
#include <Common/ErrorMessageFinder.h>

namespace UnitTest
{
    using namespace AZ;

    class RenderPipelineTests
        : public RPITestFixture
    {

    protected:

        // A test pass which only include view tag and draw list tag
        class TestPass
            : public AZ::RPI::ParentPass
        {
        public:

            AZ_RTTI(TestPass, "{2056532E-286F-454F-8659-15A289432A63}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(TestPass, AZ::SystemAllocator, 0);

            TestPass(const AZ::RPI::PassDescriptor& descriptor)
                : AZ::RPI::ParentPass(descriptor)
            { }

            void Initialize(const Name& drawListTagString, const AZ::RPI::PipelineViewTag& viewTag)
            {
                RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();
                m_drawListTag = drawListTagRegistry->AcquireTag(drawListTagString);
                m_viewTag = viewTag;
                m_flags.m_hasDrawListTag = true;
                m_flags.m_hasPipelineViewTag = true;
            }

            AZ::RHI::DrawListTag GetDrawListTag() const override
            {
                return m_drawListTag;
            }

            const AZ::RPI::PipelineViewTag& GetPipelineViewTag() const override
            {
                return m_viewTag;
            }

            virtual ~TestPass()
            {
                if (m_drawListTag.IsValid())
                {
                    RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();
                    drawListTagRegistry->ReleaseTag(m_drawListTag);
                }
            }

        private:
            AZ::RHI::DrawListTag m_drawListTag;
            AZ::RPI::PipelineViewTag m_viewTag;
        };
    };

    /**
     * Validate view and view tag related functions
     */
    TEST_F(RenderPipelineTests, ViewFunctionsTest)
    {
        using namespace AZ;

        RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();

        const RPI::PipelineViewTag viewTag1{ "viewTag1" };
        const RPI::PipelineViewTag viewTag2{ "viewTag2" };
        const Name drawListTagString1{ "drawListTag1" };
        const Name drawListTagString2{ "drawListTag2" };

        // Create render pipeline
        RPI::RenderPipelineDescriptor desc;
        desc.m_mainViewTagName = viewTag2.GetStringView();
        desc.m_name = "TestPipeline";
        RPI::RenderPipelinePtr pipeline = RPI::RenderPipeline::CreateRenderPipeline(desc);
        
        auto& rootPass = pipeline->GetRootPass();
        AZ::RPI::PassDescriptor passDescriptor;

        passDescriptor.m_passName = "TestPassA";
        RPI::Ptr<TestPass> testPassA = aznew TestPass(passDescriptor);
        testPassA->Initialize(drawListTagString1, viewTag1);

        passDescriptor.m_passName = "TestPassB";
        RPI::Ptr<TestPass> testPassB = aznew TestPass(passDescriptor);
        testPassB->Initialize(drawListTagString1, viewTag2);

        passDescriptor.m_passName = "TestPassC";
        RPI::Ptr<TestPass> testPassC = aznew TestPass(passDescriptor);
        testPassC->Initialize(drawListTagString2, viewTag2);

        EXPECT_TRUE(testPassA->HasDrawListTag());
        EXPECT_TRUE(testPassB->HasDrawListTag());
        EXPECT_TRUE(testPassC->HasDrawListTag());

        RHI::DrawListTag drawListTag1 = drawListTagRegistry->FindTag(drawListTagString1);
        RHI::DrawListTag drawListTag2 = drawListTagRegistry->FindTag(drawListTagString2);

        EXPECT_TRUE(testPassA->GetDrawListTag() == drawListTag1);
        EXPECT_TRUE(testPassB->GetDrawListTag() == drawListTag1);
        EXPECT_TRUE(testPassC->GetDrawListTag() == drawListTag2);

        rootPass->AddChild(testPassA);
        testPassA->AddChild(testPassB);
        testPassA->AddChild(testPassC);

        pipeline->OnPassModified();

        EXPECT_TRUE(pipeline->HasViewTag(viewTag1));

        auto& views = pipeline->GetViews(viewTag2);

        EXPECT_TRUE(views.size() == 0);

        RHI::DrawListMask drawListMask;
        RPI::PassesByDrawList passesByDrawList;
        // viewTag1 only associates with drawListTag1
        rootPass->GetViewDrawListInfo(drawListMask, passesByDrawList, viewTag1);
        EXPECT_TRUE(drawListMask[drawListTag1.GetIndex()] == true);
        EXPECT_TRUE(drawListMask[drawListTag2.GetIndex()] == false);

        rootPass->GetViewDrawListInfo(drawListMask, passesByDrawList, viewTag2);
        // viewTag2 associates with drawlistTag1 and drawListTag2
        EXPECT_TRUE(drawListMask[drawListTag1.GetIndex()] == true);
        EXPECT_TRUE(drawListMask[drawListTag2.GetIndex()] == true);

        // View functions
        RPI::ViewPtr view1 = RPI::View::CreateView(AZ::Name("testViewA"), RPI::View::UsageCamera);
        RPI::ViewPtr view2 = RPI::View::CreateView(AZ::Name("testViewB"), RPI::View::UsageCamera);

        // Persistent view
        pipeline->SetPersistentView(viewTag1, view1);
        EXPECT_TRUE(pipeline->GetViews(viewTag1).size() == 1);
        pipeline->SetPersistentView(viewTag1, view2);
        auto& viewsFromTag1 = pipeline->GetViews(viewTag1);
        EXPECT_TRUE(viewsFromTag1.size() == 1);
        EXPECT_TRUE(view2 == viewsFromTag1[0]);

        // Try to add a transient view to view tag was associated with persistent view. 
        AZ_TEST_START_ASSERTTEST;
        pipeline->AddTransientView(viewTag1, view1);
        AZ_TEST_STOP_ASSERTTEST(1); 
        EXPECT_TRUE(pipeline->GetViews(viewTag1).size() == 1);

        // Transient view
        pipeline->AddTransientView(viewTag2, view1);
        pipeline->AddTransientView(viewTag2, view2);
        auto& viewsFromTag2 = pipeline->GetViews(viewTag2);
        EXPECT_TRUE(viewsFromTag2.size() == 2);
    }

}  // namespace UnitTest
