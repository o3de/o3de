/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Android/Utils.h>
#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/JNI/Signature.h>

// Include Testing Framework Here


using namespace AZ::Android;


namespace UnitTest
{
    struct SimpleJavaObject
    {
        SimpleJavaObject()
            : m_classRef(nullptr)
            , m_objectRef(nullptr)
        {
            m_classRef = JNI::LoadClass("com/amazon/test/SimpleObject");

            JNIEnv* jniEnv = JNI::GetEnv();

            jmethodID constructorMethodId = jniEnv->GetMethodID(m_classRef, "<init>", "()V");
            jobject localObjectRef = jniEnv->NewObject(m_classRef, constructorMethodId);

            m_objectRef = jniEnv->NewGlobalRef(localObjectRef);

            jniEnv->DeleteLocalRef(localObjectRef);
        }

        ~SimpleJavaObject()
        {
            JNI::DeleteRef(m_objectRef);
        }

        jclass m_classRef;
        jobject m_objectRef;
    };


    // ----

    TEST(Signature, Sanity)
    {
        EXPECT_EQ(1, 1);
    }


    // ----
    // Generation Tests
    // ----

    TEST(Signature, Generate_NoArgs_IsEmptyString)
    {
        AZStd::string emptyStr = JNI::GetSignature();
        ASSERT_TRUE(emptyStr.empty());
    }

    TEST(Signature, Generate_DefaultNativeBooleanTypes_IsZ)
    {
        AZStd::string nativeTrueType = JNI::GetSignature(true);
        ASSERT_STREQ(nativeTrueType.c_str(), "Z");

        AZStd::string nativeFalseType = JNI::GetSignature(false);
        ASSERT_STREQ(nativeFalseType.c_str(), "Z");

        AZStd::string boolType = JNI::GetSignature(bool());
        ASSERT_STREQ(boolType.c_str(), "Z");

        AZStd::string allBoolTypes = JNI::GetSignature(true, false, bool());
        ASSERT_STREQ(allBoolTypes.c_str(), "ZZZ");
    }

    TEST(Signature, Generate_DefaultJBooleanTypes_IsZ)
    {
        AZStd::string jniTrueType = JNI::GetSignature(JNI_TRUE);
        ASSERT_STREQ(jniTrueType.c_str(), "Z");

        AZStd::string jniFalseType = JNI::GetSignature(JNI_FALSE);
        ASSERT_STREQ(jniFalseType.c_str(), "Z");

        AZStd::string jboolType = JNI::GetSignature(jboolean());
        ASSERT_STREQ(jboolType.c_str(), "Z");

        AZStd::string jniBoolArrayType = JNI::GetSignature(jbooleanArray());
        ASSERT_STREQ(jniBoolArrayType.c_str(), "[Z");

        AZStd::string allJBoolTypes = JNI::GetSignature(JNI_TRUE, JNI_FALSE, jboolean(), jbooleanArray());
        ASSERT_STREQ(allJBoolTypes.c_str(), "ZZZ[Z");
    }

    TEST(Signature, Generate_AllDefaultBooleanTypes_IsZ)
    {
        AZStd::string allBoolTypes = JNI::GetSignature(true, false, bool(), JNI_TRUE, JNI_FALSE, jboolean(), jbooleanArray());
        ASSERT_STREQ(allBoolTypes.c_str(), "ZZZZZZ[Z");
    }

    TEST(Signature, Generate_DefaultJByteTypes_IsB)
    {
        AZStd::string jbyteType = JNI::GetSignature(jbyte());
        ASSERT_STREQ(jbyteType.c_str(), "B");

        AZStd::string jbyteArrayType = JNI::GetSignature(jbyteArray());
        ASSERT_STREQ(jbyteArrayType.c_str(), "[B");

        AZStd::string allJByteTypes = JNI::GetSignature(jbyte(), jbyteArray());
        ASSERT_STREQ(allJByteTypes.c_str(), "B[B");
    }

