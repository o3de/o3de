/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


namespace AZ
{
    namespace SerializeContextEnumInternal
    {
        struct EnumConstantBase
        {
            AZ_RTTI(EnumConstantBase, "{22241040-3640-481C-BE3B-31C4917E98E1}");
            virtual ~EnumConstantBase() = default;
            virtual const AZStd::string& GetEnumValueName() const = 0;
            virtual uint64_t GetEnumValueAsUInt() const = 0;
            virtual int64_t GetEnumValueAsInt() const = 0;
        };

        using EnumConstantBasePtr = AZStd::unique_ptr<SerializeContextEnumInternal::EnumConstantBase>;

        template<class EnumType>
        struct EnumConstant
            : EnumConstantBase
        {
            AZ_RTTI((EnumConstant, "{165E0327-C060-45F6-9522-D1CC992E7ECC}", EnumType), EnumConstantBase);
            using UnderlyingType = AZStd::RemoveEnumT<EnumType>;

            EnumConstant() = default;
            EnumConstant(AZStd::string_view description, EnumType value)
                : m_value{ value }
                , m_name{ description }
            {
            }

            const AZStd::string& GetEnumValueName() const override
            {
                return m_name;
            }
            uint64_t GetEnumValueAsUInt() const override
            {
                // This specifically uses static_cast instead of aznumeric_cast because int64 -> uint64
                // conversions are valid and expected.  aznumeric_cast would assert due to loss of sign, but
                // users of this function intentionally perform the conversion to do bit-level comparisons,
                // so the assert is incorrect here.
                return static_cast<uint64_t>(m_value);
            }
            int64_t GetEnumValueAsInt() const override
            {
                return aznumeric_cast<int64_t>(m_value);
            }

            EnumType m_value{};
            AZStd::string m_name;
        };

        template<class EnumType>
        class EnumSerializer
            : public Serialize::IDataSerializer
        {
            static_assert(AZStd::is_enum<EnumType>::value, "Enum Serializer can only be used with enum types");
            using UnderlyingType = AZStd::RemoveEnumT<EnumType>;
            /// Load the class data from a binary buffer.
            bool Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
            {
                if (stream.GetLength() < sizeof(EnumType))
                {
                    return false;
                }

                EnumType* dataPtr = reinterpret_cast<EnumType*>(classPtr);
                if (stream.Read(sizeof(EnumType), reinterpret_cast<void*>(dataPtr)) == sizeof(EnumType))
                {
                    AZ_SERIALIZE_SWAP_ENDIAN(*dataPtr, isDataBigEndian);
                    return true;
                }
                return false;
            }

            /// Store the class data into a stream.
            size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
            {
                EnumType value = *reinterpret_cast<const EnumType*>(classPtr);
                AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
                return static_cast<size_t>(stream.Write(sizeof(EnumType), reinterpret_cast<const void*>(&value)));
            }

            /// Convert binary data to text
            size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
            {
                AZ_Assert(in.GetLength() == sizeof(EnumType), "Invalid data size");

                EnumType value;
                in.Read(sizeof(EnumType), reinterpret_cast<void*>(&value));
                AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
                AZStd::string outText = AZStd::to_string(static_cast<UnderlyingType>(value));
                return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
            }

            /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
            size_t TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
            {
                AZ_Assert(textVersion == 0, "Unknown enum version!");
                (void)textVersion;
                UnderlyingType value{};
                if (AZStd::is_signed<UnderlyingType>::value)
                {
                    long long convertValue = strtoll(text, nullptr, 10);
                    value = aznumeric_cast<UnderlyingType>(convertValue);
                }
                else
                {
                    unsigned long long convertValue = strtoull(text, nullptr, 10);
                    value = aznumeric_cast<UnderlyingType>(convertValue);
                }
                AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
                return static_cast<size_t>(stream.Write(sizeof(EnumType), reinterpret_cast<void*>(&value)));
            }

            bool CompareValueData(const void* lhs, const void* rhs) override
            {
                return SerializeContext::EqualityCompareHelper<EnumType>::CompareValues(lhs, rhs);
            }
        };
    }

    template<typename EnumType>
    SerializeContext::EnumBuilder SerializeContext::Enum()
    {
        return Enum<EnumType>(&Serialize::StaticInstance<Serialize::InstanceFactory<EnumType>>::s_instance);
    }

