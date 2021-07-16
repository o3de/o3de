/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/JNI/shared_ref.h>

// Include Testing Framework Here


using namespace AZ::Android::JNI;
using JniObject = Object;


namespace UnitTest
{
    struct JavaTestObject
    {
        JavaTestObject()
            : m_object("com/amazon/test/SimpleObject")
        {
            m_object.CreateInstance("()V");

            // Register a non-static Java method with the associated Java object instance.
            // This is required in order to call Java instance methods in native code.
            m_object.RegisterMethod("GetBool", "()Z");
            m_object.RegisterMethod("GetBoolArray", "()[Z");

            m_object.RegisterMethod("GetChar", "()C");
            m_object.RegisterMethod("GetCharArray", "()[C");

            m_object.RegisterMethod("GetByte", "()B");
            m_object.RegisterMethod("GetByteArray", "()[B");

            m_object.RegisterMethod("GetShort", "()S");
            m_object.RegisterMethod("GetShortArray", "()[S");

            m_object.RegisterMethod("GetInt", "()I");
            m_object.RegisterMethod("GetIntArray", "()[I");

            m_object.RegisterMethod("GetFloat", "()F");
            m_object.RegisterMethod("GetFloatArray", "()[F");

            m_object.RegisterMethod("GetDouble", "()D");
            m_object.RegisterMethod("GetDoubleArray", "()[D");

            m_object.RegisterMethod("GetClass", "()Ljava/lang/Class;");

            m_object.RegisterMethod("GetString", "()Ljava/lang/String;");

            m_object.RegisterMethod("GetObject", "()Lcom/amazon/test/SimpleObject$Foo;");
            m_object.RegisterMethod("GetObjectArray", "()[Lcom/amazon/test/SimpleObject$Foo;");
        }

        JniObject m_object;
    };


    // ----

    TEST(SharedRef, Sanity)
    {
        EXPECT_EQ(1, 1);
    }


    // ----
    // EvalAsBool Tests
    // ----

    TEST(SharedRef, EvalAsBool_DefaultCtorAndNullptrCtor_BothAreFalse)
    {
        shared_ref<jobject> defaultRef;
        shared_ref<jobject> nullptrRef(nullptr);

        EXPECT_FALSE(defaultRef);
        EXPECT_FALSE(nullptrRef);

        // explicit bool comparisons not supported
        // should not compile
        //EXPECT_TRUE(false == default_ref);
        //EXPECT_TRUE(false == nullptr_ref);
    }

    TEST(SharedRef, EvalAsBool_JobjectCtor_IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> validRef(jObject.InvokeObjectMethod<jobject>("GetObject"));
        EXPECT_TRUE(validRef);

        // explicit bool comparisons not supported
        // should not compile
        //EXPECT_TRUE(true == validRef);
    }

    TEST(SharedRef, EvalAsBool_CopyCtorWithDefault_BothAreFalse)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> copy(original);

