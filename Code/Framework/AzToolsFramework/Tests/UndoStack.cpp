/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        SequencePointTest* child_2 = aznew SequencePointTest("Child", 2);
        SequencePointTest* child_3 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 4);
        SequencePointTest* child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(6, AZ::Uuid::Create());
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, Find_TypeNotPresent_ExpectNullptr)
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        SequencePointTest* child_2 = aznew SequencePointTest("Child", 2);
        SequencePointTest* child_3 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 4);
        SequencePointTest* child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(5, AZ::Uuid::Create());
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, Find_IdNotPresent_ExpectNullptr)
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        DifferentTypeSequencePointTest* child_2 = aznew DifferentTypeSequencePointTest("Child", 2);
        SequencePointTest* child_3 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 4);
        SequencePointTest* child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(6, AZ::Uuid("{D7A42B6F-DCF8-443F-B4F1-57731B1D3CB8}"));
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, Find_MatchIsDirectChild_IdFound)
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        DifferentTypeSequencePointTest* child_2 = aznew DifferentTypeSequencePointTest("Child", static_cast<AZ::u64>(2));
        SequencePointTest* child_3 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 4);
        SequencePointTest* child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(2, AZ::AzTypeInfo<DifferentTypeSequencePointTest>::Uuid());
        EXPECT_EQ(result, child_2);
    }

    TEST(URSequencePoint, Find_IdIsIndirectChild_IdFound)
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        DifferentTypeSequencePointTest* child_2 = aznew DifferentTypeSequencePointTest("Child", static_cast<AZ::u64>(2));
        SequencePointTest* child_3 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 4);
        DifferentTypeSequencePointTest* child_1_2 = aznew DifferentTypeSequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        auto result = parent->Find(5, AZ::AzTypeInfo<DifferentTypeSequencePointTest>::Uuid());
        EXPECT_EQ(result, child_1_2);
    }

    TEST(URSequencePoint, RemoveChild) 
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        SequencePointTest* child_2 = aznew SequencePointTest("Child", 2);
        SequencePointTest* child_3 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_4 = aznew SequencePointTest("Child", 4);
        SequencePointTest* child_5 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
        child_4->SetParent(parent);
        child_5->SetParent(parent);

        auto children = parent->GetChildren();
        EXPECT_EQ(children.size(), 5) << "children were not added properly";

        parent->RemoveChild(child_5);

        children = parent->GetChildren();
        EXPECT_EQ(children.size(), 4) << "child was not removed properly";
    }

    TEST(URSequencePoint, SetParent_NotChildOfParent) 
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child = aznew SequencePointTest("Child", 1);

        child->SetParent(parent);

        auto children = parent->GetChildren();
        EXPECT_EQ(children.size(), 1) << "child was not added properly";
        EXPECT_EQ(children[0]->GetName(), child->GetName()) << "child was not added properly";
    }

    TEST(URSequencePoint, SetParent_AlreadyChildOfParent) 
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        SequencePointTest* child_2 = aznew SequencePointTest("Child", 2);
        SequencePointTest* child_3 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_4 = aznew SequencePointTest("Child", 4);
        SequencePointTest* child_5 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
        child_4->SetParent(parent);
        child_5->SetParent(parent);

        auto children = parent->GetChildren();
        EXPECT_EQ(children.size(), 5) << "children were not added properly";

        child_5->SetParent(parent);

        children = parent->GetChildren();
        EXPECT_EQ(children.size(), 5) << "the parent did not de-dupe its children";

        bool childFound = false;
        for (auto tempChild : children)
        {
            if (tempChild->GetName() == child_5->GetName())
            {
                childFound = true;
                break;
            }
        }

        EXPECT_TRUE(childFound);
    }

    TEST(URSequencePoint, SetParent_AlreadyChildOfDifferentParent)
    {
        SequencePointTest* parent_1 = aznew SequencePointTest("Parent", 0);
        SequencePointTest* parent_2 = aznew SequencePointTest("Parent", 1);
        SequencePointTest* child = aznew SequencePointTest("Child", 5);

        child->SetParent(parent_1);

        auto children = parent_1->GetChildren();
        EXPECT_EQ(children.size(), 1) << "child was not added properly";

        child->SetParent(parent_2);

        children = parent_1->GetChildren();
        EXPECT_EQ(children.size(), 0) << "the original parent did not remove the child";

        children = parent_2->GetChildren();
        EXPECT_EQ(children[0]->GetName(), child->GetName()) << "child was not added to new parent properly";
    }

    TEST(URSequencePoint, RunUndo_NoChildren_UndoIsCalled) 
    {
        SequencePointTest* object = aznew SequencePointTest("Object", 0);
        object->m_undoCalled = false;

        object->RunUndo();

        EXPECT_TRUE(object->m_undoCalled) << "Undo was not called on the object";
    }

    TEST(URSequencePoint, RunUndo_HasChildren_UndoIsCalled) 
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        SequencePointTest* child_2 = aznew SequencePointTest("Child", 2);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_1_2 = aznew SequencePointTest("Child", 4);

        parent->m_undoCalled = false;

        child_1->SetParent(parent);
        child_1->m_undoCalled = false;

        child_2->SetParent(parent);
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
        SequencePointTest* object = aznew SequencePointTest("Object", 0);
        object->m_redoCalled = false;

        object->RunRedo();

        EXPECT_TRUE(object->m_redoCalled) << "Redo was not called on the object";
    }

    TEST(URSequencePoint, RunRedo_HasChildren_RedoIsCalled) 
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        SequencePointTest* child_2 = aznew SequencePointTest("Child", 2);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_1_2 = aznew SequencePointTest("Child", 4);

        parent->m_redoCalled = false;

        child_1->SetParent(parent);
        child_1->m_redoCalled = false;

        child_2->SetParent(parent);
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
        SequencePointTest* testPoint = aznew SequencePointTest("Test Point", 0);

        EXPECT_EQ(testPoint->GetName(), test_1);

        testPoint->SetName("A different Test Point");

        EXPECT_EQ(testPoint->GetName(), test_2);
    }

    TEST(URSequencePoint, HasRealChildren_NoChildren_ExpectFalse) 
    {
        SequencePointTest* testPoint = aznew SequencePointTest("Test Point", 0);
        bool result = testPoint->HasRealChildren();
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, HasRealChildren_AllChildrenAreFake_ExpectFalse)
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        SequencePointTest* child_2 = aznew SequencePointTest("Child", 2);
        SequencePointTest* child_3 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 4);
        SequencePointTest* child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        bool result = parent->HasRealChildren();
        EXPECT_FALSE(result);
    }

    TEST(URSequencePoint, HasRealChildren_OneChildIsReal_ExpectTrue)
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        SequencePointTest* child_2 = aznew SequencePointTest("Child", 2);
        DifferentTypeSequencePointTest* child_3 = aznew DifferentTypeSequencePointTest("Child", 3);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 4);
        SequencePointTest* child_1_2 = aznew SequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
        child_1_1->SetParent(child_1);
        child_1_2->SetParent(child_1);

        bool result = parent->HasRealChildren();
        EXPECT_TRUE(result);
    }

    TEST(URSequencePoint, HasRealChildren_OneGrandChildIsReal_ExpectTrue)
    {
        SequencePointTest* parent = aznew SequencePointTest("Parent", 0);
        SequencePointTest* child_1 = aznew SequencePointTest("Child", 1);
        SequencePointTest* child_2 = aznew SequencePointTest("Child", 2);
        SequencePointTest* child_3 = aznew SequencePointTest("Child", 3);
        SequencePointTest* child_1_1 = aznew SequencePointTest("Child", 4);
        DifferentTypeSequencePointTest* child_1_2 = aznew DifferentTypeSequencePointTest("Child", 5);

        child_1->SetParent(parent);
        child_2->SetParent(parent);
        child_3->SetParent(parent);
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
