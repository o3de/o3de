/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableMetaDataBuilder.h>

namespace UnitTest
{
    class SpawnableMetaDataTests
        : public LeakDetectionFixture
    {
    };

    template<typename T>
    class TypedSpawnableMetaDataTests
        : public SpawnableMetaDataTests
    {
    public:
        using SetType = T;
        using GetType = AZStd::conditional_t<AZStd::is_same_v<T, AZStd::string>, AZStd::string_view, T>;

        SetType GetValue()
        {
            if constexpr (AZStd::is_same_v<SetType, bool>)
            {
                return true;
            }
            else if constexpr(AZStd::is_same_v<SetType, AZ::u64>)
            {
                return 42;
            }
            else if constexpr(AZStd::is_same_v<SetType, AZ::s64>)
            {
                return -42;
            }
            else if constexpr(AZStd::is_same_v<SetType, double>)
            {
                return 42.0;
            }
            else if constexpr(AZStd::is_same_v<SetType, AZStd::string>)
            {
                return AZStd::string("The number 42");
            }
        }

        AzFramework::SpawnableMetaData::ValueType GetValueType()
        {
            if constexpr (AZStd::is_same_v<SetType, bool>)
            {
                return AzFramework::SpawnableMetaData::ValueType::Boolean;
            }
            else if constexpr (AZStd::is_same_v<SetType, AZ::u64>)
            {
                return AzFramework::SpawnableMetaData::ValueType::UnsignedInteger;
            }
            else if constexpr (AZStd::is_same_v<SetType, AZ::s64>)
            {
                return AzFramework::SpawnableMetaData::ValueType::SignedInteger;
            }
            else if constexpr (AZStd::is_same_v<SetType, double>)
            {
                return AzFramework::SpawnableMetaData::ValueType::FloatingPoint;
            }
            else if constexpr (AZStd::is_same_v<SetType, AZStd::string>)
            {
                return AzFramework::SpawnableMetaData::ValueType::String;
            }
        }

        template<typename LT, typename RT>
        void ExpectEq(const LT& lhs, const RT& rhs)
        {
            if constexpr (AZStd::is_same_v<LT, AZStd::string> || AZStd::is_same_v<RT, AZStd::string>)
            {
                EXPECT_STREQ(lhs.data(), rhs.data());
            }
            else
            {
                EXPECT_EQ(lhs, rhs);
            }
        }
    };

    TYPED_TEST_CASE_P(TypedSpawnableMetaDataTests);

