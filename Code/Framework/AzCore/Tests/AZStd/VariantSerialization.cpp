/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SerializeContextFixture.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Utils.h>

namespace UnitTest
{
    class VariantSerializationTest
        : public AllocatorsFixture
    {
    public:
        // We must expose the class for serialization first.
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            AZ::Entity::Reflect(m_serializeContext.get());
            
        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection();
            AZ::Entity::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();

            m_serializeContext.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };


    struct VariantWrapper
    {
        AZ_TYPE_INFO(VariantWrapper, "{B086FD5B-1E6F-4CB1-9379-80C35DA3B430}");
        AZ_CLASS_ALLOCATOR(VariantWrapper, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<VariantWrapper>()
                    ->Field("WrappedField", &VariantWrapper::m_wrappedVariant)
                    ;
            }
        }
        AZStd::variant<uint64_t>* m_wrappedVariant{};
    };

    TEST_F(VariantSerializationTest, VariantWithMonostateAlternativeSerializesCorrectly)
    {
        using TestVariant1 = AZStd::variant<AZStd::monostate>;
        ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<TestVariant1>();
            } });

        TestVariant1 testVariant;

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&testVariant);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        TestVariant1 loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));
        EXPECT_EQ(testVariant, loadVariant);
    }

    TEST_F(VariantSerializationTest, VariantWithOneAlternativeSerializesCorrectly)
    {
        using TestVariant1 = AZStd::variant<AZ::Entity>;
        ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<TestVariant1>();
            }});

        TestVariant1 testVariant{ AZ::Entity("Variant") };

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&testVariant);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        TestVariant1 loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));
        EXPECT_EQ(testVariant.index(), loadVariant.index());
        ASSERT_EQ(0U, testVariant.index());
        EXPECT_EQ(AZStd::get<0>(testVariant).GetName(), AZStd::get<0>(loadVariant).GetName());
    }


    TEST_F(VariantSerializationTest, VariantWithOneAlternativeWhichIsPointerTypeSerializesCorrectly)
    {
        using TestVariant1 = AZStd::variant<AZ::Entity*>;
        ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<TestVariant1>();
            } });

        TestVariant1 testVariant{ aznew AZ::Entity(AZ::EntityId(42), "Variant Pointer") };

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&testVariant);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        TestVariant1 loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));
        EXPECT_EQ(testVariant.index(), loadVariant.index());
        ASSERT_EQ(0U, testVariant.index());
        AZ::Entity* testEntity = AZStd::get<0>(testVariant);
        AZ::Entity* loadEntity = AZStd::get<0>(loadVariant);
        EXPECT_NE(testEntity, loadEntity);
        ASSERT_NE(loadEntity, nullptr);
        EXPECT_EQ(testEntity->GetId(), loadEntity->GetId());
        EXPECT_EQ(testEntity->GetName(), loadEntity->GetName());
        delete testEntity;
        delete loadEntity;
        AZStd::get<0>(testVariant) = nullptr;
        AZStd::get<0>(loadVariant) = nullptr;
    }

    TEST_F(VariantSerializationTest, MultipleAlternativeSerializesCorrectly)
    {
        using TestVariant1 = AZStd::variant<int32_t, AZStd::string>;
        ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<TestVariant1>();
            } });

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 0x85;
        TestVariant1 sourceVariant{ expectedIntValue };

        TestVariant1 loadIntVariant;
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&sourceVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadIntVariant, m_serializeContext.get()));
        }
        EXPECT_EQ(sourceVariant.index(), loadIntVariant.index());
        ASSERT_EQ(0U, loadIntVariant.index());
        int32_t loadIntValue = AZStd::get<0>(loadIntVariant);
        EXPECT_EQ(expectedIntValue, loadIntValue);

        // Update source variant with string value and attempt to serialize it out and back in
        const AZStd::string expectedStringValue = "Our Dog Food Eats the Dog";
        sourceVariant = expectedStringValue;
        TestVariant1 loadStringVariant;
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
            objStream->WriteClass(&sourceVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadStringVariant, m_serializeContext.get()));
        }

        EXPECT_EQ(sourceVariant.index(), loadStringVariant.index());
        ASSERT_EQ(1U, loadStringVariant.index());
        const AZStd::string& loadStringValue = AZStd::get<1>(loadStringVariant);
        EXPECT_EQ(expectedStringValue, loadStringValue);
    }

    TEST_F(VariantSerializationTest, VariantStoringAnyAlternativeSerializesCorrectly)
    {
        using TestVariant1 = AZStd::variant<AZStd::any>;
        ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<TestVariant1>();
            } });

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 0x2A;
        TestVariant1 sourceVariant{ AZStd::make_any<int32_t>(expectedIntValue) };
        AZStd::any& sourceAnyValue = AZStd::get<0>(sourceVariant);
        EXPECT_TRUE(sourceAnyValue.is<int32_t>());
        int32_t* sourceIntValue = AZStd::any_cast<int32_t>(&sourceAnyValue);
        ASSERT_NE(nullptr, sourceIntValue);
        EXPECT_EQ(expectedIntValue, *sourceIntValue);

        TestVariant1 loadAnyVariant1;
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&sourceVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadAnyVariant1, m_serializeContext.get()));
        }
        EXPECT_EQ(sourceVariant.index(), loadAnyVariant1.index());
        ASSERT_EQ(0U, loadAnyVariant1.index());
        
        AZStd::any& loadAnyValue = AZStd::get<0>(loadAnyVariant1);
        EXPECT_TRUE(loadAnyValue.is<int32_t>());
        
        int32_t* loadIntValue = AZStd::any_cast<int32_t>(&loadAnyValue);
        ASSERT_NE(nullptr, loadIntValue);
        EXPECT_EQ(expectedIntValue, *loadIntValue);
    }

    TEST_F(VariantSerializationTest, AnyStoringVariantSavesAlternativeAndLoadsAlternativeCorrectly)
    {
        using TestVariant1 = AZStd::variant<int32_t>;
        ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<TestVariant1>();
            } });

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 0170;
        AZStd::any sourceAny = AZStd::make_any<TestVariant1>(AZStd::in_place_type_t<int32_t>{}, expectedIntValue);
        EXPECT_TRUE(sourceAny.is<TestVariant1>());
        TestVariant1* sourceVariantValue = AZStd::any_cast<TestVariant1>(&sourceAny);
        ASSERT_NE(nullptr, sourceVariantValue);
        EXPECT_EQ(expectedIntValue, AZStd::get<0>(*sourceVariantValue));

        AZStd::any loadAny;
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_JSON);
            objStream->WriteClass(&sourceAny);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadAny, m_serializeContext.get()));
        }

        // Due to the Variant Serialization only writing out the alternative to disk and the AZStd::any class
        // being only able to determine the type dynamically, the type that is stored in the any is the 
        // alternative, not the variant
        EXPECT_TRUE(loadAny.is<int>());
        int* loadIntValue = AZStd::any_cast<int>(&loadAny);
        ASSERT_NE(nullptr, loadIntValue);
        EXPECT_EQ(expectedIntValue, *loadIntValue);
    }

    TEST_F(VariantSerializationTest, TypeWhichWrapsVariantSavesAndLoadsCorrectly)
    {
        ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                VariantWrapper::Reflect(serializeContext);
            } });

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 7001;

        VariantWrapper saveWrapper;

        saveWrapper.m_wrappedVariant = new AZStd::variant<uint64_t>(expectedIntValue);        
        EXPECT_EQ(expectedIntValue, AZStd::get<0>(*saveWrapper.m_wrappedVariant));

        VariantWrapper loadWrapper;
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_JSON);
            objStream->WriteClass(&saveWrapper);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadWrapper, m_serializeContext.get()));
        }

        ASSERT_NE(nullptr, loadWrapper.m_wrappedVariant);
        EXPECT_NE(saveWrapper.m_wrappedVariant, loadWrapper.m_wrappedVariant);
        ASSERT_EQ(0U, loadWrapper.m_wrappedVariant->index());
        EXPECT_EQ(expectedIntValue, AZStd::get<0>(*loadWrapper.m_wrappedVariant));

        delete saveWrapper.m_wrappedVariant;
        // SerializeCotnext IObjectFactory allocates memory for types without AZClassAllocator using azmalloc
        // None of the AZStd::containers implement the AZ_CLASS_ALLOCATOR, so it uses the os allocator by default
        azdestroy(loadWrapper.m_wrappedVariant);
    }


    TEST_F(VariantSerializationTest, VariantStoringVariantSerializesCorrectly)
    {
        using InnerVariant = AZStd::variant<float, int32_t>;
        using VariantCeption = AZStd::variant<bool, InnerVariant, bool>;
        ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<VariantCeption>();
            } });

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = -43;
        // Sets the int32_t element of the inner variant
        // Therefore the outer variant index should be 1 and the inner variant index should be 1
        VariantCeption sourceCeptionVariant{ InnerVariant{expectedIntValue} };

        VariantCeption loadCeptionVariant;
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&sourceCeptionVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadCeptionVariant, m_serializeContext.get()));
        }

        EXPECT_EQ(sourceCeptionVariant.index(), loadCeptionVariant.index());
        ASSERT_EQ(1U, loadCeptionVariant.index());

        InnerVariant& innerVariant = AZStd::get<1>(loadCeptionVariant);
        EXPECT_EQ(1U, innerVariant.index());
        EXPECT_EQ(expectedIntValue, AZStd::get<1>(innerVariant));
    }

    TEST_F(VariantSerializationTest, SavingVariantWithIntAlternativeCanBeLoadedByVariantWithIntAlternativeAtDifferentIndex)
    {
        using SaveVariant = AZStd::variant<int32_t>;
        using LoadVariant = AZStd::variant<bool, double, const int32_t, int32_t>;

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 72;
        // Sets the int32_t element of the source variant which is the zeroth index
        SaveVariant sourceVariant{ expectedIntValue };
        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        
        {
            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<SaveVariant>();
                }}
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&sourceVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        ScopedSerializeContextReflector loadReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<LoadVariant>();
            } }
        );

        LoadVariant loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));

        // Source variant should have different index than loaded variant
        EXPECT_EQ(0U, sourceVariant.index());
        EXPECT_NE(sourceVariant.index(), loadVariant.index());
        // The AZ Serialization system does not distinguish between const and non-const.
        // As the LoadVariant type has const int32_t as the 2nd index, it is the alternative that will be loaded
        constexpr size_t expectedLoadIndex = 2U;
        ASSERT_EQ(expectedLoadIndex, loadVariant.index());
        EXPECT_TRUE(AZStd::holds_alternative<const int32_t>(loadVariant));

        EXPECT_EQ(AZStd::get<0>(sourceVariant), AZStd::get<expectedLoadIndex>(loadVariant));
    }

    TEST_F(VariantSerializationTest, SavingVariantWithIntAlternativeAndLoadingToVariantWithInnerVariantPointerSucceeds)
    {
        using SaveVariant = AZStd::variant<int32_t>;
        using LoadVariant = AZStd::variant<bool, AZStd::variant<int32_t>*>;

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 146;
        // Sets the int32_t element of the source variant which is the zeroth index
        SaveVariant sourceVariant{ expectedIntValue };
        EXPECT_EQ(0U, sourceVariant.index());

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        {
            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<SaveVariant>();
                } }
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&sourceVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        ScopedSerializeContextReflector loadReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<LoadVariant>();
            } }
        );

        LoadVariant loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));

        constexpr size_t expectedLoadIndex = 1U;
        ASSERT_EQ(expectedLoadIndex, loadVariant.index());
        ASSERT_TRUE(AZStd::holds_alternative<AZStd::variant<int32_t>*>(loadVariant));
        AZStd::variant<int32_t>*& innerVariant = AZStd::get<expectedLoadIndex>(loadVariant);
        ASSERT_NE(nullptr, innerVariant);
        EXPECT_TRUE(AZStd::holds_alternative<int32_t>(*innerVariant));
        int32_t* loadInt = AZStd::get_if<int32_t>(innerVariant);
        ASSERT_NE(nullptr, loadInt);
        EXPECT_EQ(expectedIntValue, *loadInt);
        azdestroy(innerVariant);
    }

    TEST_F(VariantSerializationTest, SavingVariantWithIntAlternativeAndLoadingToVariantWithInnerVariantPointerWhichHasAnIntPointerSucceeds)
    {
        using SaveVariant = AZStd::variant<int32_t>;
        using LoadVariant = AZStd::variant<AZStd::variant<int32_t*>*, double>;

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 146;
        // Sets the int32_t element of the source variant which is the zeroth index
        SaveVariant sourceVariant{ expectedIntValue };
        EXPECT_EQ(0U, sourceVariant.index());

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        {
            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<SaveVariant>();
                } }
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&sourceVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        ScopedSerializeContextReflector loadReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<LoadVariant>();
            } }
        );

        LoadVariant loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));

        constexpr size_t expectedLoadIndex = 0U;
        ASSERT_EQ(expectedLoadIndex, loadVariant.index());
        ASSERT_TRUE(AZStd::holds_alternative<AZStd::variant<int32_t*>*>(loadVariant));
        AZStd::variant<int32_t*>*& innerVariant = AZStd::get<expectedLoadIndex>(loadVariant);
        ASSERT_NE(nullptr, innerVariant);
        EXPECT_TRUE(AZStd::holds_alternative<int32_t*>(*innerVariant));
        int32_t* loadInt = AZStd::get<0>(*innerVariant);
        ASSERT_NE(nullptr, loadInt);
        EXPECT_EQ(expectedIntValue, *loadInt);
        azdestroy(loadInt);
        azdestroy(innerVariant);
    }

    TEST_F(VariantSerializationTest, SavingVariantWithIntAlternativeAndLoadingToVariantWithInnerVariantIntTypeAndIntPointerTypeAndIntValueTypeChoosesIntValueType)
    {
        using SaveVariant = AZStd::variant<int32_t>;
        using LoadVariant = AZStd::variant<AZStd::variant<int32_t*>*, int32_t*, int32_t>;

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 146;
        // Sets the int32_t element of the source variant which is the zeroth index
        SaveVariant sourceVariant{ expectedIntValue };
        EXPECT_EQ(0U, sourceVariant.index());

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        {
            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<SaveVariant>();
                } }
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&sourceVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        ScopedSerializeContextReflector loadReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<LoadVariant>();
            } }
        );

        LoadVariant loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));

        constexpr size_t expectedLoadIndex = 2U;
        ASSERT_EQ(expectedLoadIndex, loadVariant.index());
        int32_t loadInt = AZStd::get<expectedLoadIndex>(loadVariant);
        EXPECT_EQ(expectedIntValue, loadInt);
    }

    TEST_F(VariantSerializationTest, SavingIntAlternativeAndLoadingToRootVariantSucceeds)
    {
        using LoadVariant = AZStd::variant<int32_t*>;

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 146;

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        {
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&expectedIntValue);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        ScopedSerializeContextReflector loadReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<LoadVariant>();
            } }
        );

        LoadVariant loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));

        constexpr size_t expectedLoadIndex = 0U;
        ASSERT_EQ(expectedLoadIndex, loadVariant.index());
        int32_t* loadInt = AZStd::get<expectedLoadIndex>(loadVariant);
        EXPECT_EQ(expectedIntValue, *loadInt);
        azdestroy(loadInt);
    }

    TEST_F(VariantSerializationTest, SavingAssetAlternativeAndLoadingToRootVariantSucceeds)
    {
        using SaveVariant = AZStd::variant<AZ::Data::Asset<AZ::Data::AssetData>>;
        using LoadVariant = AZStd::variant<AZ::Data::Asset<AZ::Data::AssetData>>;

        AZ::Data::AssetType sliceAssetTypeId("{C62C7A87-9C09-4148-A985-12F2C99C0A45}");
        AZ::Data::Asset<AZ::Data::AssetData> saveAsset(AZ::Data::AssetId{}, sliceAssetTypeId);

        SaveVariant saveVariant(saveAsset);

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        {
            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<SaveVariant>();
                } }
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&saveVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        ScopedSerializeContextReflector loadReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<LoadVariant>();
            } }
        );

        LoadVariant loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));

        constexpr size_t expectedLoadIndex = 0U;
        ASSERT_EQ(expectedLoadIndex, loadVariant.index());
        AZ::Data::Asset<AZ::Data::AssetData>& loadAsset= AZStd::get<expectedLoadIndex>(loadVariant);
        EXPECT_FALSE(loadAsset.GetId().IsValid());
    }

    TEST_F(VariantSerializationTest, SavingVariantWithIntAlternativeAndLoadingToVariantWithoutIntAlternativeFails)
    {
        using SaveVariant = AZStd::variant<int32_t>;
        using LoadVariant = AZStd::variant<bool, double>;

        // Store integer in variant and attempt to serialize it out and back in
        constexpr int32_t expectedIntValue = 72;
        // Sets the int32_t element of the source variant which is the zeroth index
        SaveVariant sourceVariant{ expectedIntValue };
        EXPECT_EQ(0U, sourceVariant.index());

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        {
            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<SaveVariant>();
                } }
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&sourceVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        ScopedSerializeContextReflector loadReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<LoadVariant>();
            } }
        );

        LoadVariant loadVariant;
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(VariantSerializationTest, SavingVariantWithVectorOfStringAlternativeAndIsAbleToLoadCorrectly)
    {
        using SaveVariant = AZStd::variant<AZStd::vector<AZStd::string>, float>;
        using LoadVariant = AZStd::variant<bool, AZStd::vector<AZStd::string>>;

        // Store a vector of strings and attempt serialized to a stream and back
        const AZStd::string expectedStringValue1{ "ChimeYard" };
        const AZStd::string expectedStringValue2{ "BirdCakeFactory" };
        // Sets the vector of string element which corresponds to index 1 of the source variant
        SaveVariant sourceVariant{ AZStd::vector<AZStd::string>{expectedStringValue1, expectedStringValue2} };
        EXPECT_EQ(0U, sourceVariant.index());

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        {
            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<SaveVariant>();
                } }
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
            objStream->WriteClass(&sourceVariant);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        ScopedSerializeContextReflector loadReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<LoadVariant>();
            } }
        );

        LoadVariant loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));

        ASSERT_EQ(1, loadVariant.index());
        EXPECT_EQ(AZStd::get<0>(sourceVariant), AZStd::get<1>(loadVariant));
    }

    TEST_F(VariantSerializationTest, SavingVectorOfVectorTypeIsAbleToLoadIntoVariantCorrectly)
    {
        using SaveType = AZStd::vector<AZStd::vector<AZStd::string>>;
        using LoadVariant = AZStd::variant<SaveType>;

        // Store a vector of vector of strings and attempt serialized to a stream and back
        const AZStd::string expectedStringValue1{ "Zubat Key" };
        const AZStd::string expectedStringValue2{ "Yubioh Key" };
        const AZStd::string expectedStringValue3{ "Gelato Token" };
        
        SaveType twoStepsVectorAndTwoStepsBack;
        // Set the inner vector first element to have an expected string value of "Yubioh Key" and "Gelato Token"
        twoStepsVectorAndTwoStepsBack.emplace_back();
        twoStepsVectorAndTwoStepsBack.back().push_back(expectedStringValue2);
        twoStepsVectorAndTwoStepsBack.back().push_back(expectedStringValue3);
        // Set the inner vector second element to have an expected string value of "Gelato Token" and "Zubat Key"
        twoStepsVectorAndTwoStepsBack.emplace_back();
        twoStepsVectorAndTwoStepsBack.back().push_back(expectedStringValue3);
        twoStepsVectorAndTwoStepsBack.back().push_back(expectedStringValue1);

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        {
            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<SaveType>();
                } }
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
            objStream->WriteClass(&twoStepsVectorAndTwoStepsBack);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        ScopedSerializeContextReflector loadReflector(*m_serializeContext, {
            [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->RegisterGenericType<LoadVariant>();
            } }
        );

        LoadVariant loadVariant;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));

        ASSERT_EQ(0, loadVariant.index());
        EXPECT_EQ(twoStepsVectorAndTwoStepsBack, AZStd::get<0>(loadVariant));
    }

    TEST_F(VariantSerializationTest, SavingandLoadingVectorOfVariants_IsAbleToLoadAlternativesAtIndex1OrHigher_WithoutCrashing)
    {
        using VariantVectorA = AZStd::vector<AZStd::variant<AZStd::string, int32_t>>;
        using VariantVectorB = AZStd::vector<AZStd::variant<int32_t, AZStd::string>>;

        VariantVectorA varA;
        VariantVectorB varB;

        varA.push_back(1);
        varA.push_back("str");
        varB.push_back(1);
        varB.push_back("str");

        // VariantVectorA, works fine.
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<VariantVectorA>();
                } }
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
            objStream->WriteClass(&varA);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            VariantVectorA loadVariant;
            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));
        }

        // VariantVectorB, should also work fine.
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

            ScopedSerializeContextReflector scopedReflector(*m_serializeContext, {
                [](AZ::SerializeContext* serializeContext)
                {
                    serializeContext->RegisterGenericType<VariantVectorB>();
                } }
            );
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
            objStream->WriteClass(&varB);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            VariantVectorB loadVariant;
            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadVariant, m_serializeContext.get()));
        }
    }
}
