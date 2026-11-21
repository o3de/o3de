/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Editor/GraphAutomation.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Variable/GraphVariable.h>

namespace ScriptCanvasEditor
{
    class GraphAutomationTests : public UnitTest::LeakDetectionFixture
    {
    protected:
        //! Assigns value the way SetValue and SetVariable do.
        static AutomationResult Assign(ScriptCanvas::ModifiableDatumView& view, const AZStd::any& value)
        {
            return GraphAutomation::AssignValue(view, GraphAutomation::CopyDatumValue(value));
        }

        static GraphAutomation::PaletteChoices Choices(AZStd::unique_ptr<GraphCanvas::GraphCanvasMimeEvent> event)
        {
            return GraphAutomation::ExpandPaletteChoices(AZStd::move(event));
        }
    };

    //! A node factory that counts its destruction.
    class ChoiceLeaf final : public GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        explicit ChoiceLeaf(int& destroyed)
            : m_destroyed(destroyed)
        {
        }

        ~ChoiceLeaf() override
        {
            ++m_destroyed;
        }

        bool ExecuteEvent(const AZ::Vector2&, AZ::Vector2&, const AZ::EntityId&) override
        {
            return false;
        }

    private:
        int& m_destroyed;
    };

    //! A multiple-choice palette event offering two ChoiceLeaf factories. Fails the test if it is executed.
    class ChoiceMenu final : public MultiCreateNodeMimeEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(ChoiceMenu, AZ::SystemAllocator);
        AZ_RTTI(ChoiceMenu, "{72D9CF14-7212-48CD-9A45-AED8BF06BA26}", MultiCreateNodeMimeEvent);

        ChoiceMenu(int& wrapperDestroyed, int& leafDestroyed)
            : m_wrapper(wrapperDestroyed)
            , m_leaf(leafDestroyed)
        {
        }

        ~ChoiceMenu() override
        {
            ++m_wrapper;
        }

        bool ExecuteEvent(const AZ::Vector2&, AZ::Vector2&, const AZ::EntityId&) override
        {
            ADD_FAILURE();
            return false;
        }

        NodeIdPair ConstructNode(const AZ::EntityId&, const AZ::Vector2&) override
        {
            ADD_FAILURE();
            return {};
        }

        AZStd::vector<GraphCanvas::GraphCanvasMimeEvent*> CreateMimeEvents() const override
        {
            return { aznew ChoiceLeaf(m_leaf), aznew ChoiceLeaf(m_leaf) };
        }

    private:
        int& m_wrapper;
        int& m_leaf;
    };

    TEST_F(GraphAutomationTests, PaletteExpandsNativeChoicesWithoutExecutingTheMenuAndOwnsEveryFactory)
    {
        int wrappers = 0;
        int leaves = 0;
        {
            auto choices = Choices(AZStd::make_unique<ChoiceMenu>(wrappers, leaves));
            ASSERT_EQ(choices.size(), 2);
            EXPECT_EQ(wrappers, 1);
            EXPECT_EQ(leaves, 0);
            EXPECT_FALSE(azrtti_istypeof<MultiCreateNodeMimeEvent>(choices[0].get()));
        }

        EXPECT_EQ(leaves, 2);
        EXPECT_TRUE(Choices(nullptr).empty());
    }

    TEST_F(GraphAutomationTests, ReflectionRetainsNativeStringGraphIdsAndAllDocumentOperations)
    {
        AZ::SerializeContext serialize;
        AZ::BehaviorContext behavior;
        GraphAutomation::Reflect(&serialize);
        GraphAutomation::Reflect(&behavior);

        const auto found = behavior.m_ebuses.find("GraphAutomationRequestBus");
        ASSERT_NE(found, behavior.m_ebuses.end());

        const char* const events[] = {
            "Create",
            "Open",
            "Reload",
            "Describe",
            "Palette",
            "AddNode",
            "Connect",
            "SetValue",
            "SetVariable",
            "RemoveNodes",
            "RemoveConnections",
            "Save",
            "PollSave",
            "CancelSave",
            "Attach",
            "Attachment",
            "Undo",
            "Redo",
        };
        for (const char* event : events)
        {
            EXPECT_NE(found->second->m_events.find(event), found->second->m_events.end()) << event;
        }

        const AZ::BehaviorMethod* describe = found->second->m_events.at("Describe").m_broadcast;
        ASSERT_NE(describe, nullptr);
        ASSERT_EQ(describe->GetNumArguments(), 1);
        EXPECT_EQ(describe->GetArgument(0)->m_typeId, azrtti_typeid<AZStd::string>());
    }

    TEST_F(GraphAutomationTests, CreateRefusesInvalidSourcePathsWithoutTouchingTheEditor)
    {
        GraphAutomation owner;
        owner.Activate();

        const AZStd::string embeddedNul("bad\0.scriptcanvas", 17);
        const AZStd::string paths[] = { AZStd::string(), AZStd::string("relative.scriptcanvas"), embeddedNul };
        for (const AZStd::string& path : paths)
        {
            AutomationResult result;
            GraphAutomationRequestBus::BroadcastResult(result, &GraphAutomationRequests::Create, path);
            EXPECT_FALSE(result);
            EXPECT_FALSE(result.GetError().empty());
        }

        owner.Deactivate();
    }

    TEST_F(GraphAutomationTests, UnobservedSaveNotificationCannotBecomeAStoredCompletion)
    {
        GraphAutomation owner;
        owner.Activate();

        GraphAutomationNotificationBus::Broadcast(
            &GraphAutomationNotifications::OnAutomationSaveComplete, AZStd::string("unused.scriptcanvas"), AZStd::string());

        AutomationResult result;
        GraphAutomationRequestBus::BroadcastResult(result, &GraphAutomationRequests::PollSave);
        EXPECT_FALSE(result);

        GraphAutomationRequestBus::Broadcast(&GraphAutomationRequests::CancelSave);
        GraphAutomationRequestBus::BroadcastResult(result, &GraphAutomationRequests::PollSave);
        EXPECT_FALSE(result);

        owner.Deactivate();
    }

    TEST_F(GraphAutomationTests, NativeDatumViewUpdatesStringAndRefusesDifferentTypeWithoutMutation)
    {
        ScriptCanvas::GraphVariable variable(ScriptCanvas::Datum(AZStd::string("before")));
        ScriptCanvas::ModifiableDatumView view;
        variable.ConfigureDatumView(view);
        ASSERT_TRUE(view.IsValid());

        const AutomationResult refused = Assign(view, AZStd::any(true));
        EXPECT_FALSE(refused);
        EXPECT_EQ(*variable.GetDatum()->GetAs<AZStd::string>(), "before");

        const AutomationResult changed = Assign(view, AZStd::any(AZStd::string("after")));
        ASSERT_TRUE(changed) << changed.GetError().c_str();
        EXPECT_EQ(*variable.GetDatum()->GetAs<AZStd::string>(), "after");
    }

    TEST_F(GraphAutomationTests, BorrowedNativeStringIsCopiedAndNumericTypesRemainExact)
    {
        ScriptCanvas::GraphVariable text(ScriptCanvas::Datum(AZStd::string("before")));
        ScriptCanvas::ModifiableDatumView view;
        text.ConfigureDatumView(view);

        AZStd::string source("borrowed");
        const AutomationResult changed = Assign(view, AZStd::any(AZStd::string_view(source)));
        ASSERT_TRUE(changed) << changed.GetError().c_str();
        source.assign("changed");
        EXPECT_EQ(*text.GetDatum()->GetAs<AZStd::string>(), "borrowed");

        ScriptCanvas::GraphVariable number(ScriptCanvas::Datum(1.0));
        number.ConfigureDatumView(view);
        EXPECT_FALSE(Assign(view, AZStd::any(AZ::s32(7))));
        EXPECT_EQ(*number.GetDatum()->GetAs<double>(), 1.0);
        EXPECT_TRUE(Assign(view, AZStd::any(7.0)));
        EXPECT_EQ(*number.GetDatum()->GetAs<double>(), 7.0);
    }
} // namespace ScriptCanvasEditor