    TEST(Signature, Generate_DefaultJCharTypes_IsC)
    {
        AZStd::string jcharType = JNI::GetSignature(jchar());
        ASSERT_STREQ(jcharType.c_str(), "C");

        AZStd::string jcharArrayType = JNI::GetSignature(jcharArray());
        ASSERT_STREQ(jcharArrayType.c_str(), "[C");

        AZStd::string allJCharTypes = JNI::GetSignature(jchar(), jcharArray());
        ASSERT_STREQ(allJCharTypes.c_str(), "C[C");
    }

    TEST(Signature, Generate_DefaultJShortTypes_IsS)
    {
        AZStd::string jshortType = JNI::GetSignature(jshort());
        ASSERT_STREQ(jshortType.c_str(), "S");

        AZStd::string jshortArrayType = JNI::GetSignature(jshortArray());
        ASSERT_STREQ(jshortArrayType.c_str(), "[S");

        AZStd::string allJShortTypes = JNI::GetSignature(jshort(), jshortArray());
        ASSERT_STREQ(allJShortTypes.c_str(), "S[S");
    }

    TEST(Signature, Generate_DefaultJIntTypes_IsI)
    {
        AZStd::string jintType = JNI::GetSignature(jint());
        ASSERT_STREQ(jintType.c_str(), "I");

        AZStd::string jintArrayType = JNI::GetSignature(jintArray());
        ASSERT_STREQ(jintArrayType.c_str(), "[I");

        AZStd::string allJIntTypes = JNI::GetSignature(jint(), jintArray());
        ASSERT_STREQ(allJIntTypes.c_str(), "I[I");
    }

    TEST(Signature, Generate_DefaultJLongTypes_IsJ)
    {
        AZStd::string jlongType = JNI::GetSignature(jlong());
        ASSERT_STREQ(jlongType.c_str(), "J");

        AZStd::string jlongArrayType = JNI::GetSignature(jlongArray());
        ASSERT_STREQ(jlongArrayType.c_str(), "[J");

        AZStd::string allJLongTypes = JNI::GetSignature(jlong(), jlongArray());
        ASSERT_STREQ(allJLongTypes.c_str(), "J[J");
    }

    TEST(Signature, Generate_DefaultJFloatTypes_IsF)
    {
        AZStd::string jfloatType = JNI::GetSignature(jfloat());
        ASSERT_STREQ(jfloatType.c_str(), "F");

        AZStd::string jfloatArrayType = JNI::GetSignature(jfloatArray());
        ASSERT_STREQ(jfloatArrayType.c_str(), "[F");

        AZStd::string allJFloatTypes = JNI::GetSignature(jfloat(), jfloatArray());
        ASSERT_STREQ(allJFloatTypes.c_str(), "F[F");
    }

    TEST(Signature, Generate_DefaultJDoubleTypes_IsD)
    {
        AZStd::string jdoubleType = JNI::GetSignature(jdouble());
        ASSERT_STREQ(jdoubleType.c_str(), "D");

        AZStd::string jdoubleArrayType = JNI::GetSignature(jdoubleArray());
        ASSERT_STREQ(jdoubleArrayType.c_str(), "[D");

        AZStd::string allJDoubleTypes = JNI::GetSignature(jdouble(), jdoubleArray());
        ASSERT_STREQ(allJDoubleTypes.c_str(), "D[D");
    }

    TEST(Signature, Generate_DefaultJStringTypes_IsLjava_lang_String)
    {
        AZStd::string jstringType = JNI::GetSignature(jstring());
        ASSERT_STREQ(jstringType.c_str(), "Ljava/lang/String;");
    }

    TEST(Signature, Generate_DefaultJClassTypes_IsLjava_lang_Class)
    {
        AZStd::string jclassType = JNI::GetSignature(jclass());
        ASSERT_STREQ(jclassType.c_str(), "Ljava/lang/Class;");
    }

