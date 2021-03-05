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


/*
* This suite of tests focus on the unique features the ConvertIterator
* adds as an iterator. The basic functionality and iterator conformity is tested
* in the Iterator Conformity Tests (see IteratorConformityTests.cpp).
*/

#include <AzTest/AzTest.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/Tests/Containers/Views/IteratorTestsBase.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                using ::testing::Const;

                class MockClass
                {
                public:
                    virtual ~MockClass() = default;

                    MOCK_METHOD0(TestFunction, void());
                    MOCK_CONST_METHOD0(TestFunction, void());
                };

                // Google mock has problems with putting mock classes in
                // containers so for those cases use a simpler mock.
                class TestClass
                {
                public:
                    enum Caller
                    {
                        NotCalled,
                        NonConstFunction,
                        ConstFunction
                    };
                    void TestFunction()
                    {
                        m_calling = NonConstFunction;
                    }

                    void TestFunction() const
                    {
                        m_calling = ConstFunction;
                    }

                    mutable Caller m_calling = { NotCalled };
                };

                inline float ConvertIntToFloat(int& value)
                {
                    return static_cast<float>(value);
                }

                inline int ConvertFloatToInt(float& value)
                {
                    return static_cast<int>(value);
                }

                template<typename CollectionType>
                class ConvertIteratorTests
                    : public IteratorTypedTestsBase<CollectionType>
                {
                public:
                    ConvertIteratorTests() = default;
                    ~ConvertIteratorTests() override = default;
                };

                TYPED_TEST_CASE_P(ConvertIteratorTests);

                // MakeConvertIterator
                TYPED_TEST_P(ConvertIteratorTests, MakeConvertIterator_FunctionComparedWithExplicitlyDeclaredIterator_IteratorsAreEqual)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    auto lhsIterator = MakeConvertIterator(this->GetBaseIterator(0), ConvertIntToFloat);
                    auto rhsIterator = ConvertIterator<BaseIterator, float>(this->GetBaseIterator(0), ConvertIntToFloat);
                    EXPECT_EQ(lhsIterator, rhsIterator);
                }

                // MakeConvertView
                TYPED_TEST_P(ConvertIteratorTests, MakeConvertView_IteratorVersionComparedWithExplicitlyDeclaredIterators_ViewHasEquivalentBeginAndEnd)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    auto view = MakeConvertView(this->m_testCollection.begin(), this->m_testCollection.end(), ConvertIntToFloat);
                    auto begin = ConvertIterator<BaseIterator, float>(this->m_testCollection.begin(), ConvertIntToFloat);
                    auto end = ConvertIterator<BaseIterator, float>(this->m_testCollection.end(), ConvertIntToFloat);
                    EXPECT_EQ(view.begin(), begin);
                    EXPECT_EQ(view.end(), end);
                }

                TYPED_TEST_P(ConvertIteratorTests, MakeConvertView_ViewVersionComparedWithExplicitlyDeclaredIterators_ViewHasEquivalentBeginAndEnd)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    auto sourceView = MakeView(this->m_testCollection.begin(), this->m_testCollection.end());
                    auto view = MakeConvertView(sourceView, ConvertIntToFloat);
                    auto begin = ConvertIterator<BaseIterator, float>(this->m_testCollection.begin(), ConvertIntToFloat);
                    auto end = ConvertIterator<BaseIterator, float>(this->m_testCollection.end(), ConvertIntToFloat);
                    EXPECT_EQ(view.begin(), begin);
                    EXPECT_EQ(view.end(), end);
                }

                REGISTER_TYPED_TEST_CASE_P(ConvertIteratorTests,
                    MakeConvertIterator_FunctionComparedWithExplicitlyDeclaredIterator_IteratorsAreEqual,
                    MakeConvertView_IteratorVersionComparedWithExplicitlyDeclaredIterators_ViewHasEquivalentBeginAndEnd,
                    MakeConvertView_ViewVersionComparedWithExplicitlyDeclaredIterators_ViewHasEquivalentBeginAndEnd
                    );

                INSTANTIATE_TYPED_TEST_CASE_P(CommonTests, ConvertIteratorTests, BasicCollectionTypes);

                // Casting
                TEST(ConvertIterator, Casting_CanBetweenValueTypes_GetCastedValueAsInt)
                {
                    AZStd::vector<float> floatValues = { 3.1415f };
                    auto iterator = MakeConvertIterator(floatValues.begin(), ConvertFloatToInt);
                    auto value = *iterator;
                    EXPECT_EQ(3, value);
                }

                const MockClass* ConvertMockToConstMock(MockClass* value)
                {
                    return const_cast<const MockClass*>(value);
                }

                TEST(ConvertIterator, Casting_CanApplyConstToPointerThroughStaticFunction_ConstFunctionCalled)
                {
                    MockClass mock;
                    EXPECT_CALL(Const(mock), TestFunction()).Times(1);
                    EXPECT_CALL(mock, TestFunction()).Times(0);

                    AZStd::vector<MockClass*> mocks = { &mock };

                    auto iterator = MakeConvertIterator(mocks.begin(), ConvertMockToConstMock);
                    (*iterator)->TestFunction();
                }

                TEST(ConvertIterator, Casting_CanApplyConstToPointerThroughLamba_ConstFunctionCalled)
                {
                    MockClass mock;
                    EXPECT_CALL(Const(mock), TestFunction()).Times(1);
                    EXPECT_CALL(mock, TestFunction()).Times(0);

                    AZStd::vector<MockClass*> mocks = { &mock };

                    auto iterator = MakeConvertIterator(mocks.begin(),
                            [](MockClass* value) -> const MockClass*
                            {
                                return const_cast<const MockClass*>(value);
                            });
                    (*iterator)->TestFunction();
                }

                TEST(ConvertIterator, Casting_CanApplyConstToValueThroughDereference_ConstFunctionCalled)
                {
                    TestClass test;
                    AZStd::vector<TestClass> tests = { test };

                    auto iterator = MakeConvertIterator(tests.begin(),
                            [](TestClass& value) -> const TestClass&
                            {
                                return value;
                            });
                    (*iterator).TestFunction();
                    EXPECT_EQ(TestClass::ConstFunction, tests[0].m_calling);
                }

                TEST(ConvertIterator, Casting_CanApplyConstToValueThroughArrowOperator_ConstFunctionCalled)
                {
                    TestClass test;
                    AZStd::vector<TestClass> tests = { test };

                    auto iterator = MakeConvertIterator(tests.begin(),
                            [](TestClass& value) -> const TestClass&
                            {
                                return value;
                            });
                    iterator->TestFunction();
                    EXPECT_EQ(TestClass::ConstFunction, tests[0].m_calling);
                }

                TEST(ConvertIterator, Casting_CanApplyConstToValueThroughIndex_ConstFunctionCalled)
                {
                    TestClass test;
                    AZStd::vector<TestClass> tests = { test };

                    auto iterator = MakeConvertIterator(tests.begin(),
                            [](TestClass& value) -> const TestClass&
                            {
                                return value;
                            });
                    iterator[0].TestFunction();
                    EXPECT_EQ(TestClass::ConstFunction, tests[0].m_calling);
                }

                TEST(ConvertIterator, Casting_CanApplyConstToUniquePtrValue_ConstFunctionCalled)
                {
                    AZStd::unique_ptr<MockClass> mock(new MockClass());
                    EXPECT_CALL(Const(*mock), TestFunction()).Times(1);
                    EXPECT_CALL(*mock, TestFunction()).Times(0);

                    AZStd::vector<AZStd::unique_ptr<MockClass> > tests;
                    tests.push_back(AZStd::move(mock));

                    auto iterator = MakeConvertIterator(tests.begin(),
                            [](AZStd::unique_ptr<MockClass>& value)
                            -> const MockClass*
                            {
                                return value.get();
                            });
                    (*iterator)->TestFunction();
                }

                TEST(ConvertIterator, Casting_CanApplyConstToSharedPtr_ConstFunctionCalled)
                {
                    AZStd::shared_ptr<MockClass> mock = AZStd::make_shared<MockClass>();
                    EXPECT_CALL(Const(*mock), TestFunction()).Times(1);
                    EXPECT_CALL(*mock, TestFunction()).Times(0);

                    AZStd::vector<AZStd::shared_ptr<MockClass> > tests;
                    tests.push_back(AZStd::move(mock));

                    auto iterator = MakeConvertIterator(tests.begin(),
                            [](AZStd::shared_ptr<MockClass>& value)
                            {
                                return AZStd::shared_ptr<const MockClass>(value);
                            });
                    (*iterator)->TestFunction();
                }

                // Algorithms
                TEST(ConvertIterator, Algorithms_Find3_FindsFirstInstanceOf3)
                {
                    AZStd::vector<float> values = { 8.3f, 3.1f, 4.6f, 3.3f, 9.9f, 6.1f };

                    auto convertView = MakeConvertView(values.begin(), values.end(), ConvertFloatToInt);
                    auto result = AZStd::find(convertView.begin(), convertView.end(), 3);
                    ASSERT_NE(convertView.end(), result);
                    EXPECT_EQ(3, *result);
                    EXPECT_EQ(1, AZStd::distance(convertView.begin(), result));
                }

                TEST(ConvertIterator, Algorithms_Count_ThreeInstanceOfFourAreFound)
                {
                    AZStd::vector<float> values = { 8.3f, 4.1f, 4.6f, 4.3f, 9.9f, 6.1f };

                    auto convertView = MakeConvertView(values.begin(), values.end(), ConvertFloatToInt);
                    size_t result = AZStd::count_if(convertView.begin(), convertView.end(),
                            [](int value)
                            {
                                return value == 4;
                            });
                    EXPECT_EQ(3, result);
                }
            } // namespace Views
        } // namespace Containers
    } // namespace SceneAPI
} // namespace AZ