    template<typename EnumType>
    SerializeContext::EnumBuilder SerializeContext::Enum(IObjectFactory* factory)
    {
        static_assert(AZStd::is_enum<EnumType>::value, "SerializeContext::Enum only supports a C++ enum type");
        using UnderlyingType = std::underlying_type_t<EnumType>;

        const AZ::TypeId& enumTypeId = AzTypeInfo<EnumType>::Uuid();
        const char* name = AzTypeInfo<EnumType>::Name();
        const AZ::TypeId& underlyingTypeId = AzTypeInfo<UnderlyingType>::Uuid();

        AZ_Assert(!enumTypeId.IsNull(), "Enum Type has invalid AZ::TypeId. Has it been specialized with AZ_TYPE_INFO_INTERNAL_SPECIALIZE macro?");
        AZ_Assert(!underlyingTypeId.IsNull(), "Underlying Type of enum has invalid AZ::TypeId. Has it been specialized with AZ_TYPE_INFO_INTERNAL_SPECIALIZE macro?");

        auto enumTypeIter = m_uuidMap.find(enumTypeId);
        if (IsRemovingReflection())
        {
            if (enumTypeIter != m_uuidMap.end())
            {
                RemoveClassData(&enumTypeIter->second);

                auto classNameRange = m_classNameToUuid.equal_range(Crc32(name));
                for (auto classNameRangeIter = classNameRange.first; classNameRangeIter != classNameRange.second;)
                {
                    if (classNameRangeIter->second == enumTypeId)
                    {
                        classNameRangeIter = m_classNameToUuid.erase(classNameRangeIter);
                    }
                    else
                    {
                        ++classNameRangeIter;
                    }
                }
                m_uuidAnyCreationMap.erase(enumTypeId);
                m_uuidMap.erase(enumTypeIter);
            }
        }
        else
        {
            AZ_Error("Serialization", enumTypeIter == m_uuidMap.end(), "Type '%s' with TypeId %s is already registered with SerializeContext."
                " Enum type '%s' could not be registered due to a duplicate typeid.",
                enumTypeIter->second.m_name, underlyingTypeId.ToString<AZStd::string>().data(), name);
            if (enumTypeIter == m_uuidMap.end())
            {
                typename UuidToClassMap::pair_iter_bool enumTypeInsertIter = m_uuidMap.emplace(enumTypeId, ClassData::Create<EnumType>(name, enumTypeId, factory));
                ClassData& enumClassData = enumTypeInsertIter.first->second;
                enumClassData.m_serializer = Serialize::IDataSerializerPtr{ new SerializeContextEnumInternal::EnumSerializer<EnumType>(), IDataSerializer::CreateDefaultDeleteDeleter() };

                m_classNameToUuid.emplace(Crc32(name), enumTypeId);
                m_uuidAnyCreationMap.emplace(enumTypeId, &AnyTypeInfoConcept<EnumType>::CreateAny);

                // Store the underlying type as an attribute within the ClassData
                enumClassData.m_attributes.emplace_back(Serialize::Attributes::EnumUnderlyingType, aznew AZ::AttributeContainerType<AZ::TypeId>(underlyingTypeId));
                enumTypeIter = enumTypeInsertIter.first;
                return EnumBuilder(this, enumTypeIter);
            }
        }

        return EnumBuilder(this, m_uuidMap.end());
    }

    template<class EnumType>
    auto SerializeContext::EnumBuilder::Value(const char* name, EnumType value) -> EnumBuilder*
    {
        static_assert(AZStd::is_enum<EnumType>::value, "EnumBuilder::Value function can only accept enumeration values");

        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data for this class
        }

        AZ_Assert(m_classData->second.m_typeId == AzTypeInfo<EnumType>::Uuid(), "Enum Value '%s' must be a member of enum type %s."
            " The supplied type is %s", name, m_classData->second.m_name, AzTypeInfo<EnumType>::Name());

        AZStd::unique_ptr<SerializeContextEnumInternal::EnumConstantBase> enumConstantPtr = AZStd::make_unique<SerializeContextEnumInternal::EnumConstant<EnumType>>(AZStd::string_view(name), value);
        Attribute(Serialize::Attributes::EnumValueKey, AZStd::move(enumConstantPtr));

        return this;
    }

    template<typename SerializerImplementation>
    auto SerializeContext::EnumBuilder::Serializer() -> EnumBuilder*
    {
        return Serializer({ new SerializerImplementation, IDataSerializer::CreateDefaultDeleteDeleter() });
    }

    template<typename EventHandlerImplementation>
    auto SerializeContext::EnumBuilder::EventHandler() -> EnumBuilder*
    {
        return EventHandler(&Serialize::StaticInstance<EventHandlerImplementation>::s_instance);
    }

    template<typename DataContainerType>
    auto SerializeContext::EnumBuilder::DataContainer() -> EnumBuilder*
    {
        return DataContainer(&Serialize::StaticInstance<DataContainerType>::s_instance);
    }

    template<class T>
    auto SerializeContext::EnumBuilder::Attribute(Crc32 idCrc, T&& value) -> EnumBuilder*
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data for this class
        }

        using ContainerType = AttributeContainerType<AZStd::decay_t<T>>;

        AZ_Error("Serialization", m_currentAttributes, "Current data type doesn't support attributes!");
        if (m_currentAttributes)
        {
            m_currentAttributes->emplace_back(idCrc, aznew ContainerType(AZStd::forward<T>(value)));
        }
        return this;
    }
}
