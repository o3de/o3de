/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/variant.h>

namespace AZ
{
    namespace VariantSerializationInternal
    {
        template <class ValueType>
        void SetupClassElementFromType(SerializeContext::ClassElement& classElement)
        {
            using ValueTypeNoPointer = AZStd::remove_pointer_t<ValueType>;

            classElement.m_dataSize = sizeof(ValueType);
            classElement.m_azRtti = GetRttiHelper<ValueTypeNoPointer>();
            classElement.m_flags = (AZStd::is_pointer<ValueType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0);
            classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueTypeNoPointer>::GetGenericInfo();
            classElement.m_typeId = SerializeGenericTypeInfo<ValueTypeNoPointer>::GetClassTypeId();
            classElement.m_editData = nullptr;
            classElement.m_attributeOwnership = SerializeContext::ClassElement::AttributeOwnership::Self;
            /**
             * This should technically bind the reference value from the GetCurrentSerializeContextModule() call
             * as that will always return the current module that set the allocator.
             * But as the AZStdAssociativeContainer instance will not be accessed outside of the module it was
             * created within then this will return this .dll/.exe module allocator
             */
            classElement.m_attributes.set_allocator(AZStdFunctorAllocator([]() -> IAllocatorAllocate& { return GetCurrentSerializeContextModule().GetAllocator(); }));
        }

        template<size_t Index, size_t... Digits>
        struct IndexToCStrHelper : IndexToCStrHelper<Index / 10, Index % 10, Digits...> {};

        template<size_t... Digits>
        struct IndexToCStrHelper<0, Digits...>
        {
            static constexpr char value[] = { 'V', 'a', 'l', 'u', 'e', ('0' + Digits)..., '\0' };
        };

        template<size_t... Digits>
        constexpr char IndexToCStrHelper<0, Digits...>::value[];

        // Converts an integral index into a string with value = "Value" + #Index, where #Index is the stringification of the Index value
        template<size_t Index>
        struct IndexToCStr : IndexToCStrHelper<Index> {};

