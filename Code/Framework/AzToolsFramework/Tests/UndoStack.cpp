/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>

#include <AzToolsFramework/Undo/UndoSystem.h>

using namespace AZ;
using namespace AzToolsFramework;
using namespace AzToolsFramework::UndoSystem;

namespace UnitTest
{
    class SequencePointTest 
        : public URSequencePoint
    {
    public:
        SequencePointTest(AZStd::string friendlyName, URCommandID id)
            : URSequencePoint(friendlyName, id)
        {}

        bool Changed() const override { return true; }

        void Redo() override { m_redoCalled = true; }

        void Undo() override { m_undoCalled = true; }

        using URSequencePoint::RemoveChild;

        bool m_redoCalled = false;
        bool m_undoCalled = false;
    };

    class DifferentTypeSequencePointTest
        : public URSequencePoint
    {
    public:
        AZ_RTTI(DifferentTypeSequencePointTest, "{D7A42B6F-DCF8-443F-B4F1-57731B1D3CB8}")

        DifferentTypeSequencePointTest(AZStd::string friendlyName, URCommandID id)
            : URSequencePoint(friendlyName, id)
        {}

        bool Changed() const override { return true; }
    };

    ///////////////////////////////////////////////////////////////////////////
    // URSequencePoint
    ///////////////////////////////////////////////////////////////////////////

