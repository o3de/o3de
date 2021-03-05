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
#include "TestTypes.h"

#include <AzCore/Math/Random.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Network/EntityIdMarshaler.h>
#include <AzFramework/Network/DynamicSerializableFieldMarshaler.h>

#include <GridMate/Serialize/Buffer.h>

namespace UnitTest
{
    template<class T>
    class MarshalerTester
        : public AllocatorsFixture
    {
    public:
        MarshalerTester()
            : m_writeBuffer(GridMate::EndianType::BigEndian)
            , m_readBuffer(GridMate::EndianType::BigEndian)
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_random.SetSeed(AZStd::chrono::milliseconds().count());
        }
        
        void PopulateReadBuffer()
        {
            m_readBuffer = GridMate::ReadBuffer(m_writeBuffer.GetEndianType(), m_writeBuffer.Get(), m_writeBuffer.Size());
        }
        
        AZ::SimpleLcgRandom m_random;
        
        GridMate::Marshaler<T>        m_marshaler;
        GridMate::WriteBufferStatic<> m_writeBuffer;
        GridMate::ReadBuffer          m_readBuffer;
    };
    
    // EntityIdMarshalerTest
    typedef MarshalerTester<AZ::EntityId> EntityIdMarshalerTest;

    TEST_F(EntityIdMarshalerTest, SingleMarshalUnmarshalTest_EquivalentEmptyValue)
    {        
        AZ::EntityId initialId;
        m_marshaler.Marshal(m_writeBuffer, initialId);        
        
        PopulateReadBuffer();        
        
        AZ::EntityId receivedId;        
        m_marshaler.Unmarshal(receivedId, m_readBuffer);
        
        EXPECT_EQ(initialId,receivedId);
        EXPECT_FALSE(receivedId.IsValid());
    }
    
    TEST_F(EntityIdMarshalerTest, SingleMarshalUnmarshalTest_EquivalentRandomValue)
    {
        AZ::EntityId initialId = AZ::EntityId(m_random.GetRandom());
        m_marshaler.Marshal(m_writeBuffer, initialId);
        
        PopulateReadBuffer();
        
        AZ::EntityId receivedId;
        m_marshaler.Unmarshal(receivedId, m_readBuffer);
        
        EXPECT_EQ(initialId,receivedId);
    }

    TEST_F(EntityIdMarshalerTest, MultipleMarshalUnmarshalTest_EquivalentEmptyRandomEmptyRandomValueChain)
    {        
        AZ::EntityId sentId1_empty;
        AZ::EntityId sentId2_random = AZ::EntityId(m_random.GetRandom());
        AZ::EntityId sentId3_empty;
        AZ::EntityId sentId4_random = AZ::EntityId(m_random.GetRandom());
        
        m_marshaler.Marshal(m_writeBuffer, sentId1_empty);
        m_marshaler.Marshal(m_writeBuffer, sentId2_random);
        m_marshaler.Marshal(m_writeBuffer, sentId3_empty);
        m_marshaler.Marshal(m_writeBuffer, sentId4_random);
        
        PopulateReadBuffer();
        
        AZ::EntityId receivedId1_empty;
        AZ::EntityId receivedId2_random;
        AZ::EntityId receivedId3_empty;
        AZ::EntityId receivedId4_random;
        
        m_marshaler.Unmarshal(receivedId1_empty, m_readBuffer);
        m_marshaler.Unmarshal(receivedId2_random, m_readBuffer);
        m_marshaler.Unmarshal(receivedId3_empty, m_readBuffer);
        m_marshaler.Unmarshal(receivedId4_random, m_readBuffer);
        
        EXPECT_EQ(sentId1_empty, receivedId1_empty);
        EXPECT_EQ(sentId2_random, receivedId2_random);
        EXPECT_EQ(sentId3_empty, receivedId3_empty);
        EXPECT_EQ(sentId4_random, receivedId4_random);
    }
    
    // AZ::DynamicSerializableFieldMarshaler
    class FooSerializable
    {
    public:
        AZ_RTTI(FooSerializable, "{A60F0B2B-6085-4FF1-BD17-A0B0143BB03D}");
        AZ_CLASS_ALLOCATOR(FooSerializable, AZ::SystemAllocator,0);

        static void Reflect(AZ::SerializeContext& serializeContext)
        {
            serializeContext.Class<FooSerializable>()
                ->Version(1)
                ->Field("IntValue", &FooSerializable::m_intValue)
                ->Field("FloatValue", &FooSerializable::m_floatValue)
            ;
        }
        
        FooSerializable()
            : m_intValue(0)
            , m_floatValue(0.0f)
        {
        }        
        
        bool operator==(const FooSerializable& other) const
        {
            return m_intValue == other.m_intValue && AZ::IsClose(m_floatValue, other.m_floatValue,0.0001f);
        }
        
        AZ::u32 m_intValue;
        float   m_floatValue;
    };
    
    class BarSerializable
    {
    public:
        AZ_RTTI(BarSerializable, "{2389C23F-D247-420B-A385-71AB8455CD2E}");
        AZ_CLASS_ALLOCATOR(BarSerializable, AZ::SystemAllocator,0);

        static void Reflect(AZ::SerializeContext& serializeContext)
        {
            serializeContext.Class<BarSerializable>()
                ->Version(1)
                ->Field("LongValue", &BarSerializable::m_longValue)
                ->Field("DoubleValue", &BarSerializable::m_doubleValue)
            ;
        }
        
        BarSerializable()
            : m_longValue(0)
            , m_doubleValue(0.0)
        {
        }
        
        bool operator==(const BarSerializable& other) const
        {
            return m_longValue == other.m_longValue && AZ::IsClose(m_doubleValue,other.m_doubleValue,0.0001);
        }
        
        long    m_longValue;
        double  m_doubleValue;
    };
    
    class ComplexSerializable
    {
    public:
        AZ_RTTI(ComplexSerializable,"{055CB45C-702C-499F-8221-E9ABB21CF1D4}");
        AZ_CLASS_ALLOCATOR(ComplexSerializable, AZ::SystemAllocator,0);
        
        static void Reflect(AZ::SerializeContext& serializeContext)
        {
            serializeContext.Class<ComplexSerializable>()
                ->Version(1)
                ->Field("FooSerializable",&ComplexSerializable::m_fooField)
                ->Field("BarSerializable",&ComplexSerializable::m_barField)
            ;
        }
        
        bool operator==(const ComplexSerializable& other) const
        {
            return m_fooField == other.m_fooField && m_barField == other.m_barField;
        }
        
        FooSerializable m_fooField;
        BarSerializable m_barField;
    };
    
    class DynamicSerializableFieldMarshalerTest
        : public MarshalerTester<AZ::DynamicSerializableField>
        , public AZ::ComponentApplicationBus::Handler
    {
    public:
        DynamicSerializableFieldMarshalerTest()
            : MarshalerTester<AZ::DynamicSerializableField>()
        {
        }

        void SetUp() override
        {
            MarshalerTester<AZ::DynamicSerializableField>::SetUp();
            
            FooSerializable::Reflect(m_serializeContext);
            BarSerializable::Reflect(m_serializeContext);
            ComplexSerializable::Reflect(m_serializeContext);

            // Create the Marshaler with access to our custom serialize context.
            m_marshaler = GridMate::Marshaler<AZ::DynamicSerializableField>(&m_serializeContext);

            AZ::ComponentApplicationBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            MarshalerTester<AZ::DynamicSerializableField>::TearDown();

            AZ::ComponentApplicationBus::Handler::BusDisconnect();
        }
        
        FooSerializable* GenerateFooSerializable()
        {
            FooSerializable* field = new FooSerializable();

            RandomizeFooSerializable((*field));
            
            return field;
        }        

        void RandomizeFooSerializable(FooSerializable& serializable)
        {
            serializable.m_intValue = m_random.GetRandom();
            serializable.m_floatValue = m_random.GetRandomFloat();
        }
        
        BarSerializable* GenerateBarSerializable()
        {
            BarSerializable* field = new BarSerializable();
            
            return field;
        }

        void RandomizeBarSerializable(BarSerializable& serializable)
        {   
            serializable.m_longValue = static_cast<long>(m_random.GetRandom());
            serializable.m_doubleValue = static_cast<double>(m_random.GetRandomFloat());
        }
        
        ComplexSerializable* GenerateComplexSerializable()
        {
            ComplexSerializable* complexField = new ComplexSerializable();
            
            RandomizeFooSerializable(complexField->m_fooField);
            RandomizeBarSerializable(complexField->m_barField);            
            
            return complexField;
        }

        // Used Component Application Methods
        AZ::SerializeContext* GetSerializeContext() { return &m_serializeContext; }

        // Unused ComponentApplication methods
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor* descriptor) override { (void)descriptor; AZ_Assert(false,"Unsupported method in Unit Test"); }
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor* descriptor) override { (void)descriptor; AZ_Assert(false,"Unsupported method in Unit Test"); }
        
        AZ::ComponentApplication*   GetApplication() override { AZ_Assert(false,"Unsupported method in Unit Test"); return nullptr; }
        bool AddEntity(AZ::Entity* entity) override { (void)entity; AZ_Assert(false,"Unsupported method in Unit Test"); return false; }
        bool RemoveEntity(AZ::Entity* entity) override { (void)entity; AZ_Assert(false,"Unsupported method in Unit Test"); return false; }
        bool DeleteEntity(const AZ::EntityId& id) override { (void)id; AZ_Assert(false,"Unsupported method in Unit Test"); return false; }
        AZ::Entity* FindEntity(const AZ::EntityId& id) override { (void)id; AZ_Assert(false,"Unsupported method in Unit Test"); return nullptr; }
        void EnumerateEntities(const EntityCallback& callback) override { (void)callback; AZ_Assert(false,"Unsupported method in Unit Test"); }
        AZ::BehaviorContext*  GetBehaviorContext() override { AZ_Assert(false,"Unsupported method in Unit Test"); return nullptr; }
        const char* GetAppRoot() override { AZ_Assert(false,"Unsupported method in Unit Test"); return nullptr; }
        const char* GetExecutableFolder() override { AZ_Assert(false,"Unsupported method in Unit Test"); return nullptr; }
        AZ::Debug::DrillerManager* GetDrillerManager() override { AZ_Assert(false,"Unsupported method in Unit Test"); return nullptr; }
        void ReloadModule(const char* moduleFullPath) override { (void)moduleFullPath; AZ_Assert(false,"Unsupported method in Unit Test"); }
        
        AZ::SerializeContext m_serializeContext;
    };
    
    TEST_F(DynamicSerializableFieldMarshalerTest, SingleMarshalUnmarshalTest_EquivalentEmptyValue)
    {
        AZ::DynamicSerializableField sentField;
        m_marshaler.Marshal(m_writeBuffer, sentField);        
        
        PopulateReadBuffer();        
        
        AZ::DynamicSerializableField receivedField;        
        m_marshaler.Unmarshal(receivedField,m_readBuffer);
        
        EXPECT_TRUE(sentField.IsEqualTo(receivedField, &m_serializeContext));
    }
    
    TEST_F(DynamicSerializableFieldMarshalerTest, SingleMarshalUnmarshalTest_EquivalentFooValue)
    {    
        AZ::DynamicSerializableField sentField;
        
        FooSerializable* fooSerializable = GenerateFooSerializable();
        sentField.Set(fooSerializable);
        
        m_marshaler.Marshal(m_writeBuffer, sentField);
        
        PopulateReadBuffer();

        AZ::DynamicSerializableField receivedField;
        m_marshaler.Unmarshal(receivedField,m_readBuffer);        

        EXPECT_TRUE(sentField.IsEqualTo(receivedField, &m_serializeContext));

        sentField.DestroyData(&m_serializeContext);
        receivedField.DestroyData(&m_serializeContext);
    }
    
    TEST_F(DynamicSerializableFieldMarshalerTest, SingleMarshalUnmarshalTest_EquivalentBarValue)
    {    
        AZ::DynamicSerializableField sentField;
        
        BarSerializable* barSerializable = GenerateBarSerializable();
        sentField.Set(barSerializable);
        
        m_marshaler.Marshal(m_writeBuffer, sentField);
        
        PopulateReadBuffer();

        AZ::DynamicSerializableField receivedField;
        m_marshaler.Unmarshal(receivedField,m_readBuffer);        
     
        EXPECT_TRUE(sentField.IsEqualTo(receivedField, &m_serializeContext));

        sentField.DestroyData(&m_serializeContext);
        receivedField.DestroyData(&m_serializeContext);
    }
    
    TEST_F(DynamicSerializableFieldMarshalerTest, SingleMarshalUnmarshalTest_EquivalentComplexValue)
    {
        AZ::DynamicSerializableField sentField;
        
        ComplexSerializable* complexSerializable = GenerateComplexSerializable();
        sentField.Set(complexSerializable);
        
        m_marshaler.Marshal(m_writeBuffer, sentField);
        
        PopulateReadBuffer();
        
        AZ::DynamicSerializableField receivedField;
        m_marshaler.Unmarshal(receivedField,m_readBuffer);

        EXPECT_TRUE(sentField.IsEqualTo(receivedField, &m_serializeContext));

        sentField.DestroyData(&m_serializeContext);
        receivedField.DestroyData(&m_serializeContext);
    }
    
    TEST_F(DynamicSerializableFieldMarshalerTest, MultipleMarshalUnmarshalTest_EmptyEmptyChainEquivalentValue)
    {
        AZ::DynamicSerializableField sentField1;
        AZ::DynamicSerializableField sentField2;        
        
        m_marshaler.Marshal(m_writeBuffer, sentField1);
        m_marshaler.Marshal(m_writeBuffer, sentField2);
        
        PopulateReadBuffer();
        
        AZ::DynamicSerializableField receivedField1;
        AZ::DynamicSerializableField receivedField2;
        
        m_marshaler.Unmarshal(receivedField1,m_readBuffer);
        m_marshaler.Unmarshal(receivedField2,m_readBuffer);        
        
        EXPECT_TRUE(sentField1.IsEqualTo(receivedField1, &m_serializeContext));        
        EXPECT_TRUE(sentField2.IsEqualTo(receivedField2, &m_serializeContext));

        sentField1.DestroyData(&m_serializeContext);
        sentField2.DestroyData(&m_serializeContext);
        receivedField1.DestroyData(&m_serializeContext);
        receivedField2.DestroyData(&m_serializeContext);
    }
    
    TEST_F(DynamicSerializableFieldMarshalerTest, MultipleMarshalUnmarshalTest_FooBarComplexChainEquivalentValue)
    {
        AZ::DynamicSerializableField sentField1;
        FooSerializable* fooSerializable = GenerateFooSerializable();
        sentField1.Set(fooSerializable);
        
        AZ::DynamicSerializableField sentField2;
        BarSerializable* barSerializable = GenerateBarSerializable();
        sentField2.Set(barSerializable);
        
        AZ::DynamicSerializableField sentField3;
        ComplexSerializable* complexSerializable = GenerateComplexSerializable();
        sentField3.Set(complexSerializable);
        
        m_marshaler.Marshal(m_writeBuffer, sentField1);
        m_marshaler.Marshal(m_writeBuffer, sentField2);
        m_marshaler.Marshal(m_writeBuffer, sentField3);
        
        PopulateReadBuffer();
        
        AZ::DynamicSerializableField receivedField1;
        AZ::DynamicSerializableField receivedField2;
        AZ::DynamicSerializableField receivedField3;
        
        m_marshaler.Unmarshal(receivedField1, m_readBuffer);
        m_marshaler.Unmarshal(receivedField2, m_readBuffer);        
        m_marshaler.Unmarshal(receivedField3, m_readBuffer);

        EXPECT_TRUE(sentField1.IsEqualTo(receivedField1, &m_serializeContext));
        EXPECT_TRUE(sentField2.IsEqualTo(receivedField2, &m_serializeContext));
        EXPECT_TRUE(sentField3.IsEqualTo(receivedField3, &m_serializeContext));

        sentField1.DestroyData(&m_serializeContext);
        sentField2.DestroyData(&m_serializeContext);
        sentField3.DestroyData(&m_serializeContext);

        receivedField1.DestroyData(&m_serializeContext);
        receivedField2.DestroyData(&m_serializeContext);
        receivedField3.DestroyData(&m_serializeContext);
    }
    
    TEST_F(DynamicSerializableFieldMarshalerTest, MultipleMarshalUnmarshalTest_EmptyFooEmptyBarEmptyComplexChainEquivalentValue)
    {
        AZ::DynamicSerializableField emptyField;        
        
        AZ::DynamicSerializableField sentField1;
        FooSerializable* fooSerializable = GenerateFooSerializable();
        sentField1.Set(fooSerializable);
        
        AZ::DynamicSerializableField sentField2;
        BarSerializable* barSerializable = GenerateBarSerializable();
        sentField2.Set(barSerializable);
        
        AZ::DynamicSerializableField sentField3;
        ComplexSerializable* complexSerializable = GenerateComplexSerializable();
        sentField3.Set(complexSerializable);
        
        m_marshaler.Marshal(m_writeBuffer, emptyField);
        m_marshaler.Marshal(m_writeBuffer, sentField1);
        m_marshaler.Marshal(m_writeBuffer, emptyField);
        m_marshaler.Marshal(m_writeBuffer, sentField2);
        m_marshaler.Marshal(m_writeBuffer, emptyField);
        m_marshaler.Marshal(m_writeBuffer, sentField3);
        m_marshaler.Marshal(m_writeBuffer, emptyField);
        
        PopulateReadBuffer();
        
        AZ::DynamicSerializableField receivedEmptyField1;
        AZ::DynamicSerializableField receivedField1;
        AZ::DynamicSerializableField receivedEmptyField2;
        AZ::DynamicSerializableField receivedField2;
        AZ::DynamicSerializableField receivedEmptyField3;
        AZ::DynamicSerializableField receivedField3;
        AZ::DynamicSerializableField receivedEmptyField4;
        
        m_marshaler.Unmarshal(receivedEmptyField1, m_readBuffer);
        m_marshaler.Unmarshal(receivedField1, m_readBuffer);
        m_marshaler.Unmarshal(receivedEmptyField2, m_readBuffer);
        m_marshaler.Unmarshal(receivedField2, m_readBuffer);
        m_marshaler.Unmarshal(receivedEmptyField3, m_readBuffer);
        m_marshaler.Unmarshal(receivedField3, m_readBuffer);
        m_marshaler.Unmarshal(receivedEmptyField4, m_readBuffer);

        EXPECT_TRUE(emptyField.IsEqualTo(receivedEmptyField1, &m_serializeContext));
        EXPECT_TRUE(sentField1.IsEqualTo(receivedField1, &m_serializeContext));
        EXPECT_TRUE(emptyField.IsEqualTo(receivedEmptyField2, &m_serializeContext));
        EXPECT_TRUE(sentField2.IsEqualTo(receivedField2, &m_serializeContext));
        EXPECT_TRUE(emptyField.IsEqualTo(receivedEmptyField3, &m_serializeContext));
        EXPECT_TRUE(sentField3.IsEqualTo(receivedField3, &m_serializeContext));
        EXPECT_TRUE(emptyField.IsEqualTo(receivedEmptyField4, &m_serializeContext));

        emptyField.DestroyData(&m_serializeContext);
        sentField1.DestroyData(&m_serializeContext);
        sentField2.DestroyData(&m_serializeContext);
        sentField3.DestroyData(&m_serializeContext);

        receivedEmptyField1.DestroyData(&m_serializeContext);
        receivedField1.DestroyData(&m_serializeContext);
        receivedEmptyField2.DestroyData(&m_serializeContext);
        receivedField2.DestroyData(&m_serializeContext);
        receivedEmptyField3.DestroyData(&m_serializeContext);
        receivedField3.DestroyData(&m_serializeContext);
        receivedEmptyField4.DestroyData(&m_serializeContext);
    }
    
    TEST_F(DynamicSerializableFieldMarshalerTest, MultipleMarshalUnmarshalTest_RandomChainEquivalentValue)
    {
        // Need to watch out for the size of the WriteBuffer. It's about ~2048  bytes, at worst case here, I'll write ~100 Bytes to the field per test object)
        // So I need to keep this ~20 elements.
        int numValues = 5 + m_random.GetRandom()%10;
        AZStd::vector< AZ::DynamicSerializableField > sentValues;
        AZStd::vector< AZ::DynamicSerializableField > receivedValues;
        
        sentValues.resize(numValues);
        receivedValues.resize(numValues);
        
        for (auto& currentField : sentValues)
        {
            int value = m_random.GetRandom() % 4;
            switch (value)
            {
            case 0:
            {
                currentField.Set(GenerateFooSerializable());
            }
            break;
            case 1:
            {
                currentField.Set(GenerateBarSerializable());                
            }
            break;
            case 2:
            {
                currentField.Set(GenerateComplexSerializable());
            }
            break;
            case 3:
            default:
                // Empty field
                break;
            }
        }
        
        for (auto& currentField : sentValues)
        {
            m_marshaler.Marshal(m_writeBuffer,currentField);
        }
        
        PopulateReadBuffer();
        
        for (auto& currentField : receivedValues)
        {
            m_marshaler.Unmarshal(currentField,m_readBuffer);
        }
        
        for (unsigned int i=0; i < sentValues.size(); ++i)
        {
            AZ::DynamicSerializableField& sentField = sentValues[i];
            AZ::DynamicSerializableField& receivedField = receivedValues[i];
            
            EXPECT_TRUE(sentField.IsEqualTo(receivedField, &m_serializeContext));

            sentField.DestroyData(&m_serializeContext);
            receivedField.DestroyData(&m_serializeContext);
        }        
    }
}