        template<typename... Types>
        class AZStdVariantContainer
            : public SerializeContext::IDataContainer
        {
            using VariantType = AZStd::variant<Types...>;
            static constexpr size_t s_variantSize = sizeof...(Types);
            static constexpr decltype(AZStd::make_index_sequence<s_variantSize>{}) s_variantIndexSequence{ AZStd::make_index_sequence<s_variantSize>{} };
        public:

            AZStdVariantContainer()
            {
                CreateClassElementArray(AZStd::make_index_sequence<s_variantSize>{});
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                for (const SerializeContext::ClassElement& classElement : m_alternativeClassElements)
                {
                    if (classElement.m_nameCrc == elementNameCrc)
                    {
                        return &classElement;
                    }
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& resultClassElement, const SerializeContext::DataElement& dataElement) const override
            {
                for (const SerializeContext::ClassElement& alternativeClassElement : m_alternativeClassElements)
                {
                    if (alternativeClassElement.m_typeId == dataElement.m_id)
                    {
                        resultClassElement = alternativeClassElement;
                        return true;
                    }
                }

                return false;
            }

            /// Enumerate elements in the array
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                auto variantInst = reinterpret_cast<VariantType*>(instance);
                auto enumElementsVisitor = [cb, altClassElement = m_alternativeClassElements[variantInst->index()]](auto&& elementAlt)
                {
                    cb(&const_cast<AZStd::remove_cvref_t<decltype(elementAlt)>&>(elementAlt), altClassElement.m_typeId, altClassElement.m_genericClassInfo ? altClassElement.m_genericClassInfo->GetClassData() : nullptr, &altClassElement);
                };
                AZStd::visit(AZStd::move(enumElementsVisitor), *variantInst);
            }

            void EnumTypes(const ElementTypeCB& cb) override
            {
                for (const SerializeContext::ClassElement& alternativeClassElement : m_alternativeClassElements)
                {
                    cb(alternativeClassElement.m_typeId, &alternativeClassElement);
                }
            }

            /// Return number of elements in the container.
            size_t  Size(void*) const override
            {
                return 1;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void*) const override
            {
                return 1;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool IsStableElements() const override { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            bool IsFixedSize() const override { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            bool IsFixedCapacity() const override { return true; }

            /// Returns true if the container is a smart pointer.
            bool IsSmartPointer() const override { return false; }

            /// Returns true if elements can be retrieved by index.
            bool CanAccessElementsByIndex() const override { return false; }

            /// Reserve element
            /*
             * Reserves an element in the variant using the type of the class element and not the index that the
             * class element is within the variant
             */
            void* ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                return ReserveElementImpl(instance, classElement, s_variantIndexSequence);
            }

            /// Get an element's address by its index (called before the element is loaded).
            void* GetElementByIndex(void*, const SerializeContext::ClassElement*, size_t) override
            {
                return nullptr;
            }

            /// Store element
            void StoreElement(void*, void*) override {}

            /// Remove element in the container.
            bool RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                if (!deletePointerDataContext)
                {
                    return false;
                }
                auto variantInst = reinterpret_cast<VariantType*>(instance);
                if (variantInst->valueless_by_exception())
                {
                    return false;
                }

                auto removeAlternativeVisitor = [this, index = variantInst->index(), element, deletePointerDataContext](auto& elementAlt)
                {
                    if (&elementAlt == element)
                    {
                        DeletePointerData(deletePointerDataContext, &m_alternativeClassElements[index], &elementAlt);
                        return true;
                    }
                    return false;
                };
                return AZStd::visit(AZStd::move(removeAlternativeVisitor), *variantInst); // you can't remove element from this container.
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                if (deletePointerDataContext)
                {
                    for (size_t i = 0; i < numElements; ++i)
                    {
                        RemoveElement(instance, elements[i], deletePointerDataContext);
                    }
                }
                return 0; // you can't remove elements from this container.
            }

            /// Clear elements in the instance.
            void ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                if (deletePointerDataContext)
                {
                    auto variantInst = reinterpret_cast<VariantType*>(instance);
                    if (!variantInst->valueless_by_exception())
                    {
                        auto clearAlternativeVisitor = [this, index = variantInst->index(), deletePointerDataContext](auto& elementAlt)
                        {
                            DeletePointerData(deletePointerDataContext, &m_alternativeClassElements[index], &elementAlt);
                            return true;
                        };
                        AZStd::visit(AZStd::move(clearAlternativeVisitor), *variantInst);
                    }
                }
            }

            SerializeContext::ClassElement m_alternativeClassElements[s_variantSize];
        private:
            template <typename AltType>
            void CreateClassElementAtIndex(SerializeContext::ClassElement& altClassElement, const char* elementName)
            {
                altClassElement.m_name = elementName;
                altClassElement.m_nameCrc = Crc32(altClassElement.m_name);
                // All elements of union has the starting offset at 0
                altClassElement.m_offset = 0;
                SetupClassElementFromType<AltType>(altClassElement);
            }
            template <size_t... Indices>
            void CreateClassElementArray(AZStd::index_sequence<Indices...>)
            {
                int expander[]{ 0, (CreateClassElementAtIndex<Types>(m_alternativeClassElements[Indices], IndexToCStr<Indices + 1>::value), 0)... };
                (void)expander;
            }

            template <size_t AltIndex>
            void ReserveElementAtIndex(void*& reservedElement, void* instance, const SerializeContext::ClassElement* classElement)
            {
                auto variantInst = reinterpret_cast<VariantType*>(instance);
                if (!reservedElement && classElement->m_typeId == m_alternativeClassElements[AltIndex].m_typeId &&
                    (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER) == (m_alternativeClassElements[AltIndex].m_flags & SerializeContext::ClassElement::FLG_POINTER))
                {
                    variantInst->template emplace<AltIndex>();
                    reservedElement = &const_cast<AZStd::remove_const_t<AZStd::variant_alternative_t<AltIndex, VariantType>>&>(AZStd::get<AltIndex>(*variantInst));
                }
            }

            template <size_t... Indices>
            void* ReserveElementImpl(void* instance, const SerializeContext::ClassElement* classElement, AZStd::index_sequence<Indices...>)
            {
                void* reservedElement{};
                int expander[]{ 0, (ReserveElementAtIndex<Indices>(reservedElement, instance, classElement), 0)... };
                (void)expander;
                return reservedElement;
            }
        };
    }