    TYPED_TEST_P(TypedSpawnableMetaDataTests, Add_AddValueToMetaData_NoCrash)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.Add("RandomKey", this->GetValue());
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, Add_ChainAdds_NoCrash)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.Add("RandomKey1", this->GetValue()).Add("RandomKey2", this->GetValue());
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, Add_AddThenRetrieveValue_StoredIsSameAsRetrieved)
    {
        using GetType = typename TypedSpawnableMetaDataTests<TypeParam>::GetType;

        auto value = this->GetValue();
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.Add("RandomKey", value);

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());
        GetType stored{};
        EXPECT_TRUE(metaData.Get("RandomKey", stored));

        this->ExpectEq(stored, value);
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, Add_OverwritingValue_OriginalReplacedWithNewValue)
    {
        using GetType = typename TypedSpawnableMetaDataTests<TypeParam>::GetType;

        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        if constexpr (AZStd::is_same_v<GetType, bool>)
        {
            builder.Add("RandomKey", 42.0);
        }
        else
        {
            builder.Add("RandomKey", true);
        }

        builder.Add("RandomKey", this->GetValue());

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());
        EXPECT_EQ(this->GetValueType(), metaData.GetType("RandomKey"));
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, Add_OverwritingArray_ArrayElementsAreRemoved)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", true);
        builder.AppendArray("RandomKey", AZ::u64{ 42 });
        builder.AppendArray("RandomKey", AZ::s64{ -42 });
        builder.AppendArray("RandomKey", 42.0);
        builder.AppendArray("RandomKey", "Hello");
        ASSERT_EQ(6, builder.GetEntryCount()); // 5 entries plus the entry that holds the array size.

        auto value = this->GetValue();
        builder.Add("RandomKey", value);

        EXPECT_EQ(1, builder.GetEntryCount());
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, GetType_RetrieveTypeOfValue_ValueTypeMatches)
    {
        auto value = this->GetValue();
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.Add("RandomKey", value);

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());

        AzFramework::SpawnableMetaData::ValueType type = metaData.GetType("RandomKey");
        EXPECT_EQ(this->GetValueType(), type);
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, GetType_RetrieveTypeOfArrayEntry_ValueTypeMatches)
    {
        auto value = this->GetValue();
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", value);

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());

        AzFramework::SpawnableMetaData::ValueType type = metaData.GetType("RandomKey", 0);
        EXPECT_EQ(this->GetValueType(), type);
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, AppendArray_AddValueToMetaDataArray_NoCrash)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", this->GetValue());
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, AppendArray_AppendThenRetrieveValue_StoredIsSameAsRetrieved)
    {
        using GetType = typename TypedSpawnableMetaDataTests<TypeParam>::GetType;

        auto value = this->GetValue();
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", value);

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());
        GetType stored{};
        EXPECT_TRUE(metaData.Get("RandomKey", 0, stored));

        this->ExpectEq(stored, value);
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, AppendArray_ReplaceExistingValue_ArraySizeReplacesOriginalEntry)
    {
        auto value = this->GetValue();
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.Add("RandomKey", value);
        builder.AppendArray("RandomKey", value);

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());
        EXPECT_EQ(AzFramework::SpawnableMetaData::ValueType::ArraySize, metaData.GetType("RandomKey"));
    }

    TYPED_TEST_P(TypedSpawnableMetaDataTests, Get_WrongType_ReturnsFalse)
    {
        using GetType = typename TypedSpawnableMetaDataTests<TypeParam>::GetType;

        auto value = this->GetValue();
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", value);

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());
        if constexpr (AZStd::is_same_v<GetType, bool>)
        {
            double stored{};
            EXPECT_FALSE(metaData.Get("RandomKey", stored));
        }
        else
        {
            bool stored{};
            EXPECT_FALSE(metaData.Get("RandomKey", stored));
        }
    }

    REGISTER_TYPED_TEST_CASE_P(TypedSpawnableMetaDataTests,
        Add_AddValueToMetaData_NoCrash,
        Add_ChainAdds_NoCrash,
        Add_AddThenRetrieveValue_StoredIsSameAsRetrieved,
        Add_OverwritingValue_OriginalReplacedWithNewValue,
        Add_OverwritingArray_ArrayElementsAreRemoved,
        GetType_RetrieveTypeOfValue_ValueTypeMatches,
        GetType_RetrieveTypeOfArrayEntry_ValueTypeMatches,
        AppendArray_AddValueToMetaDataArray_NoCrash,
        AppendArray_AppendThenRetrieveValue_StoredIsSameAsRetrieved,
        AppendArray_ReplaceExistingValue_ArraySizeReplacesOriginalEntry,
        Get_WrongType_ReturnsFalse);

    using SpawnableMetaDataTestTypes = ::testing::Types<bool, AZ::u64, AZ::s64, double, AZStd::string>;
    INSTANTIATE_TYPED_TEST_CASE_P(SpawnableMetaDataTests, TypedSpawnableMetaDataTests, SpawnableMetaDataTestTypes);


    TEST_F(SpawnableMetaDataTests, Get_UnknownKey_ReturnsFalse)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.Add("RandomKey", AZ::u64{ 42 });

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());

        AZ::u64 stored{};
        EXPECT_FALSE(metaData.Get("UnknownKey", stored));
    }

    TEST_F(SpawnableMetaDataTests, Get_ArraySize_ReturnsNumberOfEntries)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", true);
        builder.AppendArray("RandomKey", AZ::u64{ 42 });
        builder.AppendArray("RandomKey", AZ::s64{ -42 });
        builder.AppendArray("RandomKey", 42.0);
        builder.AppendArray("RandomKey", "Hello");

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());
        AzFramework::SpawnableMetaDataArraySize stored{};
        EXPECT_TRUE(metaData.Get("RandomKey", stored));

        EXPECT_EQ(AzFramework::SpawnableMetaDataArraySize{ 5 }, stored);
    }

    TEST_F(SpawnableMetaDataTests, Get_ArrayElementsAtVariousIndices_ReturnsValues)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        for (AZ::u64 i = 42; i < 88; ++i)
        {
            builder.AppendArray("RandomKey", i);
        }

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());

        for (size_t i = 0; i < 88 - 42; ++i)
        {
            AZ::u64 stored;
            EXPECT_TRUE(metaData.Get("RandomKey", i, stored));
            EXPECT_EQ(42 + i, stored);
        }
    }

    TEST_F(SpawnableMetaDataTests, Get_ArrayIndexOutOfBounds_ReturnsFalse)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", true);
        builder.AppendArray("RandomKey", AZ::u64{ 42 });
        builder.AppendArray("RandomKey", AZ::s64{ -42 });
        builder.AppendArray("RandomKey", 42.0);
        builder.AppendArray("RandomKey", "Hello");

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());

        AZ::u64 stored{};
        EXPECT_FALSE(metaData.Get("RandomKey", 5, stored));
    }

    TEST_F(SpawnableMetaDataTests, Get_RetrieveArrayType_ReturnArrayType)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", true);
        builder.AppendArray("RandomKey", AZ::u64{ 42 });
        builder.AppendArray("RandomKey", AZ::s64{ -42 });
        builder.AppendArray("RandomKey", 42.0);
        builder.AppendArray("RandomKey", "Hello");

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());

        EXPECT_EQ(AzFramework::SpawnableMetaData::ValueType::ArraySize, metaData.GetType("RandomKey"));
    }

    TEST_F(SpawnableMetaDataTests, Get_RetrieveNonExistingValue_ReturnUnavailable)
    {
        AzFramework::SpawnableMetaData metaData;
        EXPECT_EQ(AzFramework::SpawnableMetaData::ValueType::Unavailable, metaData.GetType("RandomKey"));
    }

    TEST_F(SpawnableMetaDataTests, Get_RetrieveNonExistingArrayIndex_ReturnUnavailable)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", true);
        builder.AppendArray("RandomKey", AZ::u64{ 42 });
        builder.AppendArray("RandomKey", AZ::s64{ -42 });
        builder.AppendArray("RandomKey", 42.0);
        builder.AppendArray("RandomKey", "Hello");

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());

        EXPECT_EQ(AzFramework::SpawnableMetaData::ValueType::Unavailable, metaData.GetType("RandomKey", 42));
    }

    TEST_F(SpawnableMetaDataTests, Remove_RemoveExistingEntry_EntryNotFound)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.Add("RandomKey", AZ::u64{ 42 });

        ASSERT_TRUE(builder.Remove("RandomKey"));

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());

        AZ::u64 stored{};
        EXPECT_FALSE(metaData.Get("RandomKey", stored));
    }

    TEST_F(SpawnableMetaDataTests, Remove_RemoveNonExistingEntry_ReturnsFalse)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        EXPECT_FALSE(builder.Remove("UnknownKey"));
    }

    TEST_F(SpawnableMetaDataTests, Remove_RemoveArray_AllArrayEntriesAreRemovedAsWell)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", AZ::u64{ 10 });
        builder.AppendArray("RandomKey", AZ::u64{ 11 });
        builder.AppendArray("RandomKey", AZ::u64{ 12 });
        builder.AppendArray("RandomKey", AZ::u64{ 13 });
        builder.AppendArray("RandomKey", AZ::u64{ 14 });
        builder.AppendArray("RandomKey", AZ::u64{ 15 });

        EXPECT_TRUE(builder.Remove("RandomKey"));

        EXPECT_EQ(0, builder.GetEntryCount());
    }

    TEST_F(SpawnableMetaDataTests, RemoveArrayEntry_RemoveExistingEntry_EntryNotFound)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", AZ::u64{ 10 });
        builder.AppendArray("RandomKey", AZ::u64{ 11 });
        builder.AppendArray("RandomKey", AZ::u64{ 12 });
        builder.AppendArray("RandomKey", AZ::u64{ 13 });
        builder.AppendArray("RandomKey", AZ::u64{ 14 });
        builder.AppendArray("RandomKey", AZ::u64{ 15 });

        ASSERT_TRUE(builder.RemoveArrayEntry("RandomKey", 3));

        AzFramework::SpawnableMetaData metaData(builder.BuildMetaData());

        AzFramework::SpawnableMetaDataArraySize stored{};
        EXPECT_TRUE(metaData.Get("RandomKey", stored));
        EXPECT_EQ(stored, AzFramework::SpawnableMetaDataArraySize{ 5 });

        AZ::u64 storedValue;
        EXPECT_TRUE(metaData.Get("RandomKey", 0, storedValue));
        EXPECT_EQ(10, storedValue);

        EXPECT_TRUE(metaData.Get("RandomKey", 1, storedValue));
        EXPECT_EQ(11, storedValue);

        EXPECT_TRUE(metaData.Get("RandomKey", 2, storedValue));
        EXPECT_EQ(12, storedValue);

        EXPECT_TRUE(metaData.Get("RandomKey", 3, storedValue));
        EXPECT_EQ(14, storedValue);

        EXPECT_TRUE(metaData.Get("RandomKey", 4, storedValue));
        EXPECT_EQ(15, storedValue);

        EXPECT_FALSE(metaData.Get("RandomKey", 5, storedValue));
    }

    TEST_F(SpawnableMetaDataTests, RemoveArrayEntry_RemoveNonExistingKey_ReturnFalse)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        ASSERT_FALSE(builder.RemoveArrayEntry("RandomKey", 0));
    }

    TEST_F(SpawnableMetaDataTests, RemoveArrayEntry_RemoveNonExistingEntry_ReturnFalse)
    {
        AzToolsFramework::Prefab::PrefabConversionUtils::SpawnableMetaDataBuilder builder;
        builder.AppendArray("RandomKey", AZ::u64{ 0 });
        builder.AppendArray("RandomKey", AZ::u64{ 1 });
        builder.AppendArray("RandomKey", AZ::u64{ 2 });
        builder.AppendArray("RandomKey", AZ::u64{ 3 });
        builder.AppendArray("RandomKey", AZ::u64{ 4 });
        builder.AppendArray("RandomKey", AZ::u64{ 5 });

        ASSERT_FALSE(builder.RemoveArrayEntry("RandomKey", 42));
    }
}