    TEST(Signature, Generate_DefaultJObjectType_IsEmptyString)
    {
        AZStd::string jobjectType = JNI::GetSignature(jobject());
        ASSERT_TRUE(jobjectType.empty());

        AZStd::string jobjectArrayType = JNI::GetSignature(jobjectArray());
        ASSERT_TRUE(jobjectArrayType.empty());
    }

    TEST(Signature, Generate_SimpleJObjectType_IsLcom_amazon_test_SimpleObject)
    {
        SimpleJavaObject simpleObject;

        AZStd::string simpleObjectType = JNI::GetSignature(simpleObject.m_objectRef);
        ASSERT_STREQ(simpleObjectType.c_str(), "Lcom/amazon/test/SimpleObject;");
    }

    TEST(Signature, Generate_AllDefaultPrimtiveTypes_IsZZZBBCCSSIIJJFFDD)
    {
        AZStd::string allPrimitiveTypes = JNI::GetSignature(
                bool(), jboolean(), jbooleanArray(),
                jbyte(), jbyteArray(),
                jchar(), jcharArray(),
                jshort(), jshortArray(),
                jint(), jintArray(),
                jlong(), jlongArray(),
                jfloat(), jfloatArray(),
                jdouble(), jdoubleArray()
        );
        ASSERT_STREQ(allPrimitiveTypes.c_str(), "ZZ[ZB[BC[CS[SI[IJ[JF[FD[D");
    }

    TEST(Signature, Generate_DefaultJStringJClassTypes_IsLjava_lang_StringLjava_lang_Class)
    {
        AZStd::string jstringJClassTypes  = JNI::GetSignature(jstring(), jclass());
        ASSERT_STREQ(jstringJClassTypes.c_str(), "Ljava/lang/String;Ljava/lang/Class;");
    }

    TEST(Signature, Generate_AllTypes_IsZZZBBCCSSIIJJFFDDLjava_lang_StringLjava_lang_ClassLcom_amazon_test_SimpleObject)
    {
        SimpleJavaObject simpleObject;

        AZStd::string allTypes  = JNI::GetSignature(
                bool(), jboolean(), jbooleanArray(),
                jbyte(), jbyteArray(),
                jchar(), jcharArray(),
                jshort(), jshortArray(),
                jint(), jintArray(),
                jlong(), jlongArray(),
                jfloat(), jfloatArray(),
                jdouble(), jdoubleArray(),
                jstring(), jclass(),
                simpleObject.m_objectRef
        );
        ASSERT_STREQ(allTypes.c_str(), "ZZ[ZB[BC[CS[SI[IJ[JF[FD[DLjava/lang/String;Ljava/lang/Class;Lcom/amazon/test/SimpleObject;");
    }


    // ----
    // Validation Tests
    // ----


    TEST(Signature, Validate_NoArgs_IsEmptyString)
    {
        ASSERT_TRUE(JNI::ValidateSignature(""));
    }

    TEST(Signature, Validate_DefaultNativeBooleanTypes_IsZ)
    {
        ASSERT_TRUE(JNI::ValidateSignature("Z", true));
        ASSERT_TRUE(JNI::ValidateSignature("Z", false));
        ASSERT_TRUE(JNI::ValidateSignature("Z", bool()));
    }

    TEST(Signature, Validate_DefaultJBooleanTypes_IsZ)
    {
        ASSERT_TRUE(JNI::ValidateSignature("Z", JNI_TRUE));
        ASSERT_TRUE(JNI::ValidateSignature("Z", JNI_FALSE));
        ASSERT_TRUE(JNI::ValidateSignature("Z", jboolean()));
        ASSERT_TRUE(JNI::ValidateSignature("[Z", jbooleanArray()));
    }