    TEST(URSequencePoint, Find_IdAndTypeNotPresent_ExpectNullptr)
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew SequencePointTest("Child", 2);
        auto child_3 = aznew SequencePointTest("Child", 3);
        auto child_1_1 = aznew SequencePointTest("Child", 4);
        auto child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(6, AZ::Uuid::Create());
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, Find_TypeNotPresent_ExpectNullptr)
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew SequencePointTest("Child", 2);
        auto child_3 = aznew SequencePointTest("Child", 3);
        auto child_1_1 = aznew SequencePointTest("Child", 4);
        auto child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(5, AZ::Uuid::Create());
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, Find_IdNotPresent_ExpectNullptr)
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew DifferentTypeSequencePointTest("Child", 2);
        auto child_3 = aznew SequencePointTest("Child", 3);
        auto child_1_1 = aznew SequencePointTest("Child", 4);
        auto child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(6, AZ::Uuid("{D7A42B6F-DCF8-443F-B4F1-57731B1D3CB8}"));
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, Find_MatchIsDirectChild_IdFound)
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew DifferentTypeSequencePointTest("Child", static_cast<AZ::u64>(2));
        auto child_3 = aznew SequencePointTest("Child", 3);
        auto child_1_1 = aznew SequencePointTest("Child", 4);
        auto child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(2, AZ::AzTypeInfo<DifferentTypeSequencePointTest>::Uuid());
        EXPECT_EQ(result, child_2);
    }

    TEST(URSequencePoint, Find_IdIsIndirectChild_IdFound)
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew DifferentTypeSequencePointTest("Child", static_cast<AZ::u64>(2));
        auto child_3 = aznew SequencePointTest("Child", 3);
        auto child_1_1 = aznew SequencePointTest("Child", 4);
        auto child_1_2 = aznew DifferentTypeSequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(5, AZ::AzTypeInfo<DifferentTypeSequencePointTest>::Uuid());
        EXPECT_EQ(result, child_1_2);
    }

    TEST(URSequencePoint, RemoveChild) 
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew SequencePointTest("Child", 2);
        auto child_3 = aznew SequencePointTest("Child", 3);
        auto child_4 = aznew SequencePointTest("Child", 4);
        auto child_5 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_4->SetParent(parent.get());
        child_5->SetParent(parent.get());

        EXPECT_THAT(parent->GetChildren(), ::testing::Pointwise(::testing::Eq(), {child_1, child_2, child_3, child_4, child_5}));

        parent->RemoveChild(child_5);

        EXPECT_THAT(parent->GetChildren(), ::testing::Pointwise(::testing::Eq(), {child_1, child_2, child_3, child_4}));

        delete child_5;
        // The other children are owned by parent
    }

    TEST(URSequencePoint, SetParent_NotChildOfParent) 
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child = aznew SequencePointTest("Child", 1);

        child->SetParent(parent.get());

        EXPECT_THAT(parent->GetChildren(), ::testing::Pointwise(::testing::Eq(), {child}));
    }

    TEST(URSequencePoint, SetParent_AlreadyChildOfParent) 
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew SequencePointTest("Child", 2);
        auto child_3 = aznew SequencePointTest("Child", 3);
        auto child_4 = aznew SequencePointTest("Child", 4);
        auto child_5 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_4->SetParent(parent.get());
        child_5->SetParent(parent.get());

        EXPECT_THAT(parent->GetChildren(), ::testing::Pointwise(::testing::Eq(), {child_1, child_2, child_3, child_4, child_5}));

        child_5->SetParent(parent.get());

        EXPECT_THAT(parent->GetChildren(), ::testing::Pointwise(::testing::Eq(), {child_1, child_2, child_3, child_4, child_5})) << "the parent did not de-dupe its children";
    }

    TEST(URSequencePoint, SetParent_AlreadyChildOfDifferentParent)
    {
        auto parent_1 = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto parent_2 = AZStd::make_unique<SequencePointTest>("Parent", 1);
        auto child = aznew SequencePointTest("Child", 5);

        child->SetParent(parent_1.get());

        EXPECT_THAT(parent_1->GetChildren(), ::testing::Pointwise(::testing::Eq(), {child}));

        child->SetParent(parent_2.get());

        EXPECT_THAT(parent_1->GetChildren(), ::testing::IsEmpty()) << "the original parent did not remove the child";
        EXPECT_THAT(parent_2->GetChildren(), ::testing::Pointwise(::testing::Eq(), {child})) << "child was not added to new parent properly";
    }

    TEST(URSequencePoint, RunUndo_NoChildren_UndoIsCalled) 
    {
        auto object = AZStd::make_unique<SequencePointTest>("Object", 0);
        object->m_undoCalled = false;

        object->RunUndo();

        EXPECT_TRUE(object->m_undoCalled) << "Undo was not called on the object";
    }

    TEST(URSequencePoint, RunUndo_HasChildren_UndoIsCalled) 
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew SequencePointTest("Child", 2);
        auto child_1_1 = aznew SequencePointTest("Child", 3);
        auto child_1_2 = aznew SequencePointTest("Child", 4);

        parent->m_undoCalled = false;

        child_1->SetParent(parent.get());
        child_1->m_undoCalled = false;

        child_2->SetParent(parent.get());
        child_2->m_undoCalled = false;

        child_1_1->SetParent(child_1);
        child_1_1->m_undoCalled = false;

        child_1_2->SetParent(child_1);
        child_1_2->m_undoCalled = false;

        parent->RunUndo();

        EXPECT_TRUE(parent->m_undoCalled) << "Undo was not called on the parent";
        EXPECT_TRUE(child_1->m_undoCalled) << "Undo was not called on the child";
        EXPECT_TRUE(child_2->m_undoCalled) << "Undo was not called on the child";
        EXPECT_TRUE(child_1_1->m_undoCalled) << "Undo was not called on the grandchild";
        EXPECT_TRUE(child_1_2->m_undoCalled) << "Undo was not called on the grandchild";
    }

    TEST(URSequencePoint, RunRedo_NoChildren_RedoIsCalled)
    {
        auto object = AZStd::make_unique<SequencePointTest>("Object", 0);
        object->m_redoCalled = false;

        object->RunRedo();

        EXPECT_TRUE(object->m_redoCalled) << "Redo was not called on the object";
    }

    TEST(URSequencePoint, RunRedo_HasChildren_RedoIsCalled) 
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew SequencePointTest("Child", 2);
        auto child_1_1 = aznew SequencePointTest("Child", 3);
        auto child_1_2 = aznew SequencePointTest("Child", 4);

        parent->m_redoCalled = false;

        child_1->SetParent(parent.get());
        child_1->m_redoCalled = false;

        child_2->SetParent(parent.get());
        child_2->m_redoCalled = false;

        child_1_1->SetParent(child_1);
        child_1_1->m_redoCalled = false;

        child_1_2->SetParent(child_1);
        child_1_2->m_redoCalled = false;

        parent->RunRedo();

        EXPECT_TRUE(parent->m_redoCalled) << "Redo was not called on the parent";
        EXPECT_TRUE(child_1->m_redoCalled) << "Redo was not called on the child";
        EXPECT_TRUE(child_2->m_redoCalled) << "Redo was not called on the child";
        EXPECT_TRUE(child_1_1->m_redoCalled) << "Redo was not called on the grandchild";
        EXPECT_TRUE(child_1_2->m_redoCalled) << "Redo was not called on the grandchild";
    }

    TEST(URSequencePoint, SetName) 
    {
        AZStd::string test_1("Test Point");
        AZStd::string test_2("A different Test Point");
        auto testPoint = AZStd::make_unique<SequencePointTest>("Test Point", 0);

        EXPECT_EQ(testPoint->GetName(), test_1);

        testPoint->SetName("A different Test Point");

        EXPECT_EQ(testPoint->GetName(), test_2);
    }

    TEST(URSequencePoint, HasRealChildren_NoChildren_ExpectFalse) 
    {
        auto testPoint = AZStd::make_unique<SequencePointTest>("Test Point", 0);
        bool result = testPoint->HasRealChildren();
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, HasRealChildren_AllChildrenAreFake_ExpectFalse)
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew SequencePointTest("Child", 2);
        auto child_3 = aznew SequencePointTest("Child", 3);
        auto child_1_1 = aznew SequencePointTest("Child", 4);
        auto child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        bool result = parent->HasRealChildren();
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, HasRealChildren_OneChildIsReal_ExpectTrue)
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew SequencePointTest("Child", 2);
        auto child_3 = aznew DifferentTypeSequencePointTest("Child", 3);
        auto child_1_1 = aznew SequencePointTest("Child", 4);
        auto child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        bool result = parent->HasRealChildren();
        EXPECT_TRUE(result);
    }

    TEST(URSequencePoint, HasRealChildren_OneGrandChildIsReal_ExpectTrue)
    {
        auto parent = AZStd::make_unique<SequencePointTest>("Parent", 0);
        auto child_1 = aznew SequencePointTest("Child", 1);
        auto child_2 = aznew SequencePointTest("Child", 2);
        auto child_3 = aznew SequencePointTest("Child", 3);
        auto child_1_1 = aznew SequencePointTest("Child", 4);
        auto child_1_2 = aznew DifferentTypeSequencePointTest("Child", 5);

        child_1->SetParent(parent.get());
        child_2->SetParent(parent.get());
        child_3->SetParent(parent.get());
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        bool result = parent->HasRealChildren();
        EXPECT_TRUE(result);
    }

    ///////////////////////////////////////////////////////////////////////////
    // UndoStack
    ///////////////////////////////////////////////////////////////////////////

    class UndoDestructorTest : public URSequencePoint
    {
    public:
        UndoDestructorTest(bool* completedFlag)
            : URSequencePoint("UndoDestructorTest", 0)
            , m_completedFlag(completedFlag)
        {
            *m_completedFlag = false;
        }

        ~UndoDestructorTest() override
        {
            *m_completedFlag = true;
        }

        bool Changed() const override { return true; }

    private:
        bool* m_completedFlag;
    };

    TEST(UndoStack, UndoRedoMemory)
    {
        UndoStack undoStack(nullptr);

        bool flag = false;
        undoStack.Post(aznew UndoDestructorTest(&flag));

        undoStack.Undo();
        undoStack.Slice();

        EXPECT_EQ(flag, true);
    }

    class UndoIntSetter : public URSequencePoint
    {
    public:
        UndoIntSetter(int* value, int newValue)
            : URSequencePoint("UndoIntSetter", 0)
            , m_value(value)
            , m_newValue(newValue)
            , m_oldValue(*value)
        {
            Redo();
        }

        void Undo() override
        {
            *m_value = m_oldValue;
        }

        void Redo() override
        {
            *m_value = m_newValue;
        }

        bool Changed() const override { return true; }

    private:
        int* m_value;
        int m_newValue;
        int m_oldValue;
    };

    TEST(UndoStack, UndoRedoSequence)
    {
        UndoStack undoStack(nullptr);

        int tracker = 0;

        undoStack.Post(aznew UndoIntSetter(&tracker, 1));
        EXPECT_EQ(tracker, 1);

        undoStack.Undo();
        EXPECT_EQ(tracker, 0);

        undoStack.Redo();
        EXPECT_EQ(tracker, 1);

        undoStack.Undo();
        EXPECT_EQ(tracker, 0);

        undoStack.Redo();
        EXPECT_EQ(tracker, 1);

        undoStack.Post(aznew UndoIntSetter(&tracker, 100));
        EXPECT_EQ(tracker, 100);

        undoStack.Undo();
        EXPECT_EQ(tracker, 1);

        undoStack.Undo();
        EXPECT_EQ(tracker, 0);

        undoStack.Redo();
        EXPECT_EQ(tracker, 1);
    }

    TEST(UndoStack, UndoRedoLotsOfUndos)
    {
        UndoStack undoStack(nullptr);

        int tracker = 0;

        const int numUndos = 1000;
        for (int i = 0; i < 1000; i++)
        {
            undoStack.Post(aznew UndoIntSetter(&tracker, i + 1));
            EXPECT_EQ(tracker, i + 1);
        }

        int counter = 0;
        while (undoStack.CanUndo())
        {
            undoStack.Undo();
            counter++;
        }

        EXPECT_EQ(numUndos, counter);
        EXPECT_EQ(tracker, 0);

        counter = 0;
        while (undoStack.CanRedo())
        {
            undoStack.Redo();
            counter++;
        }

        EXPECT_EQ(numUndos, counter);
        EXPECT_EQ(tracker, numUndos);
    }
}