    template<typename... Types>
    struct SerializeGenericTypeInfo<AZStd::variant<Types...>>
    {
        using VariantType = AZStd::variant<Types...>;

        class VariantDataConverter
            : public SerializeContext::IDataConverter
        {
            bool CanConvertFromType(const TypeId& convertibleTypeId, const SerializeContext::ClassData& classData, SerializeContext& serializeContext) override
            {
                // Check that the typeid is exactly equal to VariantType
                if (convertibleTypeId == classData.m_typeId)
                {
                    return true;
                }

                // Next check if the convertible type id is exactly equal to one of the non-pointer alternatives 
                // i.e Variant<int*, int> would match can only match the `int` alternative at this point
                for (const SerializeContext::ClassElement& altClassElement : m_dataContainer.m_alternativeClassElements)
                {
                    const bool isPointerType = (altClassElement.m_flags & SerializeContext::ClassElement::FLG_POINTER) != 0;
                    if (!isPointerType && altClassElement.m_typeId == convertibleTypeId)
                    {
                        return true;
                    }
                }

                // Finally check if the alternative has an IDataConverter instance located on it's ClassData and invoke its CanConvertFromType function
                for (const SerializeContext::ClassElement& altClassElement : m_dataContainer.m_alternativeClassElements)
                {
                    // The Asset Class Id case must be handled in a special way since it does not register it's asset data with the serialize context
                    const SerializeContext::ClassData* altClassData = serializeContext.FindClassData(altClassElement.m_typeId);
                    AZ_Error("Serialize", altClassData, R"("Unable to find class data for class element with type "%s" for variant "%s".)"
                                " Has the type it represents been reflected to the SerializeContext",
                        altClassElement.m_typeId.ToString<AZStd::string>().data(), AzTypeInfo<VariantType>::Name());

                    bool viableAlternative = altClassData && altClassData->CanConvertFromType(convertibleTypeId, serializeContext);
                    if (viableAlternative)
                    {
                        return true;
                    }
                }

                return false;
            }