    TEST(Signature, Validate_AllDefaultBooleanTypes_IsZ)
    {
        ASSERT_TRUE(JNI::ValidateSignature("ZZZ", true, false, bool()));
        ASSERT_TRUE(JNI::ValidateSignature("ZZZ[Z", JNI_TRUE, JNI_FALSE, jboolean(), jbooleanArray()));

        ASSERT_TRUE(JNI::ValidateSignature("ZZZZZZ[Z",
                true, false, bool(),
                JNI_TRUE, JNI_FALSE, jboolean(),
                jbooleanArray()));
    }

    TEST(Signature, Validate_DefaultJByteTypes_IsB)
    {
        ASSERT_TRUE(JNI::ValidateSignature("B", jbyte()));
        ASSERT_TRUE(JNI::ValidateSignature("[B", jbyteArray()));
    }

    TEST(Signature, Validate_AllDefaultJByteTypes_IsB)
    {
        ASSERT_TRUE(JNI::ValidateSignature("B[B", jbyte(), jbyteArray()));
    }

    TEST(Signature, Validate_DefaultJCharTypes_IsC)
    {
        ASSERT_TRUE(JNI::ValidateSignature("C", jchar()));
        ASSERT_TRUE(JNI::ValidateSignature("[C", jcharArray()));
    }

    TEST(Signature, Validate_AllDefaultJCharTypes_IsC)
    {
        ASSERT_TRUE(JNI::ValidateSignature("C[C", jchar(), jcharArray()));
    }

    TEST(Signature, Validate_DefaultJShortTypes_IsS)
    {
        ASSERT_TRUE(JNI::ValidateSignature("S", jshort()));
        ASSERT_TRUE(JNI::ValidateSignature("[S", jshortArray()));
    }

    TEST(Signature, Validate_AllDefaultJShortTypes_IsS)
    {
        ASSERT_TRUE(JNI::ValidateSignature("S[S", jshort(), jshortArray()));
    }

    TEST(Signature, Validate_DefaultJIntTypes_IsI)
    {
        ASSERT_TRUE(JNI::ValidateSignature("I", jint()));
        ASSERT_TRUE(JNI::ValidateSignature("[I", jintArray()));
    }

    TEST(Signature, Validate_AllDefaultJIntTypes_IsI)
    {
        ASSERT_TRUE(JNI::ValidateSignature("I[I", jint(), jintArray()));
    }
    TEST(Signature, Validate_DefaultJLongTypes_IsJ)
    {

        ASSERT_TRUE(JNI::ValidateSignature("J", jlong()));
        ASSERT_TRUE(JNI::ValidateSignature("[J", jlongArray()));
    }

    TEST(Signature, Validate_AllDefaultJLongTypes_IsJ)
    {
        ASSERT_TRUE(JNI::ValidateSignature("J[J", jlong(), jlongArray()));
    }

    TEST(Signature, Validate_DefaultJFloatTypes_IsF)
    {
        ASSERT_TRUE(JNI::ValidateSignature("F", jfloat()));
        ASSERT_TRUE(JNI::ValidateSignature("[F", jfloatArray()));
    }

    TEST(Signature, Validate_AllDefaultJFloatTypes_IsF)
    {
        ASSERT_TRUE(JNI::ValidateSignature("F[F", jfloat(), jfloatArray()));
    }

    TEST(Signature, Validate_DefaultJDoubleTypes_IsD)
    {
        ASSERT_TRUE(JNI::ValidateSignature("D", jdouble()));
        ASSERT_TRUE(JNI::ValidateSignature("[D", jdoubleArray()));
    }

    TEST(Signature, Validate_AllDefaultJDoubleTypes_IsD)
    {
        ASSERT_TRUE(JNI::ValidateSignature("D[D", jdouble(), jdoubleArray()));
    }

