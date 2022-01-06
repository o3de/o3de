/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/TupleSerializer.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    namespace TupleSerializerTestsInternal
    {
        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features)
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.m_fixedSizeArray = true;
            features.m_requiresTypeIdLookups = true;
        }
    }

    class PairTestDescription final :
        public JsonSerializerConformityTestDescriptor<AZStd::pair<int, double>>
    {
    public:
        using Pair = AZStd::pair<int, double>;

        // Needed to make sure the instance of the pair can be registered properly.
        struct PairPlaceholder
        {
            AZ_TYPE_INFO(PairPlaceholder, "{27A2191C-989A-4A9D-8B0C-73909F65A87C}");
            Pair m_pair;
        };

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonTupleSerializer>();
        }

        AZStd::shared_ptr<Pair> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Pair>(0, 0.0);
        }

        AZStd::shared_ptr<Pair> CreatePartialDefaultInstance() override
        {
            return AZStd::make_shared<Pair>(0, 288.0);
        }

        AZStd::shared_ptr<Pair> CreateFullySetInstance() override
        {
            return AZStd::make_shared<Pair>(188, 288.0);
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return "[{}, 288.0]";
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[188, 288.0]";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            TupleSerializerTestsInternal::ConfigureFeatures(features);
        }

        using JsonSerializerConformityTestDescriptor<AZStd::pair<int, double>>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->Class<PairPlaceholder>()->Field("pair", &PairPlaceholder::m_pair);
        }

        bool AreEqual(const Pair& lhs, const Pair& rhs) override
        {
            return lhs == rhs;
        }
    };



    class TupleTestDescription final :
        public JsonSerializerConformityTestDescriptor<AZStd::tuple<int, double, float>>
    {
    public:
        using Tuple = AZStd::tuple<int, double, float>;

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonTupleSerializer>();
        }

        AZStd::shared_ptr<Tuple> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Tuple>(0, 0.0, 0.0f);
        }

        AZStd::shared_ptr<Tuple> CreatePartialDefaultInstance() override
        {
            return AZStd::make_shared<Tuple>(0, 288.0, 0.0f);
        }

        AZStd::shared_ptr<Tuple> CreateFullySetInstance() override
        {
            return AZStd::make_shared<Tuple>(188, 288.0, 388.0f);
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return "[{}, 288.0, {}]";
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[188, 288.0, 388.0]";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            TupleSerializerTestsInternal::ConfigureFeatures(features);
        }

        using JsonSerializerConformityTestDescriptor<Tuple>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Tuple>();
        }

        bool AreEqual(const Tuple& lhs, const Tuple& rhs) override
        {
            return lhs == rhs;
        }
    };



    class TupleClass
    {
    public:
        AZ_CLASS_ALLOCATOR(TupleClass, AZ::SystemAllocator, 0);
        AZ_RTTI(TupleClass, "{DF7BDAFC-34D5-48EE-85CB-845856971D9E}");

        int m_var1{ 142 };
        double m_var2{ 242.0 };

        TupleClass() = default;
        explicit TupleClass(double var2)
            : m_var2(var2)
        {}
        
        TupleClass(int var1, double var2)
            : m_var1(var1)
            , m_var2(var2)
        {}
        
        virtual ~TupleClass() = default;
    };

    class TupleBaseClass
    {
    public:
        AZ_CLASS_ALLOCATOR(TupleBaseClass, AZ::SystemAllocator, 0);
        AZ_RTTI(TupleBaseClass, "{1073A9F8-6D99-4FBB-958D-E29D2A66C6E7}");

        int m_var1{ 142 };

        TupleBaseClass() = default;
        explicit TupleBaseClass(int var1)
            : m_var1(var1)
        {}
        virtual ~TupleBaseClass() = default;
    };
    
    class TupleDefaultDerivedClass
        : public TupleBaseClass
    {
    public:
        AZ_CLASS_ALLOCATOR(TupleDefaultDerivedClass, AZ::SystemAllocator, 0);
        AZ_RTTI(TupleDefaultDerivedClass, "{979B6935-5779-461C-A537-A03472315D8D}", TupleBaseClass);
        ~TupleDefaultDerivedClass() override = default;

        double m_varAlternative{ -444.0 };
    };

    class TupleDerivedClass
        : public TupleBaseClass
    {
    public:
        AZ_CLASS_ALLOCATOR(TupleDerivedClass, AZ::SystemAllocator, 0);
        AZ_RTTI(TupleDerivedClass, "{8D2515EC-F85F-4D61-9024-7E1C184E135E}", TupleBaseClass);

        double m_var2{ 242.0 };

        TupleDerivedClass() = default;
        explicit TupleDerivedClass(double var2)
            : m_var2(var2)
        {}

        TupleDerivedClass(int var1, double var2)
            : TupleBaseClass(var1)
            , m_var2(var2)
        {}
        ~TupleDerivedClass() override = default;
    };

    class ComplexTupleTestDescription final :
        public JsonSerializerConformityTestDescriptor
        <
            AZStd::tuple<TupleClass, TupleClass*, TupleBaseClass*, TupleClass*, TupleBaseClass*>
        >
    {
    public:
        using Tuple = AZStd::tuple<TupleClass, TupleClass*, TupleBaseClass*, TupleClass*, TupleBaseClass*>;

        static void Delete(Tuple* tuple)
        {
            delete AZStd::get<1>(*tuple);
            delete AZStd::get<2>(*tuple);
            delete AZStd::get<3>(*tuple);
            delete AZStd::get<4>(*tuple);
            delete tuple;
        }

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonTupleSerializer>();
        }

        AZStd::shared_ptr<Tuple> CreateDefaultInstance() override
        {
            return AZStd::shared_ptr<Tuple>
                (new Tuple(
                    TupleClass(), 
                    aznew TupleClass(), 
                    aznew TupleDefaultDerivedClass(),
                    nullptr,
                    nullptr
                ), &ComplexTupleTestDescription::Delete);
        }

        AZStd::shared_ptr<Tuple> CreatePartialDefaultInstance() override
        {
            return AZStd::shared_ptr<Tuple>(
                new Tuple(
                    TupleClass(288.0), 
                    aznew TupleClass(288.0), 
                    aznew TupleDerivedClass(288.0),
                    aznew TupleClass(288.0),
                    aznew TupleDerivedClass(288.0)
                ), &ComplexTupleTestDescription::Delete);
        }

        AZStd::shared_ptr<Tuple> CreateFullySetInstance() override
        {
            return AZStd::shared_ptr<Tuple>(
                new Tuple(
                    TupleClass(188, 288.0), 
                    aznew TupleClass(188, 288.0), 
                    aznew TupleDerivedClass(188, 288.0),
                    aznew TupleClass(188, 288.0),
                    aznew TupleDerivedClass(188, 288.0)
                ), &ComplexTupleTestDescription::Delete);
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
                    [
                        { "var2": 288.0 },
                        { "var2": 288.0 },
                        { "$type": "TupleDerivedClass", "var2": 288.0 },
                        { "var2": 288.0 },
                        { "$type": "TupleDerivedClass", "var2": 288.0 }
                    ]
                )";
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                    [
                        {
                            "var1": 188, 
                            "var2": 288.0
                        },
                        {
                            "var1": 188, 
                            "var2": 288.0
                        },
                        {
                            "$type": "TupleDerivedClass",
                            "var1": 188, 
                            "var2": 288.0
                        },
                        {
                            "var1": 188, 
                            "var2": 288.0
                        },
                        {
                            "$type": "TupleDerivedClass",
                            "var1": 188, 
                            "var2": 288.0
                        }
                    ]
                )";
        }

        AZStd::string_view GetCorruptedJson() override
        {
            return R"(
                    [
                        null,
                        {
                            "var1": 188, 
                            "var2": 288.0
                        },
                        {
                            "$type": "NonExistingClass",
                            "var1": 188, 
                            "var2": 288.0
                        },
                        {
                            "var1": 188, 
                            "var2": 288.0
                        },
                        {
                            "$type": "TupleDerivedClass",
                            "var1": 188, 
                            "var2": 288.0
                        }
                    ]
                )";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            TupleSerializerTestsInternal::ConfigureFeatures(features);
            features.m_supportsPartialInitialization = true;
            features.m_enableNewInstanceTests = false;
        }

        using JsonSerializerConformityTestDescriptor::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->Class<TupleClass>()
                ->Field("var1", &TupleClass::m_var1)
                ->Field("var2", &TupleClass::m_var2);
            context->Class<TupleBaseClass>()
                ->Field("var1", &TupleBaseClass::m_var1);
            context->Class<TupleDefaultDerivedClass, TupleBaseClass>()
                ->Field("var_alternative", &TupleDefaultDerivedClass::m_varAlternative);
            context->Class<TupleDerivedClass, TupleBaseClass>()
                ->Field("var2", &TupleDerivedClass::m_var2);
            context->RegisterGenericType<Tuple>();
        }

        bool AreEqual(const Tuple& lhs, const Tuple& rhs) override
        {
            if (!AZStd::get<3>(lhs) && !AZStd::get<3>(lhs) &&
                !AZStd::get<4>(lhs) && !AZStd::get<4>(lhs))
            {
                TupleDefaultDerivedClass* lhs2 = azrtti_cast<TupleDefaultDerivedClass*>(AZStd::get<2>(lhs));
                TupleDefaultDerivedClass* rhs2 = azrtti_cast<TupleDefaultDerivedClass*>(AZStd::get<2>(rhs));

                if (!lhs2 || !rhs2)
                {
                    return false;
                }

                return
                    AZStd::get<0>(lhs).m_var1 == AZStd::get<0>(rhs).m_var1 &&
                    AZStd::get<0>(lhs).m_var2 == AZStd::get<0>(rhs).m_var2 &&

                    AZStd::get<1>(lhs)->m_var1 == AZStd::get<1>(rhs)->m_var1 &&
                    AZStd::get<1>(lhs)->m_var2 == AZStd::get<1>(rhs)->m_var2 &&

                    lhs2->m_var1 == rhs2->m_var1 &&
                    lhs2->m_varAlternative == rhs2->m_varAlternative;
            }
            else
            {
                if (azrtti_typeid(AZStd::get<2>(lhs)) != azrtti_typeid<TupleDerivedClass>())
                {
                    return false;
                }
                if (azrtti_typeid(AZStd::get<2>(rhs)) != azrtti_typeid<TupleDerivedClass>())
                {
                    return false;
                }

                if (azrtti_typeid(AZStd::get<4>(lhs)) != azrtti_typeid<TupleDerivedClass>())
                {
                    return false;
                }
                if (azrtti_typeid(AZStd::get<4>(rhs)) != azrtti_typeid<TupleDerivedClass>())
                {
                    return false;
                }

                TupleDerivedClass* lhs2 = azrtti_cast<TupleDerivedClass*>(AZStd::get<2>(lhs));
                TupleDerivedClass* rhs2 = azrtti_cast<TupleDerivedClass*>(AZStd::get<2>(rhs));

                TupleDerivedClass* lhs4 = azrtti_cast<TupleDerivedClass*>(AZStd::get<4>(lhs));
                TupleDerivedClass* rhs4 = azrtti_cast<TupleDerivedClass*>(AZStd::get<4>(rhs));

                return
                    AZStd::get<0>(lhs).m_var1 == AZStd::get<0>(rhs).m_var1 &&
                    AZStd::get<0>(lhs).m_var2 == AZStd::get<0>(rhs).m_var2 &&

                    AZStd::get<1>(lhs)->m_var1 == AZStd::get<1>(rhs)->m_var1 &&
                    AZStd::get<1>(lhs)->m_var2 == AZStd::get<1>(rhs)->m_var2 &&

                    lhs2->m_var1 == rhs2->m_var1 &&
                    lhs2->m_var2 == rhs2->m_var2 &&

                    AZStd::get<3>(lhs)->m_var1 == AZStd::get<3>(rhs)->m_var1 &&
                    AZStd::get<3>(lhs)->m_var2 == AZStd::get<3>(rhs)->m_var2 &&

                    lhs4->m_var1 == rhs4->m_var1 &&
                    lhs4->m_var2 == rhs4->m_var2;
            }
        }
    };

    class NestedTupleTestDescription final :
        public JsonSerializerConformityTestDescriptor
        <
            AZStd::tuple<AZStd::vector<int>, AZStd::pair<int, AZStd::string>>
        >
    {
    public:
        using Tuple = AZStd::tuple<AZStd::vector<int>, AZStd::pair<int, AZStd::string>>;

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonTupleSerializer>();
        }

        AZStd::shared_ptr<Tuple> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Tuple>(
                AZStd::vector<int>(), 
                AZStd::make_pair(0, ""));
        }

        AZStd::shared_ptr<Tuple> CreatePartialDefaultInstance() override
        {
            return AZStd::make_shared<Tuple>(
                AZStd::vector<int>(),
                AZStd::make_pair(0, "hello"));
        }

        AZStd::shared_ptr<Tuple> CreateFullySetInstance() override
        {
            return AZStd::make_shared<Tuple>(
                AZStd::vector<int>{ 188, 288, 388 }, 
                AZStd::make_pair(488, "hello"));
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"([{}, [{}, "hello"]])";
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"([[188, 288, 388], [488, "hello"]])";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            TupleSerializerTestsInternal::ConfigureFeatures(features);
            features.m_typeToInject = rapidjson::kNullType;
        }

        using JsonSerializerConformityTestDescriptor<Tuple>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Tuple>();
        }

        bool AreEqual(const Tuple& lhs, const Tuple& rhs) override
        {
            const AZStd::vector<int>& lhsVector = AZStd::get<0>(lhs);
            const AZStd::vector<int>& rhsVector = AZStd::get<0>(rhs);
            
            if (lhsVector.size() != rhsVector.size())
            {
                return false;
            }

            size_t vectorSize = lhsVector.size();
            for (size_t i = 0; i < vectorSize; ++i)
            {
                if (lhsVector[i] != rhsVector[i])
                {
                    return false;
                }
            }
            
            const AZStd::pair<int, AZStd::string>& lhsPair = AZStd::get<1>(lhs);
            const AZStd::pair<int, AZStd::string>& rhsPair = AZStd::get<1>(rhs);

            return
                lhsPair.first == rhsPair.first &&
                lhsPair.second == rhsPair.second;
        }
    };

    using TupleConformityTestTypes = ::testing::Types<
        PairTestDescription, 
        TupleTestDescription,
        ComplexTupleTestDescription,
        NestedTupleTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(Tuple, JsonSerializerConformityTests, TupleConformityTestTypes);

    class JsonTupleSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        using Pair = AZStd::pair<int, int>;
        
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<AZ::JsonTupleSerializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        using BaseJsonSerializerFixture::RegisterAdditional;
        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext) override
        {
            SimpleClass::Reflect(serializeContext, true);
            serializeContext->RegisterGenericType<Pair>();
        }

    protected:
        AZStd::unique_ptr<AZ::JsonTupleSerializer> m_serializer;
    };

    TEST_F(JsonTupleSerializerTests, Load_NonContainerType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testVal(rapidjson::kArrayType);
        // Explicitly use a type that's registered but doesn't have a container to trigger the result for types
        // without containers.
        SimpleClass instance;
        ResultCode result = m_serializer->Load(&instance, azrtti_typeid<SimpleClass>(), testVal, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(Processing::Altered, result.GetProcessing());
    }

    TEST_F(JsonTupleSerializerTests, Store_NonContainerType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        // Explicitly use a type that's registered but doesn't have a container to trigger the result for types
        // without containers.
        SimpleClass instance;
        ResultCode result = m_serializer->Store(*m_jsonDocument, &instance, nullptr, azrtti_typeid<SimpleClass>(), *m_jsonSerializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(Processing::Altered, result.GetProcessing());
    }
} // namespace JsonSerializationTests
