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

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/WindowContext.h>

#include <AzTest/AzTest.h>

#include <Common/ErrorMessageFinder.h>
#include <Common/RPITestFixture.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::RPI;

    class ViewportContextTests : public RPITestFixture
    {
    protected:
        ViewportContextRequestsInterface* m_viewportContextManager;
        // Heap allocate our ref-tracked contexts to avoid a memory leak detection in the dtor
        AZStd::unique_ptr<AZStd::vector<ViewportContextPtr>> m_contexts;

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_contexts = AZStd::make_unique<AZStd::vector<ViewportContextPtr>>();
            m_viewportContextManager = ViewportContextRequests::Get();
        }

        void TearDown() override
        {
            m_contexts.reset();
            m_viewportContextManager = nullptr;

            RPITestFixture::TearDown();
        }

        void CreateSeveralViewportContexts()
        {
            constexpr int contextsToCreate = 5;
            for (int i = 0; i < contextsToCreate; ++i)
            {
                WindowContext window;
                ViewportContextRequestsInterface::CreationParameters params;
                params.device = RHI::RHISystemInterface::Get()->GetDevice();
                ViewportContextPtr viewportContext = m_viewportContextManager->CreateViewportContext(AZ::Name(), params);
                m_contexts->push_back(AZStd::move(viewportContext));
            }
        }

        ViewportContextPtr CreateDefaultViewportContext()
        {
            WindowContext window;
            ViewportContextRequestsInterface::CreationParameters params;
            params.device = RHI::RHISystemInterface::Get()->GetDevice();
            ViewportContextPtr viewportContext =
                m_viewportContextManager->CreateViewportContext(m_viewportContextManager->GetDefaultViewportContextName(), params);
            if (viewportContext)
            {
                m_contexts->push_back(viewportContext);
            }
            return viewportContext;
        }
    };

    TEST_F(ViewportContextTests, CreateAndRename)
    {
        AZ::Name defaultContextName = m_viewportContextManager->GetDefaultViewportContextName();
        CreateSeveralViewportContexts();

        // Attempt to rename each one to the default viewport context name, therefore making each the default viewport context
        ViewportContextPtr firstViewportContext = nullptr;
        ViewportContextPtr lastDefaultViewportContext = nullptr;
        AZ::Name lastViewportContextName;
        m_viewportContextManager->EnumerateViewportContexts(
            [&](ViewportContextPtr context)
            {
                if (!firstViewportContext)
                {
                    firstViewportContext = context;
                }
                if (lastDefaultViewportContext)
                {
                    m_viewportContextManager->RenameViewportContext(lastDefaultViewportContext, lastViewportContextName);
                }
                lastViewportContextName = context->GetName();
                lastDefaultViewportContext = context;
                m_viewportContextManager->RenameViewportContext(context, defaultContextName);

                EXPECT_EQ(context->GetName(), defaultContextName);
                // If we're using the default context name, we should be considered the default viewport context
                EXPECT_EQ(m_viewportContextManager->GetDefaultViewportContext(), context);
            });

        // Attempt to rename a context to an already existing name and verify that it fails
        AZ_TEST_START_TRACE_SUPPRESSION;
        AZ::Name nameBeforeAttemptedRename = firstViewportContext->GetName();
        m_viewportContextManager->RenameViewportContext(firstViewportContext, defaultContextName);
        EXPECT_EQ(firstViewportContext->GetName(), nameBeforeAttemptedRename);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        // Attempt to create a viewport context with the default name, which should also fail
        AZ_TEST_START_TRACE_SUPPRESSION;
        ViewportContextPtr invalidContext = CreateDefaultViewportContext();
        EXPECT_EQ(invalidContext, nullptr);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(ViewportContextTests, ViewStack)
    {
        CreateSeveralViewportContexts();
        ViewportContextPtr context = m_contexts->at(0);
        Name contextName = context->GetName();

        ViewPtr view1 = View::CreateView(Name("View 1"), View::UsageFlags::UsageCamera);
        ViewPtr view2 = View::CreateView(Name("View 2"), View::UsageFlags::UsageCamera);
        ViewPtr view3 = View::CreateView(Name("View 3"), View::UsageFlags::UsageCamera);

        // Pushing a view to the stack should make it the active view
        m_viewportContextManager->PushView(contextName, view1);
        EXPECT_EQ(m_viewportContextManager->GetCurrentView(contextName), view1);
        m_viewportContextManager->PushView(contextName, view2);
        EXPECT_EQ(m_viewportContextManager->GetCurrentView(contextName), view2);
        m_viewportContextManager->PushView(contextName, view3);
        EXPECT_EQ(m_viewportContextManager->GetCurrentView(contextName), view3);

        // Popping a non-active view should not affect the active view
        EXPECT_TRUE(m_viewportContextManager->PopView(contextName, view2));
        EXPECT_FALSE(m_viewportContextManager->PopView(contextName, view2));
        EXPECT_EQ(m_viewportContextManager->GetCurrentView(contextName), view3);

        // Popping the current view should make the most recently pushed view still on the stack active
        EXPECT_TRUE(m_viewportContextManager->PopView(contextName, view3));
        EXPECT_FALSE(m_viewportContextManager->PopView(contextName, view3));
        EXPECT_EQ(m_viewportContextManager->GetCurrentView(contextName), view1);
        EXPECT_EQ(context->GetDefaultView(), view1);

        // Other view stacks should be independent
        ViewportContextPtr otherContext = m_contexts->at(1);
        EXPECT_NE(otherContext->GetDefaultView(), view1);

        // Stacks are associated by name, so if we rename our context the view should change
        Name oldName = context->GetName();
        m_viewportContextManager->RenameViewportContext(context, Name("NewName"));
        EXPECT_NE(context->GetDefaultView(), view1);
        EXPECT_NE(otherContext->GetDefaultView(), view1);
        m_viewportContextManager->RenameViewportContext(otherContext, oldName);
        EXPECT_EQ(otherContext->GetDefaultView(), view1);

        // Stack pushing should continue to work with a new viewport context
        m_viewportContextManager->PushView(contextName, view3);
        EXPECT_EQ(m_viewportContextManager->GetCurrentView(contextName), view3);
        EXPECT_EQ(otherContext->GetDefaultView(), view3);
        EXPECT_NE(context->GetDefaultView(), view3);
    }

    template<class T>
    class EventParameters
    {
        EventParameters()
        {
            static_assert(false, "EventParameters specified for a non-AZ::Event");
        }
    };

    // Records all parameters from an AZ::Event callback by value
    template<class... Params>
    class EventParameters<AZ::Event<Params...>> : public AZStd::tuple<typename AZStd::remove_reference<Params>::type...>
    {
    public:
        EventParameters(Params... params)
            : AZStd::tuple<typename AZStd::remove_reference<Params>::type...>(params...)
        {
        }
    };

    // Records all signals for a given event and asserts if any are unaccounted for via Pop
    template<typename TEvent>
    class EventListener
    {
    public:
        typename TEvent::Handler m_handler;
        AZStd::deque<EventParameters<typename TEvent>> m_occurrences;

        EventListener()
        {
            m_handler = typename TEvent::Handler(
                [this](auto... params)
                {
                    m_occurrences.push_back(typename EventParameters<typename TEvent>(params...));
                });
        }

        ~EventListener()
        {
            // Ensure no unexpected events got logged
            EXPECT_EQ(m_occurrences.size(), 0);
        }

        auto Pop()
        {
            // Fail if there's no event
            EXPECT_NE(m_occurrences.size(), 0);
            auto value = m_occurrences.front();
            m_occurrences.pop_front();
            return value;
        }

        auto PopFirstValue()
        {
            return AZStd::get<0>(Pop());
        }
    };

    TEST_F(ViewportContextTests, Events)
    {
        auto viewportContext = CreateDefaultViewportContext();

        EventListener<ViewportContext::SizeChangedEvent> sizeChanged;
        EventListener<ViewportContext::MatrixChangedEvent> viewMatrixChanged;
        EventListener<ViewportContext::MatrixChangedEvent> projectionMatrixChanged;
        EventListener<ViewportContext::TransformChangedEvent> cameraTransformChanged;
        EventListener<ViewportContext::SceneChangedEvent> sceneChanged;
        EventListener<ViewportContext::PipelineChangedEvent> currentPipelineChanged;
        EventListener<ViewportContext::ViewChangedEvent> defaultViewChanged;
        EventListener<ViewportContext::ViewportIdEvent> aboutToBeDestroyed;

        viewportContext->ConnectSizeChangedHandler(sizeChanged.m_handler);
        viewportContext->ConnectViewMatrixChangedHandler(viewMatrixChanged.m_handler);
        viewportContext->ConnectProjectionMatrixChangedHandler(projectionMatrixChanged.m_handler);
        viewportContext->ConnectCameraTransformChangedHandler(cameraTransformChanged.m_handler);
        viewportContext->ConnectSceneChangedHandler(sceneChanged.m_handler);
        viewportContext->ConnectCurrentPipelineChangedHandler(currentPipelineChanged.m_handler);
        viewportContext->ConnectDefaultViewChangedHandler(defaultViewChanged.m_handler);
        viewportContext->ConnectAboutToBeDestroyedHandler(aboutToBeDestroyed.m_handler);

        // Check change events on SetCameraTransform
        AZ::Vector3 translation(10.f, 0.f, 0.f);
        viewportContext->SetCameraTransform(AZ::Transform(translation, AZ::Quaternion::CreateIdentity(), 1.0f));
        EXPECT_TRUE(cameraTransformChanged.PopFirstValue().GetTranslation().IsClose(translation));
        viewMatrixChanged.Pop();
        projectionMatrixChanged.Pop();

        // Check matrix events on SetCameraViewMatrix
        translation = AZ::Vector3(0.f, 10.f, -10.f);
        AZ::Matrix4x4 viewMatrix = AZ::Matrix4x4::CreateIdentity();
        viewMatrix.SetTranslation(translation);
        viewportContext->SetCameraViewMatrix(viewMatrix);
        EXPECT_TRUE(viewMatrixChanged.PopFirstValue().IsClose(viewMatrix));
        cameraTransformChanged.Pop();
        projectionMatrixChanged.Pop();

        // Check matrix events on directly calling SetWorldToView matrix on the default view
        translation = AZ::Vector3(5.f, 0.f, 0.f);
        viewMatrix.SetTranslation(translation);
        viewportContext->GetDefaultView()->SetWorldToViewMatrix(viewMatrix);
        EXPECT_TRUE(viewMatrixChanged.PopFirstValue().IsClose(viewMatrix));
        cameraTransformChanged.Pop();
        projectionMatrixChanged.Pop();

        // Test view change event (which also triggers the matrix change events)
        ViewPtr view1 = View::CreateView(Name("View 1"), View::UsageFlags::UsageCamera);
        ViewPtr view2 = View::CreateView(Name("View 2"), View::UsageFlags::UsageCamera);

        translation = AZ::Vector3(10.f, 100.f, 1000.f);
        viewMatrix.SetTranslation(translation);
        view1->SetWorldToViewMatrix(viewMatrix);

        m_viewportContextManager->PushView(viewportContext->GetName(), view1);
        EXPECT_EQ(defaultViewChanged.PopFirstValue(), view1);
        EXPECT_TRUE(viewMatrixChanged.PopFirstValue().IsClose(viewMatrix));
        cameraTransformChanged.Pop();
        projectionMatrixChanged.Pop();

        m_viewportContextManager->PushView(viewportContext->GetName(), view2);
        EXPECT_EQ(defaultViewChanged.PopFirstValue(), view2);
        EXPECT_TRUE(viewMatrixChanged.PopFirstValue().IsClose(view2->GetWorldToViewMatrix()));
        cameraTransformChanged.Pop();
        projectionMatrixChanged.Pop();

        m_viewportContextManager->PopView(viewportContext->GetName(), view2);
        EXPECT_EQ(defaultViewChanged.PopFirstValue(), view1);
        EXPECT_TRUE(viewMatrixChanged.PopFirstValue().IsClose(viewMatrix));
        cameraTransformChanged.Pop();
        projectionMatrixChanged.Pop();

        // Check matrix events on SetCameraProjectionMatrix
        viewportContext->SetCameraProjectionMatrix(AZ::Matrix4x4::CreateZero());
        EXPECT_TRUE(projectionMatrixChanged.PopFirstValue().IsClose(AZ::Matrix4x4::CreateZero()));

        // Check scene changed event (also will trigger currentPipelineChanged)
        SceneDescriptor sceneDescriptor;
        RPI::ScenePtr scene = RPI::Scene::CreateScene(sceneDescriptor);
        viewportContext->SetRenderScene(scene);
        EXPECT_EQ(sceneChanged.PopFirstValue(), scene);
        EXPECT_EQ(currentPipelineChanged.PopFirstValue(), scene->GetDefaultRenderPipeline());

        // Check window size changed
        AzFramework::WindowSize windowSize(800, 600);
        // Invoke OnWindowResized directly, broadcasting OnWindowResized can cause undesirable allocations
        viewportContext->OnWindowResized(windowSize.m_width, windowSize.m_height);
        AzFramework::WindowSize newSize = sizeChanged.PopFirstValue();
        EXPECT_EQ(newSize.m_width, windowSize.m_width);
        EXPECT_EQ(newSize.m_height, windowSize.m_height);

        // Check aboutToBeDestroyed
        // ViewportContext is ref counted, so release all refs to check the destruction signal
        viewportContext.reset();
        m_contexts->clear();
        aboutToBeDestroyed.Pop();
    }
} // namespace UnitTest
