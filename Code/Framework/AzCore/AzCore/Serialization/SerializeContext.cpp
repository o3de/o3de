/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/DataOverlay.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Asset/AssetSerializer.h>

/// include AZStd any serializer support
#include <AzCore/Serialization/AZStdAnyDataContainer.inl>
#include <AzCore/std/containers/variant.h>

#include <AzCore/std/functional.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/stack.h>

#include <AzCore/Math/MathReflection.h>
#include <AzCore/Math/MathUtils.h>

#include <AzCore/Debug/Profiler.h>

#if defined(AZ_ENABLE_TRACING) && !defined(AZ_DISABLE_SERIALIZER_DEBUG)
#   define AZ_ENABLE_SERIALIZER_DEBUG
#endif

namespace AZ
{
    AZ_THREAD_LOCAL void* Internal::AZStdArrayEvents::m_indices = nullptr;

    //////////////////////////////////////////////////////////////////////////
    // Integral types serializers

    template<class T>
    class BinaryValueSerializer
        : public SerializeContext::IDataSerializer
    {
        /// Load the class data from a binary buffer.
        bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
        {
            if (stream.GetLength() < sizeof(T))
            {
                return false;
            }

            T* dataPtr = reinterpret_cast<T*>(classPtr);
            if (stream.Read(sizeof(T), reinterpret_cast<void*>(dataPtr)) == sizeof(T))
            {
                AZ_SERIALIZE_SWAP_ENDIAN(*dataPtr, isDataBigEndian);
                return true;
            }
            return false;
        }

        /// Store the class data into a stream.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            T value = *reinterpret_cast<const T*>(classPtr);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<const void*>(&value)));
        }
    };


    // char, short, int
    template<class T>
    class IntSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%d", value);
            return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown char, short, int version!");
            (void)textVersion;
            long value = strtol(text, nullptr, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // unsigned char, short, int, long
    template<class T>
    class UIntSerializer
        : public BinaryValueSerializer<T>
    {
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%u", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            unsigned long value = strtoul(text, nullptr, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // long
    template<class T>
    class LongSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%ld", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            long value = strtol(text, nullptr, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // unsigned long
    template<class T>
    class ULongSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%lu", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            unsigned long value = strtoul(text, nullptr, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // long long
    class LongLongSerializer
        : public BinaryValueSerializer<AZ::s64>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(AZ::s64), "Invalid data size");

            AZ::s64 value;
            in.Read(sizeof(AZ::s64), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%lld", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            AZ::s64 value = strtoll(text, nullptr, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(AZ::s64), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<AZ::s64>::CompareValues(lhs, rhs);
        }
    };

    // unsigned long long
    class ULongLongSerializer
        : public BinaryValueSerializer<AZ::u64>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(AZ::u64), "Invalid data size");

            AZ::u64 value;
            in.Read(sizeof(AZ::u64), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%llu", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            unsigned long long value = strtoull(text, nullptr, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(AZ::u64), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<AZ::u64>::CompareValues(lhs, rhs);
        }
    };

    // float, double
    template<class T, T GetEpsilon()>
    class FloatSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%.7f", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown float/double version!");
            (void)textVersion;
            double value = strtod(text, nullptr);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);

            T data = static_cast<T>(value);
            size_t bytesWritten = static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&data)));

            AZ_Assert(bytesWritten == sizeof(T), "Invalid data size");

            return bytesWritten;
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return AZ::IsClose(*reinterpret_cast<const T*>(lhs), *reinterpret_cast<const T*>(rhs), GetEpsilon());
        }
    };

    // bool
    class BoolSerializer
        : public BinaryValueSerializer<bool>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(bool), "Invalid data size");

            bool value;
            in.Read(sizeof(bool), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);

            char outStr[7];
            azsnprintf(outStr, 6, "%s", value ? "true" : "false");

            return static_cast<size_t>(out.Write(strlen(outStr), outStr));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            (void)isDataBigEndian;
            (void)textVersion;
            bool value = strcmp("true", text) == 0;
            return static_cast<size_t>(stream.Write(sizeof(bool), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<bool>::CompareValues(lhs, rhs);
        }
    };

    // serializer without any data write
    class EmptySerializer
        : public SerializeContext::IDataSerializer
    {
    public:
        /// Store the class data into a stream.
        size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian) override
        {
            (void)classPtr;
            (void)stream;
            (void)isDataBigEndian;
            return 0;
        }

        /// Load the class data from a stream.
        bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian) override
        {
            (void)classPtr;
            (void)stream;
            (void)isDataBigEndian;
            return true;
        }

        /// Convert binary data to text.
        size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian) override
        {
            (void)in;
            (void)out;
            (void)isDataBigEndian;
            return 0;
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            (void)text;
            (void)textVersion;
            (void)stream;
            (void)isDataBigEndian;
            return 0;
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            (void)lhs;
            (void)rhs;
            return true;
        }
    };

    struct EnumerateBaseRTTIEnumCallbackData
    {
        AZStd::fixed_vector<Uuid, 32> m_reportedTypes; ///< Array with reported types (with more data - classData was found).
        const SerializeContext::TypeInfoCB* m_callback; ///< Pointer to the callback.
    };

    //=========================================================================
    // SerializeContext
    // [4/27/2012]
    //=========================================================================
    SerializeContext::SerializeContext(bool registerIntegralTypes, bool createEditContext)
        : m_editContext(nullptr)
    {
        if (registerIntegralTypes)
        {
            Class<char>()->
                Serializer<IntSerializer<char> >();
            Class<AZ::s8>()->
                Serializer<IntSerializer<AZ::s8>>();
            Class<short>()->
                Serializer<IntSerializer<short> >();
            Class<int>()->
                Serializer<IntSerializer<int> >();
            Class<long>()->
                Serializer<LongSerializer<long> >();
            Class<AZ::s64>()->
                Serializer<LongLongSerializer>();

            Class<unsigned char>()->
                Serializer<UIntSerializer<unsigned char> >();
            Class<unsigned short>()->
                Serializer<UIntSerializer<unsigned short> >();
            Class<unsigned int>()->
                Serializer<UIntSerializer<unsigned int> >();
            Class<unsigned long>()->
                Serializer<ULongSerializer<unsigned long> >();
            Class<AZ::u64>()->
                Serializer<ULongLongSerializer>();

            Class<float>()->
                Serializer<FloatSerializer<float, &std::numeric_limits<float>::epsilon>>();
            Class<double>()->
                Serializer<FloatSerializer<double, &std::numeric_limits<double>::epsilon>>();

            Class<bool>()->
                Serializer<BoolSerializer>();

            MathReflect(this);

            Class<DataOverlayToken>()->
                Field("Uri", &DataOverlayToken::m_dataUri);
            Class<DataOverlayInfo>()->
                Field("ProviderId", &DataOverlayInfo::m_providerId)->
                Field("DataToken", &DataOverlayInfo::m_dataToken);

            Class<DynamicSerializableField>()->
                Field("TypeId", &DynamicSerializableField::m_typeId);
                // Value data is injected into the hierarchy per-instance, since type is dynamic.

            // Reflects variant monostate class for serializing a variant with an empty alternative
            Class<AZStd::monostate>();
        }

        if (createEditContext)
        {
            CreateEditContext();
        }

        if (registerIntegralTypes)
        {
            Internal::ReflectAny(this);
        }
    }

    //=========================================================================
    // DestroyEditContext
    // [10/26/2012]
    //=========================================================================
    SerializeContext::~SerializeContext()
    {
        DestroyEditContext();
        decltype(m_perModuleSet) moduleSet = AZStd::move(m_perModuleSet);
        for(PerModuleGenericClassInfo* module : moduleSet)
        {
            module->UnregisterSerializeContext(this);
        }
    }

    void SerializeContext::CleanupModuleGenericClassInfo()
    {
        GetCurrentSerializeContextModule().UnregisterSerializeContext(this);
    }

    //=========================================================================
    // CreateEditContext
    // [10/26/2012]
    //=========================================================================
    EditContext* SerializeContext::CreateEditContext()
    {
        if (m_editContext == nullptr)
        {
            m_editContext = aznew EditContext(*this);
        }

        return m_editContext;
    }

    //=========================================================================
    // DestroyEditContext
    // [10/26/2012]
    //=========================================================================
    void SerializeContext::DestroyEditContext()
    {
        if (m_editContext)
        {
            delete m_editContext;
            m_editContext = nullptr;
        }
    }

    //=========================================================================
    // GetEditContext
    // [11/8/2012]
    //=========================================================================
    EditContext* SerializeContext::GetEditContext() const
    {
        return m_editContext;
    }

    auto SerializeContext::RegisterType(const AZ::TypeId& typeId, AZ::SerializeContext::ClassData&& classData, CreateAnyFunc createAnyFunc) -> ClassBuilder
    {
        auto [typeToClassIter, inserted] = m_uuidMap.try_emplace(typeId, AZStd::move(classData));
        m_classNameToUuid.emplace(AZ::Crc32(typeToClassIter->second.m_name), typeId);
        m_uuidAnyCreationMap.emplace(typeId, createAnyFunc);

        return ClassBuilder(this, typeToClassIter);
    }

    bool SerializeContext::UnregisterType(const AZ::TypeId& typeId)
    {
        if (auto typeToClassIter = m_uuidMap.find(typeId); typeToClassIter != m_uuidMap.end())
        {
            ClassData& classData = typeToClassIter->second;
            RemoveClassData(&classData);

            auto [classNameRangeFirst, classNameRangeLast] = m_classNameToUuid.equal_range(Crc32(classData.m_name));
            while (classNameRangeFirst != classNameRangeLast)
            {
                if (classNameRangeFirst->second == typeId)
                {
                    classNameRangeFirst = m_classNameToUuid.erase(classNameRangeFirst);
                }
                else
                {
                    ++classNameRangeFirst;
                }
            }
            m_uuidAnyCreationMap.erase(typeId);
            m_uuidMap.erase(typeToClassIter);
            return true;
        }

        return false;
    }

    //=========================================================================
    // ClassDeprecate
    // [11/8/2012]
    //=========================================================================
    void SerializeContext::ClassDeprecate(const char* name, const AZ::Uuid& typeUuid, VersionConverter converter)
    {
        if (IsRemovingReflection())
        {
            m_uuidMap.erase(typeUuid);
            return;
        }

        UuidToClassMap::pair_iter_bool result = m_uuidMap.insert_key(typeUuid);
        AZ_Assert(result.second, "This class type %s has already been registered", name /*,typeUuid.ToString()*/);

        ClassData& cd = result.first->second;
        cd.m_name = name;
        cd.m_typeId = typeUuid;
        cd.m_version = VersionClassDeprecated;
        cd.m_converter = converter;
        cd.m_serializer = nullptr;
        cd.m_factory = nullptr;
        cd.m_container = nullptr;
        cd.m_azRtti = nullptr;
        cd.m_eventHandler = nullptr;
        cd.m_editData = nullptr;

        m_classNameToUuid.emplace(AZ::Crc32(name), typeUuid);
    }

    //=========================================================================
    // FindClassId
    // [5/31/2017]
    //=========================================================================
    AZStd::vector<AZ::Uuid> SerializeContext::FindClassId(const AZ::Crc32& classNameCrc) const
    {
        auto&& findResult = m_classNameToUuid.equal_range(classNameCrc);
        AZStd::vector<AZ::Uuid> retVal;
        for (auto&& currentIter = findResult.first; currentIter != findResult.second; ++currentIter)
        {
            retVal.push_back(currentIter->second);
        }
        return retVal;
    }

    //=========================================================================
    // FindClassData
    // [5/11/2012]
    //=========================================================================
    const SerializeContext::ClassData*
    SerializeContext::FindClassData(const Uuid& classId, const SerializeContext::ClassData* parent, u32 elementNameCrc) const
    {
        SerializeContext::UuidToClassMap::const_iterator it = m_uuidMap.find(classId);
        const SerializeContext::ClassData* cd = it != m_uuidMap.end() ? &it->second : nullptr;

        if (!cd)
        {
            // this is not a registered type try to find it in the parent scope by name and check / type and flags
            if (parent)
            {
                if (parent->m_container)
                {
                    const SerializeContext::ClassElement* classElement = parent->m_container->GetElement(elementNameCrc);
                    if (classElement && classElement->m_genericClassInfo)
                    {
                        if (classElement->m_genericClassInfo->CanStoreType(classId))
                        {
                            cd = classElement->m_genericClassInfo->GetClassData();
                        }
                    }
                }
                else if (elementNameCrc)
                {
                    for (size_t i = 0; i < parent->m_elements.size(); ++i)
                    {
                        const SerializeContext::ClassElement& classElement = parent->m_elements[i];
                        if (classElement.m_nameCrc == elementNameCrc && classElement.m_genericClassInfo)
                        {
                            if (classElement.m_genericClassInfo->CanStoreType(classId))
                            {
                                cd = classElement.m_genericClassInfo->GetClassData();
                            }
                            break;
                        }
                    }
                }
            }

            /* If the ClassData could not be found in the normal UuidMap, then the GenericUuid map will be searched.
             The GenericUuid map contains a mapping of Uuids to ClassData from which registered GenericClassInfo
             when reflecting
             */
            if (!cd)
            {
                auto genericClassInfo = FindGenericClassInfo(classId);
                if (genericClassInfo)
                {
                    cd = genericClassInfo->GetClassData();
                }
            }

            // The supplied Uuid will be searched in the enum -> underlying type map to fallback to using the integral type for enum fields reflected to a class
            // but not being explicitly reflected to the SerializeContext using the EnumBuilder
            if (!cd)
            {
                auto enumToUnderlyingTypeIdIter = m_enumTypeIdToUnderlyingTypeIdMap.find(classId);
                if (enumToUnderlyingTypeIdIter != m_enumTypeIdToUnderlyingTypeIdMap.end())
                {
                    const AZ::TypeId& underlyingTypeId = enumToUnderlyingTypeIdIter->second;
                    auto underlyingTypeIter = m_uuidMap.find(underlyingTypeId);
                    cd = underlyingTypeIter != m_uuidMap.end() ? &underlyingTypeIter->second : nullptr;
                }
            }
        }

        return cd;
    }

    //=========================================================================
    // FindGenericClassInfo
    //=========================================================================
    GenericClassInfo* SerializeContext::FindGenericClassInfo(const Uuid& classId) const
    {
        Uuid resolvedUuid = classId;
        // If the classId is not in the registered typeid to GenericClassInfo Map attempt to look it up in the legacy specialization
        // typeid map
        if (m_uuidGenericMap.count(resolvedUuid) == 0)
        {
            auto foundIt = m_legacySpecializeTypeIdToTypeIdMap.find(classId);
            if (foundIt != m_legacySpecializeTypeIdToTypeIdMap.end())
            {
                resolvedUuid = foundIt->second;
            }
        }
        auto genericClassIt = m_uuidGenericMap.find(resolvedUuid);
        if (genericClassIt != m_uuidGenericMap.end())
        {
            return genericClassIt->second;
        }

        return nullptr;
    }

    const TypeId& SerializeContext::GetUnderlyingTypeId(const TypeId& enumTypeId) const
    {
        auto enumToUnderlyingTypeIdIter = m_enumTypeIdToUnderlyingTypeIdMap.find(enumTypeId);
        return enumToUnderlyingTypeIdIter != m_enumTypeIdToUnderlyingTypeIdMap.end() ? enumToUnderlyingTypeIdIter->second : enumTypeId;
    }

    void SerializeContext::RegisterGenericClassInfo(const Uuid& classId, GenericClassInfo* genericClassInfo, const CreateAnyFunc& createAnyFunc)
    {
        if (!genericClassInfo)
        {
            return;
        }

        if (IsRemovingReflection())
        {
            RemoveGenericClassInfo(genericClassInfo);
        }
        else
        {
            // Make sure that the ModuleCleanup structure has the serializeContext set on it.
            GetCurrentSerializeContextModule().RegisterSerializeContext(this);
            // Find the module GenericClassInfo in the SerializeContext GenericClassInfo multimap and add if it is not part of the SerializeContext multimap
            auto scGenericClassInfoRange = m_uuidGenericMap.equal_range(classId);
            auto scGenericInfoFoundIt = AZStd::find_if(scGenericClassInfoRange.first, scGenericClassInfoRange.second, [genericClassInfo](const AZStd::pair<AZ::Uuid, GenericClassInfo*>& genericPair)
            {
                return genericClassInfo == genericPair.second;
            });

            if (scGenericInfoFoundIt == scGenericClassInfoRange.second)
            {
                m_uuidGenericMap.emplace(classId, genericClassInfo);
                m_uuidAnyCreationMap.emplace(classId, createAnyFunc);
                m_classNameToUuid.emplace(genericClassInfo->GetClassData()->m_name, classId);
                m_legacySpecializeTypeIdToTypeIdMap.emplace(genericClassInfo->GetLegacySpecializedTypeId(), classId);
            }
        }
    }

    AZStd::any SerializeContext::CreateAny(const Uuid& classId)
    {
        auto anyCreationIt = m_uuidAnyCreationMap.find(classId);
        return anyCreationIt != m_uuidAnyCreationMap.end() ? anyCreationIt->second(this) : AZStd::any();
    }

    //=========================================================================
    // CanDowncast
    // [6/5/2012]
    //=========================================================================
    bool SerializeContext::CanDowncast(const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper, const IRttiHelper* toClassHelper) const
    {
        // Same type
        if (fromClassId == toClassId || GetUnderlyingTypeId(fromClassId) == toClassId)
        {
            return true;
        }

        // rtti info not provided, see if we can cast using reflection data first
        if (!fromClassHelper)
        {
            const ClassData* fromClass = FindClassData(fromClassId);
            if (!fromClass)
            {
                return false;   // Can't cast without class data or rtti
            }
            for (size_t i = 0; i < fromClass->m_elements.size(); ++i)
            {
                if ((fromClass->m_elements[i].m_flags & ClassElement::FLG_BASE_CLASS) != 0)
                {
                    if (fromClass->m_elements[i].m_typeId == toClassId)
                    {
                        return true;
                    }
                }
            }

            if (!fromClass->m_azRtti)
            {
                return false;   // Reflection info failed to cast and we can't find rtti info
            }
            fromClassHelper = fromClass->m_azRtti;
        }

        if (!toClassHelper)
        {
            const ClassData* toClass = FindClassData(toClassId);
            if (!toClass || !toClass->m_azRtti)
            {
                return false;   // We can't cast without class data or rtti helper
            }
            toClassHelper = toClass->m_azRtti;
        }

        // Rtti available, use rtti cast
        return fromClassHelper->IsTypeOf(toClassHelper->GetTypeId());
    }

    //=========================================================================
    // DownCast
    // [6/5/2012]
    //=========================================================================
    void* SerializeContext::DownCast(void* instance, const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper, const IRttiHelper* toClassHelper) const
    {
        // Same type
        if (fromClassId == toClassId || GetUnderlyingTypeId(fromClassId) == toClassId)
        {
            return instance;
        }

        // rtti info not provided, see if we can cast using reflection data first
        if (!fromClassHelper)
        {
            const ClassData* fromClass = FindClassData(fromClassId);
            if (!fromClass)
            {
                return nullptr;
            }

            for (size_t i = 0; i < fromClass->m_elements.size(); ++i)
            {
                if ((fromClass->m_elements[i].m_flags & ClassElement::FLG_BASE_CLASS) != 0)
                {
                    if (fromClass->m_elements[i].m_typeId == toClassId)
                    {
                        return (char*)(instance) + fromClass->m_elements[i].m_offset;
                    }
                }
            }

            if (!fromClass->m_azRtti)
            {
                return nullptr;    // Reflection info failed to cast and we can't find rtti info
            }
            fromClassHelper = fromClass->m_azRtti;
        }

        if (!toClassHelper)
        {
            const ClassData* toClass = FindClassData(toClassId);
            if (!toClass || !toClass->m_azRtti)
            {
                return nullptr;    // We can't cast without class data or rtti helper
            }
            toClassHelper = toClass->m_azRtti;
        }

        // Rtti available, use rtti cast
        return fromClassHelper->Cast(instance, toClassHelper->GetTypeId());
    }

    //=========================================================================
    // DataElement
    // [5/22/2012]
    //=========================================================================
    SerializeContext::DataElement::DataElement()
        : m_name(nullptr)
        , m_nameCrc(0)
        , m_dataSize(0)
        , m_byteStream(&m_buffer)
        , m_stream(nullptr)
    {
    }

    //=========================================================================
    // ~DataElement
    // [5/22/2012]
    //=========================================================================
    SerializeContext::DataElement::~DataElement()
    {
        m_buffer.clear();
    }

    //=========================================================================
    // DataElement
    // [5/22/2012]
    //=========================================================================
    SerializeContext::DataElement::DataElement(const DataElement& rhs)
        : m_name(rhs.m_name)
        , m_nameCrc(rhs.m_nameCrc)
        , m_dataSize(rhs.m_dataSize)
        , m_byteStream(&m_buffer)
    {
        m_id = rhs.m_id;
        m_version = rhs.m_version;
        m_dataType = rhs.m_dataType;

        m_buffer = rhs.m_buffer;
        m_byteStream = IO::ByteContainerStream<AZStd::vector<char> >(&m_buffer);
        m_byteStream.Seek(rhs.m_byteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);
        if (rhs.m_stream == &rhs.m_byteStream)
        {
            AZ_Assert(rhs.m_dataSize == rhs.m_buffer.size(), "Temp buffer must contain only the data for current element!");
            m_stream = &m_byteStream;
        }
        else
        {
            m_stream = rhs.m_stream;
        }
    }

    //=========================================================================
    // DataElement
    // [5/22/2012]
    //=========================================================================
    SerializeContext::DataElement& SerializeContext::DataElement::operator= (const DataElement& rhs)
    {
        m_name = rhs.m_name;
        m_nameCrc = rhs.m_nameCrc;
        m_id = rhs.m_id;
        m_version = rhs.m_version;
        m_dataSize = rhs.m_dataSize;
        m_dataType = rhs.m_dataType;

        m_buffer = rhs.m_buffer;
        m_byteStream = IO::ByteContainerStream<AZStd::vector<char> >(&m_buffer);
        m_byteStream.Seek(rhs.m_byteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);

        if (rhs.m_stream == &rhs.m_byteStream)
        {
            AZ_Assert(rhs.m_dataSize == rhs.m_buffer.size(), "Temp buffer must contain only the data for current element!");
            m_stream = &m_byteStream;
        }
        else
        {
            m_stream = rhs.m_stream;
        }

        return *this;
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    SerializeContext::DataElement::DataElement(DataElement&& rhs)
        : m_name(rhs.m_name)
        , m_nameCrc(rhs.m_nameCrc)
        , m_dataSize(rhs.m_dataSize)
        , m_byteStream(&m_buffer)
    {
        m_id = rhs.m_id;
        m_version = rhs.m_version;
        m_dataType = rhs.m_dataType;
        if (rhs.m_stream == &rhs.m_byteStream)
        {
            AZ_Assert(rhs.m_dataSize == rhs.m_buffer.size(), "Temp buffer must contain only the data for current element!");
            m_stream = &m_byteStream;
        }
        else
        {
            m_stream = rhs.m_stream;
        }

        m_buffer = AZStd::move(rhs.m_buffer);
        m_byteStream = IO::ByteContainerStream<AZStd::vector<char> >(&m_buffer);
        m_byteStream.Seek(rhs.m_byteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);

        rhs.m_name = nullptr;
        rhs.m_nameCrc = 0;
        rhs.m_id = Uuid::CreateNull();
        rhs.m_version = 0;
        rhs.m_dataSize = 0;
        rhs.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        if (m_stream == &m_byteStream)
        {
            rhs.m_stream = &rhs.m_byteStream;
        }
        else
        {
            rhs.m_stream = nullptr;
        }
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    SerializeContext::DataElement& SerializeContext::DataElement::operator= (DataElement&& rhs)
    {
        m_name = rhs.m_name;
        m_nameCrc = rhs.m_nameCrc;
        m_id = rhs.m_id;
        m_version = rhs.m_version;
        m_dataSize = rhs.m_dataSize;
        m_dataType = rhs.m_dataType;
        if (rhs.m_stream == &rhs.m_byteStream)
        {
            AZ_Assert(rhs.m_dataSize == rhs.m_buffer.size(), "Temp buffer must contain only the data for current element!");
            m_stream = &m_byteStream;
        }
        else
        {
            m_stream = rhs.m_stream;
        }


        m_buffer = AZStd::move(rhs.m_buffer);
        m_byteStream = IO::ByteContainerStream<AZStd::vector<char> >(&m_buffer);
        m_byteStream.Seek(rhs.m_byteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);

        rhs.m_name = nullptr;
        rhs.m_nameCrc = 0;
        rhs.m_id = Uuid::CreateNull();
        rhs.m_version = 0;
        rhs.m_dataSize = 0;
        rhs.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        if (m_stream == &m_byteStream)
        {
            rhs.m_stream = &rhs.m_byteStream;
        }
        else
        {
            rhs.m_stream = nullptr;
        }

        return *this;
    }

    //=========================================================================
    // Convert
    //=========================================================================
    bool SerializeContext::DataElementNode::Convert(SerializeContext& sc, const Uuid& id)
    {
        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.m_id = id;
        m_element.m_dataSize = 0;
        m_element.m_buffer.clear();
        m_element.m_stream = &m_element.m_byteStream;

        m_classData = sc.FindClassData(m_element.m_id);
        AZ_Assert(m_classData, "You are adding element to an unregistered class!");
        m_element.m_version = m_classData->m_version;

        return true;
    }

    //=========================================================================
    // Convert
    //=========================================================================
    bool SerializeContext::DataElementNode::Convert(SerializeContext& sc, const char* name, const Uuid& id)
    {
        AZ_Assert(name != nullptr && strlen(name) > 0, "Empty name is an INVALID element name!");
        u32 nameCrc = Crc32(name);

#if defined(AZ_ENABLE_TRACING)
        if (FindElement(nameCrc) != -1)
        {
            AZ_Error("Serialize", false, "During reflection of class '%s' - member %s is declared more than once.", m_classData->m_name ? m_classData->m_name : "<unknown class>", name);
            return false;
        }
#endif // AZ_ENABLE_TRACING

        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.m_name = name;
        m_element.m_nameCrc = nameCrc;
        m_element.m_id = id;
        m_element.m_dataSize = 0;
        m_element.m_buffer.clear();
        m_element.m_stream = &m_element.m_byteStream;

        m_classData = sc.FindClassData(m_element.m_id);
        AZ_Assert(m_classData, "You are adding element to an unregistered class!");
        m_element.m_version = m_classData->m_version;

        return true;
    }

    //=========================================================================
    // FindElement
    // [5/22/2012]
    //=========================================================================
    int SerializeContext::DataElementNode::FindElement(u32 crc)
    {
        for (size_t i = 0; i < m_subElements.size(); ++i)
        {
            if (m_subElements[i].m_element.m_nameCrc == crc)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    //=========================================================================
    // FindSubElement
    //=========================================================================
    SerializeContext::DataElementNode* SerializeContext::DataElementNode::FindSubElement(u32 crc)
    {
        int index = FindElement(crc);
        return index >= 0 ? &m_subElements[index] : nullptr;
    }

    //=========================================================================
    // RemoveElement
    // [5/22/2012]
    //=========================================================================
    void SerializeContext::DataElementNode::RemoveElement(int index)
    {
        AZ_Assert(index >= 0 && index < static_cast<int>(m_subElements.size()), "Invalid index passed to RemoveElement");

        DataElementNode& node = m_subElements[index];

        node.m_element.m_dataSize = 0;
        node.m_element.m_buffer.clear();
        node.m_element.m_stream = nullptr;

        while (!node.m_subElements.empty())
        {
            node.RemoveElement(static_cast<int>(node.m_subElements.size()) - 1);
        }

        m_subElements.erase(m_subElements.begin() + index);
    }

    //=========================================================================
    // RemoveElementByName
    //=========================================================================
    bool SerializeContext::DataElementNode::RemoveElementByName(u32 crc)
    {
        int index = FindElement(crc);
        if (index >= 0)
        {
            RemoveElement(index);
            return true;
        }
        return false;
    }

    //=========================================================================
    // AddElement
    // [5/22/2012]
    //=========================================================================
    int SerializeContext::DataElementNode::AddElement(const DataElementNode& elem)
    {
        m_subElements.push_back(elem);
        return static_cast<int>(m_subElements.size() - 1);
    }

    //=========================================================================
    // AddElement
    // [5/22/2012]
    //=========================================================================
    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, const char* name, const Uuid& id)
    {
        const AZ::SerializeContext::ClassData* classData = sc.FindClassData(id);
        AZ_Assert(classData, "You are adding element to an unregistered class!");
        return AddElement(sc, name, *classData);
    }

    //=========================================================================
    // AddElement
    //=========================================================================
    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, const char* name, const ClassData& classData)
    {
        (void)sc;
        AZ_Assert(name != nullptr && strlen(name) > 0, "Empty name is an INVALID element name!");
        u32 nameCrc = Crc32(name);

    #if defined(AZ_ENABLE_TRACING)
        if (!m_classData->m_container && FindElement(nameCrc) != -1)
        {
            AZ_Error("Serialize", false, "During reflection of class '%s' - member %s is declared more than once.", classData.m_name ? classData.m_name : "<unknown class>", name);
            return -1;
        }
    #endif // AZ_ENABLE_TRACING

        DataElementNode node;
        node.m_element.m_name = name;
        node.m_element.m_nameCrc = nameCrc;
        node.m_element.m_id = classData.m_typeId;
        node.m_classData = &classData;
        node.m_element.m_version = classData.m_version;

        m_subElements.push_back(node);
        return static_cast<int>(m_subElements.size() - 1);
    }

    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, AZStd::string_view name, GenericClassInfo* genericClassInfo)
    {
        (void)sc;
        AZ_Assert(!name.empty(), "Empty name is an INVALID element name!");

        if (!genericClassInfo)
        {
            AZ_Assert(false, "Supplied GenericClassInfo is nullptr. ClassData cannot be retrieved.");
            return -1;
        }

        u32 nameCrc = Crc32(name.data());
        DataElementNode node;
        node.m_element.m_name = name.data();
        node.m_element.m_nameCrc = nameCrc;
        node.m_element.m_id = genericClassInfo->GetClassData()->m_typeId;
        node.m_classData = genericClassInfo->GetClassData();
        node.m_element.m_version = node.m_classData->m_version;

        m_subElements.push_back(node);
        return static_cast<int>(m_subElements.size() - 1);
    }

    //=========================================================================
    // ReplaceElement
    //=========================================================================
    int SerializeContext::DataElementNode::ReplaceElement(SerializeContext& sc, int index, const char* name, const Uuid& id)
    {
        DataElementNode& node = m_subElements[index];
        if (node.Convert(sc, name, id))
        {
            return index;
        }
        else
        {
            return -1;
        }
    }

    //=========================================================================
    // SetName
    // [1/16/2013]
    //=========================================================================
    void SerializeContext::DataElementNode::SetName(const char* newName)
    {
        m_element.m_name = newName;
        m_element.m_nameCrc = Crc32(newName);
    }

    //=========================================================================
    // SetDataHierarchy
    //=========================================================================
    bool SerializeContext::DataElementNode::SetDataHierarchy(SerializeContext& sc, const void* objectPtr, const Uuid& classId, ErrorHandler* errorHandler, const ClassData* classData)
    {
        AZ_Assert(m_element.m_id == classId, "SetDataHierarchy called with mismatched class type {%s} for element %s",
            classId.ToString<AZStd::string>().c_str(), m_element.m_name);

        AZStd::vector<DataElementNode*> nodeStack;
        DataElementNode* topNode = this;
        bool success = false;

        auto beginCB = [&sc, &nodeStack, topNode, &success, errorHandler](void* ptr, const SerializeContext::ClassData* elementClassData, const SerializeContext::ClassElement* elementData) -> bool
            {
                if (nodeStack.empty())
                {
                    success = true;
                    nodeStack.push_back(topNode);
                    return true;
                }

                DataElementNode* parentNode = nodeStack.back();
                parentNode->m_subElements.reserve(64);

                AZ_Assert(elementData, "Missing element data");
                AZ_Assert(elementClassData, "Missing class data for element %s", elementData ? elementData->m_name : "<unknown>");

                int elementIndex = -1;
                if (elementData)
                {
                    elementIndex = elementData->m_genericClassInfo
                        ? parentNode->AddElement(sc, elementData->m_name, elementData->m_genericClassInfo)
                        : parentNode->AddElement(sc, elementData->m_name, *elementClassData);
                }
                if (elementIndex >= 0)
                {
                    DataElementNode& newNode = parentNode->GetSubElement(elementIndex);

                    if (elementClassData->m_serializer)
                    {
                        void* resolvedObject = ptr;
                        if (elementData && elementData->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                        {
                            resolvedObject = *(void**)(resolvedObject);

                            if (resolvedObject && elementData->m_azRtti)
                            {
                                AZ::Uuid actualClassId = elementData->m_azRtti->GetActualUuid(resolvedObject);
                                if (actualClassId != elementData->m_typeId)
                                {
                                    const AZ::SerializeContext::ClassData* actualClassData = sc.FindClassData(actualClassId);
                                    if (actualClassData)
                                    {
                                        resolvedObject = elementData->m_azRtti->Cast(resolvedObject, actualClassData->m_azRtti->GetTypeId());
                                    }
                                }
                            }
                        }

                        AZ_Assert(newNode.m_element.m_byteStream.GetCurPos() == 0, "This stream should be only for our data element");
                        newNode.m_element.m_dataSize = elementClassData->m_serializer->Save(resolvedObject, newNode.m_element.m_byteStream);
                        newNode.m_element.m_byteStream.Truncate();
                        newNode.m_element.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                        newNode.m_element.m_stream = &newNode.m_element.m_byteStream;

                        newNode.m_element.m_dataType = DataElement::DT_BINARY;
                    }

                    nodeStack.push_back(&newNode);
                    success = true;
                }
                else
                {
                    const AZStd::string error =
                        AZStd::string::format("Failed to add sub-element \"%s\" to element \"%s\".",
                            elementData ? elementData->m_name : "<unknown>",
                            parentNode->m_element.m_name);

                    if (errorHandler)
                    {
                        errorHandler->ReportError(error.c_str());
                    }
                    else
                    {
                        AZ_Error("Serialize", false, "%s", error.c_str());
                    }

                    success = false;
                    return false; // Stop enumerating.
                }

                return true;
            };

        auto endCB = [&nodeStack, &success]() -> bool
            {
                if (success)
                {
                    nodeStack.pop_back();
                }

                return true;
            };

        EnumerateInstanceCallContext callContext(
            beginCB,
            endCB,
            &sc,
            SerializeContext::ENUM_ACCESS_FOR_READ,
            errorHandler);

        sc.EnumerateInstanceConst(
            &callContext,
            objectPtr,
            classId,
            classData,
            nullptr
            );

        return success;
    }

    bool SerializeContext::DataElementNode::GetClassElement(ClassElement& classElement, const DataElementNode& parentDataElement, ErrorHandler* errorHandler) const
    {
        bool elementFound = false;
        if (parentDataElement.m_classData)
        {
            const ClassData* parentClassData = parentDataElement.m_classData;
            if (parentClassData->m_container)
            {
                IDataContainer* classContainer = parentClassData->m_container;
                // store the container element in classElementMetadata
                ClassElement classElementMetadata;
                bool classElementFound = classContainer->GetElement(classElementMetadata, m_element);
                AZ_Assert(classElementFound, "'%s'(0x%x) is not a valid element name for container type %s", m_element.m_name ? m_element.m_name : "NULL", m_element.m_nameCrc, parentClassData->m_name);
                if (classElementFound)
                {
                    // if the container contains pointers, then the elements could be a derived type,
                    // otherwise we need the uuids to be exactly the same.
                    if (classElementMetadata.m_flags & SerializeContext::ClassElement::FLG_POINTER)
                    {
                        bool downcastPossible = m_element.m_id == classElementMetadata.m_typeId;
                        downcastPossible = downcastPossible || (m_classData->m_azRtti && classElementMetadata.m_azRtti && m_classData->m_azRtti->IsTypeOf(classElementMetadata.m_azRtti->GetTypeId()));
                        if (!downcastPossible)
                        {
                            AZStd::string error = AZStd::string::format("Element of type %s cannot be added to container of pointers to type %s!"
                                , m_element.m_id.ToString<AZStd::string>().c_str(), classElementMetadata.m_typeId.ToString<AZStd::string>().c_str());
                            errorHandler->ReportError(error.c_str());

                            return false;
                        }
                    }
                    else
                    {
                        if (m_element.m_id != classElementMetadata.m_typeId)
                        {
                            AZStd::string error = AZStd::string::format("Element of type %s cannot be added to container of type %s!"
                                , m_element.m_id.ToString<AZStd::string>().c_str(), classElementMetadata.m_typeId.ToString<AZStd::string>().c_str());
                            errorHandler->ReportError(error.c_str());

                            return false;
                        }
                    }
                    classElement = classElementMetadata;
                    elementFound = true;
                }
            }
            else
            {
                for (size_t i = 0; i < parentClassData->m_elements.size(); ++i)
                {
                    const SerializeContext::ClassElement* childElement = &parentClassData->m_elements[i];
                    if (childElement->m_nameCrc == m_element.m_nameCrc)
                    {
                        // if the member is a pointer type, then the pointer could be a derived type,
                        // otherwise we need the uuids to be exactly the same.
                        if (childElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                        {
                            // Verify that a downcast is possible
                            bool downcastPossible = m_element.m_id == childElement->m_typeId;
                            downcastPossible = downcastPossible || (m_classData->m_azRtti && childElement->m_azRtti && m_classData->m_azRtti->IsTypeOf(childElement->m_azRtti->GetTypeId()));
                            if (downcastPossible)
                            {
                                classElement = *childElement;
                                elementFound = true;
                            }
                        }
                        else
                        {
                            if (m_element.m_id == childElement->m_typeId)
                            {
                                classElement = *childElement;
                                elementFound = true;
                            }
                        }
                        break;
                    }
                }
            }
        }
        return elementFound;
    }

    bool SerializeContext::DataElementNode::GetDataHierarchyEnumerate(ErrorHandler* errorHandler, NodeStack& nodeStack)
    {
        if (nodeStack.empty())
        {
            return true;
        }

        void* parentPtr = nodeStack.back().m_ptr;
        DataElementNode* parentDataElement = nodeStack.back().m_dataElement;
        AZ_Assert(parentDataElement, "parentDataElement is null, cannot enumerate data from data element (%s:%s)",
            m_element.m_name ? m_element.m_name : "", m_element.m_id.ToString<AZStd::string>().data());
        if (!parentDataElement)
        {
            return false;
        }

        bool success = true;

        if (!m_classData)
        {
            AZ_Error("Serialize", false, R"(Cannot enumerate data from data element (%s:%s) from parent data element (%s:%s) with class name "%s" because the ClassData does not exist.)"
                " This can indicate that the class is not reflected at the point of this call. If this is class is reflected as part of a gem"
                " check if that gem is loaded", m_element.m_name ? m_element.m_name : "", m_element.m_id.ToString<AZStd::string>().data(),
                parentDataElement->m_element.m_name ? parentDataElement->m_element.m_name : "", parentDataElement->m_element.m_id.ToString<AZStd::string>().data(), parentDataElement->m_classData->m_name);
            return false;
        }

        AZ::SerializeContext::ClassElement classElement;
        bool classElementFound = parentDataElement && GetClassElement(classElement, *parentDataElement, errorHandler);
        AZ_Warning("Serialize", classElementFound, R"(Unable to find class element for data element(%s:%s) with class name "%s" that is a child of parent data element(%s:%s) with class name "%s")",
            m_element.m_name ? m_element.m_name : "", m_element.m_id.ToString<AZStd::string>().data(), m_classData->m_name,
            parentDataElement->m_element.m_name ? parentDataElement->m_element.m_name : "", parentDataElement->m_element.m_id.ToString<AZStd::string>().data(), parentDataElement->m_classData->m_name);

        if (classElementFound)
        {
            void* dataAddress = nullptr;
            IDataContainer* dataContainer = parentDataElement->m_classData->m_container;
            if (dataContainer) // container elements
            {
                int& parentCurrentContainerElementIndex = nodeStack.back().m_currentContainerElementIndex;
                // add element to the array
                if (dataContainer->CanAccessElementsByIndex() && dataContainer->Size(parentPtr) > parentCurrentContainerElementIndex)
                {
                    dataAddress = dataContainer->GetElementByIndex(parentPtr, &classElement, parentCurrentContainerElementIndex);
                }
                else
                {
                    dataAddress = dataContainer->ReserveElement(parentPtr, &classElement);
                }

                if (dataAddress == nullptr)
                {
                    AZStd::string error = AZStd::string::format("Failed to reserve element in container. The container may be full. Element %u will not be added to container.", static_cast<unsigned int>(parentCurrentContainerElementIndex));
                    errorHandler->ReportError(error.c_str());
                }

                parentCurrentContainerElementIndex++;
            }
            else
            {   // normal elements
                dataAddress = reinterpret_cast<char*>(parentPtr) + classElement.m_offset;
            }

            void* reserveAddress = dataAddress;

            // create a new instance if needed
            if (classElement.m_flags & SerializeContext::ClassElement::FLG_POINTER)
            {
                // create a new instance if we are referencing it by pointer
                AZ_Assert(m_classData->m_factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!", m_classData->m_name, classElement.m_name);
                void* newDataAddress = m_classData->m_factory->Create(m_classData->m_name);

                // we need to account for additional offsets if we have a pointer to
                // a base class.
                void* basePtr = nullptr;
                if (m_element.m_id == classElement.m_typeId)
                {
                    basePtr = newDataAddress;
                }
                else if(m_classData->m_azRtti && classElement.m_azRtti)
                {
                    basePtr = m_classData->m_azRtti->Cast(newDataAddress, classElement.m_azRtti->GetTypeId());
                }
                AZ_Assert(basePtr != nullptr, dataContainer
                    ? "Can't cast container element %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                    : "Can't cast %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                    , m_element.m_name ? m_element.m_name : "NULL"
                    , m_element.m_nameCrc
                    , m_classData->m_name);

                *reinterpret_cast<void**>(dataAddress) = basePtr; // store the pointer in the class
                dataAddress = newDataAddress;
            }

            DataElement& rawElement = m_element;
            if (m_classData->m_serializer && rawElement.m_dataSize != 0)
            {
                rawElement.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                if (rawElement.m_dataType == DataElement::DT_TEXT)
                {
                    // convert to binary so we can load the data
                    AZStd::string text;
                    text.resize_no_construct(rawElement.m_dataSize);
                    rawElement.m_byteStream.Read(text.size(), reinterpret_cast<void*>(text.data()));
                    rawElement.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                    rawElement.m_dataSize = m_classData->m_serializer->TextToData(text.c_str(), rawElement.m_version, rawElement.m_byteStream);
                    rawElement.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                    rawElement.m_dataType = DataElement::DT_BINARY;
                }

                bool isLoaded = m_classData->m_serializer->Load(dataAddress, rawElement.m_byteStream, rawElement.m_version, rawElement.m_dataType == DataElement::DT_BINARY_BE);
                rawElement.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                if (!isLoaded)
                {
                    const AZStd::string error =
                        AZStd::string::format(R"(Failed to load rawElement "%s" for class "%s" with parent element "%s".)",
                            rawElement.m_name ? rawElement.m_name : "<unknown>",
                            m_classData->m_name,
                            m_element.m_name);

                    if (errorHandler)
                    {
                        errorHandler->ReportError(error.c_str());
                    }
                    else
                    {
                        AZ_Error("Serialize", false, "%s", error.c_str());
                    }

                    success = false;
                }
            }

            // Push current DataElementNode to stack
            DataElementInstanceData node;
            node.m_ptr = dataAddress;
            node.m_dataElement = this;
            nodeStack.push_back(node);

            for (int dataElementIndex = 0; dataElementIndex < m_subElements.size(); ++dataElementIndex)
            {
                DataElementNode& subElement = m_subElements[dataElementIndex];
                if (!subElement.GetDataHierarchyEnumerate(errorHandler, nodeStack))
                {
                    success = false;
                    break;
                }
            }

            // Pop stack
            nodeStack.pop_back();

            if (dataContainer)
            {
                dataContainer->StoreElement(parentPtr, reserveAddress);
            }
        }

        return success;
    }

    //=========================================================================
    // GetDataHierarchy
    //=========================================================================
    bool SerializeContext::DataElementNode::GetDataHierarchy(void* objectPtr, const Uuid& classId, ErrorHandler* errorHandler)
    {
        (void)classId;
        AZ_Assert(m_element.m_id == classId, "GetDataHierarchy called with mismatched class type {%s} for element %s",
            classId.ToString<AZStd::string>().c_str(), m_element.m_name);

        NodeStack nodeStack;
        DataElementInstanceData topNode;
        topNode.m_ptr = objectPtr;
        topNode.m_dataElement = this;
        nodeStack.push_back(topNode);

        bool success = true;
        for (size_t i = 0; i < m_subElements.size(); ++i)
        {
            if (!m_subElements[i].GetDataHierarchyEnumerate(errorHandler, nodeStack))
            {
                success = false;
                break;
            }
        }

        return success;
    }

    //=========================================================================
    // ClassBuilder::~ClassBuilder
    //=========================================================================
    SerializeContext::ClassBuilder::~ClassBuilder()
    {
#if defined(AZ_ENABLE_TRACING)
        if (!m_context->IsRemovingReflection())
        {
            if (m_classData->second.m_serializer)
            {
                AZ_Assert(
                    m_classData->second.m_elements.empty(),
                    "Reflection error for class %s.\n"
                    "Classes with custom serializers are not permitted to also declare serialized elements.\n"
                    "This is often caused by calling SerializeWithNoData() or specifying a custom serializer on a class which \n"
                    "is derived from a base class that has serialized elements.",
                    m_classData->second.m_name ? m_classData->second.m_name : "<Unknown Class>");
            }
        }
#endif // AZ_ENABLE_TRACING
    }

    //=========================================================================
    // ClassBuilder::NameChange
    // [4/10/2019]
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::NameChange(unsigned int fromVersion, unsigned int toVersion, AZStd::string_view oldFieldName, AZStd::string_view newFieldName)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data for this class
        }

        AZ_Error("Serialize", !m_classData->second.m_serializer, "Class has a custom serializer, and can not have per-node version upgrades.");
        AZ_Error("Serialize", !m_classData->second.m_elements.empty(), "Class has no defined elements to add per-node version upgrades to.");

        if (m_classData->second.m_serializer || m_classData->second.m_elements.empty())
        {
            return this;
        }

        m_classData->second.m_dataPatchUpgrader.AddFieldUpgrade(aznew DataPatchNameUpgrade(fromVersion, toVersion, oldFieldName, newFieldName));

        return this;
    }

    //=========================================================================
    // ClassBuilder::Version
    // [10/05/2012]
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Version(unsigned int version, VersionConverter converter)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        AZ_Assert(version != VersionClassDeprecated, "You cannot use %u as the version, it is reserved by the system!", version);
        m_classData->second.m_version = version;
        m_classData->second.m_converter = converter;
        return this;
    }

    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Serializer(IDataSerializerPtr serializer)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }

        AZ_Assert(m_classData->second.m_elements.empty(),
            "Class %s has a custom serializer, and can not have additional fields. Classes can either have a custom serializer or child fields.",
            m_classData->second.m_name);

        m_classData->second.m_serializer = AZStd::move(serializer);
        return this;

    }

    //=========================================================================
    // ClassBuilder::Serializer
    // [10/05/2012]
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Serializer(IDataSerializer* serializer)
    {
        return Serializer(IDataSerializerPtr(serializer, IDataSerializer::CreateNoDeleteDeleter()));
    }

    //=========================================================================
    // ClassBuilder::Serializer
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::SerializeWithNoData()
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_serializer = IDataSerializerPtr(&Serialize::StaticInstance<EmptySerializer>::s_instance, IDataSerializer::CreateNoDeleteDeleter());
        return this;
    }

    //=========================================================================
    // ClassBuilder::EventHandler
    // [10/05/2012]
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::EventHandler(IEventHandler* eventHandler)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_eventHandler = eventHandler;
        return this;
    }

    //=========================================================================
    // ClassBuilder::DataContainer
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::DataContainer(IDataContainer* dataContainer)
    {
        if (m_context->IsRemovingReflection())
        {
            return this;
        }
        m_classData->second.m_container = dataContainer;
        return this;
    }

    //=========================================================================
    // ClassBuilder::PersistentId
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::PersistentId(ClassPersistentId persistentId)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_persistentId = persistentId;
        return this;
    }

    //=========================================================================
    // ClassBuilder::SerializerDoSave
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::SerializerDoSave(ClassDoSave doSave)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_doSave = doSave;
        return this;
    }

    //=========================================================================
    // EnumerateInstanceConst
    // [10/31/2012]
    //=========================================================================
    bool SerializeContext::EnumerateInstanceConst(SerializeContext::EnumerateInstanceCallContext* callContext, const void* ptr, const Uuid& classId, const ClassData* classData, const ClassElement* classElement) const
    {
        AZ_Assert((callContext->m_accessFlags & ENUM_ACCESS_FOR_WRITE) == 0, "You are asking the serializer to lock the data for write but you only have a const pointer!");
        return EnumerateInstance(callContext, const_cast<void*>(ptr), classId, classData, classElement);
    }

    //=========================================================================
    // EnumerateInstance
    // [10/31/2012]
    //=========================================================================
    bool SerializeContext::EnumerateInstance(SerializeContext::EnumerateInstanceCallContext* callContext, void* ptr, const Uuid& classId, const ClassData* classData, const ClassElement* classElement) const
    {
        // if useClassData is provided, just use it, otherwise try to find it using the classId provided.
        void* objectPtr = ptr;
        const AZ::Uuid* classIdPtr = &classId;
        const SerializeContext::ClassData* dataClassInfo = classData;

        if (classElement)
        {
            // if we are a pointer, then we may be pointing to a derived type.
            if (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
            {
                // if ptr is a pointer-to-pointer, cast its value to a void* (or const void*) and dereference to get to the actual object pointer.
                objectPtr = *(void**)(ptr);

                if (!objectPtr)
                {
                    return true;    // nothing to serialize
                }
                if (classElement->m_azRtti)
                {
                    const AZ::Uuid& actualClassId = classElement->m_azRtti->GetActualUuid(objectPtr);
                    if (actualClassId != *classIdPtr)
                    {
                        // we are pointing to derived type, adjust class data, uuid and pointer.
                        classIdPtr = &actualClassId;
                        dataClassInfo = FindClassData(actualClassId);
                        if ( (dataClassInfo) && (dataClassInfo->m_azRtti) ) // it could be missing RTTI if its deprecated.
                        {
                            objectPtr = classElement->m_azRtti->Cast(objectPtr, dataClassInfo->m_azRtti->GetTypeId());

                            AZ_Assert(objectPtr, "Potential Data Loss: AZ_RTTI Cast to type %s Failed on element: %s", dataClassInfo->m_name, classElement->m_name);
                            if (!objectPtr)
                            {
                                #if defined (AZ_ENABLE_SERIALIZER_DEBUG)
                                // Additional error information: Provide Type IDs and the serialization stack from our enumeration
                                AZStd::string sourceTypeID = dataClassInfo->m_typeId.ToString<AZStd::string>();
                                AZStd::string targetTypeID = classElement->m_typeId.ToString<AZStd::string>();
                                AZStd::string error = AZStd::string::format("EnumarateElements RTTI Cast Error: %s -> %s", sourceTypeID.c_str(), targetTypeID.c_str());
                                callContext->m_errorHandler->ReportError(error.c_str());
                                #endif
                                return true;    // RTTI Error. Continue serialization
                            }
                        }
                    }
                }
            }
        }

        if (!dataClassInfo)
        {
            dataClassInfo = FindClassData(*classIdPtr);
        }

    #if defined(AZ_ENABLE_SERIALIZER_DEBUG)
        {
            DbgStackEntry de;
            de.m_dataPtr = objectPtr;
            de.m_uuidPtr = classIdPtr;
            de.m_elementName = classElement ? classElement->m_name : nullptr;
            de.m_classData = dataClassInfo;
            de.m_classElement = classElement;
            callContext->m_errorHandler->Push(de);
        }
    #endif // AZ_ENABLE_SERIALIZER_DEBUG

        if (dataClassInfo == nullptr)
        {
    #if defined (AZ_ENABLE_SERIALIZER_DEBUG)
            AZStd::string error;

            // output an error
            if (classElement && classElement->m_flags & SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                error = AZStd::string::format("Element with class ID '%s' was declared as a base class of another type but is not registered with the serializer.  Either remove it from the Class<> call or reflect it.", classIdPtr->ToString<AZStd::string>().c_str());
            }
            else
            {
                error = AZStd::string::format("Element with class ID '%s' is not registered with the serializer!", classIdPtr->ToString<AZStd::string>().c_str());
            }

            callContext->m_errorHandler->ReportError(error.c_str());

            callContext->m_errorHandler->Pop();
    #endif // AZ_ENABLE_SERIALIZER_DEBUG

            return true;    // we errored, but return true to continue enumeration of our siblings and other unrelated hierarchies
        }

        if (dataClassInfo->m_eventHandler)
        {
            if ((callContext->m_accessFlags & ENUM_ACCESS_FOR_WRITE) == ENUM_ACCESS_FOR_WRITE)
            {
                dataClassInfo->m_eventHandler->OnWriteBegin(objectPtr);
            }
            else
            {
                dataClassInfo->m_eventHandler->OnReadBegin(objectPtr);
            }
        }

        bool keepEnumeratingSiblings = true;

        // Call beginElemCB for this element if there is one. If the callback
        // returns false, stop enumeration of this branch
        // pass the original ptr to the user instead of objectPtr because
        // he may want to replace the actual object.
        if (!callContext->m_beginElemCB || callContext->m_beginElemCB(ptr, dataClassInfo, classElement))
        {
            if (dataClassInfo->m_container)
            {
                dataClassInfo->m_container->EnumElements(objectPtr, callContext->m_elementCallback);
            }
            else
            {
                for (size_t i = 0, n = dataClassInfo->m_elements.size(); i < n; ++i)
                {
                    const SerializeContext::ClassElement& ed = dataClassInfo->m_elements[i];
                    void* dataAddress = (char*)(objectPtr) + ed.m_offset;
                    if (dataAddress)
                    {
                        const SerializeContext::ClassData* elemClassInfo = ed.m_genericClassInfo ? ed.m_genericClassInfo->GetClassData() : FindClassData(ed.m_typeId, dataClassInfo, ed.m_nameCrc);

                        keepEnumeratingSiblings = EnumerateInstance(callContext, dataAddress, ed.m_typeId, elemClassInfo, &ed);
                        if (!keepEnumeratingSiblings)
                        {
                            break;
                        }
                    }
                }

                if (dataClassInfo->m_typeId == SerializeTypeInfo<DynamicSerializableField>::GetUuid())
                {
                    AZ::DynamicSerializableField* dynamicFieldDesc = reinterpret_cast<AZ::DynamicSerializableField*>(objectPtr);
                    if (dynamicFieldDesc->IsValid())
                    {
                        const AZ::SerializeContext::ClassData* dynamicTypeMetadata = FindClassData(dynamicFieldDesc->m_typeId);
                        if (dynamicTypeMetadata)
                        {
                            AZ::SerializeContext::ClassElement dynamicElementData;
                            dynamicElementData.m_name = "m_data";
                            dynamicElementData.m_nameCrc = AZ_CRC("m_data", 0x335cc942);
                            dynamicElementData.m_typeId = dynamicFieldDesc->m_typeId;
                            dynamicElementData.m_dataSize = sizeof(void*);
                            dynamicElementData.m_offset = reinterpret_cast<size_t>(&(reinterpret_cast<AZ::DynamicSerializableField const volatile*>(0)->m_data));
                            dynamicElementData.m_azRtti = nullptr; // we won't need this because we always serialize top classes.
                            dynamicElementData.m_genericClassInfo = FindGenericClassInfo(dynamicFieldDesc->m_typeId);
                            dynamicElementData.m_editData = nullptr; // we cannot have element edit data for dynamic fields.
                            dynamicElementData.m_flags = ClassElement::FLG_DYNAMIC_FIELD | ClassElement::FLG_POINTER;
                            EnumerateInstance(callContext, &dynamicFieldDesc->m_data, dynamicTypeMetadata->m_typeId, dynamicTypeMetadata, &dynamicElementData);
                        }
                        else
                        {
                            AZ_Error("Serialization", false, "Failed to find class data for 'Dynamic Serializable Field' with type=%s address=%p. Make sure this type is reflected, \
                                otherwise you will lose data during serialization!\n", dynamicFieldDesc->m_typeId.ToString<AZStd::string>().c_str(), dynamicFieldDesc->m_data);
                        }
                    }
                }
            }
        }

        // call endElemCB
        if (callContext->m_endElemCB)
        {
            keepEnumeratingSiblings = callContext->m_endElemCB();
        }

        if (dataClassInfo->m_eventHandler)
        {
            if ((callContext->m_accessFlags & ENUM_ACCESS_HOLD) == 0)
            {
                if ((callContext->m_accessFlags & ENUM_ACCESS_FOR_WRITE) == ENUM_ACCESS_FOR_WRITE)
                {
                    dataClassInfo->m_eventHandler->OnWriteEnd(objectPtr);
                }
                else
                {
                    dataClassInfo->m_eventHandler->OnReadEnd(objectPtr);
                }
            }
        }

    #if defined(AZ_ENABLE_SERIALIZER_DEBUG)
        callContext->m_errorHandler->Pop();
    #endif // AZ_ENABLE_SERIALIZER_DEBUG

        return keepEnumeratingSiblings;
    }

    //=========================================================================
    // EnumerateInstanceConst (deprecated overload)
    //=========================================================================
    bool SerializeContext::EnumerateInstanceConst(const void* ptr, const Uuid& classId, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, const ClassData* classData, const ClassElement* classElement, ErrorHandler* errorHandler /*= nullptr*/) const
    {
        EnumerateInstanceCallContext callContext(
            beginElemCB,
            endElemCB,
            this,
            accessFlags,
            errorHandler);

        return EnumerateInstanceConst(
            &callContext,
            ptr,
            classId,
            classData,
            classElement
        );
    }

    //=========================================================================
    // EnumerateInstance (deprecated overload)
    //=========================================================================
    bool SerializeContext::EnumerateInstance(void* ptr, const Uuid& classId, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, const ClassData* classData, const ClassElement* classElement, ErrorHandler* errorHandler /*= nullptr*/) const
    {
        EnumerateInstanceCallContext callContext(
            beginElemCB,
            endElemCB,
            this,
            accessFlags,
            errorHandler);

        return EnumerateInstance(
            &callContext,
            ptr,
            classId,
            classData,
            classElement
        );
    }

    struct ObjectCloneData
    {
        ObjectCloneData()
        {
            m_parentStack.reserve(10);
        }
        struct ParentInfo
        {
            void* m_ptr;
            void* m_reservePtr; ///< Used for associative containers like set to store the original address returned by ReserveElement
            const SerializeContext::ClassData* m_classData;
            size_t m_containerIndexCounter; ///< Used for fixed containers like array, where the container doesn't store the size.
        };
        typedef AZStd::vector<ParentInfo> ObjectParentStack;

        void*               m_ptr;
        ObjectParentStack   m_parentStack;
    };

    //=========================================================================
    // CloneObject
    //=========================================================================
    void* SerializeContext::CloneObject(const void* ptr, const Uuid& classId)
    {
        AZStd::vector<char> scratchBuffer;

        ObjectCloneData cloneData;
        ErrorHandler m_errorLogger;

        AZ_Assert(ptr, "SerializeContext::CloneObject - Attempt to clone a nullptr.");

        if (!ptr)
        {
            return nullptr;
        }

        EnumerateInstanceCallContext callContext(
            AZStd::bind(&SerializeContext::BeginCloneElement, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, &cloneData, &m_errorLogger, &scratchBuffer),
            AZStd::bind(&SerializeContext::EndCloneElement, this, &cloneData),
            this,
            SerializeContext::ENUM_ACCESS_FOR_READ,
            &m_errorLogger);

        EnumerateInstance(
            &callContext
            , const_cast<void*>(ptr)
            , classId
            , nullptr
            , nullptr
            );

        return cloneData.m_ptr;
    }

    //=========================================================================
    // CloneObjectInplace
    //=========================================================================
    void SerializeContext::CloneObjectInplace(void* dest, const void* ptr, const Uuid& classId)
    {
        AZStd::vector<char> scratchBuffer;

        ObjectCloneData cloneData;
        cloneData.m_ptr = dest;
        ErrorHandler m_errorLogger;

        AZ_Assert(ptr, "SerializeContext::CloneObjectInplace - Attempt to clone a nullptr.");

        if (ptr)
        {
            EnumerateInstanceCallContext callContext(
            AZStd::bind(&SerializeContext::BeginCloneElementInplace, this, dest, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, &cloneData, &m_errorLogger, &scratchBuffer),
                AZStd::bind(&SerializeContext::EndCloneElement, this, &cloneData),
                this,
                SerializeContext::ENUM_ACCESS_FOR_READ,
                &m_errorLogger);

            EnumerateInstance(
                &callContext
                , const_cast<void*>(ptr)
                , classId
                , nullptr
                , nullptr
            );
        }
    }

    AZ::SerializeContext::DataPatchUpgrade::DataPatchUpgrade(AZStd::string_view fieldName, unsigned int fromVersion, unsigned int toVersion)
        : m_targetFieldName(fieldName)
        , m_targetFieldCRC(m_targetFieldName.data(), m_targetFieldName.size(), true)
        , m_fromVersion(fromVersion)
        , m_toVersion(toVersion)
    {}

    bool AZ::SerializeContext::DataPatchUpgrade::operator==(const DataPatchUpgrade& RHS) const
    {
        return m_upgradeType == RHS.m_upgradeType
            && m_targetFieldCRC == RHS.m_targetFieldCRC
            && m_fromVersion == RHS.m_fromVersion
            && m_toVersion == RHS.m_toVersion;
    }

    bool AZ::SerializeContext::DataPatchUpgrade::operator<(const DataPatchUpgrade& RHS) const
    {
        if (m_fromVersion < RHS.m_fromVersion)
        {
            return true;
        }

        if (m_fromVersion > RHS.m_fromVersion)
        {
            return false;
        }

        // We sort on to version in reverse order
        if (m_toVersion > RHS.m_toVersion)
        {
            return true;
        }

        if (m_toVersion < RHS.m_toVersion)
        {
            return false;
        }

        // When versions are equal, upgrades are prioritized by type in the
        // order in which they appear in the DataPatchUpgradeType enum.
        return m_upgradeType < RHS.m_upgradeType;
    }

    unsigned int AZ::SerializeContext::DataPatchUpgrade::FromVersion() const
    {
        return m_fromVersion;
    }

    unsigned int AZ::SerializeContext::DataPatchUpgrade::ToVersion() const
    {
        return m_toVersion;
    }

    const AZStd::string& AZ::SerializeContext::DataPatchUpgrade::GetFieldName() const
    {
        return m_targetFieldName;
    }

    AZ::Crc32 AZ::SerializeContext::DataPatchUpgrade::GetFieldCRC() const
    {
        return m_targetFieldCRC;
    }

    AZ::SerializeContext::DataPatchUpgradeType AZ::SerializeContext::DataPatchUpgrade::GetUpgradeType() const
    {
        return m_upgradeType;
    }

    AZ::SerializeContext::DataPatchUpgradeHandler::~DataPatchUpgradeHandler()
    {
        for (const auto& fieldUpgrades : m_upgrades)
        {
            for (const auto& versionUpgrades : fieldUpgrades.second)
            {
                for (auto* upgrade : versionUpgrades.second)
                {
                    delete upgrade;
                }
            }
        }
    }

    void AZ::SerializeContext::DataPatchUpgradeHandler::AddFieldUpgrade(DataPatchUpgrade* upgrade)
    {
        // Find the field
        auto fieldUpgrades = m_upgrades.find(upgrade->GetFieldCRC());

        // If we don't have any upgrades for the field, add this item.
        if (fieldUpgrades == m_upgrades.end())
        {
            m_upgrades[upgrade->GetFieldCRC()][upgrade->FromVersion()].insert(upgrade);
        }
        else
        {
            auto versionUpgrades = fieldUpgrades->second.find(upgrade->FromVersion());
            if (versionUpgrades == fieldUpgrades->second.end())
            {
                fieldUpgrades->second[upgrade->FromVersion()].insert(upgrade);
            }
            else
            {
                for (auto* existingUpgrade : versionUpgrades->second)
                {
                    if (*existingUpgrade == *upgrade)
                    {
                        AZ_Assert(false, "Duplicate upgrade to field %s from version %u to version %u", upgrade->GetFieldName().c_str(), upgrade->FromVersion(), upgrade->ToVersion());

                        // In a failure case, delete the upgrade as we've assumed control of it.
                        delete upgrade;
                        return;
                    }
                }

                m_upgrades[upgrade->GetFieldCRC()][upgrade->FromVersion()].insert(upgrade);
            }
        }
    }

    const AZ::SerializeContext::DataPatchFieldUpgrades& AZ::SerializeContext::DataPatchUpgradeHandler::GetUpgrades() const
    {
        return m_upgrades;
    }

    bool AZ::SerializeContext::DataPatchNameUpgrade::operator<(const DataPatchUpgrade& RHS) const
    {
        // If the right side is also a Field Name Upgrade, forward this to the
        // appropriate equivalence operator.
        return DataPatchUpgrade::operator<(RHS);
    }

    bool AZ::SerializeContext::DataPatchNameUpgrade::operator<(const DataPatchNameUpgrade& RHS) const
    {
        // The default operator is fine for name upgrades
        return DataPatchUpgrade::operator<(RHS);
    }

    void AZ::SerializeContext::DataPatchNameUpgrade::Apply(AZ::SerializeContext& context, SerializeContext::DataElementNode& node) const
    {
        AZ_UNUSED(context);

        int targetElementIndex = node.FindElement(m_targetFieldCRC);

        AZ_Assert(targetElementIndex >= 0, "Invalid node. Field %s is not a valid element of class %s (Version %u). Check your reflection function.", m_targetFieldName.c_str(), node.GetNameString(), node.GetVersion());

        if (targetElementIndex >= 0)
        {
            auto& targetElement = node.GetSubElement(targetElementIndex);
            targetElement.SetName(m_newNodeName.c_str());
        }
    }

    AZStd::string AZ::SerializeContext::DataPatchNameUpgrade::GetNewName() const
    {
        return m_newNodeName;
    }

    //=========================================================================
    // BeginCloneElement (internal element clone callbacks)
    //=========================================================================
    bool SerializeContext::BeginCloneElement(void* ptr, const ClassData* classData, const ClassElement* elementData, void* data, ErrorHandler* errorHandler, AZStd::vector<char>* scratchBuffer)
    {
        ObjectCloneData* cloneData = reinterpret_cast<ObjectCloneData*>(data);

        if (cloneData->m_parentStack.empty())
        {
            // Since this is the root element, we will need to allocate it using the creator provided
            AZ_Assert(classData->m_factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!", classData->m_name, elementData->m_name);
            cloneData->m_ptr = classData->m_factory->Create(classData->m_name);
        }

        return BeginCloneElementInplace(cloneData->m_ptr, ptr, classData, elementData, data, errorHandler, scratchBuffer);
    }

    //=========================================================================
    // BeginCloneElementInplace (internal element clone callbacks)
    //=========================================================================
    bool SerializeContext::BeginCloneElementInplace(void* rootDestPtr, void* ptr, const ClassData* classData, const ClassElement* elementData, void* data, ErrorHandler* errorHandler, AZStd::vector<char>* scratchBuffer)
    {
        ObjectCloneData* cloneData = reinterpret_cast<ObjectCloneData*>(data);

        void* srcPtr = ptr;
        void* destPtr = nullptr;
        void* reservePtr = nullptr;

        if (classData->m_version == VersionClassDeprecated)
        {
            if (classData->m_converter)
            {
                AZ_Assert(false, "A deprecated element with a data converter was passed to CloneObject, this is not supported.");
            }
            // push a dummy node in the stack
            cloneData->m_parentStack.push_back();
            ObjectCloneData::ParentInfo& parentInfo = cloneData->m_parentStack.back();
            parentInfo.m_ptr = destPtr;
            parentInfo.m_reservePtr = reservePtr;
            parentInfo.m_classData = classData;
            parentInfo.m_containerIndexCounter = 0;
            return false;    // do not iterate further.
        }


        if (!cloneData->m_parentStack.empty())
        {
            AZ_Assert(elementData, "Non-root nodes need to have a valid elementData!");
            ObjectCloneData::ParentInfo& parentInfo = cloneData->m_parentStack.back();
            void* parentPtr = parentInfo.m_ptr;
            const ClassData* parentClassData = parentInfo.m_classData;
            if (parentClassData->m_container)
            {
                if (parentClassData->m_container->CanAccessElementsByIndex() && parentClassData->m_container->Size(parentPtr) > parentInfo.m_containerIndexCounter)
                {
                    destPtr = parentClassData->m_container->GetElementByIndex(parentPtr, elementData, parentInfo.m_containerIndexCounter);
                }
                else
                {
                    destPtr = parentClassData->m_container->ReserveElement(parentPtr, elementData);
                }

                ++parentInfo.m_containerIndexCounter;
            }
            else
            {
                // Allocate memory for our element using the creator provided
                destPtr = reinterpret_cast<char*>(parentPtr) + elementData->m_offset;
            }

            reservePtr = destPtr;
            if (elementData->m_flags & ClassElement::FLG_POINTER)
            {
                AZ_Assert(classData->m_factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide a factory or change data member '%s' to value not pointer!", classData->m_name, elementData->m_name);
                void* newElement = classData->m_factory->Create(classData->m_name);
                void* basePtr = DownCast(newElement, classData->m_typeId, elementData->m_typeId, classData->m_azRtti, elementData->m_azRtti);
                *reinterpret_cast<void**>(destPtr) = basePtr; // store the pointer in the class
                destPtr = newElement;
            }

            if (!destPtr && errorHandler)
            {
                AZStd::string error = AZStd::string::format("Failed to reserve element in container. The container may be full. Element %u will not be added to container.", static_cast<unsigned int>(parentInfo.m_containerIndexCounter));
                errorHandler->ReportError(error.c_str());
            }
        }
        else
        {
            destPtr = rootDestPtr;
            reservePtr = rootDestPtr;
        }

        if (!destPtr)
        {
            // There is no valid destination pointer so a dummy node is added to the clone data parent stack
            // and further descendent type iteration is halted
            // An error has been reported to the supplied errorHandler in the code above
            cloneData->m_parentStack.push_back();
            ObjectCloneData::ParentInfo& parentInfo = cloneData->m_parentStack.back();
            parentInfo.m_ptr = destPtr;
            parentInfo.m_reservePtr = reservePtr;
            parentInfo.m_classData = classData;
            parentInfo.m_containerIndexCounter = 0;
            return false;
        }

        if (elementData && elementData->m_flags & ClassElement::FLG_POINTER)
        {
            // if ptr is a pointer-to-pointer, cast its value to a void* (or const void*) and dereference to get to the actual object pointer.
            srcPtr = *(void**)(ptr);
            if (elementData->m_azRtti)
            {
                srcPtr = elementData->m_azRtti->Cast(srcPtr, classData->m_azRtti->GetTypeId());
            }
        }

        if (classData->m_eventHandler)
        {
            classData->m_eventHandler->OnWriteBegin(destPtr);
        }

        if (classData->m_serializer)
        {
            if (classData->m_typeId == GetAssetClassId())
            {
                // Optimized clone path for asset references.
                static_cast<AssetSerializer*>(classData->m_serializer.get())->Clone(srcPtr, destPtr);
            }
            else
            {
                scratchBuffer->clear();
                IO::ByteContainerStream<AZStd::vector<char>> stream(scratchBuffer);

                classData->m_serializer->Save(srcPtr, stream);
                stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

                classData->m_serializer->Load(destPtr, stream, classData->m_version);
            }
        }

        // If it is a container, clear it before loading the child
        // nodes, otherwise we end up with more elements than the ones
        // we should have
        if (classData->m_container)
        {
            classData->m_container->ClearElements(destPtr, this);
        }

        // push this node in the stack
        cloneData->m_parentStack.push_back();
        ObjectCloneData::ParentInfo& parentInfo = cloneData->m_parentStack.back();
        parentInfo.m_ptr = destPtr;
        parentInfo.m_reservePtr = reservePtr;
        parentInfo.m_classData = classData;
        parentInfo.m_containerIndexCounter = 0;
        return true;
    }

    //=========================================================================
    // EndCloneElement (internal element clone callbacks)
    //=========================================================================
    bool SerializeContext::EndCloneElement(void* data)
    {
        ObjectCloneData* cloneData = reinterpret_cast<ObjectCloneData*>(data);
        void* dataPtr = cloneData->m_parentStack.back().m_ptr;
        void* reservePtr = cloneData->m_parentStack.back().m_reservePtr;

        if (!dataPtr)
        {
            // we failed to clone an object - an assertion was already raised if it needed to be.
            cloneData->m_parentStack.pop_back();
            return true; // continue on to siblings.
        }

        const ClassData* classData = cloneData->m_parentStack.back().m_classData;
        if (classData->m_eventHandler)
        {
            classData->m_eventHandler->OnWriteEnd(dataPtr);
            classData->m_eventHandler->OnObjectCloned(dataPtr);
        }

        if (classData->m_serializer)
        {
            classData->m_serializer->PostClone(dataPtr);
        }

        cloneData->m_parentStack.pop_back();
        if (!cloneData->m_parentStack.empty())
        {
            const ClassData* parentClassData = cloneData->m_parentStack.back().m_classData;
            if (parentClassData->m_container)
            {
                // Pass in the address returned by IDataContainer::ReserveElement.
                //AZStdAssociativeContainer is the only DataContainer that uses the second argument passed into IDataContainer::StoreElement
                parentClassData->m_container->StoreElement(cloneData->m_parentStack.back().m_ptr, reservePtr);
            }
        }
        return true;
    }

    //=========================================================================
    // EnumerateDerived
    // [11/13/2012]
    //=========================================================================
    void SerializeContext::EnumerateDerived(const TypeInfoCB& callback, const Uuid& classId, const Uuid& typeId) const
    {
        // right now this function is SLOW, traverses all serialized types. If we need faster
        // we will need to cache/store derived type in the base type.
        for (SerializeContext::UuidToClassMap::const_iterator it = m_uuidMap.begin(); it != m_uuidMap.end(); ++it)
        {
            const ClassData& cd = it->second;

            if (cd.m_typeId == classId)
            {
                continue;
            }

            if (cd.m_azRtti && typeId != Uuid::CreateNull())
            {
                if (cd.m_azRtti->IsTypeOf(typeId))
                {
                    if (!callback(&cd, nullptr))
                    {
                        return;
                    }
                }
            }

            if (!classId.IsNull())
            {
                for (size_t i = 0; i < cd.m_elements.size(); ++i)
                {
                    if ((cd.m_elements[i].m_flags & ClassElement::FLG_BASE_CLASS) != 0)
                    {
                        if (cd.m_elements[i].m_typeId == classId)
                        {
                            // if both classes have azRtti they will be enumerated already by the code above (azrtti)
                            if (cd.m_azRtti == nullptr || cd.m_elements[i].m_azRtti == nullptr)
                            {
                                if (!callback(&cd, nullptr))
                                {
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //=========================================================================
    // EnumerateBase
    // [11/13/2012]
    //=========================================================================
    void SerializeContext::EnumerateBase(const TypeInfoCB& callback, const Uuid& classId)
    {
        const ClassData* cd = FindClassData(classId);
        if (cd)
        {
            EnumerateBaseRTTIEnumCallbackData callbackData;
            callbackData.m_callback = &callback;
            callbackData.m_reportedTypes.push_back(cd->m_typeId);
            for (size_t i = 0; i < cd->m_elements.size(); ++i)
            {
                if ((cd->m_elements[i].m_flags & ClassElement::FLG_BASE_CLASS) != 0)
                {
                    const ClassData* baseClassData = FindClassData(cd->m_elements[i].m_typeId);
                    if (baseClassData)
                    {
                        callbackData.m_reportedTypes.push_back(baseClassData->m_typeId);
                        if (!callback(baseClassData, nullptr))
                        {
                            return;
                        }
                    }
                }
            }
            if (cd->m_azRtti)
            {
                cd->m_azRtti->EnumHierarchy(&SerializeContext::EnumerateBaseRTTIEnumCallback, &callbackData);
            }
        }
    }

    //=========================================================================
    // EnumerateAll
    //=========================================================================
    void SerializeContext::EnumerateAll(const TypeInfoCB& callback, bool includeGenerics) const
    {
        for (auto& uuidToClassPair : m_uuidMap)
        {
            const ClassData& classData = uuidToClassPair.second;
            if (!callback(&classData, classData.m_typeId))
            {
                return;
            }
        }

        if (includeGenerics)
        {
            for (auto& uuidToGenericPair : m_uuidGenericMap)
            {
                const ClassData* classData = uuidToGenericPair.second->GetClassData();
                if (classData)
                {
                    if (!callback(classData, classData->m_typeId))
                    {
                        return;
                    }
                }
            }
        }
    }

    void SerializeContext::RegisterDataContainer(AZStd::unique_ptr<IDataContainer> dataContainer)
    {
        m_dataContainers.push_back(AZStd::move(dataContainer));
    }

    //=========================================================================
    // EnumerateBaseRTTIEnumCallback
    // [11/13/2012]
    //=========================================================================
    void SerializeContext::EnumerateBaseRTTIEnumCallback(const Uuid& id, void* userData)
    {
        EnumerateBaseRTTIEnumCallbackData* callbackData = reinterpret_cast<EnumerateBaseRTTIEnumCallbackData*>(userData);
        // if not reported, report
        if (AZStd::find(callbackData->m_reportedTypes.begin(), callbackData->m_reportedTypes.end(), id) == callbackData->m_reportedTypes.end())
        {
            (*callbackData->m_callback)(nullptr, id);
        }
    }

    //=========================================================================
    // ClassData
    //=========================================================================
    SerializeContext::ClassData::ClassData()
    {
        m_name = nullptr;
        m_typeId = Uuid::CreateNull();
        m_version = 0;
        m_converter = nullptr;
        m_serializer = nullptr;
        m_factory = nullptr;
        m_persistentId = nullptr;
        m_doSave = nullptr;
        m_eventHandler = nullptr;
        m_container = nullptr;
        m_azRtti = nullptr;
        m_editData = nullptr;
    }

    //=========================================================================
    // ClassData
    //=========================================================================
    void SerializeContext::ClassData::ClearAttributes()
    {
        m_attributes.clear();

        for (ClassElement& classElement : m_elements)
        {
            if (classElement.m_attributeOwnership == ClassElement::AttributeOwnership::Parent)
            {
                classElement.ClearAttributes();
            }
        }
    }

    SerializeContext::ClassPersistentId SerializeContext::ClassData::GetPersistentId(const SerializeContext& context) const
    {
        ClassPersistentId persistentIdFunction = m_persistentId;
        if (!persistentIdFunction)
        {
            // check the base classes
            for (const SerializeContext::ClassElement& element : m_elements)
            {
                if (element.m_flags & ClassElement::FLG_BASE_CLASS)
                {
                    const SerializeContext::ClassData* baseClassData = context.FindClassData(element.m_typeId);
                    if (baseClassData)
                    {
                        persistentIdFunction = baseClassData->GetPersistentId(context);
                        if (persistentIdFunction)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    // base classes are in the beginning of the array
                    break;
                }
            }
        }
        return persistentIdFunction;
    }

    Attribute* SerializeContext::ClassData::FindAttribute(AttributeId attributeId) const
    {
        for (const AZ::AttributeSharedPair& attributePair : m_attributes)
        {
            if (attributePair.first == attributeId)
            {
                return attributePair.second.get();
            }
        }
        return nullptr;
    }

    bool SerializeContext::ClassData::CanConvertFromType(const TypeId& convertibleTypeId, AZ::SerializeContext& serializeContext) const
    {
        // If the convertible type is exactly the type being stored by the ClassData.
        // True will always be returned in this case
        if (convertibleTypeId == m_typeId)
        {
            return true;
        }

        return m_dataConverter ? m_dataConverter->CanConvertFromType(convertibleTypeId, *this, serializeContext) : false;
    }

    bool SerializeContext::ClassData::ConvertFromType(void*& convertibleTypePtr, const TypeId& convertibleTypeId, void* classPtr, AZ::SerializeContext& serializeContext) const
    {
        // If the convertible type is exactly the type being stored by the ClassData.
        // the result convertTypePtr is equal to the classPtr
        if (convertibleTypeId == m_typeId)
        {
            convertibleTypePtr = classPtr;
            return true;
        }

        return m_dataConverter ? m_dataConverter->ConvertFromType(convertibleTypePtr, convertibleTypeId, classPtr, *this, serializeContext) : false;
    }

    //=========================================================================
    // ToString
    // [11/1/2012]
    //=========================================================================
    void SerializeContext::DbgStackEntry::ToString(AZStd::string& str) const
    {
        str += "[";
        if (m_elementName)
        {
            str += AZStd::string::format(" Element: '%s' of", m_elementName);
        }
        //if( m_classElement )
        //  str += AZStd::string::format(" Offset: %d",m_classElement->m_offset);
        if (m_classData)
        {
            str += AZStd::string::format(" Class: '%s' Version: %d", m_classData->m_name, m_classData->m_version);
        }
        str += AZStd::string::format(" Address: %p Uuid: %s", m_dataPtr, m_uuidPtr->ToString<AZStd::string>().c_str());
        str += " ]\n";
    }

    //=========================================================================
    // FreeElementPointer
    // [12/7/2012]
    //=========================================================================
    void SerializeContext::IDataContainer::DeletePointerData(SerializeContext* context, const ClassElement* classElement, const void* element)
    {
        AZ_Assert(context != nullptr && classElement != nullptr && element != nullptr, "Invalid input");
        const AZ::Uuid* elemUuid = &classElement->m_typeId;
        // find the class data for the specific element
        const SerializeContext::ClassData* classData = classElement->m_genericClassInfo ? classElement->m_genericClassInfo->GetClassData() : context->FindClassData(*elemUuid, nullptr, 0);
        if (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
        {
            const void* dataPtr = *reinterpret_cast<void* const*>(element);
            // if dataAddress is a pointer in this case, cast it's value to a void* (or const void*) and dereference to get to the actual class.
            if (dataPtr && classElement->m_azRtti)
            {
                const AZ::Uuid* actualClassId = &classElement->m_azRtti->GetActualUuid(dataPtr);
                if (*actualClassId != *elemUuid)
                {
                    // we are pointing to derived type, adjust class data, uuid and pointer.
                    classData = context->FindClassData(*actualClassId, nullptr, 0);
                    elemUuid = actualClassId;
                    if (classData)
                    {
                        dataPtr = classElement->m_azRtti->Cast(dataPtr, classData->m_azRtti->GetTypeId());
                    }
                }
            }
        }
        if (classData == nullptr)
        {
            if ((classElement->m_flags & ClassElement::FLG_POINTER) != 0)
            {
                const void* dataPtr = *reinterpret_cast<void* const*>(element);
                AZ_UNUSED(dataPtr); // this prevents a L4 warning if the below line is stripped out in release.
                AZ_Warning("Serialization", false, "Failed to find class id%s for %p! Memory could leak.", elemUuid->ToString<AZStd::string>().c_str(), dataPtr);
            }
            return;
        }

        if (classData->m_container)  // if element is container forward the message
        {
            // clear all container data
            classData->m_container->ClearElements(const_cast<void*>(element), context);
        }
        else
        {
            if ((classElement->m_flags & ClassElement::FLG_POINTER) == 0)
            {
                return; // element is stored by value nothing to free
            }

            // if we get here, its a FLG_POINTER
            const void* dataPtr = *reinterpret_cast<void* const*>(element);
            if (classData->m_factory)
            {
                classData->m_factory->Destroy(dataPtr);
            }
            else
            {
                AZ_Warning("Serialization", false, "Failed to delete %p '%s' element, no destructor is provided! Memory could leak.", dataPtr, classData->m_name);
            }
        }
    }

    //=========================================================================
    // RemoveClassData
    //=========================================================================
    void SerializeContext::RemoveClassData(ClassData* classData)
    {
        if (m_editContext)
        {
            m_editContext->RemoveClassData(classData);
        }
    }

    void SerializeContext::RemoveGenericClassInfo(GenericClassInfo* genericClassInfo)
    {
        const Uuid& classId = genericClassInfo->GetSpecializedTypeId();
        RemoveClassData(genericClassInfo->GetClassData());
        // Find the module GenericClassInfo in the SerializeContext GenericClassInfo multimap and remove it from there
        auto scGenericClassInfoRange = m_uuidGenericMap.equal_range(classId);
        auto scGenericInfoFoundIt = AZStd::find_if(scGenericClassInfoRange.first, scGenericClassInfoRange.second, [genericClassInfo](const AZStd::pair<AZ::Uuid, GenericClassInfo*>& genericPair)
        {
            return genericClassInfo == genericPair.second;
        });

        if (scGenericInfoFoundIt != scGenericClassInfoRange.second)
        {
            m_uuidGenericMap.erase(scGenericInfoFoundIt);
            if (m_uuidGenericMap.count(classId) == 0)
            {
                m_uuidAnyCreationMap.erase(classId);
                auto classNameRange = m_classNameToUuid.equal_range(Crc32(genericClassInfo->GetClassData()->m_name));
                for (auto classNameRangeIter = classNameRange.first; classNameRangeIter != classNameRange.second;)
                {
                    if (classNameRangeIter->second == classId)
                    {
                        classNameRangeIter = m_classNameToUuid.erase(classNameRangeIter);
                    }
                    else
                    {
                        ++classNameRangeIter;
                    }
                }

                auto legacyTypeIdRangeIt = m_legacySpecializeTypeIdToTypeIdMap.equal_range(genericClassInfo->GetLegacySpecializedTypeId());
                for (auto legacySpecializedTypeIdIt = legacyTypeIdRangeIt.first; legacySpecializedTypeIdIt != legacyTypeIdRangeIt.second; ++legacySpecializedTypeIdIt)
                {
                    if (classId == legacySpecializedTypeIdIt->second)
                    {
                        m_legacySpecializeTypeIdToTypeIdMap.erase(classId);
                        break;
                    }
                }
            }
        }
    }

    //=========================================================================
    // GetStackDescription
    //=========================================================================
    AZStd::string SerializeContext::ErrorHandler::GetStackDescription() const
    {
        AZStd::string stackDescription;

    #ifdef AZ_ENABLE_SERIALIZER_DEBUG
        if (!m_stack.empty())
        {
            stackDescription += "\n=== Serialize stack ===\n";
            for (size_t i = 0; i < m_stack.size(); ++i)
            {
                m_stack[i].ToString(stackDescription);
            }
            stackDescription += "\n";
        }
    #endif // AZ_ENABLE_SERIALIZER_DEBUG

        return stackDescription;
    }

    //=========================================================================
    // ReportError
    // [12/11/2012]
    //=========================================================================
    void SerializeContext::ErrorHandler::ReportError(const char* message)
    {
        (void)message;
        AZ_Error("Serialize", false, "%s\n%s", message, GetStackDescription().c_str());
        m_nErrors++;
    }

    //=========================================================================
    // ReportWarning
    //=========================================================================
    void SerializeContext::ErrorHandler::ReportWarning(const char* message)
    {
        (void)message;
        AZ_Warning("Serialize", false, "%s\n%s", message, GetStackDescription().c_str());
        m_nWarnings++;
    }

    //=========================================================================
    // Push
    // [1/3/2013]
    //=========================================================================
    void SerializeContext::ErrorHandler::Push(const DbgStackEntry& de)
    {
        (void)de;
    #ifdef AZ_ENABLE_SERIALIZER_DEBUG
        m_stack.push_back((de));
    #endif // AZ_ENABLE_SERIALIZER_DEBUG
    }

    //=========================================================================
    // Pop
    // [1/3/2013]
    //=========================================================================
    void SerializeContext::ErrorHandler::Pop()
    {
    #ifdef AZ_ENABLE_SERIALIZER_DEBUG
        m_stack.pop_back();
    #endif // AZ_ENABLE_SERIALIZER_DEBUG
    }

    //=========================================================================
    // Reset
    // [1/23/2013]
    //=========================================================================
    void SerializeContext::ErrorHandler::Reset()
    {
    #ifdef AZ_ENABLE_SERIALIZER_DEBUG
        m_stack.clear();
    #endif // AZ_ENABLE_SERIALIZER_DEBUG
        m_nErrors = 0;
    }

    //=========================================================================
    // EnumerateInstanceCallContext
    //=========================================================================

    SerializeContext::EnumerateInstanceCallContext::EnumerateInstanceCallContext(
        const SerializeContext::BeginElemEnumCB& beginElemCB,
        const SerializeContext::EndElemEnumCB& endElemCB,
        const SerializeContext* context,
        unsigned int accessFlags,
        SerializeContext::ErrorHandler* errorHandler)
        : m_beginElemCB(beginElemCB)
        , m_endElemCB(endElemCB)
        , m_accessFlags(accessFlags)
        , m_context(context)
    {
        m_errorHandler = errorHandler ? errorHandler : &m_defaultErrorHandler;

        m_elementCallback = AZStd::bind(static_cast<bool (SerializeContext::*)(EnumerateInstanceCallContext*, void*, const Uuid&, const ClassData*, const ClassElement*) const>(&SerializeContext::EnumerateInstance)
            , m_context
            , this
            , AZStd::placeholders::_1
            , AZStd::placeholders::_2
            , AZStd::placeholders::_3
            , AZStd::placeholders::_4
        );
    }

    //=========================================================================
    // ~ClassElement
    //=========================================================================
    SerializeContext::ClassElement::~ClassElement()
    {
        if (m_attributeOwnership == AttributeOwnership::Self)
        {
            ClearAttributes();
        }
    }

    //=========================================================================
    // ClassElement::operator=
    //=========================================================================
    SerializeContext::ClassElement& SerializeContext::ClassElement::operator=(const SerializeContext::ClassElement& other)
    {
        m_name = other.m_name;
        m_nameCrc = other.m_nameCrc;
        m_typeId = other.m_typeId;
        m_dataSize = other.m_dataSize;
        m_offset = other.m_offset;

        m_azRtti = other.m_azRtti;
        m_genericClassInfo = other.m_genericClassInfo;
        m_editData = other.m_editData;
        m_attributes = other.m_attributes;
        // If we're a copy, we don't assume attribute ownership
        m_attributeOwnership = AttributeOwnership::None;
        m_flags = other.m_flags;

        return *this;
    }

    //=========================================================================
    // ClearAttributes
    //=========================================================================
    void SerializeContext::ClassElement::ClearAttributes()
    {
        m_attributes.clear();
    }

    //=========================================================================
    // FindAttribute
    //=========================================================================

    Attribute* SerializeContext::ClassElement::FindAttribute(AttributeId attributeId) const
    {
        for (const AZ::AttributeSharedPair& attributePair : m_attributes)
        {
            if (attributePair.first == attributeId)
            {
                return attributePair.second.get();
            }
        }
        return nullptr;
    }

    void SerializeContext::IDataContainer::ElementsUpdated(void* instance)
    {
        (void)instance;
    }

    void Internal::AZStdArrayEvents::OnWriteBegin(void* classPtr)
    {
        (void)classPtr;
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // Pointer is already in use to store an index so convert it to a stack
                size_t previousIndex = reinterpret_cast<uintptr_t>(m_indices) >> 1;
                Stack* stack = new Stack();
                AZ_Assert((reinterpret_cast<uintptr_t>(stack) & 1) == 0, "Expected memory allocation to be at least 2 byte aligned.");
                stack->push(previousIndex);
                stack->push(0);
                m_indices = stack;
            }
            else
            {
                Stack* stack = reinterpret_cast<Stack*>(m_indices);
                stack->push(0);
            }
        }
        else
        {
            // Use the pointer to just store the one counter instead of allocating memory. Using 1 bit to identify this as a regular
            // index and not a pointer.
            m_indices = reinterpret_cast<void*>(1);
        }
    }

    void Internal::AZStdArrayEvents::OnWriteEnd(void* classPtr)
    {
        (void)classPtr;
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // There was only one entry so no stack. Clear out the final bit that indicated this was an index and not a pointer.
                m_indices = nullptr;
            }
            else
            {
                Stack* stack = reinterpret_cast<Stack*>(m_indices);
                stack->pop();
                if (stack->empty())
                {
                    delete stack;
                    m_indices = nullptr;
                }
            }
        }
        else
        {
            AZ_Warning("Serialization", false, "AZStdArrayEvents::OnWriteEnd called too often.");
        }

    }

    size_t Internal::AZStdArrayEvents::GetIndex() const
    {
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // The first bit is used to indicate this is a regular index instead of a pointer so shift down one to get the actual index.
                return reinterpret_cast<uintptr_t>(m_indices) >> 1;
            }
            else
            {
                const Stack* stack = reinterpret_cast<const Stack*>(m_indices);
                return stack->top();
            }
        }
        else
        {
            AZ_Warning("Serialization", false, "AZStdArrayEvents is not in a valid state to return an index.");
            return 0;
        }
    }

    void Internal::AZStdArrayEvents::Increment()
    {
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // Increment by 2 because the first bit is used to indicate whether or not a stack is used so the real
                //      value starts one bit later.
                size_t index = reinterpret_cast<uintptr_t>(m_indices) + (1 << 1);
                m_indices = reinterpret_cast<void*>(index);
            }
            else
            {
                Stack* stack = reinterpret_cast<Stack*>(m_indices);
                stack->top()++;
            }
        }
        else
        {
            AZ_Warning("Serialization", false, "AZStdArrayEvents is not in a valid state to increment.");
        }
    }

    void Internal::AZStdArrayEvents::Decrement()
    {
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // Decrement by 2 because the first bit is used to indicate whether or not a stack is used so the real
                //      value starts one bit later. This assumes that index is check to be larger than 0 before calling
                //      this function.
                size_t index = reinterpret_cast<uintptr_t>(m_indices) - (1 << 1);
                m_indices = reinterpret_cast<void*>(index);
            }
            else
            {
                Stack* stack = reinterpret_cast<Stack*>(m_indices);
                stack->top()--;
            }
        }
        else
        {
            AZ_Warning("Serialization", false, "AZStdArrayEvents is not in a valid state to decrement.");
        }
    }

    bool Internal::AZStdArrayEvents::IsEmpty() const
    {
        return m_indices == nullptr;
    }

    bool SerializeContext::IsTypeReflected(AZ::Uuid typeId) const
    {
        const AZ::SerializeContext::ClassData* reflectedClassData = FindClassData(typeId);
        return (reflectedClassData != nullptr);
    }

    // Create the member OSAllocator and construct the unordered_map with that allocator
    SerializeContext::PerModuleGenericClassInfo::PerModuleGenericClassInfo()
        : m_moduleLocalGenericClassInfos(AZ::AZStdIAllocator(&m_moduleOSAllocator))
        , m_serializeContextSet(AZ::AZStdIAllocator(&m_moduleOSAllocator))
    {
    }

    SerializeContext::PerModuleGenericClassInfo::~PerModuleGenericClassInfo()
    {
        Cleanup();

        // Reconstructs the module generic info map with the OSAllocator so that it the previous allocated memory is cleared
        // Afterwards destroy the OSAllocator
        {
            m_moduleLocalGenericClassInfos = GenericInfoModuleMap(AZ::AZStdIAllocator(&m_moduleOSAllocator));
            m_serializeContextSet = SerializeContextSet(AZ::AZStdIAllocator(&m_moduleOSAllocator));
        }
    }

    void SerializeContext::PerModuleGenericClassInfo::Cleanup()
    {
        decltype(m_moduleLocalGenericClassInfos) genericClassInfoContainer = AZStd::move(m_moduleLocalGenericClassInfos);
        decltype(m_serializeContextSet) serializeContextSet = AZStd::move(m_serializeContextSet);
        // Un-reflect GenericClassInfo from each serialize context registered with the module
        // The SerailizeContext uses the SystemAllocator so it is required to be ready in order to remove the data
        if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
        {
            for (AZ::SerializeContext* serializeContext : serializeContextSet)
            {
                for (const AZStd::pair<AZ::Uuid, AZ::GenericClassInfo*>& moduleGenericClassInfoPair : genericClassInfoContainer)
                {
                    serializeContext->RemoveGenericClassInfo(moduleGenericClassInfoPair.second);
                }

                serializeContext->m_perModuleSet.erase(this);
            }
        }

        // Cleanup the memory for the GenericClassInfo objects.
        // This isn't explicitly needed as the OSAllocator owned by this class will take the memory with it.
        for (const AZStd::pair<AZ::Uuid, AZ::GenericClassInfo*>& moduleGenericClassInfoPair : genericClassInfoContainer)
        {
            GenericClassInfo* genericClassInfo = moduleGenericClassInfoPair.second;
            // Explicitly invoke the destructor and clear the memory from the module OSAllocator
            genericClassInfo->~GenericClassInfo();
            m_moduleOSAllocator.DeAllocate(genericClassInfo);
        }
    }

    void SerializeContext::PerModuleGenericClassInfo::RegisterSerializeContext(AZ::SerializeContext* serializeContext)
    {
        m_serializeContextSet.emplace(serializeContext);
        serializeContext->m_perModuleSet.emplace(this);
    }

    void SerializeContext::PerModuleGenericClassInfo::UnregisterSerializeContext(AZ::SerializeContext* serializeContext)
    {
        m_serializeContextSet.erase(serializeContext);
        serializeContext->m_perModuleSet.erase(this);
        for (const AZStd::pair<AZ::Uuid, AZ::GenericClassInfo*>& moduleGenericClassInfoPair : m_moduleLocalGenericClassInfos)
        {
            serializeContext->RemoveGenericClassInfo(moduleGenericClassInfoPair.second);
        }
    }

    void SerializeContext::PerModuleGenericClassInfo::AddGenericClassInfo(AZ::GenericClassInfo* genericClassInfo)
    {
        if (!genericClassInfo)
        {
            AZ_Error("SerializeContext", false, "The supplied generic class info object is nullptr. It cannot be added to the SerializeContext module structure");
            return;
        }

        m_moduleLocalGenericClassInfos.emplace(genericClassInfo->GetSpecializedTypeId(), genericClassInfo);
    }

    void SerializeContext::PerModuleGenericClassInfo::RemoveGenericClassInfo(const AZ::TypeId& genericTypeId)
    {
        if (genericTypeId.IsNull())
        {
            AZ_Error("SerializeContext", false, "The supplied generic typeidis invalid. It is not stored the SerializeContext module structure ");
            return;
        }

        auto genericClassInfoFoundIt = m_moduleLocalGenericClassInfos.find(genericTypeId);
        if (genericClassInfoFoundIt != m_moduleLocalGenericClassInfos.end())
        {
            m_moduleLocalGenericClassInfos.erase(genericClassInfoFoundIt);
        }
    }

    AZ::GenericClassInfo* SerializeContext::PerModuleGenericClassInfo::FindGenericClassInfo(const AZ::TypeId& genericTypeId) const
    {
        auto genericClassInfoFoundIt = m_moduleLocalGenericClassInfos.find(genericTypeId);
        return genericClassInfoFoundIt != m_moduleLocalGenericClassInfos.end() ? genericClassInfoFoundIt->second : nullptr;
    }

    AZ::IAllocatorAllocate& SerializeContext::PerModuleGenericClassInfo::GetAllocator()
    {
        return m_moduleOSAllocator;
    }

    // Take advantage of static variables being unique per dll module to clean up module specific registered classes when the module unloads
    SerializeContext::PerModuleGenericClassInfo& GetCurrentSerializeContextModule()
    {
        static SerializeContext::PerModuleGenericClassInfo s_ModuleCleanupInstance;
        return s_ModuleCleanupInstance;
    }

    SerializeContext::IDataSerializerDeleter SerializeContext::IDataSerializer::CreateDefaultDeleteDeleter()
    {
        return [](IDataSerializer* ptr)
        {
            delete ptr;
        };
    }
    SerializeContext::IDataSerializerDeleter SerializeContext::IDataSerializer::CreateNoDeleteDeleter()
    {
        return [](IDataSerializer*)
        {
        };
    }


} // namespace AZ
