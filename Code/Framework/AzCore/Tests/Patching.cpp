/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FileIOBaseTestTypes.h"

#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/any.h>

namespace AZ
{
    // Added definition of type info and rtti for the DataPatchTypeUpgrade class
    // to this Unit Test file to allow rtti functions to be accessible from the SerializeContext::TypeChange
    // call
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_IMPL(SerializeContext::DataPatchTypeUpgrade, "DataPatchTypeUpgrade", "{E5A2F519-261C-4B81-925F-3730D363AB9C}", AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS);
    AZ_RTTI_NO_TYPE_INFO_IMPL((SerializeContext::DataPatchTypeUpgrade, AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS), DataPatchUpgrade);
}
using namespace AZ;

namespace UnitTest
{
    /**
    * Tests generating and applying patching to serialized structures.
    * \note There a lots special... \TODO add notes depending on the final solution
    */
    namespace Patching
    {
        // Object that we will store in container and patch in the complex case
        class ContainedObjectPersistentId
        {
        public:
            AZ_TYPE_INFO(ContainedObjectPersistentId, "{D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA}")

                ContainedObjectPersistentId()
                : m_data(0)
                , m_persistentId(0)
            {}

            u64 GetPersistentId() const { return m_persistentId; }
            void SetPersistentId(u64 pesistentId) { m_persistentId = pesistentId; }

            int m_data;
            u64 m_persistentId; ///< Returns the persistent object ID

            static u64 GetPersistentIdWrapper(const void* instance)
            {
                return reinterpret_cast<const ContainedObjectPersistentId*>(instance)->GetPersistentId();
            }

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ContainedObjectPersistentId>()->
                    PersistentId(&ContainedObjectPersistentId::GetPersistentIdWrapper)->
                    Field("m_data", &ContainedObjectPersistentId::m_data)->
                    Field("m_persistentId", &ContainedObjectPersistentId::m_persistentId);
            }
        };

        class ContainedObjectDerivedPersistentId
            : public ContainedObjectPersistentId
        {
        public:
            AZ_TYPE_INFO(ContainedObjectDerivedPersistentId, "{1c3ba36a-ceee-4118-89e7-807930bf2bec}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ContainedObjectDerivedPersistentId, ContainedObjectPersistentId>();
            }
        };

        class ContainedObjectNoPersistentId
        {
        public:
            AZ_CLASS_ALLOCATOR(ContainedObjectNoPersistentId, SystemAllocator);
            AZ_TYPE_INFO(ContainedObjectNoPersistentId, "{A9980498-6E7A-42C0-BF9F-DFA48142DDAB}");

            ContainedObjectNoPersistentId()
                : m_data(0)
            {}

            ContainedObjectNoPersistentId(int data)
                : m_data(data)
            {}