    TEST(Signature, Validate_AllDefaultPrimtiveTypes_IsZZZBBCCSSIIJJFFDD)
    {
        ASSERT_TRUE(JNI::ValidateSignature(
                "ZZ[ZB[BC[CS[SI[IJ[JF[FD[D",
                bool(), jboolean(), jbooleanArray(),
                jbyte(), jbyteArray(),
                jchar(), jcharArray(),
                jshort(), jshortArray(),
                jint(), jintArray(),
                jlong(), jlongArray(),
                jfloat(), jfloatArray(),
                jdouble(), jdoubleArray()
        ));
    }

    TEST(Signature, Validate_JClass_IsL_java_lang_Class)
    {
        jclass signatureClass = JNI::LoadClass("com/amazon/test/SimpleObject");
        ASSERT_TRUE(JNI::ValidateSignature("Ljava/lang/Class;", signatureClass));
    }

    TEST(Signature, Validate_JString_IsL_java_lang_String)
    {
        JNIEnv* jniEnv = JNI::GetEnv();
        jstring javaString = jniEnv->NewStringUTF("Test");
        EXPECT_TRUE(JNI::ValidateSignature("Ljava/lang/String;", javaString));
        jniEnv->DeleteLocalRef(javaString);
    }

    TEST(Signature, Validate_SimpleJObjectType_IsLcom_amazon_test_SimpleObject)
    {
        SimpleJavaObject simpleObject;
        ASSERT_TRUE(JNI::ValidateSignature("Lcom/amazon/test/SimpleObject;", simpleObject.m_objectRef));
    }

    TEST(Signature, Validate_PolymorphicActivityType_IsLandroid_app_Activity)
    {
        jobject activity = Utils::GetActivityRef();
        ASSERT_TRUE(JNI::ValidateSignature("Landroid/app/Activity;", activity));
    }

    TEST(Signature, Validate_JStringJClass_IsLjava_lang_StringLjava_lang_Class)
    {
        JNIEnv* jniEnv = JNI::GetEnv();
        jclass signatureClass = JNI::LoadClass("com/amazon/test/SimpleObject");
        jstring javaString = jniEnv->NewStringUTF("Test");

        EXPECT_TRUE(JNI::ValidateSignature("Ljava/lang/String;Ljava/lang/Class;", javaString, signatureClass));
        jniEnv->DeleteLocalRef(javaString);
    }

    TEST(Signature, Validate_AllTypes_IsZZZBBCCSSIIJJFFDDL_java_lang_StringL_java_lang_ClassLcom_amazon_test_SimpleObjectLandroid_app_Activity)
    {
        JNIEnv* jniEnv = JNI::GetEnv();
        jstring javaString = jniEnv->NewStringUTF("Test");

        SimpleJavaObject simpleObject;

        jobject activity = Utils::GetActivityRef();

        ASSERT_TRUE(JNI::ValidateSignature(
                "ZZ[ZB[BC[CS[SI[IJ[JF[FD[DLjava/lang/String;Ljava/lang/Class;Lcom/amazon/test/SimpleObject;Landroid/app/Activity;",
                bool(), jboolean(), jbooleanArray(),
                jbyte(), jbyteArray(),
                jchar(), jcharArray(),
                jshort(), jshortArray(),
                jint(), jintArray(),
                jlong(), jlongArray(),
                jfloat(), jfloatArray(),
                jdouble(), jdoubleArray(),
                javaString,
                simpleObject.m_classRef,
                simpleObject.m_objectRef,
                activity
        ));

        jniEnv->DeleteLocalRef(javaString);
    }

    TEST(Signature, Validate_ExtraParams_IsFalse)
    {
        ASSERT_FALSE(JNI::ValidateSignature("Z", JNI_TRUE, JNI_TRUE));
    }

    TEST(Signature, Validate_MissingParams_IsFalse)
    {
        ASSERT_FALSE(JNI::ValidateSignature("ZZ", JNI_TRUE));
    }

    TEST(Signature, Validate_WrongParms_IsFalse)
    {
        ASSERT_FALSE(JNI::ValidateSignature("ZI", JNI_TRUE, jfloat()));
    }

} // namespace UnitTest