        EXPECT_FALSE(original);
        EXPECT_FALSE(copy);
    }

    TEST(SharedRef, EvalAsBool_CopyCtorWithJobject_IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        EXPECT_TRUE(original);
        EXPECT_TRUE(copy);
    }

    TEST(SharedRef, EvalAsBool_CopyCtorWithJobjectResetOriginal_OriginalIsFalseCopyIsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        original.reset();

        EXPECT_FALSE(original);
        EXPECT_TRUE(copy);
    }

    TEST(SharedRef, EvalAsBool_CopyCtorWithJobjectResetCopy_OriginalIsTrueCopyIsFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        copy.reset();

        EXPECT_TRUE(original);
        EXPECT_FALSE(copy);
    }

    TEST(SharedRef, EvalAsBool_PolymorphicCopyCtorWithJfloatarryToJarray_BothAreTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> copy(original);

        EXPECT_TRUE(original);
        EXPECT_TRUE(copy);
    }

    TEST(SharedRef, EvalAsBool_ScopedCopyCtorWithDefault_OriginalIsStillFalseAfterScope)
    {
        shared_ref<jobject> original;
        {
            shared_ref<jobject> scopedCopy(original);
            (void)scopedCopy;
        }

        EXPECT_FALSE(original);
    }

    TEST(SharedRef, EvalAsBool_ScopedCopyCtorWithJobject_OriginalIsStillTrueAfterScope)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        {
            shared_ref<jobject> scopedCopy(original);
            (void)scopedCopy;
        }

        EXPECT_TRUE(original);
    }

    TEST(SharedRef, EvalAsBool_MoveCtorWithDefault_BothAreFalse)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> moved(AZStd::move(original));

        EXPECT_FALSE(original);
        EXPECT_FALSE(moved);
    }

    TEST(SharedRef, EvalAsBool_MoveCtorWithJobject_OriginalIsFalseMovedIsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> moved(AZStd::move(original));

        EXPECT_FALSE(original);
        EXPECT_TRUE(moved);
    }

    TEST(SharedRef, EvalAsBool_PolymorphicMoveCtorWithJfloatarryToJarray_OriginalIsFalseMovedIsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> moved(AZStd::move(original));

        EXPECT_FALSE(original);
        EXPECT_TRUE(moved);
    }

    TEST(SharedRef, EvalAsBool_AssignmentOperatorWithDefault_BothAreFalse)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> assigned = original;

        EXPECT_FALSE(original);
        EXPECT_FALSE(assigned);
    }

    TEST(SharedRef, EvalAsBool_AssignmentOperatorWithJobject_BothAreTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> assigned = original;

        EXPECT_TRUE(original);
        EXPECT_TRUE(assigned);
    }

    TEST(SharedRef, EvalAsBool_PolymorphicAssignmentOperatorWithJfloatarryToJarray_BothAreTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> assigned = original;

        EXPECT_TRUE(original);
        EXPECT_TRUE(assigned);
    }

    TEST(SharedRef, EvalAsBool_MoveOperatorWithDefault_BothAreFalse)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> moved = AZStd::move(original);

        EXPECT_FALSE(original);
        EXPECT_FALSE(moved);
    }

    TEST(SharedRef, EvalAsBool_MoveOperatorWithJobject_OriginalIsFalseMovedIsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> moved = AZStd::move(original);

        EXPECT_FALSE(original);
        EXPECT_TRUE(moved);
    }

    TEST(SharedRef, EvalAsBool_PolymorphicMoveOperatorWithJfloatarryToJarray_OriginalIsFalseMovedIsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> moved = AZStd::move(original);

        EXPECT_FALSE(original);
        EXPECT_TRUE(moved);
    }

    TEST(SharedRef, EvalAsBool_ResetDefault_IsFalse)
    {
        shared_ref<jobject> ref;
        ref.reset();

        EXPECT_FALSE(ref);
    }

    TEST(SharedRef, EvalAsBool_ResetJobject_IsFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        ref.reset();

        EXPECT_FALSE(ref);
    }

    TEST(SharedRef, EvalAsBool_ResetDefaultWithJobject_IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref;
        ref.reset(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_TRUE(ref);
    }

    TEST(SharedRef, EvalAsBool_ResetJobjectWithJobject_IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        ref.reset(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_TRUE(ref);
    }

    TEST(SharedRef, EvalAsBool_SwapDefaultWithDefault_BothAreFalse)
    {
        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2;

        ref1.swap(ref2);

        EXPECT_FALSE(ref1);
        EXPECT_FALSE(ref2);
    }

    TEST(SharedRef, EvalAsBool_SwapDefaultWithJobject_Ref1IsTrueRef2IsFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_TRUE(ref1);
        EXPECT_FALSE(ref2);
    }

    TEST(SharedRef, EvalAsBool_SwapJobjectWithDefault_Ref1IsFalseRef2IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2;

        ref1.swap(ref2);

        EXPECT_FALSE(ref1);
        EXPECT_TRUE(ref2);
    }

    TEST(SharedRef, EvalAsBool_SwapJobjectWithJobject_IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_TRUE(ref1);
        EXPECT_TRUE(ref2);
    }


    // ----
    // Negate tests
    // ----

    TEST(SharedRef, Negate_DefaultCtorAndNullptrCtor_BothAreTrue)
    {
        shared_ref<jobject> defaultRef;
        shared_ref<jobject> nullptrRef(nullptr);

        EXPECT_TRUE(!defaultRef);
        EXPECT_TRUE(!nullptrRef);
    }

    TEST(SharedRef, Negate_JobjectCtor_IsFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> validRef(jObject.InvokeObjectMethod<jobject>("GetObject"));
        EXPECT_FALSE(!validRef);
    }


    // ----
    // Get Tests
    // ----

    TEST(SharedRef, Get_DefaultCtorAndNullptrCtor_BothAreNullptr)
    {
        shared_ref<jobject> defaultRef;
        shared_ref<jobject> nullptrRef(nullptr);

        EXPECT_EQ(nullptr, defaultRef.get());
        EXPECT_EQ(nullptr, nullptrRef.get());
    }

    TEST(SharedRef, Get_JobjectCtor_IsNotNullptr)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> validRef(jObject.InvokeObjectMethod<jobject>("GetObject"));
        EXPECT_NE(nullptr, validRef.get());
    }

    TEST(SharedRef, Get_CopyCtorWithJobject_PointersAreSame)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        EXPECT_EQ(original.get(), copy.get());
    }

    TEST(SharedRef, Get_CopyCtorWithJobjectResetOriginal_PointersAreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        original.reset();

        EXPECT_NE(original.get(), copy.get());
    }

    TEST(SharedRef, Get_CopyCtorWithJobjectResetCopy_PointersAreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        copy.reset();

        EXPECT_NE(original.get(), copy.get());
    }

    TEST(SharedRef, Get_PolymorphicCopyCtorWithJfloatarryToJarray_PointersAreSame)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> copy(original);

        EXPECT_EQ(original.get(), copy.get());
    }

    TEST(SharedRef, Get_ScopedCopyCtorWithDefault_OriginalIsStillNullptrAfterScope)
    {
        shared_ref<jobject> original;
        {
            shared_ref<jobject> scopedCopy(original);
            (void)scopedCopy;
        }

        EXPECT_EQ(nullptr, original.get());
    }

    TEST(SharedRef, Get_ScopedCopyCtorWithJobject_OriginalIsStillNonNullAfterScope)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        {
            shared_ref<jobject> scopedCopy(original);
            (void)scopedCopy;
        }

        EXPECT_NE(nullptr, original.get());
    }

    TEST(SharedRef, Get_MoveCtorWithDefault_BothAreNullptr)
    {
        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2(AZStd::move(ref1));

        EXPECT_EQ(nullptr, ref1.get());
        EXPECT_EQ(nullptr, ref2.get());
    }

    TEST(SharedRef, Get_MoveCtorWithObject_OriginalIsNullptrMovedIsNonNullptr)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> moved(AZStd::move(original));

        EXPECT_EQ(nullptr, original.get());
        EXPECT_NE(nullptr, moved.get());
    }

    TEST(SharedRef, Get_PolymorphicMoveCtorWithJfloatarryToJarray_OriginalIsNullptrMovedIsNonNullptr)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> moved(AZStd::move(original));

        EXPECT_EQ(nullptr, original.get());
        EXPECT_NE(nullptr, moved.get());
    }

    TEST(SharedRef, Get_AssignmentOperatorWithDefault_BothAreNullptr)
    {
        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2 = ref1;

        EXPECT_EQ(nullptr, ref1.get());
        EXPECT_EQ(nullptr, ref2.get());
    }

    TEST(SharedRef, Get_AssignmentOperatorWithJobject_PointersAreSame)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> assigned = original;

        EXPECT_EQ(original.get(), assigned.get());
    }

    TEST(SharedRef, Get_PolymorphicAssignmentOperatorWithJfloatarryToJarray_PointersAreSame)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> assigned = original;

        EXPECT_EQ(original.get(), assigned.get());
    }

    TEST(SharedRef, Get_MoveOperatorWithDefault_BothAreNullptr)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> moved = AZStd::move(original);

        EXPECT_EQ(nullptr, original.get());
        EXPECT_EQ(nullptr, moved.get());
    }

    TEST(SharedRef, Get_MoveOperatorWithJobject_OriginalIsNullptrMovedIsNonNull)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> moved = AZStd::move(original);

        EXPECT_EQ(nullptr, original.get());
        EXPECT_NE(nullptr, moved.get());
    }

    TEST(SharedRef, Get_PolymorphicMoveOperatorWithJfloatarryToJarray_OriginalIsNullptrMovedIsNonNull)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> moved = AZStd::move(original);

        EXPECT_EQ(nullptr, original.get());
        EXPECT_NE(nullptr, moved.get());
    }

    TEST(SharedRef, Get_ResetDefault_IsNullptr)
    {
        shared_ref<jobject> ref;
        ref.reset();

        EXPECT_EQ(nullptr, ref.get());
    }

    TEST(SharedRef, Get_ResetJobject_IsNullptr)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        ref.reset();

        EXPECT_EQ(nullptr, ref.get());
    }

    TEST(SharedRef, Get_ResetDefaultWithJobject_IsNonNull)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref;
        ref.reset(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_NE(nullptr, ref.get());
    }

    TEST(SharedRef, Get_ResetJobjectWithJobject_IsNonNull)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        ref.reset(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_NE(nullptr, ref.get());
    }

    TEST(SharedRef, Get_SwapDefaultWithDefault_BothAreStillNullptr)
    {
        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2;

        ref1.swap(ref2);

        EXPECT_EQ(nullptr, ref1.get());
        EXPECT_EQ(nullptr, ref2.get());
    }

    TEST(SharedRef, Get_SwapDefaultWithJobject_Ref1IsNonNullRef2IsNullptr)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_NE(nullptr, ref1.get());
        EXPECT_EQ(nullptr, ref2.get());
    }

    TEST(SharedRef, Get_SwapJobjectWithDefault_Ref1IsNullptrRef2IsNonNull)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2;

        ref1.swap(ref2);

        EXPECT_EQ(nullptr, ref1.get());
        EXPECT_NE(nullptr, ref2.get());
    }

    TEST(SharedRef, Get_SwapJobjectWithJobject_BothAreNonNull)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_NE(nullptr, ref1.get());
        EXPECT_NE(nullptr, ref2.get());
    }


    // ----
    // UseCount Tests
    // ----

    TEST(SharedRef, UseCount_DefaultCtorAndNullptrCtor_IsZero)
    {
        shared_ref<jobject> defaultRef;
        shared_ref<jobject> nullptrRef(nullptr);

        EXPECT_EQ(0, defaultRef.use_count());
        EXPECT_EQ(0, nullptrRef.use_count());
    }

    TEST(SharedRef, UseCount_JobjectCtor_IsOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        EXPECT_EQ(1, ref.use_count());
    }

    TEST(SharedRef, UseCount_CopyCtorWithDefault_BothAreZero)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> copy(original);

        EXPECT_EQ(0, original.use_count());
        EXPECT_EQ(0, copy.use_count());

        EXPECT_EQ(original.use_count(), copy.use_count());
    }

    TEST(SharedRef, UseCount_CopyCtorWithJobject_BothAreTwo)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        EXPECT_EQ(2, original.use_count());
        EXPECT_EQ(2, copy.use_count());

        EXPECT_EQ(original.use_count(), copy.use_count());
    }

    TEST(SharedRef, UseCount_CopyCtorWithJobject2X_AllAreThree)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy1(original);
        shared_ref<jobject> copy2(copy1);

        EXPECT_EQ(3, original.use_count());
        EXPECT_EQ(3, copy1.use_count());
        EXPECT_EQ(3, copy2.use_count());

        EXPECT_EQ(original.use_count(), copy1.use_count());
        EXPECT_EQ(copy1.use_count(), copy2.use_count());
    }

    TEST(SharedRef, UseCount_CopyCtorWithJobjectResetOriginal_OriginalIsZeroCopyIsOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        original.reset();

        EXPECT_EQ(0, original.use_count());
        EXPECT_EQ(1, copy.use_count());
    }

    TEST(SharedRef, UseCount_CopyCtorWithJobjectResetCopy_OriginalIsOneCopyIsZero)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        copy.reset();

        EXPECT_EQ(1, original.use_count());
        EXPECT_EQ(0, copy.use_count());
    }

    TEST(SharedRef, UseCount_PolymorphicCopyCtorWithJfloatarryToJarray_BothAreTwo)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> copy(original);

        EXPECT_EQ(2, original.use_count());
        EXPECT_EQ(2, copy.use_count());

        EXPECT_EQ(original.use_count(), copy.use_count());
    }

    TEST(SharedRef, UseCount_ScopedCopyCtorWithDefault_IsStillZeroAfterScope)
    {
        shared_ref<jobject> original;
        {
            shared_ref<jobject> scopedCopy(original);
            (void)scopedCopy;
        }

        EXPECT_EQ(0, original.use_count());
    }

    TEST(SharedRef, UseCount_ScopedCopyCtorWithJobject_IsTwoInScopeIsOneAfterScope)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));

        int scopedUseCount = 0;
        int scopedCopyUseCount = 0;
        {
            shared_ref<jobject> scopedCopy(original);

            scopedUseCount = original.use_count();
            scopedCopyUseCount = scopedCopy.use_count();
        }

        EXPECT_EQ(2, scopedUseCount);
        EXPECT_EQ(2, scopedCopyUseCount);

        EXPECT_EQ(1, original.use_count());
    }

    TEST(SharedRef, UseCount_MoveCtorWithDefault_BothAreZero)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> moved(AZStd::move(original));

        EXPECT_EQ(0, original.use_count());
        EXPECT_EQ(0, moved.use_count());

        EXPECT_EQ(original.use_count(), moved.use_count());
    }

    TEST(SharedRef, UseCount_MoveCtorWithJobject_OriginalIsZeroMovedIsOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> moved(AZStd::move(original));

        EXPECT_EQ(0, original.use_count());
        EXPECT_EQ(1, moved.use_count());

        EXPECT_NE(original.use_count(), moved.use_count());
    }

    TEST(SharedRef, UseCount_PolymorphicMoveCtorWithJfloatarryToJarray_OriginalIsZeroMovedIsOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> moved(AZStd::move(original));

        EXPECT_EQ(0, original.use_count());
        EXPECT_EQ(1, moved.use_count());

        EXPECT_NE(original.use_count(), moved.use_count());
    }

    TEST(SharedRef, UseCount_AssignmentOperatorWithDefault_BothAreZero)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> assigned = original;

        EXPECT_EQ(0, original.use_count());
        ASSERT_EQ(0, assigned.use_count());

        EXPECT_EQ(original.use_count(), assigned.use_count());
    }

    TEST(SharedRef, UseCount_AssignmentOperatorWithJobject_BothAreTwo)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> assigned = original;

        EXPECT_EQ(2, original.use_count());
        EXPECT_EQ(2, assigned.use_count());

        EXPECT_EQ(original.use_count(), assigned.use_count());
    }

    TEST(SharedRef, UseCount_PolymorphicAssignmentOperatorWithJfloatarryToJarray_BothAreTwo)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> assigned = original;

        EXPECT_EQ(2, original.use_count());
        EXPECT_EQ(2, assigned.use_count());

        EXPECT_EQ(original.use_count(), assigned.use_count());
    }

    TEST(SharedRef, UseCount_MoveOperatorWithDefault_BothAreZero)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> moved = AZStd::move(original);

        EXPECT_EQ(0, original.use_count());
        EXPECT_EQ(0, moved.use_count());

        EXPECT_EQ(original.use_count(), moved.use_count());
    }

    TEST(SharedRef, UseCount_MoveOperatorWithJobject_OriginalIsZeroMovedIsOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> moved = AZStd::move(original);

        EXPECT_EQ(0, original.use_count());
        EXPECT_EQ(1, moved.use_count());

        EXPECT_NE(original.use_count(), moved.use_count());
    }

    TEST(SharedRef, UseCount_PolymorphicMoveOperatorWithJfloatarryToJarray_OriginalIsZeroMovedIsOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> moved = AZStd::move(original);

        EXPECT_EQ(0, original.use_count());
        EXPECT_EQ(1, moved.use_count());

        EXPECT_NE(original.use_count(), moved.use_count());
    }

    TEST(SharedRef, UseCount_ResetDefault_IsZero)
    {
        shared_ref<jobject> ref;
        ref.reset();

        EXPECT_EQ(0, ref.use_count());
    }

    TEST(SharedRef, UseCount_ResetJobject_IsZero)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        ref.reset();

        EXPECT_EQ(0, ref.use_count());
    }

    TEST(SharedRef, UseCount_ResetDefaultWithJobject_IsOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref;
        ref.reset(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_EQ(1, ref.use_count());
    }

    TEST(SharedRef, UseCount_ResetJobjectWithJobject_IsOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        ref.reset(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_EQ(1, ref.use_count());
    }

    TEST(SharedRef, UseCount_SwapDefaultWithDefault_BothAreZero)
    {
        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2;

        ref1.swap(ref2);

        EXPECT_EQ(0, ref1.use_count());
        EXPECT_EQ(0, ref2.use_count());
    }

    TEST(SharedRef, UseCount_SwapDefaultWithJobject_Ref1IsOneRef2IsZero)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_EQ(1, ref1.use_count());
        EXPECT_EQ(0, ref2.use_count());
    }

    TEST(SharedRef, UseCount_SwapJobjectWithDefault_Ref1IsZeroRef2IsOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2;

        ref1.swap(ref2);

        EXPECT_EQ(0, ref1.use_count());
        EXPECT_EQ(1, ref2.use_count());
    }

    TEST(SharedRef, UseCount_SwapJobjectWithJobject_BothAreOne)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_EQ(1, ref1.use_count());
        EXPECT_EQ(1, ref2.use_count());
    }


    // ----
    // Unique Tests
    // ----

    TEST(SharedRef, Unique_DefaultCtorAndNullptrCtor_IsFalse)
    {
        shared_ref<jobject> defaultRef;
        shared_ref<jobject> nullptrRef(nullptr);

        EXPECT_FALSE(defaultRef.unique());
        EXPECT_FALSE(nullptrRef.unique());
    }

    TEST(SharedRef, Unique_JobjectCtor_IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        EXPECT_TRUE(ref.unique());
    }

    TEST(SharedRef, Unique_CopyCtorWithDefault_BothAreFalse)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> copy(original);

        EXPECT_FALSE(original.unique());
        EXPECT_FALSE(copy.unique());
    }

    TEST(SharedRef, Unique_CopyCtorWithJobject_IsFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        EXPECT_FALSE(original.unique());
        EXPECT_FALSE(copy.unique());
    }

    TEST(SharedRef, Unique_CopyCtorWithJobjectResetOriginal_OriginalIsFalseCopyIsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        original.reset();

        EXPECT_FALSE(original.unique());
        EXPECT_TRUE(copy.unique());
    }

    TEST(SharedRef, Unique_CopyCtorWithJobjectResetCopy_OriginalIsTrueCopyIsFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        copy.reset();

        EXPECT_TRUE(original.unique());
        EXPECT_FALSE(copy.unique());
    }

    TEST(SharedRef, Unique_PolymorphicCopyCtorWithJfloatarryToJarray_BothAreFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> copy(original);

        EXPECT_FALSE(original.unique());
        EXPECT_FALSE(copy.unique());
    }

    TEST(SharedRef, Unique_ScopedCopyCtorWithDefault_IsFalseAfterScope)
    {
        shared_ref<jobject> original;
        {
            shared_ref<jobject> scopedCopy(original);
            (void)scopedCopy;
        }

        EXPECT_FALSE(original.unique());
    }

    TEST(SharedRef, Unique_ScopedCopyCtorWithJobject_IsTrueAfterScope)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        {
            shared_ref<jobject> scopedCopy(original);
            (void)scopedCopy;
        }

        EXPECT_TRUE(original.unique());
    }

    TEST(SharedRef, Unique_MoveCtorWithDefault_BothAreFalse)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> moved(AZStd::move(original));

        EXPECT_FALSE(original.unique());
        EXPECT_FALSE(moved.unique());
    }

    TEST(SharedRef, Unique_MoveCtorWithJobject_OriginalIsFalseMovedIsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> moved(AZStd::move(original));

        EXPECT_FALSE(original.unique());
        EXPECT_TRUE(moved.unique());
    }

    TEST(SharedRef, Unique_PolymorphicMoveCtorWithJfloatarryToJarray_OriginalIsFalseMovedIsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> moved(AZStd::move(original));

        EXPECT_FALSE(original.unique());
        EXPECT_TRUE(moved.unique());
    }

    TEST(SharedRef, Unique_AssignmentOperatorWithDefault_BothAreFalse)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> assigned = original;

        EXPECT_FALSE(original.unique());
        EXPECT_FALSE(assigned.unique());
    }

    TEST(SharedRef, Unique_AssignmentOperatorWithJobject_BothAreFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> assigned = original;

        EXPECT_FALSE(original.unique());
        EXPECT_FALSE(assigned.unique());
    }

    TEST(SharedRef, Unique_PolymorphicAssignmentOperatorWithJfloatarryToJarray_BothAreFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> assigned = original;

        EXPECT_FALSE(original.unique());
        EXPECT_FALSE(assigned.unique());
    }

    TEST(SharedRef, Unique_MoveOperatorWithDefault_BothAreFalse)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> moved = AZStd::move(original);

        EXPECT_FALSE(original.unique());
        EXPECT_FALSE(moved.unique());
    }

    TEST(SharedRef, Unique_MoveOperatorWithJobject_OriginalIsFalseMovedIsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> moved = AZStd::move(original);

        EXPECT_FALSE(original.unique());
        EXPECT_TRUE(moved.unique());
    }

    TEST(SharedRef, Unique_PolymorphicMoveOperatorWithJfloatarryToJarray_Ref1IsFalseRef2IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> moved = AZStd::move(original);

        EXPECT_FALSE(original.unique());
        EXPECT_TRUE(moved.unique());
    }

    TEST(SharedRef, Unique_ResetDefault_IsFalse)
    {
        shared_ref<jobject> ref;
        ref.reset();

        EXPECT_FALSE(ref.unique());
    }

    TEST(SharedRef, Unique_ResetJobject_IsFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        ref.reset();

        EXPECT_FALSE(ref.unique());
    }

    TEST(SharedRef, Unique_ResetDefaultWithJobject_IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref;
        ref.reset(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_TRUE(ref.unique());
    }

    TEST(SharedRef, Unique_ResetJobjectWithJobject_IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));
        ref.reset(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_TRUE(ref.unique());
    }

    TEST(SharedRef, Unique_SwapDefaultWithDefault_BothAreFalse)
    {
        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2;

        ref1.swap(ref2);

        EXPECT_FALSE(ref1.unique());
        EXPECT_FALSE(ref2.unique());
    }

    TEST(SharedRef, Unique_SwapDefaultWithJobject_Ref1IsTrueRef2IsFalse)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_TRUE(ref1);
        EXPECT_FALSE(ref2);
    }

    TEST(SharedRef, Unique_SwapJobjectWithDefault_Ref1IsFalseRef2IsTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2;

        ref1.swap(ref2);

        EXPECT_FALSE(ref1.unique());
        EXPECT_TRUE(ref2.unique());
    }

    TEST(SharedRef, Unique_SwapJobjectWithJobject_BothAreTrue)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_TRUE(ref1.unique());
        EXPECT_TRUE(ref2.unique());
    }


    // ----
    // ComparisonOperators Tests
    // ----

    TEST(SharedRef, ComparisonOperators_DefaultCtorAndNullptrCtor_AreEqual)
    {
        shared_ref<jobject> defaultRef;
        shared_ref<jobject> nullptrRef(nullptr);

        EXPECT_EQ(defaultRef, defaultRef);
        EXPECT_EQ(nullptrRef, nullptrRef);

        EXPECT_EQ(defaultRef, nullptrRef);
        EXPECT_EQ(nullptrRef, defaultRef);
    }

    TEST(SharedRef, ComparisonOperators_SelfJobjectCtor_AreEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_EQ(ref, ref);
    }

    TEST(SharedRef, ComparisonOperators_DefaultCtorJobjectCtor_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> defaultRef;
        shared_ref<jobject> validRef(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_NE(defaultRef, validRef);
        EXPECT_NE(validRef, defaultRef);
    }

    TEST(SharedRef, ComparisonOperators_JobjectCtorJobjectCtor_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        EXPECT_NE(ref1, ref2);
        EXPECT_NE(ref2, ref1);
    }

    TEST(SharedRef, ComparisonOperators_DefaultCtorJobjectCtorSwap_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1;
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_NE(ref1, ref2);
        EXPECT_NE(ref2, ref1);
    }


    TEST(SharedRef, ComparisonOperators_JobjectCtorDefaultCtorSwap_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2;

        ref1.swap(ref2);

        EXPECT_NE(ref1, ref2);
        EXPECT_NE(ref2, ref1);
    }

    TEST(SharedRef, ComparisonOperators_JobjectCtorJobjectCtorSwap_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2(jObject.InvokeObjectMethod<jobject>("GetObject"));

        ref1.swap(ref2);

        EXPECT_NE(ref1, ref2);
        EXPECT_NE(ref2, ref1);
    }

    TEST(SharedRef, ComparisonOperators_CopyCtorDefault_AreEqual)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> copy(original);

        EXPECT_EQ(original, copy);
        EXPECT_EQ(copy, original);
    }

    TEST(SharedRef, ComparisonOperators_CopyCtorWithJobject_AreEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(original);

        EXPECT_EQ(original, copy);
        EXPECT_EQ(copy, original);
    }

    TEST(SharedRef, ComparisonOperators_CopyCtorWithJobject2X_AllAreEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy1(original);
        shared_ref<jobject> copy2(copy1);

        EXPECT_EQ(original, copy1);
        EXPECT_EQ(copy1, original);

        EXPECT_EQ(original, copy2);
        EXPECT_EQ(copy2, original);

        EXPECT_EQ(copy2, copy1);
        EXPECT_EQ(copy1, copy2);
    }

    TEST(SharedRef, ComparisonOperators_CopyCtorWithJobjectResetRef1_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2(ref1);

        ref1.reset();

        EXPECT_NE(ref1, ref2);
        EXPECT_NE(ref2, ref1);
    }

    TEST(SharedRef, ComparisonOperators_CopyCtorWithJobjectResetRef2_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> ref1(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> ref2(ref1);

        ref2.reset();

        EXPECT_NE(ref1, ref2);
        EXPECT_NE(ref2, ref1);
    }

    TEST(SharedRef, ComparisonOperators_PolymorphicCopyCtorWithJfloatarryToJarray_AreEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> copy(original);

        EXPECT_EQ(original, copy);
        EXPECT_EQ(copy, original);
    }

    TEST(SharedRef, ComparisonOperators_MoveCtorWithJobject_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> copy(AZStd::move(original));

        EXPECT_NE(original, copy);
        EXPECT_NE(copy, original);
    }

    TEST(SharedRef, ComparisonOperators_PolymorphicMoveCtorWithJfloatarryToJarray_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> copy(AZStd::move(original));

        EXPECT_NE(original, copy);
        EXPECT_NE(copy, original);
    }

    TEST(SharedRef, ComparisonOperators_AssignmentOperatorWithDefault_AreEqual)
    {
        shared_ref<jobject> original;
        shared_ref<jobject> assigned = original;

        EXPECT_EQ(original, assigned);
        EXPECT_EQ(assigned, original);
    }

    TEST(SharedRef, ComparisonOperators_AssignmentOperatorWithJobject_AreEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> assigned = original;

        EXPECT_EQ(original, assigned);
        EXPECT_EQ(assigned, original);
    }

    TEST(SharedRef, ComparisonOperators_AssignmentOperatorWithJobject2X_AllAreEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> assigned1 = original;
        shared_ref<jobject> assigned2 = assigned1;

        EXPECT_EQ(original, assigned1);
        EXPECT_EQ(assigned1, original);

        EXPECT_EQ(original, assigned2);
        EXPECT_EQ(assigned2, original);

        EXPECT_EQ(assigned2, assigned1);
        EXPECT_EQ(assigned1, assigned2);
    }

    TEST(SharedRef, ComparisonOperators_AssignmentOperatorWithJobjectResetRef1_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> assigned = original;

        original.reset();

        EXPECT_NE(original, assigned);
        EXPECT_NE(assigned, original);
    }

    TEST(SharedRef, ComparisonOperators_AssignmentOperatorWithJobjectResetRef2_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> assigned = original;

        assigned.reset();

        EXPECT_NE(original, assigned);
        EXPECT_NE(assigned, original);
    }

    TEST(SharedRef, ComparisonOperators_PolymorphicAssignmentOperatorWithJfloatarryToJarray_AreEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> assigned = original;

        EXPECT_EQ(original, assigned);
        EXPECT_EQ(assigned, original);
    }

    TEST(SharedRef, ComparisonOperators_MoveOperatorWithJobject_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jobject> original(jObject.InvokeObjectMethod<jobject>("GetObject"));
        shared_ref<jobject> moved = AZStd::move(original);

        EXPECT_NE(original, moved);
        EXPECT_NE(moved, original);
    }

    TEST(SharedRef, ComparisonOperators_PolymorphicMoveOperatorWithJfloatarryToJarray_AreNotEqual)
    {
        JavaTestObject env;
        JniObject& jObject = env.m_object;

        shared_ref<jfloatArray> original(jObject.InvokeObjectMethod<jfloatArray>("GetFloatArray"));
        shared_ref<jarray> moved = AZStd::move(original);

        EXPECT_NE(original, moved);
        EXPECT_NE(moved, original);
    }
}