            int m_data;

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ContainedObjectNoPersistentId>()->
                    Field("m_data", &ContainedObjectNoPersistentId::m_data);
            }
        };

        class SimpleClassContainingVectorOfInts
        {
        public:
            AZ_TYPE_INFO(SimpleClassContainingVectorOfInts, "{82FE64FA-23DB-40B5-BD1B-9DC145CB86EA}");
            AZ_CLASS_ALLOCATOR(SimpleClassContainingVectorOfInts, AZ::SystemAllocator);

            virtual ~SimpleClassContainingVectorOfInts() = default;

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<SimpleClassContainingVectorOfInts>()
                    ->Field("id", &SimpleClassContainingVectorOfInts::m_id);
            }

            AZStd::vector<int> m_id;
        };

        class CommonPatch
        {
        public:
            AZ_RTTI(CommonPatch, "{81FE64FA-23DB-40B5-BD1B-9DC145CB86EA}");
            AZ_CLASS_ALLOCATOR(CommonPatch, AZ::SystemAllocator);

            virtual ~CommonPatch() = default;

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<CommonPatch>()
                    ->SerializeWithNoData();
            }
        };

        class ObjectToPatch
            : public CommonPatch
        {
        public:
            AZ_RTTI(ObjectToPatch, "{47E5CF10-3FA1-4064-BE7A-70E3143B4025}", CommonPatch);
            AZ_CLASS_ALLOCATOR(ObjectToPatch, AZ::SystemAllocator);

            ObjectToPatch() = default;
            ObjectToPatch(const ObjectToPatch&) = delete;

            int m_intValue = 0;
            AZStd::vector<ContainedObjectPersistentId> m_objectArray;
            AZStd::vector<ContainedObjectDerivedPersistentId> m_derivedObjectArray;
            AZStd::unordered_map<u32, AZStd::unique_ptr<ContainedObjectNoPersistentId>> m_objectMap;
            AZStd::vector<ContainedObjectNoPersistentId> m_objectArrayNoPersistentId;
            AZ::DynamicSerializableField m_dynamicField;

            ~ObjectToPatch() override
            {
                m_dynamicField.DestroyData();
            }

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectToPatch, CommonPatch>()->
                    Field("m_dynamicField", &ObjectToPatch::m_dynamicField)->
                    Field("m_intValue", &ObjectToPatch::m_intValue)->
                    Field("m_objectArray", &ObjectToPatch::m_objectArray)->
                    Field("m_derivedObjectArray", &ObjectToPatch::m_derivedObjectArray)->
                    Field("m_objectMap", &ObjectToPatch::m_objectMap)->
                    Field("m_objectArrayNoPersistentId", &ObjectToPatch::m_objectArrayNoPersistentId);
            }
        };

        class DifferentObjectToPatch
            : public CommonPatch
        {
        public:
            AZ_RTTI(DifferentObjectToPatch, "{2E107ABB-E77A-4188-AC32-4CA8EB3C5BD1}", CommonPatch);
            AZ_CLASS_ALLOCATOR(DifferentObjectToPatch, AZ::SystemAllocator);

            float m_data;

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<DifferentObjectToPatch, CommonPatch>()->
                    Field("m_data", &DifferentObjectToPatch::m_data);
            }
        };

        class ObjectsWithGenerics
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectsWithGenerics, SystemAllocator);
            AZ_TYPE_INFO(ObjectsWithGenerics, "{DE1EE15F-3458-40AE-A206-C6C957E2432B}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectsWithGenerics>()->
                    Field("m_string", &ObjectsWithGenerics::m_string);
            }

            AZStd::string m_string;
        };

        class ObjectWithPointer
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectWithPointer, SystemAllocator);
            AZ_TYPE_INFO(ObjectWithPointer, "{D1FD3240-A7C5-4EA3-8E55-CD18193162B8}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectWithPointer>()
                    ->Field("m_int", &ObjectWithPointer::m_int)
                    ->Field("m_pointerInt", &ObjectWithPointer::m_pointerInt)
                    ;
            }

            AZ::s32 m_int;
            AZ::s32* m_pointerInt = nullptr;
        };

        class ObjectWithMultiPointers
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectWithMultiPointers, SystemAllocator);
            AZ_TYPE_INFO(ObjectWithMultiPointers, "{EBA25BFA-CFA0-4397-929C-A765BA72DE28}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectWithMultiPointers>()
                    ->Field("m_int", &ObjectWithMultiPointers::m_int)
                    ->Field("m_pointerInt", &ObjectWithMultiPointers::m_pointerInt)
                    ->Field("m_pointerFloat", &ObjectWithMultiPointers::m_pointerFloat)
                    ;
            }

            AZ::s32 m_int;
            AZ::s32* m_pointerInt = nullptr;
            float* m_pointerFloat = nullptr;
        };


        static AZStd::string IntToString(int)
        {
            AZ_Assert(false, "Version 0 Type Converter for ObjectWithNumericFieldV1 should never be called");
            return {};
        };

        // If the version 1 to 2 version converter ran
        // A sentinel value of 32.0 is always returned which can be represented exactly
        // in floating point(power of 2)
        static double IntToDouble(int)
        {
            return 32.0;
        };

        struct ObjectWithNumericFieldV1
        {
            AZ_CLASS_ALLOCATOR(ObjectWithNumericFieldV1, SystemAllocator);
            AZ_TYPE_INFO(ObjectWithNumericFieldV1, "{556A83B0-77BC-41D1-B3BC-C1CD0A4F5845}");

            static void Reflect(AZ::ReflectContext* reflectContext)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<ObjectWithNumericFieldV1>()
                        ->Version(1)
                        ->Field("IntValue", &ObjectWithNumericFieldV1::m_value)
                        //! Provide a name converter for Version 0 -> 1
                        //! This should never be called as there is no Version 0 of this class
                        //! It is here to validate that it is never called
                        ->NameChange(0, 1, "IntValue", "NameValueThatShouldNeverBeSet")
                        //! Version 0 -> 1 type converter should never be called
                        ->TypeChange("IntValue", 0, 1, AZStd::function<AZStd::string(const int&)>(&IntToString))
                        ;
                }
            }


            int m_value{};
        };

        struct ObjectWithNumericFieldV2
        {
            AZ_CLASS_ALLOCATOR(ObjectWithNumericFieldV2, SystemAllocator);
            AZ_TYPE_INFO(ObjectWithNumericFieldV2, "{556A83B0-77BC-41D1-B3BC-C1CD0A4F5845}");

            static void Reflect(AZ::ReflectContext* reflectContext)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<ObjectWithNumericFieldV2>()
                        ->Version(2)
                        ->Field("DoubleValue", &ObjectWithNumericFieldV2::m_value)
                        //! Provide a name converter for Version 0 -> 1
                        //! This should never be called as there is no Version 0 of this class
                        //! It is here to validate that it is never called
                        ->NameChange(0, 1, "IntValue", "NameValueThatShouldNeverBeSet")
                        //! Version 0 -> 1 type converter should never be called
                        ->TypeChange("IntValue", 0, 1, AZStd::function<AZStd::string(const int&)>(&IntToString))

                        ->NameChange(1, 2, "IntValue", "DoubleValue")
                        ->TypeChange("IntValue", 1, 2, AZStd::function<double(const int&)>(&IntToDouble))
                        ;
                }
            }


            double m_value{};
        };

        class InnerObjectFieldConverterV1
        {
        public:
            AZ_CLASS_ALLOCATOR(InnerObjectFieldConverterV1, SystemAllocator);
            AZ_RTTI(InnerObjectFieldConverterV1, "{28E61B17-F321-4D4E-9F4C-00846C6631DE}");
            virtual ~InnerObjectFieldConverterV1() = default;

            static void Reflect(AZ::ReflectContext* reflectContext)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<InnerObjectFieldConverterV1>()
                        ->Version(1)
                        ->Field("InnerBaseStringField", &InnerObjectFieldConverterV1::m_stringField)
                        ->Field("InnerBaseStringVector", &InnerObjectFieldConverterV1::m_stringVector)
                        ;
                }
            }

            //! AZStd::string uses IDataSerializer for Serialization.
            //! This is to test Field Converters for patched element that are on a class that is a descendant element of the patched class
            AZStd::string m_stringField;
            //! AZStd::string uses IDataSerializer for Serialization.
            //! This is to test Field Converters for patched element that are on a class that is a descendant element of the patched class
            AZStd::vector<AZStd::string> m_stringVector;
        };

        template<typename BaseClass>
        class InnerObjectFieldConverterDerivedV1Template
            : public BaseClass
        {
        public:
            AZ_CLASS_ALLOCATOR(InnerObjectFieldConverterDerivedV1Template, SystemAllocator);
            AZ_RTTI(InnerObjectFieldConverterDerivedV1Template, "{C68BE9B8-33F8-4969-B521-B44F5BA1C0DE}", BaseClass);

            static void Reflect(AZ::ReflectContext* reflectContext)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<InnerObjectFieldConverterDerivedV1Template, BaseClass>()
                        ->Version(1)
                        ->Field("InnerDerivedNumericField", &InnerObjectFieldConverterDerivedV1Template::m_objectWithNumericField)
                        ;
                }
            }

            //! ObjectWithNumericFieldV1 uses the normal SerializeContext::ClassBulder for for Serialization.
            //! This is to test Field Converters for a patched element serialized in a pointer to the base class
            ObjectWithNumericFieldV1 m_objectWithNumericField;
        };

        using InnerObjectFieldConverterDerivedV1 = InnerObjectFieldConverterDerivedV1Template<InnerObjectFieldConverterV1>;

        class ObjectFieldConverterV1
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectFieldConverterV1, SystemAllocator);
            AZ_TYPE_INFO(ObjectFieldConverterV1, "{5722C4E4-25DE-48C5-BC89-0EE9D38DF433}");

            static void Reflect(AZ::ReflectContext* reflectContext)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<ObjectFieldConverterV1>()
                        ->Version(1)
                        ->Field("RootStringField", &ObjectFieldConverterV1::m_rootStringField)
                        ->Field("RootStringVector", &ObjectFieldConverterV1::m_rootStringVector)
                        ->Field("RootInnerObjectValue", &ObjectFieldConverterV1::m_rootInnerObject)
                        ->Field("RootInnerObjectPointer", &ObjectFieldConverterV1::m_baseInnerObjectPolymorphic)
                        ;
                }
            }

            //! AZStd::string uses IDataSerializer for Serialization.
            //! This is to test Field Converters for patched element that are directly on the patched class
            AZStd::string m_rootStringField;
            //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
            //! This is to test Field Converters for patched element that are directly on the patched class
            AZStd::vector<AZStd::string> m_rootStringVector;
            //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
            //! This is to test Field Converters for patched element that are directly on the patched class
            InnerObjectFieldConverterV1 m_rootInnerObject{};
            InnerObjectFieldConverterV1* m_baseInnerObjectPolymorphic{};
        };
        class ObjectBaseClass
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectBaseClass, SystemAllocator);
            AZ_RTTI(ObjectBaseClass, "{9CFEC143-9C78-4566-A541-46F9CA6FE66E}");

            virtual ~ObjectBaseClass() = default;

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectBaseClass>();
            }
        };

        class ObjectDerivedClass1 : public ObjectBaseClass
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectDerivedClass1, SystemAllocator);
            AZ_RTTI(ObjectDerivedClass1, "{9D6502E8-999D-46B8-AF37-EAAA0D53385A}", ObjectBaseClass);
            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectDerivedClass1>();
            }
        };


        class ObjectDerivedClass2 : public ObjectBaseClass
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectDerivedClass2, SystemAllocator);
            AZ_RTTI(ObjectDerivedClass2, "{91D1812E-17A2-4BC3-A9A1-13196BE50803}", ObjectBaseClass);
            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectDerivedClass2>();
            }
        };

        class ObjectDerivedClass3 : public ObjectBaseClass
        {
            public:
            AZ_CLASS_ALLOCATOR(ObjectDerivedClass3, SystemAllocator);
            AZ_RTTI(ObjectDerivedClass3, "{E80E926B-5750-4E8D-80E0-D06057692847}", ObjectBaseClass);
            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectDerivedClass3>();
            }
        };

        static bool ConvertDerivedClass2ToDerivedClass3(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            classElement.Convert(context, AZ::AzTypeInfo<ObjectDerivedClass3>::Uuid());
            return true;
        }

        class ObjectWithVectorOfBaseClasses
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectWithVectorOfBaseClasses, SystemAllocator);
            AZ_TYPE_INFO(ObjectWithVectorOfBaseClasses, "{BC9D5346-1BC5-41C4-8CF0-7ACD96F7790F}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectWithVectorOfBaseClasses>()
                    ->Field("m_vectorOfBaseClasses", &ObjectWithVectorOfBaseClasses::m_vectorOfBaseClasses);
            }

            virtual ~ObjectWithVectorOfBaseClasses()
            {
                for (auto element : m_vectorOfBaseClasses)
                {
                    delete element;
                }
                m_vectorOfBaseClasses.clear();
            }

            AZStd::vector<ObjectBaseClass*> m_vectorOfBaseClasses;
        };
    }

    class PatchingTest
        : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_serializeContext = AZStd::make_unique<SerializeContext>();

            using namespace Patching;
            CommonPatch::Reflect(*m_serializeContext);
            ContainedObjectPersistentId::Reflect(*m_serializeContext);
            ContainedObjectDerivedPersistentId::Reflect(*m_serializeContext);
            ContainedObjectNoPersistentId::Reflect(*m_serializeContext);
            ObjectToPatch::Reflect(*m_serializeContext);
            DifferentObjectToPatch::Reflect(*m_serializeContext);
            ObjectsWithGenerics::Reflect(*m_serializeContext);
            ObjectWithPointer::Reflect(*m_serializeContext);
            ObjectWithMultiPointers::Reflect(*m_serializeContext);
            AZ::DataPatch::Reflect(m_serializeContext.get());

            const SerializeContext::ClassData* addressTypeSerializerClassData = m_serializeContext.get()->FindClassData(azrtti_typeid<DataPatch::AddressType>());

            AZ_Assert(addressTypeSerializerClassData, "AddressType class not reflected, required to run DataPatch Unit Tests");
            m_addressTypeSerializer = static_cast<DataPatchInternal::AddressTypeSerializer*>(addressTypeSerializerClassData->m_serializer.get());
            AZ_Assert(m_addressTypeSerializer, "AddressTypeSerializer not provided in class AddressType's reflection, required to run DataPatch Unit Tests");
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            m_addressTypeSerializer = nullptr;
            LeakDetectionFixture::TearDown();
        }

        void LoadPatchFromXML(const AZStd::string_view& xmlSrc, DataPatch& patchDest)
        {
            AZ::IO::MemoryStream xmlStream(xmlSrc.data(), xmlSrc.size());
            Utils::LoadObjectFromStreamInPlace(xmlStream, patchDest, m_serializeContext.get());
        }

        void LoadPatchFromByteStream(const AZStd::vector<AZ::u8>& byteStreamSrc, DataPatch& patchDest)
        {
            AZ::IO::MemoryStream streamRead(byteStreamSrc.data(), byteStreamSrc.size());
            AZ::Utils::LoadObjectFromStreamInPlace(streamRead, patchDest, m_serializeContext.get());
        }

        void WritePatchToByteStream(const DataPatch& patchSrc, AZStd::vector<AZ::u8>& byteStreamDest)
        {
            byteStreamDest.clear();
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> streamWrite(&byteStreamDest);
            AZ::Utils::SaveObjectToStream(streamWrite, AZ::DataStream::ST_XML, &patchSrc, m_serializeContext.get());
        }

        // Template XML that can be formatted for multiple tests

        // ObjectToPatch m_intValue override
        const char* m_XMLDataPatchV1AddressTypeIntOverrideTemplate = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{BFF7A3F5-9014-4000-92C7-9B2BC7913DA9}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CEA836FC-77E0-5E46-BD0F-2E5A39D845E9}">
                                    <Class name="AZStd::pair" field="element" type="{FED51EB4-F646-51FF-9646-9852CF90F353}">
                                            <Class name="AddressType" field="value1" value="%s" version="1" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="any" field="value2" type="{03924488-C7F4-4D6D-948B-ABC2D1AE2FD3}">
                                                    <Class name="int" field="m_data" value="%i" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                            </Class>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

        // Valid address for above XML for easy formatting of tests expected to pass
        const char* m_XMLDataPatchV1AddressTypeIntOverrideValidAddress = R"(int({72039442-EB38-4D42-A1AD-CB68F7E0EEF6})::m_intValue%s0%s)";

        // ObjectToPatch m_objectArray overrides. Container is size 5. Can format the first address and each element's data and persistent ids
        const char* m_XMLDataPatchV1AddressTypeIntVectorOverrideTemplate = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{BFF7A3F5-9014-4000-92C7-9B2BC7913DA9}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CEA836FC-77E0-5E46-BD0F-2E5A39D845E9}">
                                    <Class name="AZStd::pair" field="element" type="{FED51EB4-F646-51FF-9646-9852CF90F353}">
                                            <Class name="AddressType" field="value1" value="%s" version="1" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="any" field="value2" type="{03924488-C7F4-4D6D-948B-ABC2D1AE2FD3}">
                                                    <Class name="ContainedObjectPersistentId" field="m_data" type="{D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA}">
                                                            <Class name="int" field="m_data" value="%i" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                            <Class name="AZ::u64" field="m_persistentId" value="%i" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                    </Class>
                                            </Class>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{FED51EB4-F646-51FF-9646-9852CF90F353}">
                                            <Class name="AddressType" field="value1" value="AZStd::vector({861A12B0-BD91-528E-9CEC-505246EE98DE})::m_objectArray%s0%sContainedObjectPersistentId({D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA})#%i%s0%s" version="1" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="any" field="value2" type="{03924488-C7F4-4D6D-948B-ABC2D1AE2FD3}">
                                                    <Class name="ContainedObjectPersistentId" field="m_data" type="{D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA}">
                                                            <Class name="int" field="m_data" value="%i" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                            <Class name="AZ::u64" field="m_persistentId" value="%i" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                    </Class>
                                            </Class>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{FED51EB4-F646-51FF-9646-9852CF90F353}">
                                            <Class name="AddressType" field="value1" value="AZStd::vector({861A12B0-BD91-528E-9CEC-505246EE98DE})::m_objectArray%s0%sContainedObjectPersistentId({D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA})#%i%s0%s" version="1" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="any" field="value2" type="{03924488-C7F4-4D6D-948B-ABC2D1AE2FD3}">
                                                    <Class name="ContainedObjectPersistentId" field="m_data" type="{D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA}">
                                                            <Class name="int" field="m_data" value="%i" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                            <Class name="AZ::u64" field="m_persistentId" value="%i" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                    </Class>
                                            </Class>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{FED51EB4-F646-51FF-9646-9852CF90F353}">
                                            <Class name="AddressType" field="value1" value="AZStd::vector({861A12B0-BD91-528E-9CEC-505246EE98DE})::m_objectArray%s0%sContainedObjectPersistentId({D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA})#%i%s0%s" version="1" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="any" field="value2" type="{03924488-C7F4-4D6D-948B-ABC2D1AE2FD3}">
                                                    <Class name="ContainedObjectPersistentId" field="m_data" type="{D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA}">
                                                            <Class name="int" field="m_data" value="%i" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                            <Class name="AZ::u64" field="m_persistentId" value="%i" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                    </Class>
                                            </Class>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{FED51EB4-F646-51FF-9646-9852CF90F353}">
                                            <Class name="AddressType" field="value1" value="AZStd::vector({861A12B0-BD91-528E-9CEC-505246EE98DE})::m_objectArray%s0%sContainedObjectPersistentId({D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA})#%i%s0%s" version="1" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="any" field="value2" type="{03924488-C7F4-4D6D-948B-ABC2D1AE2FD3}">
                                                    <Class name="ContainedObjectPersistentId" field="m_data" type="{D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA}">
                                                            <Class name="int" field="m_data" value="%i" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                            <Class name="AZ::u64" field="m_persistentId" value="%i" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                    </Class>
                                            </Class>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

        // Valid address for above XML for easy formatting of tests expected to pass
        const char* m_XMLDataPatchV1AddressTypeIntVectorOverrideValidAddress = R"(AZStd::vector({861A12B0-BD91-528E-9CEC-505246EE98DE})::m_objectArray%s0%sContainedObjectPersistentId({D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA})#%i%s0%s)";

        /*
            Builds a valid address for m_XMLDataPatchV1AddressTypeIntOverrideTemplate
            while allowing formatting of the path and version delimiters
        */
        AZStd::string GetValidAddressForXMLDataPatchV1AddressTypeIntXML()
        {
            return AZStd::string::format(m_XMLDataPatchV1AddressTypeIntOverrideValidAddress,
                                         V1AddressTypeElementVersionDelimiter,
                                         V1AddressTypeElementPathDelimiter);
        }

        /*
            Sets the data value for the int type stored in m_XMLDataPatchV1AddressTypeIntOverrideTemplate's xml stream
            Can also optionally set the address, otherwise defaults to a valid address if testAddress is nullptr
        */
        AZStd::string BuildXMLDataPatchV1AddressTypeIntXML(const char* testAddress, int intValue)
        {
            AZStd::string editableAddress;

            if (testAddress)
            {
                editableAddress = testAddress;
            }
            else
            {
                editableAddress = GetValidAddressForXMLDataPatchV1AddressTypeIntXML();
            }

            return AZStd::string::format(m_XMLDataPatchV1AddressTypeIntOverrideTemplate, editableAddress.c_str(), intValue);
        }

        /*
           Sets the data values and persistentId values for the ContainedObjectPersistentId type stored in m_XMLDataPatchV1AddressTypeIntVectorOverrideTemplate's xml stream
           Can also optionally set the address of the first element otherwise defaults to a valid address if testAddress is nullptr
           The stream contains 5 elements within the vector
        */
        AZStd::string BuildXMLDataPatchV1AddressTypeIntVectorXML(const char* testAddress, int dataModifier, int persistentIdModifier)
        {
            AZStd::string editableAddress;

            // Allow customization of our delimiters
            const char* versionDelimiter = V1AddressTypeElementVersionDelimiter;
            const char* pathDelimiter = V1AddressTypeElementPathDelimiter;

            // If a testAddress was supplied then use it.
            // Otherwise format with a valid path
            if (testAddress)
            {
                editableAddress = testAddress;
            }
            else
            {
                editableAddress = AZStd::string::format(m_XMLDataPatchV1AddressTypeIntVectorOverrideValidAddress,
                                                        versionDelimiter,
                                                        pathDelimiter,
                                                        4 + persistentIdModifier,
                                                        versionDelimiter,
                                                        pathDelimiter);
            }

            // Format our xml.
            // It is size 5 so we format the data and persistent IDs to match this size.
            return AZStd::string::format(m_XMLDataPatchV1AddressTypeIntVectorOverrideTemplate,
                                         editableAddress.c_str(), 4 + dataModifier, 4 + persistentIdModifier,
                                         versionDelimiter, pathDelimiter, 3 + persistentIdModifier, versionDelimiter, pathDelimiter, 3 + dataModifier, 3 + persistentIdModifier,
                                         versionDelimiter, pathDelimiter, 2 + persistentIdModifier, versionDelimiter, pathDelimiter, 2 + dataModifier, 2 + persistentIdModifier,
                                         versionDelimiter, pathDelimiter, 1 + persistentIdModifier, versionDelimiter, pathDelimiter, 1 + dataModifier, 1 + persistentIdModifier,
                                         versionDelimiter, pathDelimiter, 0 + persistentIdModifier, versionDelimiter, pathDelimiter, 0 + dataModifier, 0 + persistentIdModifier);
        }

        // Store each AddressTypeElement version's delimiters seperately from the class so our tests don't auto update to a new version if these delimiters change in V2+
        static constexpr const char* V1AddressTypeElementPathDelimiter = "/";
        static constexpr const char* V1AddressTypeElementVersionDelimiter = AZ::DataPatchInternal::AddressTypeElement::VersionDelimiter; // utf-8 for <middledot>

        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        DataPatchInternal::AddressTypeSerializer* m_addressTypeSerializer;
    };

    namespace Patching
    {
        TEST_F(PatchingTest, UberTest)
        {
            ObjectToPatch sourceObj;
            sourceObj.m_intValue = 101;
            sourceObj.m_objectArray.emplace_back();
            sourceObj.m_objectArray.emplace_back();
            sourceObj.m_objectArray.emplace_back();
            sourceObj.m_dynamicField.Set(aznew ContainedObjectNoPersistentId(40));
            {
                // derived
                sourceObj.m_derivedObjectArray.emplace_back();
                sourceObj.m_derivedObjectArray.emplace_back();
                sourceObj.m_derivedObjectArray.emplace_back();
            }

            // test generic containers with persistent ID
            sourceObj.m_objectArray[0].m_persistentId = 1;
            sourceObj.m_objectArray[0].m_data = 201;
            sourceObj.m_objectArray[1].m_persistentId = 2;
            sourceObj.m_objectArray[1].m_data = 202;
            sourceObj.m_objectArray[2].m_persistentId = 3;
            sourceObj.m_objectArray[2].m_data = 203;
            {
                // derived
                sourceObj.m_derivedObjectArray[0].m_persistentId = 1;
                sourceObj.m_derivedObjectArray[0].m_data = 2010;
                sourceObj.m_derivedObjectArray[1].m_persistentId = 2;
                sourceObj.m_derivedObjectArray[1].m_data = 2020;
                sourceObj.m_derivedObjectArray[2].m_persistentId = 3;
                sourceObj.m_derivedObjectArray[2].m_data = 2030;
            }

            ObjectToPatch targetObj;
            targetObj.m_intValue = 121;
            targetObj.m_objectArray.emplace_back();
            targetObj.m_objectArray.emplace_back();
            targetObj.m_objectArray.emplace_back();
            targetObj.m_objectArray[0].m_persistentId = 1;
            targetObj.m_objectArray[0].m_data = 301;
            targetObj.m_dynamicField.Set(aznew ContainedObjectNoPersistentId(50));
            {
                // derived
                targetObj.m_derivedObjectArray.emplace_back();
                targetObj.m_derivedObjectArray.emplace_back();
                targetObj.m_derivedObjectArray.emplace_back();
                targetObj.m_derivedObjectArray[0].m_persistentId = 1;
                targetObj.m_derivedObjectArray[0].m_data = 3010;
            }
            // remove element 2
            targetObj.m_objectArray[1].m_persistentId = 3;
            targetObj.m_objectArray[1].m_data = 303;
            {
                // derived
                targetObj.m_derivedObjectArray[1].m_persistentId = 3;
                targetObj.m_derivedObjectArray[1].m_data = 3030;
            }
            // add new element
            targetObj.m_objectArray[2].m_persistentId = 4;
            targetObj.m_objectArray[2].m_data = 304;
            {
                // derived
                targetObj.m_derivedObjectArray[2].m_persistentId = 4;
                targetObj.m_derivedObjectArray[2].m_data = 3040;
            }

            // insert lots of objects without persistent id
            targetObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                targetObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i);
            }

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Compare the generated and original target object
            EXPECT_TRUE(generatedObj);

            EXPECT_EQ(generatedObj->m_intValue, targetObj.m_intValue);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            EXPECT_EQ(generatedObj->m_objectArray[0].m_data, targetObj.m_objectArray[0].m_data);
            EXPECT_EQ(generatedObj->m_objectArray[0].m_persistentId, targetObj.m_objectArray[0].m_persistentId);

            EXPECT_EQ(generatedObj->m_objectArray[1].m_data, targetObj.m_objectArray[1].m_data);
            EXPECT_EQ(generatedObj->m_objectArray[1].m_persistentId, targetObj.m_objectArray[1].m_persistentId);

            EXPECT_EQ(generatedObj->m_objectArray[2].m_data, targetObj.m_objectArray[2].m_data);
            EXPECT_EQ(generatedObj->m_objectArray[2].m_persistentId, targetObj.m_objectArray[2].m_persistentId);

            EXPECT_EQ(50, generatedObj->m_dynamicField.Get<ContainedObjectNoPersistentId>()->m_data);
            {
                // derived
                EXPECT_EQ(generatedObj->m_derivedObjectArray.size(), targetObj.m_derivedObjectArray.size());

                EXPECT_EQ(generatedObj->m_derivedObjectArray[0].m_data, targetObj.m_derivedObjectArray[0].m_data);
                EXPECT_EQ(generatedObj->m_derivedObjectArray[0].m_persistentId, targetObj.m_derivedObjectArray[0].m_persistentId);

                EXPECT_EQ(generatedObj->m_derivedObjectArray[1].m_data, targetObj.m_derivedObjectArray[1].m_data);
                EXPECT_EQ(generatedObj->m_derivedObjectArray[1].m_persistentId, targetObj.m_derivedObjectArray[1].m_persistentId);

                EXPECT_EQ(generatedObj->m_derivedObjectArray[2].m_data, targetObj.m_derivedObjectArray[2].m_data);
                EXPECT_EQ(generatedObj->m_derivedObjectArray[2].m_persistentId, targetObj.m_derivedObjectArray[2].m_persistentId);
            }

            // test that the relative order of elements without persistent ID is preserved
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), targetObj.m_objectArrayNoPersistentId.size());
            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId[i].m_data, targetObj.m_objectArrayNoPersistentId[i].m_data);
            }

            // \note do we need to add support for base class patching and recover for root elements with proper casting

            generatedObj->m_dynamicField.DestroyData(m_serializeContext.get());
            targetObj.m_dynamicField.DestroyData(m_serializeContext.get());
            sourceObj.m_dynamicField.DestroyData(m_serializeContext.get());

            //delete generatedObj;
        }

        TEST_F(PatchingTest, PatchArray_RemoveAllObjects_DataPatchAppliesCorrectly)
        {
            // Init Source with arbitrary Persistent IDs and data
            ObjectToPatch sourceObj;

            sourceObj.m_objectArray.resize(999);
            for (size_t i = 0; i < sourceObj.m_objectArray.size(); ++i)
            {
                sourceObj.m_objectArray[i].m_persistentId = static_cast<int>(i + 10);
                sourceObj.m_objectArray[i].m_data = static_cast<int>(i + 200);
            }

            // Init empty Target
            ObjectToPatch targetObj;

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            EXPECT_TRUE(targetObj.m_objectArray.empty());
            EXPECT_TRUE(generatedObj->m_objectArray.empty());
        }

        TEST_F(PatchingTest, PatchArray_AddObjects_DataPatchAppliesCorrectly)
        {
            // Init empty Source
            ObjectToPatch sourceObj;

            // Init Target with arbitrary Persistent IDs and data
            ObjectToPatch targetObj;

            targetObj.m_objectArray.resize(999);
            for (size_t i = 0; i < targetObj.m_objectArray.size(); ++i)
            {
                targetObj.m_objectArray[i].m_persistentId = static_cast<int>(i + 10);
                targetObj.m_objectArray[i].m_data = static_cast<int>(i + 200);
            }

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            for (size_t i = 0; i < generatedObj->m_objectArray.size(); ++i)
            {
                EXPECT_EQ(generatedObj->m_objectArray[i].m_persistentId, targetObj.m_objectArray[i].m_persistentId);
                EXPECT_EQ(generatedObj->m_objectArray[i].m_data, targetObj.m_objectArray[i].m_data);
            }
        }

        TEST_F(PatchingTest, PatchArray_EditAllObjects_DataPatchAppliesCorrectly)
        {
            // Init Source and Target with arbitrary Persistent IDs (the same) and data (different)
            ObjectToPatch sourceObj;
            sourceObj.m_objectArray.resize(999);

            ObjectToPatch targetObj;
            targetObj.m_objectArray.resize(999);

            for (size_t i = 0; i < sourceObj.m_objectArray.size(); ++i)
            {
                sourceObj.m_objectArray[i].m_persistentId = static_cast<int>(i + 10);
                sourceObj.m_objectArray[i].m_data = static_cast<int>(i + 200);

                // Keep the Persistent IDs the same but change the data
                targetObj.m_objectArray[i].m_persistentId = sourceObj.m_objectArray[i].m_persistentId;
                targetObj.m_objectArray[i].m_data = sourceObj.m_objectArray[i].m_data + 100;
            }

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            for (size_t i = 0; i < generatedObj->m_objectArray.size(); ++i)
            {
                EXPECT_EQ(generatedObj->m_objectArray[i].m_persistentId, targetObj.m_objectArray[i].m_persistentId);
                EXPECT_EQ(generatedObj->m_objectArray[i].m_data, targetObj.m_objectArray[i].m_data);
            }
        }

        TEST_F(PatchingTest, PatchArray_AddRemoveEdit_DataPatchAppliesCorrectly)
        {
            // Init Source
            ObjectToPatch sourceObj;

            sourceObj.m_objectArray.resize(3);

            sourceObj.m_objectArray[0].m_persistentId = 1;
            sourceObj.m_objectArray[0].m_data = 201;

            sourceObj.m_objectArray[1].m_persistentId = 2;
            sourceObj.m_objectArray[1].m_data = 202;

            sourceObj.m_objectArray[2].m_persistentId = 3;
            sourceObj.m_objectArray[2].m_data = 203;

            // Init Target
            ObjectToPatch targetObj;

            targetObj.m_objectArray.resize(4);

            // Edit ID 1
            targetObj.m_objectArray[0].m_persistentId = 1;
            targetObj.m_objectArray[0].m_data = 301;

            // Remove ID 2, do not edit ID 3
            targetObj.m_objectArray[1].m_persistentId = 3;
            targetObj.m_objectArray[1].m_data = 203;

            // Add ID 4 and 5
            targetObj.m_objectArray[2].m_persistentId = 4;
            targetObj.m_objectArray[2].m_data = 304;

            targetObj.m_objectArray[3].m_persistentId = 5;
            targetObj.m_objectArray[3].m_data = 305;

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            EXPECT_EQ(generatedObj->m_objectArray[0].m_persistentId, targetObj.m_objectArray[0].m_persistentId);
            EXPECT_EQ(generatedObj->m_objectArray[0].m_data, targetObj.m_objectArray[0].m_data);

            EXPECT_EQ(generatedObj->m_objectArray[1].m_persistentId, targetObj.m_objectArray[1].m_persistentId);
            EXPECT_EQ(generatedObj->m_objectArray[1].m_data, targetObj.m_objectArray[1].m_data);

            EXPECT_EQ(generatedObj->m_objectArray[2].m_persistentId, targetObj.m_objectArray[2].m_persistentId);
            EXPECT_EQ(generatedObj->m_objectArray[2].m_data, targetObj.m_objectArray[2].m_data);

            EXPECT_EQ(generatedObj->m_objectArray[3].m_persistentId, targetObj.m_objectArray[3].m_persistentId);
            EXPECT_EQ(generatedObj->m_objectArray[3].m_data, targetObj.m_objectArray[3].m_data);
        }

        TEST_F(PatchingTest, PatchArray_ObjectsHaveNoPersistentId_RemoveAllObjects_DataPatchAppliesCorrectly)
        {
            // Init Source
            ObjectToPatch sourceObj;

            sourceObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < sourceObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                sourceObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i);
            }

            // Init empty Target
            ObjectToPatch targetObj;

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), targetObj.m_objectArrayNoPersistentId.size());

            EXPECT_TRUE(targetObj.m_objectArrayNoPersistentId.empty());
            EXPECT_TRUE(generatedObj->m_objectArrayNoPersistentId.empty());
        }

        TEST_F(PatchingTest, PatchArray_ObjectsHaveNoPersistentId_AddObjects_DataPatchAppliesCorrectly)
        {
            // Init empty Source
            ObjectToPatch sourceObj;

            // Init Target
            ObjectToPatch targetObj;

            targetObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                targetObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i);
            }

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);

            EXPECT_THAT(generatedObj->m_objectArrayNoPersistentId, ::testing::Pointwise(::testing::Truly([](auto arg){ return testing::get<0>(arg).m_data == testing::get<1>(arg).m_data; }), targetObj.m_objectArrayNoPersistentId));
        }

        TEST_F(PatchingTest, SimpleClassContainingVectorOfInts)
        {
            SimpleClassContainingVectorOfInts::Reflect(*m_serializeContext.get());

            // Init empty Source
            SimpleClassContainingVectorOfInts sourceObj;

            // Init Target
            SimpleClassContainingVectorOfInts targetObj;

            targetObj.m_id.resize(20);
            for (size_t i = 0; i < targetObj.m_id.size(); ++i)
            {
                targetObj.m_id[i] = 0;
            }

            // Create and Apply Patch
            DataPatch patch;
            EXPECT_TRUE(patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get()));
            AZStd::unique_ptr<SimpleClassContainingVectorOfInts> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_THAT(generatedObj->m_id, ::testing::Pointwise(::testing::Truly([](auto arg){ return testing::get<0>(arg) == testing::get<1>(arg); }), targetObj.m_id));
        }

        TEST_F(PatchingTest, PatchArray_ObjectsHaveNoPersistentId_EditAllObjects_DataPatchAppliesCorrectly)
        {
            // Init Source
            ObjectToPatch sourceObj;

            sourceObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < sourceObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                sourceObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i);
            }

            // Init Target
            ObjectToPatch targetObj;

            targetObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                targetObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i + 1);
            }

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), targetObj.m_objectArrayNoPersistentId.size());

            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId[i].m_data, targetObj.m_objectArrayNoPersistentId[i].m_data);
            }
        }

        TEST_F(PatchingTest, PatchArray_ObjectsHaveNoPersistentId_RemoveEdit_DataPatchAppliesCorrectly)
        {
            // Init Source
            ObjectToPatch sourceObj;

            sourceObj.m_objectArrayNoPersistentId.resize(4);
            sourceObj.m_objectArrayNoPersistentId[0].m_data = static_cast<int>(1000);
            sourceObj.m_objectArrayNoPersistentId[1].m_data = static_cast<int>(1001);
            sourceObj.m_objectArrayNoPersistentId[2].m_data = static_cast<int>(1002);
            sourceObj.m_objectArrayNoPersistentId[3].m_data = static_cast<int>(1003);

            // Init Target
            ObjectToPatch targetObj;

            targetObj.m_objectArrayNoPersistentId.resize(2);
            targetObj.m_objectArrayNoPersistentId[0].m_data = static_cast<int>(2000);
            targetObj.m_objectArrayNoPersistentId[1].m_data = static_cast<int>(2001);

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), 2);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), targetObj.m_objectArrayNoPersistentId.size());

            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId[0].m_data, targetObj.m_objectArrayNoPersistentId[0].m_data);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId[1].m_data, targetObj.m_objectArrayNoPersistentId[1].m_data);
        }

        TEST_F(PatchingTest, PatchUnorderedMap_ObjectsHaveNoPersistentId_RemoveAllObjects_DataPatchAppliesCorrectly)
        {
            // test generic containers without persistent ID (by index)

            // Init Source
            ObjectToPatch sourceObj;
            sourceObj.m_objectMap.emplace(1, aznew ContainedObjectNoPersistentId(401));
            sourceObj.m_objectMap.emplace(2, aznew ContainedObjectNoPersistentId(402));
            sourceObj.m_objectMap.emplace(3, aznew ContainedObjectNoPersistentId(403));
            sourceObj.m_objectMap.emplace(4, aznew ContainedObjectNoPersistentId(404));

            // Init empty Target
            ObjectToPatch targetObj;

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectMap.size(), targetObj.m_objectMap.size());

            EXPECT_TRUE(targetObj.m_objectMap.empty());
            EXPECT_TRUE(generatedObj->m_objectMap.empty());
        }

        TEST_F(PatchingTest, PatchUnorderedMap_ObjectsHaveNoPersistentId_AddObjects_DataPatchAppliesCorrectly)
        {
            // test generic containers without persistent ID (by index)

            // Init empty Source
            ObjectToPatch sourceObj;

            // Init Target
            ObjectToPatch targetObj;
            targetObj.m_objectMap.emplace(1, aznew ContainedObjectNoPersistentId(401));
            targetObj.m_objectMap.emplace(2, aznew ContainedObjectNoPersistentId(402));
            targetObj.m_objectMap.emplace(3, aznew ContainedObjectNoPersistentId(403));
            targetObj.m_objectMap.emplace(4, aznew ContainedObjectNoPersistentId(404));

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectMap.size(), targetObj.m_objectMap.size());

            EXPECT_EQ(generatedObj->m_objectMap[1]->m_data, targetObj.m_objectMap[1]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[2]->m_data, targetObj.m_objectMap[2]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[3]->m_data, targetObj.m_objectMap[3]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[4]->m_data, targetObj.m_objectMap[4]->m_data);
        }

        TEST_F(PatchingTest, PatchUnorderedMap_ObjectsHaveNoPersistentId_EditAllObjects_DataPatchAppliesCorrectly)
        {
            // test generic containers without persistent ID (by index)

            // Init Source
            ObjectToPatch sourceObj;
            sourceObj.m_objectMap.emplace(1, aznew ContainedObjectNoPersistentId(401));
            sourceObj.m_objectMap.emplace(2, aznew ContainedObjectNoPersistentId(402));
            sourceObj.m_objectMap.emplace(3, aznew ContainedObjectNoPersistentId(403));
            sourceObj.m_objectMap.emplace(4, aznew ContainedObjectNoPersistentId(404));

            // Init Target
            ObjectToPatch targetObj;
            targetObj.m_objectMap.emplace(1, aznew ContainedObjectNoPersistentId(501));
            targetObj.m_objectMap.emplace(2, aznew ContainedObjectNoPersistentId(502));
            targetObj.m_objectMap.emplace(3, aznew ContainedObjectNoPersistentId(503));
            targetObj.m_objectMap.emplace(4, aznew ContainedObjectNoPersistentId(504));

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectMap.size(), targetObj.m_objectMap.size());

            EXPECT_EQ(generatedObj->m_objectMap[1]->m_data, targetObj.m_objectMap[1]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[2]->m_data, targetObj.m_objectMap[2]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[3]->m_data, targetObj.m_objectMap[3]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[4]->m_data, targetObj.m_objectMap[4]->m_data);
        }

        TEST_F(PatchingTest, PatchUnorderedMap_ObjectsHaveNoPersistentId_AddRemoveEdit_DataPatchAppliesCorrectly)
        {
            // test generic containers without persistent ID (by index)

            // Init Source
            ObjectToPatch sourceObj;
            sourceObj.m_objectMap.emplace(1, aznew ContainedObjectNoPersistentId(401));
            sourceObj.m_objectMap.emplace(2, aznew ContainedObjectNoPersistentId(402));
            sourceObj.m_objectMap.emplace(3, aznew ContainedObjectNoPersistentId(403));
            sourceObj.m_objectMap.emplace(4, aznew ContainedObjectNoPersistentId(404));

            // Init Target
            ObjectToPatch targetObj;
            // This will mark the object at index 1 as an edit, objects 2-4 as removed, and 5 as an addition
            targetObj.m_objectMap.emplace(1, aznew ContainedObjectNoPersistentId(501));
            targetObj.m_objectMap.emplace(5, aznew ContainedObjectNoPersistentId(405));

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectMap.size(), 2);
            EXPECT_EQ(generatedObj->m_objectMap.size(), targetObj.m_objectMap.size());

            EXPECT_EQ(generatedObj->m_objectMap[1]->m_data, targetObj.m_objectMap[1]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[5]->m_data, targetObj.m_objectMap[5]->m_data);
        }

        TEST_F(PatchingTest, ReplaceRootElement_DifferentObjects_DataPatchAppliesCorrectly)
        {
            ObjectToPatch obj1;
            DifferentObjectToPatch obj2;
            obj1.m_intValue = 99;
            obj2.m_data = 3.33f;

            DataPatch patch1;
            patch1.Create(static_cast<CommonPatch*>(&obj1), static_cast<CommonPatch*>(&obj2), DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get()); // cast to base classes
            DifferentObjectToPatch* obj2Generated = patch1.Apply<DifferentObjectToPatch>(&obj1, m_serializeContext.get());
            EXPECT_EQ(obj2.m_data, obj2Generated->m_data);

            delete obj2Generated;
        }

        TEST_F(PatchingTest, CompareWithGenerics_DifferentObjects_DataPatchAppliesCorrectly)
        {
            ObjectsWithGenerics sourceGeneric;
            sourceGeneric.m_string = "Hello";

            ObjectsWithGenerics targetGeneric;
            targetGeneric.m_string = "Ola";

            DataPatch genericPatch;
            genericPatch.Create(&sourceGeneric, &targetGeneric, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            ObjectsWithGenerics* targerGenericGenerated = genericPatch.Apply(&sourceGeneric, m_serializeContext.get());
            EXPECT_EQ(targetGeneric.m_string, targerGenericGenerated->m_string);
            delete targerGenericGenerated;
        }

        TEST_F(PatchingTest, CompareIdentical_DataPatchIsEmpty)
        {
            ObjectToPatch sourceObj;
            ObjectToPatch targetObj;

            // Patch without overrides should be empty
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            EXPECT_FALSE(patch.IsData());
        }

        TEST_F(PatchingTest, CompareIdenticalWithForceOverride_DataPatchHasData)
        {
            ObjectToPatch sourceObj;
            ObjectToPatch targetObj;

            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC_CE("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;

            DataPatch::FlagsMap targetFlagsMap;
            targetFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::ForceOverrideSet);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, sourceFlagsMap, targetFlagsMap, m_serializeContext.get());
            EXPECT_TRUE(patch.IsData());
        }

        TEST_F(PatchingTest, ChangeSourceAfterForceOverride_TargetDataUnchanged)
        {
            ObjectToPatch sourceObj;
            ObjectToPatch targetObj;

            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC_CE("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;

            DataPatch::FlagsMap targetFlagsMap;
            targetFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::ForceOverrideSet);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, sourceFlagsMap, targetFlagsMap, m_serializeContext.get());

            // change source after patch is created
            sourceObj.m_intValue = 5;

            AZStd::unique_ptr<ObjectToPatch> targetObj2(patch.Apply(&sourceObj, m_serializeContext.get()));
            EXPECT_EQ(targetObj.m_intValue, targetObj2->m_intValue);
        }

        TEST_F(PatchingTest, ForceOverrideAndPreventOverrideBothSet_DataPatchIsEmpty)
        {
            ObjectToPatch sourceObj;
            ObjectToPatch targetObj;
            targetObj.m_intValue = 43;

            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC_CE("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;
            sourceFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::PreventOverrideSet);

            DataPatch::FlagsMap targetFlagsMap;
            targetFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::ForceOverrideSet);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, sourceFlagsMap, targetFlagsMap, m_serializeContext.get());
            EXPECT_FALSE(patch.IsData());
        }

        TEST_F(PatchingTest, PreventOverrideOnSource_BlocksValueFromPatch)
        {
            // targetObj is different from sourceObj
            ObjectToPatch sourceObj;

            ObjectToPatch targetObj;
            targetObj.m_intValue = 5;

            // create patch from sourceObj -> targetObj
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            // create flags that prevent m_intValue from being patched
            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC_CE("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;
            sourceFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::PreventOverrideSet);

            DataPatch::FlagsMap targetFlagsMap;

            // m_intValue should be the same as it was in sourceObj
            AZStd::unique_ptr<ObjectToPatch> targetObj2(patch.Apply(&sourceObj, m_serializeContext.get(), ObjectStream::FilterDescriptor(), sourceFlagsMap, targetFlagsMap));
            EXPECT_EQ(sourceObj.m_intValue, targetObj2->m_intValue);
        }

        TEST_F(PatchingTest, PreventOverrideOnTarget_DoesntAffectPatching)
        {
            // targetObj is different from sourceObj
            ObjectToPatch sourceObj;

            ObjectToPatch targetObj;
            targetObj.m_intValue = 5;

            // create patch from sourceObj -> targetObj
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            // create flags that prevent m_intValue from being patched, but put them on the target instead of source
            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC_CE("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;

            DataPatch::FlagsMap targetFlagsMap;
            targetFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::PreventOverrideSet);

            // m_intValue should have been patched
            AZStd::unique_ptr<ObjectToPatch> targetObj2(patch.Apply(&sourceObj, m_serializeContext.get(), ObjectStream::FilterDescriptor(), sourceFlagsMap, targetFlagsMap));
            EXPECT_EQ(targetObj.m_intValue, targetObj2->m_intValue);
        }

        TEST_F(PatchingTest, PatchNullptrInSource)
        {
            ObjectWithPointer sourceObj;
            sourceObj.m_int = 7;

            ObjectWithPointer targetObj;
            targetObj.m_int = 8;
            targetObj.m_pointerInt = new AZ::s32(-1);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            ObjectWithPointer* patchedTargetObj = patch.Apply(&sourceObj, m_serializeContext.get());
            EXPECT_EQ(targetObj.m_int, patchedTargetObj->m_int);
            EXPECT_NE(nullptr, patchedTargetObj->m_pointerInt);
            EXPECT_EQ(*targetObj.m_pointerInt, *patchedTargetObj->m_pointerInt);

            delete targetObj.m_pointerInt;
            azdestroy(patchedTargetObj->m_pointerInt);
            delete patchedTargetObj;
        }

        TEST_F(PatchingTest, PatchNullptrInTarget)
        {
            ObjectWithPointer sourceObj;
            sourceObj.m_int = 20;
            sourceObj.m_pointerInt = new AZ::s32(500);

            ObjectWithPointer targetObj;
            targetObj.m_int = 23054;

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            ObjectWithPointer* patchedTargetObj = patch.Apply(&sourceObj, m_serializeContext.get());
            EXPECT_EQ(targetObj.m_int, patchedTargetObj->m_int);
            EXPECT_EQ(nullptr, patchedTargetObj->m_pointerInt);

            delete sourceObj.m_pointerInt;
            delete patchedTargetObj;
        }

        // prove that properly deprecated container elements are removed and do not leave nulls behind.
        TEST_F(PatchingTest, DeprecatedContainerElements_AreRemoved)
        {
            ObjectBaseClass::Reflect(*m_serializeContext);
            ObjectDerivedClass1::Reflect(*m_serializeContext);
            ObjectDerivedClass2::Reflect(*m_serializeContext);
            ObjectWithVectorOfBaseClasses::Reflect(*m_serializeContext);

            // step 1:  Make a patch that includes both classes.
            ObjectWithVectorOfBaseClasses sourceObject;
            ObjectWithVectorOfBaseClasses targetObject;
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass1());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass2());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass2());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass2());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass1()); // <-- we expect to see this second one, it should not be lost
            DataPatch patch;
            patch.Create(&sourceObject, &targetObject, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            // step 2:  DerivedClass2 no longer exists:
            m_serializeContext->EnableRemoveReflection();
            ObjectDerivedClass2::Reflect(*m_serializeContext);
            m_serializeContext->DisableRemoveReflection();
            m_serializeContext->ClassDeprecate("ObjectDerivedClass2", azrtti_typeid<ObjectDerivedClass2>());

            // generate a patch which will turn a given source object into the targetObject.
            ObjectWithVectorOfBaseClasses* patchedTargetObj = patch.Apply(&sourceObject, m_serializeContext.get());
            // at this point, the patched target object should only have ObjectDerivedClass1s on it.
            // two of them exactly.  There should be no other types and there should be no null holes in it.
            EXPECT_EQ(patchedTargetObj->m_vectorOfBaseClasses.size(), 2);
            for (auto element : patchedTargetObj->m_vectorOfBaseClasses)
            {
                EXPECT_EQ(azrtti_typeid(*element), azrtti_typeid<ObjectDerivedClass1>() );
            }
            delete patchedTargetObj;
        }

        // prove that unreadable container elements (ie, no deprecation info) generate warnings but also
        // do not leave nulls behind.
        TEST_F(PatchingTest, UnreadableContainerElements_WithNoDeprecation_GenerateWarning_AreRemoved)
        {
            ObjectBaseClass::Reflect(*m_serializeContext);
            ObjectDerivedClass1::Reflect(*m_serializeContext);
            ObjectDerivedClass2::Reflect(*m_serializeContext);
            ObjectWithVectorOfBaseClasses::Reflect(*m_serializeContext);

            // Make a patch that includes both classes.
            ObjectWithVectorOfBaseClasses sourceObject;
            ObjectWithVectorOfBaseClasses targetObject;
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass1());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass2());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass2());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass2());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass1()); // <-- we expect to see this second one, it should not be lost
            DataPatch patch;
            patch.Create(&sourceObject, &targetObject, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            // Remove DerivedClass2 from the serialize context:
            m_serializeContext->EnableRemoveReflection();
            ObjectDerivedClass2::Reflect(*m_serializeContext);
            m_serializeContext->DisableRemoveReflection();

            // apply the patch despite it containing deprecated things with no deprecation tag, expect 1 error per unknown instance:
            AZ_TEST_START_TRACE_SUPPRESSION;
            ObjectWithVectorOfBaseClasses* patchedTargetObj = patch.Apply(&sourceObject, m_serializeContext.get());
            AZ_TEST_STOP_TRACE_SUPPRESSION(0);

            // at this point, the patched target object should only have ObjectDerivedClass1s on it.
            // two of them exactly.  There should be no other types and there should be no null holes in it.
            EXPECT_EQ(patchedTargetObj->m_vectorOfBaseClasses.size(), 2);
            for (auto element : patchedTargetObj->m_vectorOfBaseClasses)
            {
                EXPECT_EQ(azrtti_typeid(*element), azrtti_typeid<ObjectDerivedClass1>() );
            }
            delete patchedTargetObj;
        }

        // note that the entire conversion subsystem is based on loading through an ObjectStream, not a direct patch.
        // It is not a real use case to deprecate a class during execution and then expect data patch upgrading to function.
        // Instead, deprecated classes always come from data "at rest" such as on disk / network stream, which means
        // they come via ObjectStream, which does perform conversion and has its own tests.
        // This test is just to ensure that when you do load a patch (Using ObjectStream) and elements in that patch have been
        // deprecated, it does not cause unexpected errors.
        TEST_F(PatchingTest, UnreadableContainerElements_WithDeprecationConverters_AreConverted)
        {
            ObjectBaseClass::Reflect(*m_serializeContext);
            ObjectDerivedClass1::Reflect(*m_serializeContext);
            ObjectDerivedClass2::Reflect(*m_serializeContext);
            ObjectDerivedClass3::Reflect(*m_serializeContext);
            ObjectWithVectorOfBaseClasses::Reflect(*m_serializeContext);

            // step 1:  Make a patch that includes deprecated classes.
            ObjectWithVectorOfBaseClasses sourceObject;
            ObjectWithVectorOfBaseClasses targetObject;
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass1());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass2());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass1());
            targetObject.m_vectorOfBaseClasses.push_back(new ObjectDerivedClass2());
            DataPatch patch;
            patch.Create(&sourceObject, &targetObject, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            // save the patch itself to a stream.
            AZStd::vector<char> charBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char> > containerStream(&charBuffer);
            bool success = AZ::Utils::SaveObjectToStream(containerStream, AZ::ObjectStream::ST_XML, &patch, m_serializeContext.get());
            EXPECT_TRUE(success);

            // step 2:  DerivedClass2 no longer exists:
            m_serializeContext->EnableRemoveReflection();
            ObjectDerivedClass2::Reflect(*m_serializeContext);
            m_serializeContext->DisableRemoveReflection();

            m_serializeContext->ClassDeprecate("Dummy UUID", azrtti_typeid<ObjectDerivedClass2>(), ConvertDerivedClass2ToDerivedClass3);

            // load it from the container
            DataPatch loadedPatch;
            // it should generate no warnings but the deprecated ones should not be there.
            success = AZ::Utils::LoadObjectFromBufferInPlace(charBuffer.data(), charBuffer.size(), loadedPatch, m_serializeContext.get());
            EXPECT_TRUE(success);

            // patch the original source object with the new patch which was loaded:
            ObjectWithVectorOfBaseClasses* patchedTargetObj = loadedPatch.Apply(&sourceObject, m_serializeContext.get());

            // prove that all deprecated classes were converted and order did not shuffle:
            ASSERT_EQ(patchedTargetObj->m_vectorOfBaseClasses.size(), 4);
            EXPECT_EQ(azrtti_typeid(patchedTargetObj->m_vectorOfBaseClasses[0]), azrtti_typeid<ObjectDerivedClass1>() );
            EXPECT_EQ(azrtti_typeid(patchedTargetObj->m_vectorOfBaseClasses[1]), azrtti_typeid<ObjectDerivedClass3>() );
            EXPECT_EQ(azrtti_typeid(patchedTargetObj->m_vectorOfBaseClasses[2]), azrtti_typeid<ObjectDerivedClass1>() );
            EXPECT_EQ(azrtti_typeid(patchedTargetObj->m_vectorOfBaseClasses[3]), azrtti_typeid<ObjectDerivedClass3>() );
            delete patchedTargetObj;
        }

        TEST_F(PatchingTest, PatchDistinctNullptrSourceTarget)
        {
            ObjectWithMultiPointers sourceObj;
            sourceObj.m_int = 54;
            sourceObj.m_pointerInt = new AZ::s32(500);

            ObjectWithMultiPointers targetObj;
            targetObj.m_int = -2493;
            targetObj.m_pointerFloat = new float(3.14f);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            ObjectWithMultiPointers* patchedTargetObj = patch.Apply(&sourceObj, m_serializeContext.get());
            EXPECT_EQ(targetObj.m_int, patchedTargetObj->m_int);
            EXPECT_EQ(nullptr, patchedTargetObj->m_pointerInt);
            EXPECT_NE(nullptr, patchedTargetObj->m_pointerFloat);
            EXPECT_EQ(*targetObj.m_pointerFloat, *patchedTargetObj->m_pointerFloat);

            delete sourceObj.m_pointerInt;
            delete targetObj.m_pointerFloat;
            delete patchedTargetObj->m_pointerInt;
            azdestroy(patchedTargetObj->m_pointerFloat);
            delete patchedTargetObj;
        }

        TEST_F(PatchingTest, Apply_LegacyDataPatchWithValidValueOverride_ApplySucceeds_FT)
        {
            // A Legacy DataPatch containing an int set to 150
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="C705B33500000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF6000000960000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the patch and complete conversion
            ObjectToPatch sourceObj;
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Verify the patch applied as expected
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_intValue, 150);
        }

        TEST_F(PatchingTest, Apply_LegacyDataPatchWithValidValueOverride_LegacyPatchUsesObjectStreamVersion1_ApplySucceeds_FT)
        {
            // A Legacy DataPatch containing an int set to 180 and using ObjectStream V1 types for the unordered map, pair, and bytestream.
            // Note: Does not use legacy types in the patch themselves (EX: a patched AZStd::string will use it's V3 typeId not V1)
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="1">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{18456A80-63CC-40C5-BF16-6AF94F9A9ECC}">
                                    <Class name="AZStd::pair" field="element" type="{9F3F5302-3390-407A-A6F7-2E011E3BB686}">
                                            <Class name="AddressType" field="value1" value="C705B33500000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF6000000B40000" type="{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the patch and complete conversion
            ObjectToPatch sourceObj;
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Verify the patch applied as expected
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_intValue, 180);
        }

        TEST_F(PatchingTest, Apply_LegacyDataPatchWithValidPointerOverride_ApplySucceeds_FT)
        {
            // A Legacy DataPatch containing a pointer to an int set to 56
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{D1FD3240-A7C5-4EA3-8E55-CD18193162B8}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="F01997AC00000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF6000000380000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the patch and complete conversion
            ObjectWithPointer sourceObj;
            AZStd::unique_ptr<ObjectWithPointer> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Verify the patch applied as expected
            EXPECT_TRUE(generatedObj);
            EXPECT_TRUE(generatedObj->m_pointerInt);

            EXPECT_EQ(*generatedObj->m_pointerInt, 56);
            azdestroy(generatedObj->m_pointerInt);
        }

        TEST_F(PatchingTest, Apply_LegacyDataPatchWithValidPointerOverride_LegacyPatchUsesObjectStreamVersion1_ApplySucceeds_FT)
        {
            // A Legacy DataPatch containing a pointer to an int set to 74 and using ObjectStream V1 types for the unordered map, pair, and bytestream.
            // Note: Does not use legacy types in the patch themselves (EX: a patched AZStd::string will use it's V3 typeId not V1)
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="1">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{D1FD3240-A7C5-4EA3-8E55-CD18193162B8}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{18456A80-63CC-40C5-BF16-6AF94F9A9ECC}">
                                    <Class name="AZStd::pair" field="element" type="{9F3F5302-3390-407A-A6F7-2E011E3BB686}">
                                            <Class name="AddressType" field="value1" value="F01997AC00000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF60000004A0000" type="{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the patch and complete conversion
            ObjectWithPointer sourceObj;
            AZStd::unique_ptr<ObjectWithPointer> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Verify the patch applied as expected
            EXPECT_TRUE(generatedObj);
            EXPECT_TRUE(generatedObj->m_pointerInt);

            EXPECT_EQ(*generatedObj->m_pointerInt, 74);
            azdestroy(generatedObj->m_pointerInt);
        }

        TEST_F(PatchingTest, Apply_LegacyDataPatchWithValidContainerOverride_ApplySucceeds_FT)
        {
            // A Legacy DataPatch containing a vector of 5 objects with incrementing values and persistent Ids
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000E00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000CC00795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000E000000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000D00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000CB00795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000D000000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000C00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000CA00795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000C000000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000B00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000C900795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000B000000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000A00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000C800795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000A000000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the Patch and complete conversion
            ObjectToPatch sourceObj;
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            constexpr int expectedSize = 5;
            constexpr int persistentIdOffset = 10;
            constexpr int dataOffset = 200;

            // Verify the patch applied as expected for each value in the patched array
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), expectedSize);

            for (int arrayIndex = 0; arrayIndex < expectedSize; ++arrayIndex)
            {
                EXPECT_EQ(generatedObj->m_objectArray[arrayIndex].m_persistentId, arrayIndex + persistentIdOffset);
                EXPECT_EQ(generatedObj->m_objectArray[arrayIndex].m_data, arrayIndex + dataOffset);
            }
        }

        TEST_F(PatchingTest, Apply_LegacyDataPatchWithValidContainerOverride_LegacyPatchUsesObjectStreamVersion1_ApplySucceeds_FT)
        {
            // A Legacy DataPatch containing a vector of 5 objects with incrementing values and persistent Ids
            // Using ObjectStream V1 types for the unordered map, pair, and bytestream.
            // Note: Does not use legacy types in the patch themselves (EX: a patched AZStd::string will use it's V3 typeId not V1)
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="1">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{18456A80-63CC-40C5-BF16-6AF94F9A9ECC}">
                                    <Class name="AZStd::pair" field="element" type="{9F3F5302-3390-407A-A6F7-2E011E3BB686}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000E00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000CC00795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000E000000" type="{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}"/>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{9F3F5302-3390-407A-A6F7-2E011E3BB686}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000D00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000CB00795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000D000000" type="{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}"/>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{9F3F5302-3390-407A-A6F7-2E011E3BB686}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000C00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000CA00795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000C000000" type="{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}"/>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{9F3F5302-3390-407A-A6F7-2E011E3BB686}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000B00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000C900795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000B000000" type="{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}"/>
                                    </Class>
                                    <Class name="AZStd::pair" field="element" type="{9F3F5302-3390-407A-A6F7-2E011E3BB686}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000A00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="000000000308D0C4D19C7EFF4F93A5F095F33FC855AA5C335CC94272039442EB384D42A1ADCB68F7E0EEF6000000C800795F998615D659793347CD4FC8B91163F3E2B0993A08000000000000000A000000" type="{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the patch and complete conversion
            ObjectToPatch sourceObj;
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            constexpr int expectedSize = 5;
            constexpr int persistentIdOffset = 10;
            constexpr int dataOffset = 200;

            // Verify the patch applied as expected for each value in the patched array
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), expectedSize);

            for (int arrayIndex = 0; arrayIndex < expectedSize; ++arrayIndex)
            {
                EXPECT_EQ(generatedObj->m_objectArray[arrayIndex].m_persistentId, arrayIndex + persistentIdOffset);
                EXPECT_EQ(generatedObj->m_objectArray[arrayIndex].m_data, arrayIndex + dataOffset);
            }
        }

        TEST_F(PatchingTest, Apply_LegacyDataPatchWithValidGenericTypeOverride_ApplySucceeds_FT)
        {
            // A Legacy DataPatch containing a string set to "Hello World"
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{DE1EE15F-3458-40AE-A206-C6C957E2432B}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="57E02DD400000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000033903AAAB3F5C475A669EBCD5FA4DB353C90B48656C6C6F20576F726C640000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the patch and complete conversion
            ObjectsWithGenerics sourceObj;
            AZStd::unique_ptr<ObjectsWithGenerics> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            constexpr const char* expectedString = "Hello World";

            // Verify the patch applied as expected
            EXPECT_TRUE(generatedObj);
            EXPECT_STREQ(generatedObj->m_string.c_str(), expectedString);
        }

        TEST_F(PatchingTest, Apply_LegacyDataPatchWithValidGenericTypeOverride_LegacyPatchUsesObjectStreamVersion1_ApplySucceeds_FT)
        {
            // A Legacy DataPatch containing a string set to "Hello World" and using ObjectStream V1 types for the unordered map, pair, and bytestream.
            // Note: Does not use legacy types in the patch themselves (EX: a patched AZStd::string will use it's V3 typeId not V1)
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="1">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{DE1EE15F-3458-40AE-A206-C6C957E2432B}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{18456A80-63CC-40C5-BF16-6AF94F9A9ECC}">
                                    <Class name="AZStd::pair" field="element" type="{9F3F5302-3390-407A-A6F7-2E011E3BB686}">
                                            <Class name="AddressType" field="value1" value="57E02DD400000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000033903AAAB3F5C475A669EBCD5FA4DB353C90B48656C6C6F20576F726C640000" type="{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the patch and complete conversion
            ObjectsWithGenerics sourceObj;
            AZStd::unique_ptr<ObjectsWithGenerics> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            const char* expectedString = "Hello World";

            // Verify the patch applied as expected
            EXPECT_TRUE(generatedObj);
            EXPECT_STREQ(generatedObj->m_string.c_str(), expectedString);
        }

        TEST_F(PatchingTest, Apply_LegacyDataPatchAppliedTwice_OnSecondApplyPatchHasBeenConverted_BothPatchAppliesSucceed_FT)
        {
            // A dataPatch containing an int set to 22
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="C705B33500000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF6000000160000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            constexpr int expectedValue = 22;

            // Load the patch from stream
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the patch
            ObjectToPatch sourceObj;
            AZStd::unique_ptr<ObjectToPatch> generatedObjFirstApply(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Verify patch applied as expected
            EXPECT_TRUE(generatedObjFirstApply);
            EXPECT_EQ(generatedObjFirstApply->m_intValue, expectedValue);

            // Apply the patch again
            AZStd::unique_ptr<ObjectToPatch> generatedObjSecondApply(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Verify patch applied successfully the second time
            EXPECT_TRUE(generatedObjSecondApply);
            EXPECT_EQ(generatedObjSecondApply->m_intValue, expectedValue);
        }

        TEST_F(PatchingTest, Apply_PatchWrittenToThenReadFromStreamBeforeApply_PatchApplySucceeds_FT)
        {
            ObjectToPatch source;
            ObjectToPatch target;

            constexpr int targetArraySize = 999;
            constexpr int targetValueScalar = 2;
            constexpr int persistentIdOffset = 100;

            // Build target array
            target.m_objectArray.resize(targetArraySize);
            for (size_t arrayIndex = 0; arrayIndex < target.m_objectArray.size(); ++arrayIndex)
            {
                target.m_objectArray[arrayIndex].m_data = static_cast<int>(arrayIndex * targetValueScalar);
                target.m_objectArray[arrayIndex].m_persistentId = static_cast<int>((arrayIndex * targetValueScalar) + persistentIdOffset);
            }

            // Create patch in memory
            DataPatch patch;
            patch.Create(&source, &target, AZ::DataPatch::FlagsMap(), AZ::DataPatch::FlagsMap(), m_serializeContext.get());

            // Serialize patch into stream
            AZStd::vector<AZ::u8> streamBuffer;
            WritePatchToByteStream(patch, streamBuffer);

            // Load patch from stream
            DataPatch loadedPatch;
            LoadPatchFromByteStream(streamBuffer, loadedPatch);

            // Verify integrity of loaded patch
            EXPECT_TRUE(loadedPatch.IsValid() && loadedPatch.IsData());

            // Apply the patch
            AZStd::unique_ptr<ObjectToPatch> generatedObj(loadedPatch.Apply(&source, m_serializeContext.get()));

            // Verify patch applied as expected
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetArraySize);

            for (int arrayIndex = 0; arrayIndex < targetArraySize; ++arrayIndex)
            {
                EXPECT_EQ(generatedObj->m_objectArray[arrayIndex].m_data, arrayIndex * targetValueScalar);
                EXPECT_EQ(generatedObj->m_objectArray[arrayIndex].m_persistentId, (arrayIndex * targetValueScalar) + persistentIdOffset);
            }
        }

        TEST_F(PatchingTest, Apply_LegacyPatchWrittenToThenReadFromStreamBeforeApply_PatchApplySucceeds_FT)
        {
            // A Legacy DataPatch containing an int set to 57
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="C705B33500000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF6000000390000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from stream
            // Loading the legacy patch will run the converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Serialize partially converted patch to stream
            AZStd::vector<AZ::u8> streamBuffer;
            WritePatchToByteStream(patch, streamBuffer);

            // Load partially converted patch from stream
            DataPatch loadedPatch;
            LoadPatchFromByteStream(streamBuffer, loadedPatch);

            // Verify integrity of loaded patch
            EXPECT_TRUE(loadedPatch.IsValid() && loadedPatch.IsData());

            // Apply the patch
            ObjectToPatch source;
            AZStd::unique_ptr<ObjectToPatch> generatedObj(loadedPatch.Apply(&source, m_serializeContext.get()));

            // Verify the patch applied as expected
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_intValue, 57);
        }

        TEST_F(PatchingTest, Apply_LegacyPatchAppliedTwice_AppliedAndWrittenToStream_LoadedFromStreamAndApplied_PatchApplySucceeds_FT)
        {
            // A Legacy DataPatch containing an int set to 92
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="C705B33500000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF60000005C0000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            constexpr int expectedValue = 92;

            // Load the patch from stream
            // Loading the legacy patch will run the converter
            // Patch Data will be wrapped in the StreamWrapper type until Apply is called
            // Apply provides the remaining class data to complete the conversion
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply provides the remaining class data to complete the conversion
            ObjectToPatch source;
            AZStd::unique_ptr<ObjectToPatch> generatedObjFirstApply(patch.Apply(&source, m_serializeContext.get()));

            EXPECT_TRUE(generatedObjFirstApply);
            EXPECT_EQ(generatedObjFirstApply->m_intValue, expectedValue);

            // Serialize fully converted patch to stream
            AZStd::vector<AZ::u8> streamBuffer;
            WritePatchToByteStream(patch, streamBuffer);

            // Load fully converted patch from stream
            DataPatch loadedPatch;
            LoadPatchFromByteStream(streamBuffer, loadedPatch);

            // Verify integrity of loaded patch
            EXPECT_TRUE(loadedPatch.IsValid() && loadedPatch.IsData());

            // Apply the patch
            AZStd::unique_ptr<ObjectToPatch> generatedObjSecondApply(loadedPatch.Apply(&source, m_serializeContext.get()));

            // Verify the patch applied as expected
            EXPECT_TRUE(generatedObjSecondApply);
            EXPECT_EQ(generatedObjSecondApply->m_intValue, expectedValue);
        }

        TEST_F(PatchingTest, LegacyDataPatchConverter_LegacyPatchXMLMissingTargetClassId_ConverterThrowsError_FT)
        {
            // A Legacy DataPatch containing an int set to 178 but missing its TargetClassId
            // This should fail conversion
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="C705B33500000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF6000000B20000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            DataPatch patch;

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // Verify the expected number of Errors/Asserts occur
            // Expected errors: Failed to get data from m_targetClassId field during conversion, found in LegacyDataPatchConverter (DataPatch.cpp)
            //                  Converter failed error found in ObjectStreamImpl::LoadClass (ObjectStream.cpp)
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(legacyPatchXML, patch);
            AZ_TEST_STOP_ASSERTTEST(2);
        }

        TEST_F(PatchingTest, LegacyDataPatchConverter_LegacyPatchXMLMissingAddressType_ConverterThrowsError_FT)
        {
            // A Legacy DataPatch containing an int set to 154 but missing its AddressType
            // This should fail conversion
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF60000009A0000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            DataPatch patch;

            // Load the patch from XML
            // This triggers the Legacy DataPatch Converter
            // Verify the expected number of Errors/Asserts occur on conversion
            // Expected errors: Failed to find both first and second values in pair during conversion, found in ConvertByteStreamMapToAnyMap (DataPatch.cpp)
            //                  Converter failed error found in ObjectStreamImpl::LoadClass (ObjectStream.cpp)
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(legacyPatchXML, patch);
            AZ_TEST_STOP_ASSERTTEST(2);
        }

        TEST_F(PatchingTest, LegacyDataPatchConverter_LegacyPatchXMLMissingByteStream_ConverterThrowsError_FT)
        {
            // A Legacy DataPatch missing its ByteStream data and is expected to fail conversion
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="C705B33500000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            DataPatch patch;

            // Load the patch from XML
            // This triggers the Legacy DataPatch Converter
            // Verify the expected nummber of Errors/Asserts occur on conversion
            // Expected errors: Failed to find both first and second values in pair during conversion, found in ConvertByteStreamMapToAnyMap (DataPatch.cpp)
            //                  Converter failed error found in ObjectStreamImpl::LoadClass (ObjectStream.cpp)
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(legacyPatchXML, patch);
            AZ_TEST_STOP_ASSERTTEST(2);
        }

        TEST_F(PatchingTest, LegacyDataPatchConverter_LegacyPatchXMLMissingAddressTypeAndByteStream_ConverterThrowsError_FT)
        {
            // A Legacy DataPatch missing both its AddressType and ByteStream and is expected to fail conversion
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            DataPatch patch;

            // Load the patch from XML
            // This triggers the Legacy DataPatch Converter
            // Verify the expected number of Errors/Asserts occur
            // Expected errors: Failed to find both first and second values in pair during conversion, found in ConvertByteStreamMapToAnyMap (DataPatch.cpp)
            //                  Converter failed error found in ObjectStreamImpl::LoadClass (ObjectStream.cpp)
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(legacyPatchXML, patch);
            AZ_TEST_STOP_ASSERTTEST(2);
        }

        TEST_F(PatchingTest, Apply_LegacyPatchXMLHasInvalidByteStream_ConverterThrowsError_FT)
        {
            // A Legacy DataPatch expecting to hold an int but containing an invalid bytestream and is expected to fail conversion
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="C705B33500000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00FFFFFFFF" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // The invalid bytestream will be stored in a StreamWrapper until Apply
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            ObjectToPatch source;

            // Apply the patch and complete conversion
            // The stored StreamWrapper will attempt to load and fail
            // Verify the expected number of Errors/Asserts occur
            // Expected errors: Stream is a newer version than object stream supports, found in ObjectStreamImpl::Start (ObjectStream.cpp)
            //                  Failed to load StreamWrapper during DataPatch Apply, found in DataNodeTree::ApplyToElements (DataPatch.cpp)
            AZ_TEST_START_ASSERTTEST;
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&source, m_serializeContext.get()));
            AZ_TEST_STOP_ASSERTTEST(2);
        }

        TEST_F(PatchingTest, Apply_LegacyPatchXMLHasIncorrectAddressType_ApplyFails_FT)
        {
            // A Legacy DataPatch with an int set to 39 but with an incorrect AddressType
            // AddressType to location for an int replaced with AddressType for value in an array structure
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{47E5CF10-3FA1-4064-BE7A-70E3143B4025}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="8C2AFF02000000000A00000000000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF6000000270000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            // The incorrect AddressType will be stored to direct patching for the valid ByteStream
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            ObjectToPatch source;
            source.m_intValue = 0;

            // Apply the patch, conversion will not complete during this stage
            // Since AddressType is invalid, the underlying data will not be requested during apply and will not be fully converted
            AZ_TEST_START_TRACE_SUPPRESSION;
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&source, m_serializeContext.get()));
            AZ_TEST_STOP_TRACE_SUPPRESSION(0);
            // We expect the value 39 to not be patched during apply and m_intValue to remain at 0
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_intValue, 0);
        }

        TEST_F(PatchingTest, Apply_LegacyPatchXMLHasIncorrectTargetClassId_ApplyFailsAndReturnsNull_FT)
        {
            // A Legacy DataPatch containing an int set to 203
            // TargetClassId set to DataPatch type Id which is incorrect for the type being contained
            AZStd::string_view legacyPatchXML = R"(<ObjectStream version="3">
                    <Class name="DataPatch" type="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}">
                            <Class name="AZ::Uuid" field="m_targetClassId" value="{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                            <Class name="AZStd::unordered_map" field="m_patch" type="{CF3B3C65-49C0-5199-B671-E75347EB25C2}">
                                    <Class name="AZStd::pair" field="element" type="{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}">
                                            <Class name="AddressType" field="value1" value="C705B33500000000" type="{90752F2D-CBD3-4EE9-9CDD-447E797C8408}"/>
                                            <Class name="ByteStream" field="value2" value="00000000031C72039442EB384D42A1ADCB68F7E0EEF6000000CB0000" type="{ADFD596B-7177-5519-9752-BC418FE42963}"/>
                                    </Class>
                            </Class>
                    </Class>
            </ObjectStream>
            )";

            // Load the patch from XML
            // This triggers the Legacy DataPatch converter
            DataPatch patch;
            LoadPatchFromXML(legacyPatchXML, patch);

            // Apply the patch, conversion will not complete during this stage
            // Since targetClassId does not match supplied source type Apply is expected to return nullptr
            ObjectToPatch source;
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&source, m_serializeContext.get()));

            // Verify Apply returned a nullptr
            EXPECT_FALSE(generatedObj);
        }
        TEST_F(PatchingTest, AddressTypeSerializerLoad_AddressTypeIsValid_AddressHasOnlyClassData_LoadSucceeds_FT)
        {
            const int expectedValue = 52;
            AZStd::string patchXML = BuildXMLDataPatchV1AddressTypeIntXML(nullptr, expectedValue);

            DataPatch patch;
            ObjectToPatch source;

            // Verify address deserializes with no errors
            AZ_TEST_START_TRACE_SUPPRESSION;
            LoadPatchFromXML(patchXML, patch);
            AZ_TEST_STOP_TRACE_SUPPRESSION(0);

            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&source, m_serializeContext.get()));

            // Verify addressed field m_int was patched correctly (verifies integrity of address)
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_intValue, expectedValue);
        }

        TEST_F(PatchingTest, AddressTypeSerializerLoad_AddressTypeIsValid_AddressHasClassAndIndexData_LoadSucceeds_FT)
        {
            const size_t expectedContainerSize = 5;
            const size_t persistentIdOffset = 10;
            const size_t dataOffset = 0;

            AZStd::string patchXML = BuildXMLDataPatchV1AddressTypeIntVectorXML(nullptr, dataOffset, persistentIdOffset);

            DataPatch patch;
            ObjectToPatch source;

            // Verify address deserializes with no errors
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(patchXML, patch);
            AZ_TEST_STOP_ASSERTTEST(0);

            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&source, m_serializeContext.get()));

            // Verify integrity of patched object
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), expectedContainerSize);

            for (size_t arrayIndex = 0; arrayIndex < expectedContainerSize; ++arrayIndex)
            {
                EXPECT_EQ(generatedObj->m_objectArray[arrayIndex].m_data, arrayIndex);
                EXPECT_EQ(generatedObj->m_objectArray[arrayIndex].m_persistentId, arrayIndex + persistentIdOffset);
            }
        }

        TEST_F(PatchingTest, AddressTypeSerializerLoad_AddressTypeIsInvalid_InvalidElementsInPath_LoadFails_FT)
        {
            AZStd::string pathWithInvalidElements = GetValidAddressForXMLDataPatchV1AddressTypeIntXML();
            pathWithInvalidElements += "not/a/valid/path";

            AZStd::string patchXML = BuildXMLDataPatchV1AddressTypeIntXML(pathWithInvalidElements.c_str(), 0);

            DataPatch patch;

            // Load the patch from XML
            // This triggers AddressTypeSerializer::Load
            // Expected error: AddressType failed to load due to invalid element in path
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(patchXML, patch);
            AZ_TEST_STOP_ASSERTTEST(2);
        }

        TEST_F(PatchingTest, AddressTypeSerializerLoad_AddressTypeIsInvalid_MissingPathDelimiter_LoadFails_FT)
        {
            // Build a path from a valid path minus the trailing "/" delimiter
            AZStd::string validPath = GetValidAddressForXMLDataPatchV1AddressTypeIntXML();
            AZStd::string validPathMissingDelimiter(validPath.c_str(), strlen(validPath.c_str()) - 1);

            AZStd::string patchXML = BuildXMLDataPatchV1AddressTypeIntXML(validPathMissingDelimiter.c_str(), 0);

            DataPatch patch;

            // Load the patch from XML
            // This triggers AddressTypeSerializer::Load
            // Expected error: AddressType failed to load due to path not containing valid delimiter "/"
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(patchXML, patch);
            AZ_TEST_STOP_ASSERTTEST(2);
        }

        TEST_F(PatchingTest, Apply_AddressTypeIsInvalid_SingleEntryInPatch_ApplyFails_FT)
        {
            AZStd::string patchXML = BuildXMLDataPatchV1AddressTypeIntXML("not/a/valid/path", 0);

            DataPatch patch;
            ObjectToPatch source;
            // Load the patch from XML
            // This triggers AddressTypeSerializer::Load
            // Expected errors: AddressType failed to load due to invalid element in path (DataPatch.cpp)
            //                  Apply fails due to patch containing Invalid address during Apply (DataPatch.cpp)
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(patchXML, patch);
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&source, m_serializeContext.get()));
            AZ_TEST_STOP_ASSERTTEST(3);
        }

        TEST_F(PatchingTest, AddressTypeSerializerLoad_AddressTypeIsEmpty_SingleEntryInPatch_ApplySucceeds_FT)
        {
            // An empty address on a single entry patch denotes that the root element is being patched
            // Validate that we succesfully load an emtpy address
            AZStd::string patchXML = BuildXMLDataPatchV1AddressTypeIntXML("", 0);

            DataPatch patch;
            ObjectToPatch source;

            // Verify address deserializes with no errors
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(patchXML, patch);
            AZ_TEST_STOP_ASSERTTEST(0);
        }

        TEST_F(PatchingTest, Apply_AddressTypeIsInvalid_MultipleEntriesInPatch_ApplyFails_FT)
        {
            AZStd::string patchXML = BuildXMLDataPatchV1AddressTypeIntVectorXML("not/a/valid/path", 0, 0);

            DataPatch patch;
            ObjectToPatch source;
            // Load the patch from XML
            // This triggers AddressTypeSerializer::Load
            // Expected errors: AddressType failed to load due to invalid element in path (DataPatch.cpp)
            //                  Apply fails due to empty AddressType (DataPatch.cpp)
            AZ_TEST_START_ASSERTTEST;
            LoadPatchFromXML(patchXML, patch);
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&source, m_serializeContext.get()));
            AZ_TEST_STOP_ASSERTTEST(3);
        }

        TEST_F(PatchingTest, AddressTypeElementLoad_PathElementHasValidPathForClassType_LoadIsSuccessful_FT)
        {
            const char* expectedTypeId = "{D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA}";
            const char* expectedAddressElement = "ClassA";
            const int expectedVersion = 5000;

            DataPatch::AddressTypeElement addressTypeElement =
                m_addressTypeSerializer->LoadAddressElementFromPath(AZStd::string::format("somecharacters(%s)::%s%s%i/",
                expectedTypeId,
                expectedAddressElement,
                V1AddressTypeElementVersionDelimiter,
                expectedVersion));

            EXPECT_TRUE(addressTypeElement.IsValid());

            EXPECT_EQ(addressTypeElement.GetElementTypeId(), AZ::Uuid(expectedTypeId));
            EXPECT_EQ(addressTypeElement.GetAddressElement(), AZ_CRC(expectedAddressElement));
            EXPECT_EQ(addressTypeElement.GetElementVersion(), expectedVersion);
        }

        TEST_F(PatchingTest, AddressTypeElementLoad_PathElementHasValidPathForIndexType_LoadIsSuccessful_FT)
        {
            const char* expectedTypeId = "{07DEDB71-0585-5BE6-83FF-1C9029B9E5DB}";
            const int expectedAddressElement = 4321;
            const int expectedVersion = 2222;

            DataPatch::AddressTypeElement addressTypeElement =
                m_addressTypeSerializer->LoadAddressElementFromPath(AZStd::string::format("somecharacters(%s)#%i%s%i/",
                expectedTypeId,
                expectedAddressElement,
                V1AddressTypeElementVersionDelimiter,
                expectedVersion));

            EXPECT_TRUE(addressTypeElement.IsValid());

            EXPECT_EQ(addressTypeElement.GetAddressElement(), expectedAddressElement);
            EXPECT_EQ(addressTypeElement.GetElementTypeId(), AZ::Uuid(expectedTypeId));
            EXPECT_EQ(addressTypeElement.GetElementVersion(), expectedVersion);
        }

        TEST_F(PatchingTest, AddressTypeElementLoad_PathElementHasValidPathForNoneType_LoadIsSuccessful_FT)
        {
            const int expectedAddressElement = 9999;

            DataPatch::AddressTypeElement addressTypeElement =
                m_addressTypeSerializer->LoadAddressElementFromPath(AZStd::string::format("%i/", expectedAddressElement));

            EXPECT_TRUE(addressTypeElement.IsValid());

            EXPECT_EQ(addressTypeElement.GetAddressElement(), expectedAddressElement);
            EXPECT_EQ(addressTypeElement.GetElementTypeId(), AZ::Uuid::CreateNull());
            EXPECT_EQ(addressTypeElement.GetElementVersion(), std::numeric_limits<AZ::u32>::max());
        }

        TEST_F(PatchingTest, AddressTypeElementLoad_PathElementHasInvalidTypeId_LoadFails_FT)
        {
            AZStd::string pathElementWithInvalidTypeId = AZStd::string::format("somecharacters(invalidTypeId)::classB%s5678%s",
                                                                               V1AddressTypeElementVersionDelimiter,
                                                                               V1AddressTypeElementPathDelimiter);

            DataPatch::AddressTypeElement addressTypeElement = m_addressTypeSerializer->LoadAddressElementFromPath(pathElementWithInvalidTypeId);

            EXPECT_FALSE(addressTypeElement.IsValid());
            EXPECT_EQ(addressTypeElement.GetElementTypeId(), AZ::Uuid::CreateNull());
        }

        TEST_F(PatchingTest, AddressTypeElementLoad_TypeIdMissingParentheses_LoadFails_FT)
        {
            AZStd::string pathMissingParentheses = AZStd::string::format("somecharacters{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}::classE%s3000%s",
                                                                         V1AddressTypeElementVersionDelimiter,
                                                                         V1AddressTypeElementPathDelimiter);

            DataPatch::AddressTypeElement addressTypeElement = m_addressTypeSerializer->LoadAddressElementFromPath(pathMissingParentheses);

            EXPECT_FALSE(addressTypeElement.IsValid());
        }

        TEST_F(PatchingTest, AddressTypeElementLoad_TypeIdMissingCurlyBraces_LoadFails_FT)
        {
            AZStd::string pathMissingCurlyBraces = AZStd::string::format("somecharacters(07DEDB71-0585-5BE6-83FF-1C9029B9E5DB)::classF%s9876%s",
                                                                         V1AddressTypeElementVersionDelimiter,
                                                                         V1AddressTypeElementPathDelimiter);

            DataPatch::AddressTypeElement addressTypeElement = m_addressTypeSerializer->LoadAddressElementFromPath(pathMissingCurlyBraces);

            EXPECT_FALSE(addressTypeElement.IsValid());
        }

        TEST_F(PatchingTest, AddressTypeElementLoad_ClassTypePathElementMissingColons_LoadFails_FT)
        {
            AZStd::string pathElementMissingColons = AZStd::string::format("somecharacters({861A12B0-BD91-528E-9CEC-505246EE98DE})classC%s5432%s",
                                                                           V1AddressTypeElementVersionDelimiter,
                                                                           V1AddressTypeElementPathDelimiter);

            DataPatch::AddressTypeElement addressTypeElement = m_addressTypeSerializer->LoadAddressElementFromPath(pathElementMissingColons);

            EXPECT_FALSE(addressTypeElement.IsValid());
        }

        TEST_F(PatchingTest, AddressTypeElementLoad_IndexTypePathElementMissingPound_LoadFails_FT)
        {
            AZStd::string pathElementMissingPound = AZStd::string::format("somecharacters({ADFD596B-7177-5519-9752-BC418FE42963})91011%s1122%s",
                                                                          V1AddressTypeElementVersionDelimiter,
                                                                          V1AddressTypeElementPathDelimiter);

            DataPatch::AddressTypeElement addressTypeElement = m_addressTypeSerializer->LoadAddressElementFromPath(pathElementMissingPound);

            EXPECT_FALSE(addressTypeElement.IsValid());
        }

        TEST_F(PatchingTest, AddressTypeElementLoad_PathElementMissingDotBeforeVersion_LoadFails_FT)
        {
            AZStd::string pathMissingDot = AZStd::string::format("somecharacters({07DEDB71-0585-5BE6-83FF-1C9029B9E5DB})::classD4000%s",
                                                                 V1AddressTypeElementPathDelimiter);

            DataPatch::AddressTypeElement addressTypeElement = m_addressTypeSerializer->LoadAddressElementFromPath(pathMissingDot);

            EXPECT_FALSE(addressTypeElement.IsValid());
        }

        TEST_F(PatchingTest, DataPatchFieldConverterForVersion1Patch_DoesNotRunVersion0To1Converter_Succeeds)
        {
            ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());

            const ObjectWithNumericFieldV1 initialObject;
            ObjectWithNumericFieldV1 testObject;
            testObject.m_value = 3946393;

            DataPatch testPatch;
            testPatch.Create(&initialObject, &testObject, {}, {}, m_serializeContext.get());
            // Unreflect ObjectWithNumericFieldV1 and reflect ObjectWithNumericFieldV2
            {
                m_serializeContext->EnableRemoveReflection();
                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                m_serializeContext->DisableRemoveReflection();
                ObjectWithNumericFieldV2::Reflect(m_serializeContext.get());
            }

            ObjectWithNumericFieldV2 initialObjectV2;
            ObjectWithNumericFieldV2* patchedObject = testPatch.Apply(&initialObjectV2, m_serializeContext.get());
            ASSERT_NE(nullptr, patchedObject);
            EXPECT_DOUBLE_EQ(32.0, patchedObject->m_value);

            // Clean up ObjectWithNumericFieldV2 patch data;
            delete patchedObject;

            // Unreflect remaining reflected classes
            m_serializeContext->EnableRemoveReflection();
            ObjectWithNumericFieldV2::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
        }

        TEST_F(PatchingTest, ObjectFieldConverter_CreateDataPatchInMemoryCanBeAppliedSuccessfully)
        {
            ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
            InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
            InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
            ObjectFieldConverterV1::Reflect(m_serializeContext.get());

            const ObjectFieldConverterV1 initialObject;
            ObjectFieldConverterV1 testObject;
            // Change the defaults of elements on the ObjectFieldConverterV1 object
            testObject.m_rootStringField = "Test1";
            testObject.m_rootStringVector.emplace_back("Test2");
            testObject.m_rootInnerObject.m_stringField = "InnerTest1";
            testObject.m_rootInnerObject.m_stringVector.emplace_back("InnerTest2");

            auto derivedInnerObject = aznew InnerObjectFieldConverterDerivedV1;
            derivedInnerObject->m_stringField = "DerivedTest1";
            derivedInnerObject->m_stringVector.emplace_back("DerivedTest2");
            derivedInnerObject->m_objectWithNumericField.m_value = 1;
            testObject.m_baseInnerObjectPolymorphic = derivedInnerObject;

            DataPatch testPatch;
            testPatch.Create(&initialObject, &testObject, {}, {}, m_serializeContext.get());
            ObjectFieldConverterV1* patchedObject = testPatch.Apply(&initialObject, m_serializeContext.get());
            EXPECT_EQ(testObject.m_rootStringField, patchedObject->m_rootStringField);
            EXPECT_EQ(testObject.m_rootStringVector, patchedObject->m_rootStringVector);
            InnerObjectFieldConverterV1& testInnerObject = testObject.m_rootInnerObject;
            InnerObjectFieldConverterV1& patchedInnerObject = patchedObject->m_rootInnerObject;
            EXPECT_EQ(testInnerObject.m_stringField, patchedInnerObject.m_stringField);
            EXPECT_EQ(testInnerObject.m_stringVector, patchedInnerObject.m_stringVector);

            auto patchedInnerObjectDerived = azrtti_cast<InnerObjectFieldConverterDerivedV1*>(patchedObject->m_baseInnerObjectPolymorphic);
            ASSERT_NE(nullptr, patchedInnerObjectDerived);
            EXPECT_EQ(derivedInnerObject->m_stringField, patchedInnerObjectDerived->m_stringField);
            EXPECT_EQ(derivedInnerObject->m_stringVector, patchedInnerObjectDerived->m_stringVector);
            EXPECT_EQ(derivedInnerObject->m_objectWithNumericField.m_value, patchedInnerObjectDerived->m_objectWithNumericField.m_value);


            // Clean up original ObjectFieldConverterV1 object
            delete testObject.m_baseInnerObjectPolymorphic;
            // Clean up patched ObjectFieldConverterV1 object
            delete patchedObject->m_baseInnerObjectPolymorphic;
            delete patchedObject;

            m_serializeContext->EnableRemoveReflection();
            ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
            InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
            InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
            ObjectFieldConverterV1::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
        }

        inline namespace NestedMemberFieldChangeConverter
        {
            class InnerObjectFieldConverterV2
            {
            public:
                AZ_CLASS_ALLOCATOR(InnerObjectFieldConverterV2, SystemAllocator);
                AZ_RTTI(InnerObjectFieldConverterV2, "{28E61B17-F321-4D4E-9F4C-00846C6631DE}");
                virtual ~InnerObjectFieldConverterV2() = default;

                static int64_t StringToInt64(const AZStd::string& value)
                {
                    return static_cast<int64_t>(value.size());
                }

                static void Reflect(AZ::ReflectContext* reflectContext)
                {
                    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                    {
                        serializeContext->Class<InnerObjectFieldConverterV2>()
                            // A class version converter is needed to load an InnerObjectFieldConverterV2 when it is stored directly in patch.
                            // This occurs when patched element is a pointer to a class. In that case the entire class is serialized out
                            // Therefore when the DataPatch is loaded, the patch Data will load it's AZStd::any, which goes through the normal
                            // serialization flow, so if the class stored in the AZStd::any is an old version it needs to run through a Version Converter
                            ->Version(2, &VersionConverter)
                            ->Field("InnerBaseIntField", &InnerObjectFieldConverterV2::m_int64Field)
                            ->TypeChange("InnerBaseStringField", 1, 2, AZStd::function<int64_t(const AZStd::string&)>(&StringToInt64))
                            ->NameChange(1, 2, "InnerBaseStringField", "InnerBaseIntField")
                            ->Field("InnerBaseStringVector", &InnerObjectFieldConverterV2::m_stringVector)
                            ;
                    }
                }

                static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
                {
                    if (rootElement.GetVersion() < 2)
                    {
                        AZStd::string stringField;
                        if (!rootElement.GetChildData(AZ_CRC_CE("InnerBaseStringField"), stringField))
                        {
                            AZ_Error("PatchingTest", false, "Unable to retrieve 'InnerBaseStringField' data for %u version of the InnerObjectFieldConverterClass",
                                rootElement.GetVersion());
                            return false;
                        }

                        rootElement.RemoveElementByName(AZ_CRC_CE("InnerBaseStringField"));
                        rootElement.AddElementWithData(context, "InnerBaseIntField", static_cast<int64_t>(stringField.size()));
                    }

                    return true;
                }

                int64_t m_int64Field;
                AZStd::vector<AZStd::string> m_stringVector;
            };

            //! InnerObjectFieldConverterDerivedV2 is exactly the same as InnerObjectFieldConverterDerivedV1
            //! It is just needed to state that InnerObjectFieldConverterV2 is a base class
            using InnerObjectFieldConverterDerivedV1WithV2Base = InnerObjectFieldConverterDerivedV1Template<InnerObjectFieldConverterV2>;

            //! ObjectFieldConverterV1WithMemberVersionChange is the same has the ObjectFieldConverterV1 class, it just substitutes out
            //! the InnerObjectFieldConverterV1 with InnerObjectFieldConverterV2 that has the same typeid, but newer version
            class ObjectFieldConverterV1WithMemberVersionChange
            {
                using ClassType = ObjectFieldConverterV1WithMemberVersionChange;
            public:
                AZ_CLASS_ALLOCATOR(ClassType, SystemAllocator);
                AZ_TYPE_INFO(ClassType, "{5722C4E4-25DE-48C5-BC89-0EE9D38DF433}");


                static void Reflect(AZ::ReflectContext* reflectContext)
                {
                    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                    {
                        serializeContext->Class<ClassType>()
                            ->Version(1)
                            ->Field("RootStringField", &ClassType::m_rootStringField)
                            ->Field("RootStringVector", &ClassType::m_rootStringVector)
                            ->Field("RootInnerObjectValue", &ClassType::m_rootInnerObject)
                            ->Field("RootInnerObjectPointer", &ClassType::m_baseInnerObjectPolymorphic)
                            ;
                    }
                }

                //! AZStd::string uses IDataSerializer for Serialization.
                //! This is to test Field Converters for patched element that are directly on the patched class
                AZStd::string m_rootStringField;
                //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                AZStd::vector<AZStd::string> m_rootStringVector;
                //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                InnerObjectFieldConverterV2 m_rootInnerObject{};
                InnerObjectFieldConverterV2* m_baseInnerObjectPolymorphic{};
            };

            TEST_F(PatchingTest, ObjectFieldConverter_ChangeInnerFieldVersion_FieldConverterRunsSuccessfully)
            {
                using OriginalObjectField = ObjectFieldConverterV1;
                using PatchedObjectField = ObjectFieldConverterV1WithMemberVersionChange;

                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                OriginalObjectField::Reflect(m_serializeContext.get());

                OriginalObjectField testObject;
                // Change the defaults of the InnerObjectFieldConverterV2 member and InnerObjectFieldConverterV2 pointer member
                testObject.m_rootInnerObject.m_stringField = "InnerTest1";

                auto derivedInnerObject = aznew InnerObjectFieldConverterDerivedV1;
                derivedInnerObject->m_stringField = "DerivedTest1";
                derivedInnerObject->m_objectWithNumericField.m_value = 10;
                testObject.m_baseInnerObjectPolymorphic = derivedInnerObject;

                AZStd::vector<uint8_t> byteBuffer;
                // Create DataPatch using ObjectFieldConverterV1
                {
                    DataPatch testPatch;
                    const OriginalObjectField initialObjectV1;
                    testPatch.Create(&initialObjectV1, &testObject, {}, {}, m_serializeContext.get());

                    // Write DataPatch to ByteStream before unreflected Version 1 of the InnerObjectFieldConverterV1 class
                    AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
                    WritePatchToByteStream(testPatch, byteBuffer);
                }
                // Now unreflect the ObjectFieldConverterV1, InnerObjectFieldConverterDerivedV1 and InnerObjectFieldConverterDerivedV1
                // and reflect ObjectFieldConverterV1WithMemberVersionChange, InnerObjectFieldConverterV2 and InnerObjectFieldConverterDerivedV1WithV2Base
                {
                    m_serializeContext->EnableRemoveReflection();
                    InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                    InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                    OriginalObjectField::Reflect(m_serializeContext.get());
                    m_serializeContext->DisableRemoveReflection();

                    InnerObjectFieldConverterV2::Reflect(m_serializeContext.get());
                    InnerObjectFieldConverterDerivedV1WithV2Base::Reflect(m_serializeContext.get());
                    PatchedObjectField::Reflect(m_serializeContext.get());
                }

                // Read DataPatch from ByteStream after reflecting Version 2 of the InnerObjectFieldConverterV2 class
                // the reason this is required is the testPatch variable has as part of the patch data AZStd::any
                // that wraps a instance of an InnerObjectFieldConverterDerivedV1 stored in an InnerObjectFieldConverterV1 pointer
                // The virtual table points to the InnerObjectFieldConverterDerivedV1 class, which in a normal patching scenario
                // The Data Patch would be loaded from disk after the new version of the InnerObjectFieldConverterV2 has reflected
                // and therefore the patch instance data would be an object of InnerObjectFieldConverterDerivedV1WithV2Base with the
                // correct vtable
                AZ::DataPatch freshDataPatch;
                LoadPatchFromByteStream(byteBuffer, freshDataPatch);

                const PatchedObjectField initialObjectV2;
                PatchedObjectField *patchedObjectV2 = freshDataPatch.Apply(&initialObjectV2, m_serializeContext.get());
                ASSERT_NE(nullptr, patchedObjectV2);
                EXPECT_EQ(10, patchedObjectV2->m_rootInnerObject.m_int64Field);
                auto patchedDerivedInnerObject = azrtti_cast<InnerObjectFieldConverterDerivedV1WithV2Base*>(patchedObjectV2->m_baseInnerObjectPolymorphic);
                ASSERT_NE(nullptr, patchedDerivedInnerObject);
                EXPECT_EQ(12, patchedDerivedInnerObject->m_int64Field);
                ASSERT_NE(nullptr, patchedDerivedInnerObject);
                EXPECT_EQ(10, patchedDerivedInnerObject->m_objectWithNumericField.m_value);

                // Clean up original ObjectFieldConverterV1 object
                delete testObject.m_baseInnerObjectPolymorphic;
                // Clean up patched ObjectFieldConverterV1 object
                delete patchedObjectV2->m_baseInnerObjectPolymorphic;
                delete patchedObjectV2;

                m_serializeContext->EnableRemoveReflection();
                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV2::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1WithV2Base::Reflect(m_serializeContext.get());
                PatchedObjectField::Reflect(m_serializeContext.get());
                m_serializeContext->DisableRemoveReflection();
            }

        }
        inline namespace RootLevelDataSerializerFieldConverter
        {
            // ObjectFieldConverterReplaceMemberDataSerializerV2, uses the same TypeId as ObjectFieldConverterV1
            // for to test the FieldConverter for an IDataSerializer
            class ObjectFieldConverterReplaceMemberDataSerializerV2
            {
                using ClassType = ObjectFieldConverterReplaceMemberDataSerializerV2;
            public:
                AZ_CLASS_ALLOCATOR(ObjectFieldConverterReplaceMemberDataSerializerV2, SystemAllocator);
                AZ_TYPE_INFO(ObjectFieldConverterReplaceMemberDataSerializerV2, "{5722C4E4-25DE-48C5-BC89-0EE9D38DF433}");

                static AZ::Uuid ConvertStringToUuid(const AZStd::string& value)
                {
                    return AZ::Uuid::CreateName(value.data());
                }

                static void Reflect(AZ::ReflectContext* reflectContext)
                {
                    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                    {
                        serializeContext->Class<ClassType>()
                            ->Version(2)
                            ->Field("RootUuidField", &ClassType::m_rootUuidField)
                            ->NameChange(1, 2, "RootStringField", "RootUuidField")
                            // NOTE!! Type Change is prioritized before Name Change, so it works on the old Field name
                            ->TypeChange("RootStringField", 1, 2, AZStd::function<AZ::Uuid(const AZStd::string&)>(&ClassType::ConvertStringToUuid))
                            ->Field("RootStringVector", &ClassType::m_rootStringVector)
                            ->Field("RootInnerObjectValue", &ClassType::m_rootInnerObject)
                            ->Field("RootInnerObjectPointer", &ClassType::m_baseInnerObjectPolymorphic)
                            ;
                    }
                }

                //! AZStd::string uses IDataSerializer for Serialization.
                //! This is to test Field Converters for patched element that are directly on the patched class
                AZ::Uuid m_rootUuidField{ AZ::Uuid::CreateNull() };
                //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                AZStd::vector<AZStd::string> m_rootStringVector;
                //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                InnerObjectFieldConverterV1 m_rootInnerObject{};
                InnerObjectFieldConverterV1* m_baseInnerObjectPolymorphic{};
            };

            // ObjectFieldConverterReplaceMemberDataSerializerV3, uses the same TypeId as ObjectFieldConverterV1
            // for to test the FieldConverter that skips a level
            class ObjectFieldConverterReplaceMemberDataSerializerV3
            {
                using ClassType = ObjectFieldConverterReplaceMemberDataSerializerV3;
            public:
                AZ_CLASS_ALLOCATOR(ObjectFieldConverterReplaceMemberDataSerializerV3, SystemAllocator);
                AZ_TYPE_INFO(ObjectFieldConverterReplaceMemberDataSerializerV3, "{5722C4E4-25DE-48C5-BC89-0EE9D38DF433}");

                static bool ConvertStringToBool(const AZStd::string& value)
                {
                     return !value.empty();
                }

                static bool ConvertUuidToBool(const AZ::Uuid& value)
                {
                    return !value.IsNull();
                }

                static void Reflect(AZ::ReflectContext* reflectContext)
                {
                    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                    {
                        serializeContext->Class<ClassType>()
                            ->Version(3)
                            ->Field("RootBoolField", &ClassType::m_rootBoolField)
                            ->NameChange(2, 3, "RootUuidField", "RootBoolField")
                            ->NameChange(1, 3, "RootStringField", "RootBoolField")
                            //! NOTE Type Change is prioritized before Name Change, so it works on the old Field name
                            ->TypeChange("RootUuidField", 2, 3, AZStd::function<bool(const AZ::Uuid&)>(&ClassType::ConvertUuidToBool))
                            ->TypeChange("RootStringField", 1, 3, AZStd::function<bool(const AZStd::string&)>(&ClassType::ConvertStringToBool))
                            ->Field("RootStringVector", &ClassType::m_rootStringVector)
                            ->Field("RootInnerObjectValue", &ClassType::m_rootInnerObject)
                            ->Field("RootInnerObjectPointer", &ClassType::m_baseInnerObjectPolymorphic)
                            ;
                    }
                }

                //! AZStd::string uses IDataSerializer for Serialization.
                //! This is to test Field Converters for patched element that are directly on the patched class
                bool m_rootBoolField{};
                //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                AZStd::vector<AZStd::string> m_rootStringVector;
                //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                InnerObjectFieldConverterV1 m_rootInnerObject{};
                InnerObjectFieldConverterV1* m_baseInnerObjectPolymorphic{};
            };

            TEST_F(PatchingTest, ObjectFieldConverter_ChangeRootDataSerializerField_ConvertsFromV1ToV2_Successfully)
            {
                using OriginalObjectField = ObjectFieldConverterV1;
                using PatchedObjectField = ObjectFieldConverterReplaceMemberDataSerializerV2;

                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                OriginalObjectField::Reflect(m_serializeContext.get());

                OriginalObjectField testObject;
                // Change the defaults of elements on the ObjectFieldConverterV1 object
                testObject.m_rootStringField = "Test1";
                testObject.m_rootStringVector.emplace_back("Test2");
                testObject.m_rootInnerObject.m_stringField = "InnerTest1";
                testObject.m_rootInnerObject.m_stringVector.emplace_back("InnerTest2");

                auto derivedInnerObject = aznew InnerObjectFieldConverterDerivedV1;
                derivedInnerObject->m_stringField = "DerivedTest1";
                derivedInnerObject->m_stringVector.emplace_back("DerivedTest2");
                derivedInnerObject->m_objectWithNumericField.m_value = 1;
                testObject.m_baseInnerObjectPolymorphic = derivedInnerObject;

                // Create DataPatch using ObjectFieldConverterV1
                DataPatch testPatch;
                const OriginalObjectField initialObjectV1;
                testPatch.Create(&initialObjectV1, &testObject, {}, {}, m_serializeContext.get());
                // Now unreflect the ObjectFieldConverterV1 and reflect ObjectFieldConverterReplaceMemberDataSerializerV2
                {
                    m_serializeContext->EnableRemoveReflection();
                    OriginalObjectField::Reflect(m_serializeContext.get());
                    m_serializeContext->DisableRemoveReflection();
                    PatchedObjectField::Reflect(m_serializeContext.get());
                }

                const PatchedObjectField initialObjectV2;
                PatchedObjectField *patchedObjectV2 = testPatch.Apply(&initialObjectV2, m_serializeContext.get());
                ASSERT_NE(nullptr, patchedObjectV2);
                EXPECT_FALSE(patchedObjectV2->m_rootUuidField.IsNull());

                // Clean up original ObjectFieldConverterV1 object
                delete testObject.m_baseInnerObjectPolymorphic;
                // Clean up patched ObjectFieldConverterV1 object
                delete patchedObjectV2->m_baseInnerObjectPolymorphic;
                delete patchedObjectV2;

                m_serializeContext->EnableRemoveReflection();
                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                PatchedObjectField::Reflect(m_serializeContext.get());
                m_serializeContext->DisableRemoveReflection();
            }

            TEST_F(PatchingTest, ObjectFieldConverter_ChangeRootDataSerializerField_ConvertsFromV2ToV3_Successfully)
            {
                using OriginalObjectField = ObjectFieldConverterReplaceMemberDataSerializerV2;
                using PatchedObjectField = ObjectFieldConverterReplaceMemberDataSerializerV3;

                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                OriginalObjectField::Reflect(m_serializeContext.get());

                OriginalObjectField testObject;
                // Change the defaults of elements on the ObjectFieldConverterV1 object
                testObject.m_rootUuidField = AZ::Uuid::CreateString("{10000000-0000-0000-0000-000000000000}");
                testObject.m_rootStringVector.emplace_back("Test2");
                testObject.m_rootInnerObject.m_stringField = "InnerTest1";
                testObject.m_rootInnerObject.m_stringVector.emplace_back("InnerTest2");

                auto derivedInnerObject = aznew InnerObjectFieldConverterDerivedV1;
                derivedInnerObject->m_stringField = "DerivedTest1";
                derivedInnerObject->m_stringVector.emplace_back("DerivedTest2");
                derivedInnerObject->m_objectWithNumericField.m_value = 1;
                testObject.m_baseInnerObjectPolymorphic = derivedInnerObject;

                // Create DataPatch using ObjectFieldConverterV1
                DataPatch testPatch;
                const OriginalObjectField initialObjectV1;
                testPatch.Create(&initialObjectV1, &testObject, {}, {}, m_serializeContext.get());
                // Now unreflect the ObjectFieldConverterV1 and reflect ObjectFieldConverterReplaceMemberDataSerializerV2
                {
                    m_serializeContext->EnableRemoveReflection();
                    OriginalObjectField::Reflect(m_serializeContext.get());
                    m_serializeContext->DisableRemoveReflection();
                    PatchedObjectField::Reflect(m_serializeContext.get());
                }

                const PatchedObjectField initialObjectV3;
                PatchedObjectField *patchedObjectV3 = testPatch.Apply(&initialObjectV3, m_serializeContext.get());
                ASSERT_NE(nullptr, patchedObjectV3);
                EXPECT_TRUE(patchedObjectV3->m_rootBoolField);

                // Clean up original ObjectFieldConverterV1 object
                delete testObject.m_baseInnerObjectPolymorphic;
                // Clean up patched ObjectFieldConverterV1 object
                delete patchedObjectV3->m_baseInnerObjectPolymorphic;
                delete patchedObjectV3;

                m_serializeContext->EnableRemoveReflection();
                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                PatchedObjectField::Reflect(m_serializeContext.get());
                m_serializeContext->DisableRemoveReflection();
            }

            TEST_F(PatchingTest, ObjectFieldConverter_ChangeRootDataSerializerField_ConvertsFromV1ToV3_Successfully)
            {
                using OriginalObjectField = ObjectFieldConverterV1;
                using PatchedObjectField = ObjectFieldConverterReplaceMemberDataSerializerV3;

                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                OriginalObjectField::Reflect(m_serializeContext.get());

                OriginalObjectField testObject;
                // Change the defaults of elements on the ObjectFieldConverterV1 object
                testObject.m_rootStringField = "StringV1";
                testObject.m_rootStringVector.emplace_back("Test2");
                testObject.m_rootInnerObject.m_stringField = "InnerTest1";
                testObject.m_rootInnerObject.m_stringVector.emplace_back("InnerTest2");

                auto derivedInnerObject = aznew InnerObjectFieldConverterDerivedV1;
                derivedInnerObject->m_stringField = "DerivedTest1";
                derivedInnerObject->m_stringVector.emplace_back("DerivedTest2");
                derivedInnerObject->m_objectWithNumericField.m_value = 1;
                testObject.m_baseInnerObjectPolymorphic = derivedInnerObject;

                // Create DataPatch using ObjectFieldConverterV1
                DataPatch testPatch;
                const OriginalObjectField initialObjectV1;
                testPatch.Create(&initialObjectV1, &testObject, {}, {}, m_serializeContext.get());
                // Now unreflect the ObjectFieldConverterV1 and reflect ObjectFieldConverterReplaceMemberDataSerializerV2
                {
                    m_serializeContext->EnableRemoveReflection();
                    OriginalObjectField::Reflect(m_serializeContext.get());
                    m_serializeContext->DisableRemoveReflection();
                    PatchedObjectField::Reflect(m_serializeContext.get());
                }

                const PatchedObjectField initialObjectV3;
                PatchedObjectField *patchedObjectV3 = testPatch.Apply(&initialObjectV3, m_serializeContext.get());
                ASSERT_NE(nullptr, patchedObjectV3);
                EXPECT_TRUE(patchedObjectV3->m_rootBoolField);

                // Clean up original ObjectFieldConverterV1 object
                delete testObject.m_baseInnerObjectPolymorphic;
                // Clean up patched ObjectFieldConverterV1 object
                delete patchedObjectV3->m_baseInnerObjectPolymorphic;
                delete patchedObjectV3;

                m_serializeContext->EnableRemoveReflection();
                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                PatchedObjectField::Reflect(m_serializeContext.get());
                m_serializeContext->DisableRemoveReflection();
            }
        }

        inline namespace RootLevelDataContainerFieldConverter
        {
            // ObjectFieldConverterReplaceMemberDataConverterV2, uses the same TypeId as ObjectFieldConverterV1
            // for to test the FieldConverter for an IDataConverter
            class ObjectFieldConverterReplaceMemberDataConverterV2
            {
                using ClassType = ObjectFieldConverterReplaceMemberDataConverterV2;
            public:
                AZ_CLASS_ALLOCATOR(ClassType, SystemAllocator);
                AZ_TYPE_INFO(ClassType, "{5722C4E4-25DE-48C5-BC89-0EE9D38DF433}");

                static AZStd::array<AZStd::string, 5> ConvertStringVectorToStringArray(const AZStd::vector<AZStd::string>& value)
                {
                    AZStd::array<AZStd::string, 5> result;
                    size_t elementsToCopy = AZStd::min(result.size(), value.size());
                    for (size_t valueIndex = 0; valueIndex < elementsToCopy; ++valueIndex)
                    {
                        result[valueIndex] = value[valueIndex];
                    }
                    return result;
                }

                static void Reflect(AZ::ReflectContext* reflectContext)
                {
                    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                    {
                        // Because containers are implicitly reflected when used as part of a class reflection
                        // the ObjectFieldConverterV1 AZStd::vector<AZStd::string> class is not reflected
                        // if the ObjectFieldConverterV1 did not reflect
                        serializeContext->RegisterGenericType<AZStd::vector<AZStd::string>>();
                        serializeContext->Class<ClassType>()
                            ->Version(2)
                            ->Field("RootStringField", &ClassType::m_rootStringField)
                            ->Field("RootStringArray", &ClassType::m_rootStringArray)
                            ->TypeChange("RootStringVector", 1, 2, AZStd::function<AZStd::array<AZStd::string, 5>(const AZStd::vector<AZStd::string>&)>(&ConvertStringVectorToStringArray))
                            ->NameChange(1, 2, "RootStringVector", "RootStringArray")
                            ->Field("RootInnerObjectValue", &ClassType::m_rootInnerObject)
                            ->Field("RootInnerObjectPointer", &ClassType::m_baseInnerObjectPolymorphic)
                            ;
                    }
                }

                //! AZStd::string uses IDataSerializer for Serialization.
                //! This is to test Field Converters for patched element that are directly on the patched class
                AZStd::string m_rootStringField;
                //! AZStd::array<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                AZStd::array<AZStd::string, 5> m_rootStringArray;
                //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                InnerObjectFieldConverterV1 m_rootInnerObject{};
                InnerObjectFieldConverterV1* m_baseInnerObjectPolymorphic{};
            };

            // ObjectFieldConverterReplaceMemberDataConverterV3, uses the same TypeId as ObjectFieldConverterV1
            // for to test the FieldConverter that skips a level
            class ObjectFieldConverterReplaceMemberDataConverterV3
            {
                using ClassType = ObjectFieldConverterReplaceMemberDataConverterV3;
            public:
                AZ_CLASS_ALLOCATOR(ClassType, SystemAllocator);
                AZ_TYPE_INFO(ClassType, "{5722C4E4-25DE-48C5-BC89-0EE9D38DF433}");

                static AZStd::list<AZStd::string> ConvertStringVectorToStringList(const AZStd::vector<AZStd::string>& value)
                {
                    AZStd::list<AZStd::string> result(value.begin(), value.end());
                    return result;
                }

                static AZStd::list<AZStd::string> ConvertStringArrayToStringList(const AZStd::array<AZStd::string, 5>& value)
                {
                    AZStd::list<AZStd::string> result(value.begin(), value.end());
                    return result;
                }

                static void Reflect(AZ::ReflectContext* reflectContext)
                {
                    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                    {
                        serializeContext->Class<ClassType>()
                            ->Version(3)
                            ->Field("RootStringField", &ClassType::m_rootStringField)
                            ->Field("RootStringList", &ClassType::m_rootStringList)
                            // The TypeChange and NameChange converters are interleaved purposefully to make sure that the order of declaration
                            // of the converters doesn't affect the conversion result
                            ->TypeChange("RootStringVector", 1, 3, AZStd::function<AZStd::list<AZStd::string>(const AZStd::vector<AZStd::string>&)>(&ConvertStringVectorToStringList))
                            ->NameChange(1, 3, "RootStringVector", "RootStringList")
                            ->NameChange(2, 3, "RootStringArray", "RootStringList")
                            ->TypeChange("RootStringArray", 2, 3, AZStd::function<AZStd::list<AZStd::string>(const AZStd::array<AZStd::string, 5>&)>(&ConvertStringArrayToStringList))
                            ->Field("RootInnerObjectValue", &ClassType::m_rootInnerObject)
                            ->Field("RootInnerObjectPointer", &ClassType::m_baseInnerObjectPolymorphic)
                            ;
                    }
                }

                //! AZStd::string uses IDataSerializer for Serialization.
                //! This is to test Field Converters for patched element that are directly on the patched class
                AZStd::string m_rootStringField;
                //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                AZStd::list<AZStd::string> m_rootStringList;
                //! AZStd::vector<AZStd::string> uses IDataContainer for Serialization. The inner AZStd::string class uses IDataSerializer for serialization
                //! This is to test Field Converters for patched element that are directly on the patched class
                InnerObjectFieldConverterV1 m_rootInnerObject{};
                InnerObjectFieldConverterV1* m_baseInnerObjectPolymorphic{};
            };

            TEST_F(PatchingTest, ObjectFieldConverter_ChangeRootDataContainerField_ConvertsFromV1ToV2_Successfully)
            {
                using OriginalObjectField = ObjectFieldConverterV1;
                using PatchedObjectField = ObjectFieldConverterReplaceMemberDataConverterV2;

                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                OriginalObjectField::Reflect(m_serializeContext.get());

                OriginalObjectField testObject;
                // Change the defaults of elements on the ObjectFieldConverterV1 object
                testObject.m_rootStringField = "Test1";
                testObject.m_rootStringVector.emplace_back("Test2");
                testObject.m_rootInnerObject.m_stringField = "InnerTest1";
                testObject.m_rootInnerObject.m_stringVector.emplace_back("InnerTest2");

                auto derivedInnerObject = aznew InnerObjectFieldConverterDerivedV1;
                derivedInnerObject->m_stringField = "DerivedTest1";
                derivedInnerObject->m_stringVector.emplace_back("DerivedTest2");
                derivedInnerObject->m_objectWithNumericField.m_value = 1;
                testObject.m_baseInnerObjectPolymorphic = derivedInnerObject;

                // Create DataPatch using ObjectFieldConverterV1
                DataPatch testPatch;
                const OriginalObjectField initialObjectV1;
                testPatch.Create(&initialObjectV1, &testObject, {}, {}, m_serializeContext.get());
                // Now unreflect the ObjectFieldConverterV1 and reflect ObjectFieldConverterReplaceMemberDataSerializerV2
                {
                    m_serializeContext->EnableRemoveReflection();
                    OriginalObjectField::Reflect(m_serializeContext.get());
                    m_serializeContext->DisableRemoveReflection();
                    PatchedObjectField::Reflect(m_serializeContext.get());
                }

                const PatchedObjectField initialObjectV2;
                PatchedObjectField *patchedObjectV2 = testPatch.Apply(&initialObjectV2, m_serializeContext.get());
                ASSERT_NE(nullptr, patchedObjectV2);
                EXPECT_EQ("Test2", patchedObjectV2->m_rootStringArray.front());

                // Clean up original ObjectFieldConverterV1 object
                delete testObject.m_baseInnerObjectPolymorphic;
                // Clean up patched ObjectFieldConverterV1 object
                delete patchedObjectV2->m_baseInnerObjectPolymorphic;
                delete patchedObjectV2;

                m_serializeContext->EnableRemoveReflection();
                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                PatchedObjectField::Reflect(m_serializeContext.get());
                m_serializeContext->DisableRemoveReflection();
            }

            TEST_F(PatchingTest, ObjectFieldConverter_ChangeRootDataContainerField_ConvertsFromV2ToV3_Successfully)
            {
                using OriginalObjectField = ObjectFieldConverterReplaceMemberDataConverterV2;
                using PatchedObjectField = ObjectFieldConverterReplaceMemberDataConverterV3;

                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                OriginalObjectField::Reflect(m_serializeContext.get());

                OriginalObjectField testObject;
                // Change the defaults of elements on the ObjectFieldConverterV1 object
                testObject.m_rootStringField = "Test1";
                testObject.m_rootStringArray[0] = "BigTest";
                testObject.m_rootStringArray[3] = "SuperTest";
                testObject.m_rootInnerObject.m_stringField = "InnerTest1";
                testObject.m_rootInnerObject.m_stringVector.emplace_back("InnerTest2");

                auto derivedInnerObject = aznew InnerObjectFieldConverterDerivedV1;
                derivedInnerObject->m_stringField = "DerivedTest1";
                derivedInnerObject->m_stringVector.emplace_back("DerivedTest2");
                derivedInnerObject->m_objectWithNumericField.m_value = 1;
                testObject.m_baseInnerObjectPolymorphic = derivedInnerObject;

                // Create DataPatch using ObjectFieldConverterV1
                DataPatch testPatch;
                const OriginalObjectField initialObjectV1;
                testPatch.Create(&initialObjectV1, &testObject, {}, {}, m_serializeContext.get());
                // Now unreflect the ObjectFieldConverterV1 and reflect ObjectFieldConverterReplaceMemberDataSerializerV2
                {
                    m_serializeContext->EnableRemoveReflection();
                    OriginalObjectField::Reflect(m_serializeContext.get());
                    m_serializeContext->DisableRemoveReflection();
                    PatchedObjectField::Reflect(m_serializeContext.get());
                }

                const PatchedObjectField initialObjectV3;
                PatchedObjectField *patchedObjectV3 = testPatch.Apply(&initialObjectV3, m_serializeContext.get());
                ASSERT_NE(nullptr, patchedObjectV3);
                ASSERT_EQ(2, patchedObjectV3->m_rootStringList.size());
                auto patchListIter = patchedObjectV3->m_rootStringList.begin();
                EXPECT_EQ("BigTest", *patchListIter++);
                EXPECT_EQ("SuperTest", *patchListIter++);

                // Clean up original ObjectFieldConverterV1 object
                delete testObject.m_baseInnerObjectPolymorphic;
                // Clean up patched ObjectFieldConverterV1 object
                delete patchedObjectV3->m_baseInnerObjectPolymorphic;
                delete patchedObjectV3;

                m_serializeContext->EnableRemoveReflection();
                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                PatchedObjectField::Reflect(m_serializeContext.get());
                m_serializeContext->DisableRemoveReflection();
            }

            TEST_F(PatchingTest, ObjectFieldConverter_ChangeRootDataConverterField_ConvertsFromV1ToV3_Successfully)
            {
                using OriginalObjectField = ObjectFieldConverterV1;
                using PatchedObjectField = ObjectFieldConverterReplaceMemberDataConverterV3;

                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                OriginalObjectField::Reflect(m_serializeContext.get());

                OriginalObjectField testObject;
                // Change the defaults of elements on the ObjectFieldConverterV1 object
                testObject.m_rootStringField = "StringV1";
                testObject.m_rootStringVector.emplace_back("Test2");
                testObject.m_rootInnerObject.m_stringField = "InnerTest1";
                testObject.m_rootInnerObject.m_stringVector.emplace_back("InnerTest2");

                auto derivedInnerObject = aznew InnerObjectFieldConverterDerivedV1;
                derivedInnerObject->m_stringField = "DerivedTest1";
                derivedInnerObject->m_stringVector.emplace_back("DerivedTest2");
                derivedInnerObject->m_objectWithNumericField.m_value = 1;
                testObject.m_baseInnerObjectPolymorphic = derivedInnerObject;

                // Create DataPatch using ObjectFieldConverterV1
                DataPatch testPatch;
                const OriginalObjectField initialObjectV1;
                testPatch.Create(&initialObjectV1, &testObject, {}, {}, m_serializeContext.get());
                // Now unreflect the ObjectFieldConverterV1 and reflect ObjectFieldConverterReplaceMemberDataSerializerV2
                {
                    m_serializeContext->EnableRemoveReflection();
                    OriginalObjectField::Reflect(m_serializeContext.get());
                    m_serializeContext->DisableRemoveReflection();
                    PatchedObjectField::Reflect(m_serializeContext.get());
                }

                const PatchedObjectField initialObjectV3;
                PatchedObjectField *patchedObjectV3 = testPatch.Apply(&initialObjectV3, m_serializeContext.get());
                ASSERT_NE(nullptr, patchedObjectV3);
                ASSERT_EQ(1, patchedObjectV3->m_rootStringList.size());
                EXPECT_EQ("Test2", patchedObjectV3->m_rootStringList.front());

                // Clean up original ObjectFieldConverterV1 object
                delete testObject.m_baseInnerObjectPolymorphic;
                // Clean up patched ObjectFieldConverterV1 object
                delete patchedObjectV3->m_baseInnerObjectPolymorphic;
                delete patchedObjectV3;

                m_serializeContext->EnableRemoveReflection();
                ObjectWithNumericFieldV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterV1::Reflect(m_serializeContext.get());
                InnerObjectFieldConverterDerivedV1::Reflect(m_serializeContext.get());
                PatchedObjectField::Reflect(m_serializeContext.get());
                m_serializeContext->DisableRemoveReflection();
            }
        }

        struct ObjectWithAnyAndBool
        {
            AZ_CLASS_ALLOCATOR(ObjectWithAnyAndBool, SystemAllocator);
            AZ_TYPE_INFO(ObjectWithAnyAndBool, "{266FD5C6-39AE-482F-99B7-DA2A1AFE1EA9}");

            static void Reflect(AZ::ReflectContext* reflectContext)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<ObjectWithAnyAndBool>()
                        ->Version(1)
                        ->Field("AnyValue", &ObjectWithAnyAndBool::m_any)
                        ->Field("BoolValue", &ObjectWithAnyAndBool::m_bool);
                }
            }

            AZStd::any m_any;
            bool m_bool;
        };

        TEST_F(PatchingTest, DataPatchingObjectWithAnyPreservesAnyData)
        {
            ObjectWithAnyAndBool::Reflect(m_serializeContext.get());

            ObjectWithAnyAndBool firstObject;
            firstObject.m_any = AZStd::make_any<bool>(false);
            firstObject.m_bool = false;

            ObjectWithAnyAndBool secondObject;
            secondObject.m_any = AZStd::make_any<bool>(false);
            secondObject.m_bool = true;

            DataPatch testPatch;
            testPatch.Create(&firstObject, &secondObject, {}, {}, m_serializeContext.get());

            ObjectWithAnyAndBool* finalObject = testPatch.Apply(&firstObject, m_serializeContext.get());

            ASSERT_FALSE(finalObject->m_any.empty());

            delete finalObject;

            m_serializeContext->EnableRemoveReflection();
            ObjectWithAnyAndBool::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
        }

    }
}