            bool ConvertFromType(void*& convertibleTypePtr, const TypeId& convertibleTypeId, void* classPtr, const SerializeContext::ClassData& classData, SerializeContext& serializeContext) override
            {
                /* Set the convertible address to the class address if the type exactly matches VariantType
                 * This is for the case if the VariantType has been serialized out with the selected alternative as a child
                 */
                if (convertibleTypeId == classData.m_typeId)
                {
                    convertibleTypePtr = classPtr;
                    return true;
                }

                /* Next check if one of the alternatives within the variant exactly matches convertible type and that the altnernative is a non-pointer.
                 * Due to the variant representing a type safe union it can be store an element of the convertible type directly
                 * if that type is one of the alternatives
                 * This is for the case of attempting to load a data element that happens to be one of the variant alternatives
                 * Ex. If the serialize context data is represented in xml in the following line as
                 * <Class name="int" field="LibrarySource" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}" value="1" />
                 * and the variant being serialized into has a type of AZStd::variant<AZStd::variant<int>, int*, int>
                 *
                 * Then the above variant should be able to serialize in the int directly to the third alternative since
                 * that alternative typeid exactly matches.
                 * The first alternative of `AZStd::variant<int>` can store an int, but as it is not an exact match it is not viable
                 * until the next check.
                 * The second alternative of `int*` is not viable at this point to as non pointer types are being explicitly checked
                 */

                for (const SerializeContext::ClassElement& altClassElement : m_dataContainer.m_alternativeClassElements)
                {
                    const bool isPointerType = (altClassElement.m_flags & SerializeContext::ClassElement::FLG_POINTER) != 0;
                    if (!isPointerType && altClassElement.m_typeId == convertibleTypeId && ReserveElement(convertibleTypePtr, classPtr, altClassElement, serializeContext))
                    {
                        return true;
                    }
                }

                /* Finally check if one of the alternative types can its SerializeContext ClassData to convert the supplied type
                 * This is for the case of loading nested variant
                 * Ex. If the serialized data is an int that is represented in xml with text similar to the next line
                 * <Class name="int" field="LibrarySource" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}" value="1" />
                 * Next the variant being serialized into is of type AZStd::variant<AZStd::variant<int>, double>
                 *
                 * Then the AZStd::variant<AZStd::variant<int>, double> should be able to serialize in the int value to inner AZStd::variant<int>
                 *
                 * Furthermore this check can also be used to convert unrelated alternatives of a variant to each other.
                 * Ex. If the serialized data is a vector of ints represented in xml by the following lines
                 * <Class name="AZStd::vector" field="Ints" type="{BE163120-C1ED-5F69-A650-DC2528A8FF94}"">
                 *     <Class name="int" field="element"  type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}" value="1 />
                 * </Class>
                 * Next the variant being serialized into is of type AZStd::variant<AZStd::list<int>, bool>
                 *
                 * Then the AZStd::list<int> is given a chance to an determine if it can stored the serialized data via its ConvertFromType function
                 */
                for (const SerializeContext::ClassElement& altClassElement : m_dataContainer.m_alternativeClassElements)
                {
                    const SerializeContext::ClassData* altClassData = serializeContext.FindClassData(altClassElement.m_typeId);
                    AZ_Error("Serialize", altClassData, R"("Unable to find class data for class element with type "%s" for variant "%s".)"
                        " Has the type it represents been reflected to the SerializeContext",
                        altClassElement.m_typeId.ToString<AZStd::string>().data(), AzTypeInfo<VariantType>::Name());

                    bool viableAlternative = altClassData && altClassData->CanConvertFromType(convertibleTypeId, serializeContext);
                    viableAlternative = viableAlternative && ReserveElement(convertibleTypePtr, classPtr, altClassElement, serializeContext);
                    viableAlternative = viableAlternative && altClassData->ConvertFromType(convertibleTypePtr, convertibleTypeId, convertibleTypePtr, serializeContext);
                    if (viableAlternative)
                    {
                        return true;
                    }
                }

                return false;
            }

        private:
            bool ReserveElement(void*& convertibleTypePtr, void* classPtr, const SerializeContext::ClassElement &altClassElement, SerializeContext &serializeContext)
            {
                void* reserveAddress = m_dataContainer.ReserveElement(classPtr, &altClassElement);
                if (!reserveAddress)
                {
                    AZ_Error("Serialize", false, R"(Failed to reserve storage in the variant "%s" for alternative of type"%s")",
                        AzTypeInfo<VariantType>::Name(), altClassElement.m_typeId.ToString<AZStd::string>().data());
                    return false;
                }

                const bool isPointerType = (altClassElement.m_flags & SerializeContext::ClassElement::FLG_POINTER) != 0;
                if (!isPointerType)
                {
                    convertibleTypePtr = reserveAddress;
                }
                else
                {
                    const SerializeContext::ClassData* altClassData = serializeContext.FindClassData(altClassElement.m_typeId);
                    if (!altClassData)
                    {
                        AZ_Error("Serialize", false, R"("Unable to find class data for class element with type "%s" for variant "%s".)"
                            " Has the type it represents been reflected to the SerializeContext",
                            altClassElement.m_typeId.ToString<AZStd::string>().data(), AzTypeInfo<VariantType>::Name());
                        return false;
                    }

                    const SerializeContext::IObjectFactory* altObjectFactory = altClassData->m_factory;
                    if (!altObjectFactory)
                    {
                        AZ_Error("Serialize", false, R"(A pointer of type "%s" cannot be allocated by the GenericClassVariant "%s"class due to not having an object factory)",
                            altClassData->m_name, AzTypeInfo<VariantType>::Name());
                        return false;
                    }

                    *reinterpret_cast<void**>(reserveAddress) = altClassData->m_factory->Create(altClassData->m_name);
                    convertibleTypePtr = *reinterpret_cast<void**>(reserveAddress);
                }

                return true;
            }

            VariantSerializationInternal::AZStdVariantContainer<Types...> m_dataContainer;
        };

        class GenericClassVariant
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassVariant, AZ::s_variantTypeId);
            GenericClassVariant()
                : m_classData{ SerializeContext::ClassData::Create<VariantType>("variant", GetSpecializedTypeId(), &m_variantInstanceFactory, nullptr, &m_variantContainer) }
            {
                m_classData.m_dataConverter = &m_dataConverter;
                // As the SerializeGenericTypeInfo is created on demand when a variant is reflected(in static memory)
                // the serialize context dll module allocator has to be used to manage the lifetime of the ClassData attributes within a module
                // If a module which reflects a variant is unloaded, then the dll module allocator will properly unreflect the variant type from the serialize context
                // for this particular module
                AZStdFunctorAllocator dllAllocator([]() -> IAllocatorAllocate& { return GetCurrentSerializeContextModule().GetAllocator(); });
                m_classData.m_attributes.set_allocator(AZStd::move(dllAllocator));

                // Create the ObjectStreamWriteOverrideCB in the current module
                m_classData.m_attributes.emplace_back(AZ_CRC("ObjectStreamWriteElementOverride", 0x35eb659f), CreateModuleAttribute(&ObjectStreamWriter));
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return sizeof...(Types);
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                if (GenericClassInfo* valueGenericClassInfo = m_variantContainer.m_alternativeClassElements[element].m_genericClassInfo)
                {
                    return valueGenericClassInfo->GetSpecializedTypeId();
                }
                return m_variantContainer.m_alternativeClassElements[element].m_typeId;
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<VariantType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<VariantType>::CreateAny);
                    for (SerializeContext::ClassElement& altClassElement : m_variantContainer.m_alternativeClassElements)
                    {
                        if (GenericClassInfo* valueGenericClassInfo = altClassElement.m_genericClassInfo)
                        {
                            valueGenericClassInfo->Reflect(serializeContext);
                        }
                    }
                }
            }
        private:
            static void ObjectStreamWriter(SerializeContext::EnumerateInstanceCallContext& callContext, const void* variantPtr,
                [[maybe_unused]] const SerializeContext::ClassData& variantClassData, const SerializeContext::ClassElement* variantClassElement)
            {
                auto alternativeVisitor = [&callContext, variantClassElement](auto&& elementAlt)
                {
                    using AltType = AZStd::remove_cvref_t<decltype(elementAlt)>;
                    const SerializeContext& context = *callContext.m_context;
                    SerializeContext::ClassElement altClassElement{};
                    if (variantClassElement)
                    {
                        altClassElement.m_name = variantClassElement->m_name;
                        altClassElement.m_nameCrc = variantClassElement->m_nameCrc;
                    }
                    altClassElement.m_offset = 0;
                    VariantSerializationInternal::SetupClassElementFromType<AltType>(altClassElement);
                    const AZ::Uuid& altTypeId = altClassElement.m_typeId;
                    
                    const SerializeContext::ClassData* altClassData = context.FindClassData(altTypeId);
                    AZ_Error("Serialize", altClassData, "Unable to find ClassData for variant alternative with name %s and typeid of %s", AzTypeInfo<AltType>::Name(), altTypeId.ToString<AZStd::string>().data());
                    return altClassData ? callContext.m_context->EnumerateInstanceConst(&callContext, &elementAlt, altTypeId, altClassData, &altClassElement) : false;
                };

                AZStd::visit(AZStd::move(alternativeVisitor), *reinterpret_cast<const VariantType*>(variantPtr));
            }

            VariantSerializationInternal::AZStdVariantContainer<Types...> m_variantContainer;
            SerializeContext::ClassData m_classData;
            Serialize::InstanceFactory<VariantType> m_variantInstanceFactory;
            VariantDataConverter m_dataConverter;
        };

        using ClassInfoType = GenericClassVariant;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<VariantType>();
        }

        static const Uuid& GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };
}
