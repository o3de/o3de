/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_SERIALIZE_AZSTD_CONTAINERS_INL
#define AZCORE_SERIALIZE_AZSTD_CONTAINERS_INL

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/tuple.h>

#include <AzCore/IO/GenericStreams.h>

namespace AZStd
{
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class vector;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class list;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class forward_list;
    template< class T, size_t Capacity >
    class fixed_vector;
    template< class T, size_t N >
    class array;
    template<class Key, class MappedType, class Compare /*= AZStd::less<Key>*/, class Allocator /*= AZStd::allocator*/>
    class map;
    template<class Key, class MappedType, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_map;
    template<class Key, class MappedType, class Hasher /* = AZStd::hash<Key>*/, class EqualKey /* = AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_multimap;
    template <class Key, class Compare, class Allocator>
    class set;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_set;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_multiset;
    template<AZStd::size_t NumBits>
    class bitset;

    template<class T>
    class intrusive_ptr;
    template<class T>
    class shared_ptr;

    template<class T, class = void>
    inline constexpr bool HasToString_v = false;
    template<class T>
    inline constexpr bool HasToString_v<T, AZStd::enable_if_t<
        AZStd::is_void_v<decltype(AZStd::to_string(AZStd::declval<AZStd::string&>(), AZStd::declval<T>()), void())>> > = true;

    inline namespace AssociativeInternal
    {
        // SFINAE expression to determine whether an associative container is an actual map with a corresponding value for each key,
        // or is simply a keyed container, like a set or an unordered_set
        template<class T, class = void>
        constexpr bool IsMapType_v = false;
        template<class T>
        constexpr bool IsMapType_v<T, enable_if_t<Internal::sfinae_trigger_v<typename T::mapped_type>>> = true;

        template <class T>
        constexpr bool IsOrderedSetImpl_v = false;

        template<class K, class C, class A>
        constexpr bool IsOrderedSetImpl_v<AZStd::set<K, C, A>> = true;

        template<class K, class C, class A>
        constexpr bool IsOrderedSetImpl_v<AZStd::multimap<K, C, A>> = true;

        template <class T>
        constexpr bool IsOrderedMapImpl_v = false;

        template<class K, class M, class C, class A>
        constexpr bool IsOrderedMapImpl_v<AZStd::map<K, M, C, A>> = true;

        template<class K, class M, class C, class A>
        constexpr bool IsOrderedMapImpl_v<AZStd::multimap<K, M, C, A>> = true;

        template <class T>
        constexpr bool IsUnorderedSetImpl_v = false;

        template <class K, class H, class E, class A>
        constexpr bool IsUnorderedSetImpl_v<AZStd::unordered_set<K, H, E, A>> = true;

        template <class K, class H, class E, class A>
        constexpr bool IsUnorderedSetImpl_v<AZStd::unordered_multiset<K, H, E, A>> = true;

        template <class T>
        constexpr bool IsUnorderedMapImpl_v = false;

        template <class K, class M, class H, class E, class A>
        constexpr bool IsUnorderedMapImpl_v<AZStd::unordered_map<K, M, H, E, A>> = true;

        template <class K, class M, class H, class E, class A>
        constexpr bool IsUnorderedMapImpl_v<AZStd::unordered_multimap<K, M, H, E, A>> = true;
    }

    template <class T>
    constexpr bool IsOrderedSet_v = AssociativeInternal::IsOrderedSetImpl_v<AZStd::remove_cvref_t<T>>;

    template <class T>
    constexpr bool IsOrderedMap_v = AssociativeInternal::IsOrderedMapImpl_v<AZStd::remove_cvref_t<T>>;

    template <class T>
    constexpr bool IsUnorderedSet_v = AssociativeInternal::IsUnorderedSetImpl_v<AZStd::remove_cvref_t<T>>;

    template <class T>
    constexpr bool IsUnorderedMap_v = AssociativeInternal::IsUnorderedMapImpl_v<AZStd::remove_cvref_t<T>>;
}

namespace AZ
{
    namespace Internal
    {
        template <class ValueType>
        void SetupClassElementFromType(SerializeContext::ClassElement& classElement)
        {
            using ValueClass = typename AZStd::remove_pointer_t<ValueType>;

            classElement.m_dataSize = sizeof(ValueType);
            classElement.m_azRtti = GetRttiHelper<ValueClass>();
            classElement.m_flags = (AZStd::is_pointer<ValueType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0);
            classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueClass>::GetGenericInfo();
            classElement.m_typeId = SerializeGenericTypeInfo<ValueClass>::GetClassTypeId();
            classElement.m_editData = nullptr;
            classElement.m_attributeOwnership = SerializeContext::ClassElement::AttributeOwnership::Self;

            // Flag the field with the EnumType attribute if we're an enumeration type aliased by RemoveEnum
            const bool isSpecializedEnum = AZStd::is_enum<ValueType>::value && !AzTypeInfo<ValueType>::Uuid().IsNull();
            if (isSpecializedEnum)
            {
                auto uuid = AzTypeInfo<ValueType>::Uuid();

                classElement.m_attributes.emplace_back(AZ_CRC_CE("EnumType"), CreateModuleAttribute(AZStd::move(uuid)));
            }
        }

        template <class T>
        AZStd::enable_if_t<std::is_pod<T>::value> InitializeDefaultIfPodType(T& t)
        {
            t = T{};
        }

        template <class T>
        AZStd::enable_if_t<!std::is_pod<T>::value> InitializeDefaultIfPodType(T& t)
        {
            (void)t;
        }

        // Simple container that allows wrapping an rvalue into a simple class that enumerates through InstanceDataHierarchy into a class with a single child with a key = value pair
        // This allows for e.g. dynamically prompting for key types in associative containers based on this type.
        template <class TRValue>
        struct RValueToLValueWrapper
        {
            TRValue m_data;
        };

        class NullFactory
            : public SerializeContext::IObjectFactory
        {
            void* Create(const char* name) override
            {
                (void)name;
                AZ_Assert(false, "Containers and pointers (%s) should be stored by value. They are made to be small and have minimal overhead when empty!", name);
                return nullptr;
            }

            void Destroy(void*) override
            {
            }
        public:
            static NullFactory* GetInstance()
            {
                static NullFactory s_nullFactory;
                return &s_nullFactory;
            }
        };

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

        template<class Container>
        constexpr bool IsForwardList_v = false;
        template<class T, class Alloc>
        inline constexpr bool IsForwardList_v<AZStd::forward_list<T, Alloc>> = true;

        template<class T, bool IsStableIterators>
        class AZStdBasicContainer
            : public SerializeContext::IDataContainer
        {
        public:
            typedef typename T::value_type ValueType;

            AZStdBasicContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_offset = 0xbad0ffe0; // bad offset mark
                SetupClassElementFromType<ValueType>(m_classElement);
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array.
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);

                typename T::iterator it = arrayPtr->begin();
                typename T::iterator end = arrayPtr->end();
                for (; it != end; ++it)
                {
                    ValueType* valuePtr = &*it;
                    if (!cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement))
                    {
                        break;
                    }
                }
            }

            void EnumTypes(const ElementTypeCB& cb) override
            {
                cb(m_classElement.m_typeId, &m_classElement);
            }

            /// Return number of elements in the container.
            size_t  Size(void* instance) const override
            {
                const T* arrayPtr = reinterpret_cast<const T*>(instance);
                return AZStd::ranges::distance(*arrayPtr);
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 0;
            }


            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool    IsStableElements() const override           { return IsStableIterators; }

            /// Returns true if the container is fixed size, otherwise false.
            bool    IsFixedSize() const override                { return false; }

            /// Returns if the container is fixed capacity, otherwise false
            bool    IsFixedCapacity() const override            { return false; }

            /// Returns true if the container is a smart pointer.
            bool    IsSmartPointer() const override             { return false; }

            /// Returns true if elements can be retrieved by index.
            bool    CanAccessElementsByIndex() const override   { return false; }

            bool SwapElements(void* instance, size_t firstIndex, size_t secondIndex) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);
                size_t containerSize = Size(instance);
                // Make sure the indices to swap are in range of the container
                if (firstIndex >= containerSize || secondIndex >= containerSize)
                {
                    return false;
                }

                AZStd::ranges::iter_swap(AZStd::next(arrayPtr->begin(), firstIndex), AZStd::next(arrayPtr->begin(), secondIndex));
                return true;
            }

            bool IsSequenceContainer() const override { return true; }

            /// Reserve element
            void*   ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                if constexpr (IsForwardList_v<T>)
                {
                    auto FindLastValidElementBefore = [](const T& forwardList,
                        typename T::const_iterator last)
                    {
                        auto lastValidIter = forwardList.before_begin();
                        for (auto first = forwardList.begin(); first != last; lastValidIter = first, ++first)
                        {
                        }

                        return lastValidIter;
                    };

                    T* forwardListPtr = reinterpret_cast<T*>(instance);
                    return AZStd::to_address(forwardListPtr->emplace_after(
                        FindLastValidElementBefore(*forwardListPtr, forwardListPtr->end())
                    ));
                }
                else
                {
                    T* arrayPtr = reinterpret_cast<T*>(instance);
                    return &arrayPtr->emplace_back();
                }
            }

            /// Get an element's address by its index (called before the element is loaded).
            void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)instance;
                (void)classElement;
                (void)index;
                return nullptr;
            }

            /// Store element
            void    StoreElement(void* instance, void* element) override
            {
                (void)instance;
                (void)element;
                // do nothing as we have already pushed the element,
                // we can assert and check if the element belongs to the container
            }

            /// Remove element in the container.
            bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);
                // forward_list can only erase the element after the found one
                // so keep track of the element before the last
                [[maybe_unused]] typename T::iterator prevIter{};
                if constexpr (IsForwardList_v<T>)
                {
                    prevIter = arrayPtr->before_begin();
                }

                for (auto foundIt = arrayPtr->begin(); foundIt != arrayPtr->end(); prevIter = foundIt, ++foundIt)
                {
                    if (void* containerElem = AZStd::to_address(foundIt);
                        containerElem == element)
                    {
                        if (deletePointerDataContext)
                        {
                            DeletePointerData(deletePointerDataContext, &m_classElement, containerElem);
                        }
                        if constexpr (IsForwardList_v<T>)
                        {
                            arrayPtr->erase_after(prevIter);
                        }
                        else
                        {
                            arrayPtr->erase(foundIt);
                        }
                        return true;
                    }
                }

                return false;
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                if (numElements == 0)
                {
                    return 0;
                }

                size_t numRemoved = 0;

                // here is a little tricky if the container doesn't have stable elements, we assume the only case if AZStd::vector.
                if (!IsStableIterators)
                {
                    // if elements are in order we can remove all of them from the container, otherwise we either need to resort locally (not done)
                    // or ask the user to pass elements in order and remove the first N we can (in order)
                    for (size_t i = 1; i < numElements; ++i)
                    {
                        if (elements[i - 1] >= elements[i])
                        {
                            AZ_TracePrintf("Serialization", "RemoveElements for AZStd::vector will perform optimally when the elements (addresses) are sorted in accending order!");
                            numElements = i;
                        }
                    }
                    // traverse the vector in reverse order, then addresses of elements should not change.
                    for (int i = static_cast<int>(numElements); i >= 0; --i)
                    {
                        if (RemoveElement(instance, elements[i], deletePointerDataContext))
                        {
                            ++numRemoved;
                        }
                    }
                }
                else
                {
                    for (size_t i = 0; i < numElements; ++i)
                    {
                        if (RemoveElement(instance, elements[i], deletePointerDataContext))
                        {
                            ++numRemoved;
                        }
                    }
                }
                return numRemoved;
            }

            /// Clear elements in the instance.
            void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);
                if (deletePointerDataContext)
                {
                    for (typename T::iterator it = arrayPtr->begin(); it != arrayPtr->end(); ++it)
                    {
                        DeletePointerData(deletePointerDataContext, &m_classElement, &(*it));
                    }
                }
                arrayPtr->clear();
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };

        template<class T, bool IsStableIterators>
        class AZStdRandomAccessContainer
            : public AZStdBasicContainer<T, IsStableIterators>
        {
            /// Returns true for RandomAccessContainers.
            bool CanAccessElementsByIndex() const override { return true; }

            /// Get an element's address by its index (called before the element is loaded).
            void* GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                auto arrayPtr = reinterpret_cast<T*>(instance);
                if (index < arrayPtr->size())
                {
                    return &(*arrayPtr)[index];
                }
                return nullptr;
            }
        };
        template<class T, bool IsStableIterators, size_t N>
        class AZStdFixedCapacityRandomAccessContainer
            : public AZStdRandomAccessContainer<T, IsStableIterators>
        {
        public:
            typedef typename T::value_type ValueType;
            typedef typename AZStd::remove_pointer<typename T::value_type>::type ValueClass;

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void* instance) const override
            {
                (void)instance;
                return N;
            }

            /// Returns if the container is fixed capacity, otherwise false
            bool    IsFixedCapacity() const override          { return true; }

            /// Reserve element
            void*   ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                T* arrayPtr = reinterpret_cast<T*>(instance);
                if (arrayPtr->size() < N)
                {
                    arrayPtr->emplace_back();
                    return &arrayPtr->back();
                }
                return nullptr;
            }
        };

        class AZStdArrayEvents : public SerializeContext::IEventHandler
        {
        public:
            using Stack = AZStd::stack<size_t, AZStd::vector<size_t, AZ::OSStdAllocator>>;

            void OnWriteBegin(void* classPtr) override;
            void OnWriteEnd(void* classPtr) override;
            size_t GetIndex() const;
            void Increment();
            void Decrement();
            bool IsEmpty() const;

        private:
            // To avoid allocating memory for the stack when there's only one AZStd::array being tracked, m_indices is used to both
            // store an integer value for the index, or when there's nested AZStd::arrays, an AZStd::stack. To tell the two apart
            // the least significant bit is set to 1 if an integer value is stored and 0 if m_indices points to an AZStd::stack.
            // Because the lsb is used for storing the indicator bit, the stored value needs to be shifted down to get the actual index.
            static AZ_THREAD_LOCAL void* m_indices;
        };
        template<typename T, size_t N>
        class AZStdArrayContainer
            : public SerializeContext::IDataContainer
        {
        public:
            typedef AZStd::array<T, N> ContainerType;
            typedef T ValueType;

            AZStdArrayContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_offset = 0xbad0ffe0; // bad offset mark
                SetupClassElementFromType<ValueType>(m_classElement);
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array.
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                ContainerType* arrayPtr = reinterpret_cast<ContainerType*>(instance);

                typename ContainerType::iterator it = arrayPtr->begin();
                typename ContainerType::iterator end = arrayPtr->end();
                for (; it != end; ++it)
                {
                    ValueType* valuePtr = &*it;

                    if (!cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement))
                    {
                        break;
                    }
                }
            }

            void EnumTypes(const ElementTypeCB& cb) override
            {
                cb(m_classElement.m_typeId, &m_classElement);
            }

            /// Return number of elements in the container.
            size_t  Size(void* instance) const override
            {
                (void)instance;
                return N;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void* instance) const override
            {
                (void)instance;
                return N;
            }


            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool    IsStableElements() const override           { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            bool    IsFixedSize() const override                { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            bool    IsFixedCapacity() const override            { return true; }

            /// Returns true if the container is a smart pointer.
            bool    IsSmartPointer() const override             { return false; }

            /// Returns true if elements can be retrieved by index.
            bool    CanAccessElementsByIndex() const override   { return true; }

            /// Reserve element
            void*   ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                GenericClassInfo* containerClassInfo = SerializeGenericTypeInfo<ContainerType>::GetGenericInfo();
                const bool eventHandlerAvailable = containerClassInfo && containerClassInfo->GetClassData() && containerClassInfo->GetClassData()->m_eventHandler;
                if (!eventHandlerAvailable)
                {
                    return nullptr;
                }

                AZStdArrayEvents* eventHandler = reinterpret_cast<AZStdArrayEvents*>(containerClassInfo->GetClassData()->m_eventHandler);
                size_t index = eventHandler->GetIndex();
                ContainerType* arrayPtr = reinterpret_cast<ContainerType*>(instance);
                if (index < N)
                {
                    eventHandler->Increment();
                    return &(arrayPtr->at(index));
                }
                else
                {
                    AZ_Warning("Serialization", false, "Unable to reserve an element for AZStd::array because all %i slots are in use.", N);
                }
                return nullptr;
            }

            /// Get an element's address by its index (called before the element is loaded).
            void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                ContainerType* arrayPtr = reinterpret_cast<ContainerType*>(instance);
                if (index < arrayPtr->size())
                {
                    return &(*arrayPtr)[index];
                }
                return nullptr;
            }

            /// Store element
            void    StoreElement(void* instance, void* element) override
            {
                (void)instance;
                (void)element;
                // do nothing as we have already pushed the element,
                // we can assert and check if the element belongs to the container
            }

            /// Remove element in the container.
            bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                (void)element;

                AZ_Assert((reinterpret_cast<uintptr_t>(element) & (alignof(ValueType) - 1)) == 0, "element memory address does not match alignment of ValueType");
                ContainerType* arrayPtr = reinterpret_cast<ContainerType*>(instance);
                ptrdiff_t arrayIndex = reinterpret_cast<const ValueType*>(element) - arrayPtr->data();
                if (arrayIndex < 0 || arrayIndex >= static_cast<ptrdiff_t>(arrayPtr->size()))
                {
                    AZ_Error("Serialization", false, "Supplied element to remove memory address of 0x%p falls outside of address of AZStd::array range of [0x%p, 0x%p]",
                        element, arrayPtr->data(), arrayPtr->data() + arrayPtr->size());
                    return false;
                }

                if (deletePointerDataContext)
                {
                    DeletePointerData(deletePointerDataContext, &m_classElement, element);
                }

                // If the element that's removed is the last added element, decrement the insertion counter.
                GenericClassInfo* containerClassInfo = SerializeGenericTypeInfo<ContainerType>::GetGenericInfo();
                const bool eventHandlerAvailable = containerClassInfo && containerClassInfo->GetClassData() && containerClassInfo->GetClassData()->m_eventHandler;
                if (eventHandlerAvailable)
                {
                    AZStdArrayEvents* eventHandler = reinterpret_cast<AZStdArrayEvents*>(containerClassInfo->GetClassData()->m_eventHandler);
                    if (static_cast<ptrdiff_t>(eventHandler->GetIndex()) == arrayIndex + 1)
                    {
                        eventHandler->Decrement();
                    }
                }

                // Still return false because no object is actually removed.
                return false;
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                (void)instance;
                (void)elements;
                (void)numElements;
                (void)deletePointerDataContext;
                return 0;
            }

            /// Clear elements in the instance.
            void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                (void)instance;
                (void)deletePointerDataContext;
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };

        template<class T>
        class AZStdAssociativeContainer
            : public Serialize::IDataContainer
            , public Serialize::IDataContainer::IAssociativeDataContainer
        {
            using ValueType = typename T::value_type;
            using ValueClass = AZStd::remove_pointer_t<ValueType>;
            using KeyType = typename T::key_type;
            using WrappedKeyType = RValueToLValueWrapper<KeyType>;

            // Helper for looking up our key type and assigning keys to T::value_type.
            // Currently we assumed that if T::value_type is an AZStd::pair, we're keyed by pair.first, otherwise we're keyed by value_type
            template <class TKey, bool = AZStd::is_same<ValueType, KeyType>::value>
            struct KeyHelper
            {
                static void SetElementKey(ValueType* element, KeyType key)
                {
                    *element = key;
                }
            };

            template <class T1, class T2>
            struct KeyHelper<AZStd::pair<T1, T2>, false>
            {
                static void SetElementKey(ValueType* element, KeyType key)
                {
                    element->first = key;
                }
            };
        public:
            AZStdAssociativeContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_offset = 0xbad0ffe0; // bad offset mark
                SetupClassElementFromType<ValueType>(m_classElement);
                // Associative containers usually do a hash insert, default value will cause a collision.
                // If we want we can check for multi_set, multi_map, but that may be too much for now.
                m_classElement.m_flags |= SerializeContext::ClassElement::FLG_NO_DEFAULT_VALUE;

                // Register our key type within an lvalue to rvalue wrapper as an attribute
                AZ::TypeId uuid = azrtti_typeid<WrappedKeyType>();

                m_classElement.m_attributes.emplace_back(AZ_CRC_CE("KeyType"), CreateModuleAttribute(AZStd::move(uuid)));
            }

            // Reflect our wrapped key and value types to serializeContext so that may later be used
            static void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericType<ValueType>();
                    serializeContext->RegisterGenericType<WrappedKeyType>();
                }
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                T* containerPtr = reinterpret_cast<T*>(instance);

                typename T::iterator it = containerPtr->begin();
                typename T::iterator end = containerPtr->end();
                for (; it != end; ++it)
                {
                    ValueType* valuePtr = &*it;
                    if (!cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement))
                    {
                        break;
                    }
                }
            }

            void EnumTypes(const ElementTypeCB& cb) override
            {
                cb(m_classElement.m_typeId, &m_classElement);
            }

            /// Return number of elements in the container.
            size_t  Size(void* instance) const override
            {
                const T* arrayPtr = reinterpret_cast<const T*>(instance);
                return arrayPtr->size();
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 0;
            }


            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool    IsStableElements() const override           { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            bool    IsFixedSize() const override                { return false; }

            /// Returns if the container is fixed capacity, otherwise false
            bool    IsFixedCapacity() const override            { return false; }

            /// Returns true if the container is a smart pointer.
            bool    IsSmartPointer() const override             { return false; }

            /// Returns true if elements can be retrieved by index.
            bool    CanAccessElementsByIndex() const override   { return false; }

            /// Returns the associative interface for this container if available, otherwise null.
            SerializeContext::IDataContainer::IAssociativeDataContainer* GetAssociativeContainerInterface() override
            {
                return this;
            }

            const SerializeContext::IDataContainer::IAssociativeDataContainer* GetAssociativeContainerInterface() const override
            {
                return this;
            }

            /// Reserve element
            void*   ReserveElement(void* instance, const  SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                T* containerPtr = reinterpret_cast<T*>(instance);
                return new (containerPtr->get_allocator().allocate(sizeof(ValueType), AZStd::alignment_of<ValueType>::value)) ValueType{};
            }

        protected:
            /// Reserve a key and get its address. Used by GetKey.
            void*   AllocateKey() override
            {
                // We use new here as we may be allocating a type without AZ_CLASS_ALLOCATOR (e.g. primitives)
                // This key is never owned by any of our container instances, so can be safely managed by our allocator.
                auto key = new KeyType;
                InitializeDefaultIfPodType(*key);
                return key;
            }

            /// Deallocates a key created by ReserveKey. Used by GetKey.
            void    FreeKey(void* key) override
            {
                delete reinterpret_cast<KeyType*>(key);
            }

        public:
            Serialize::IDataContainer::IAssociativeDataContainer::AssociativeType GetAssociativeType() const override
            {
                using AssociativeType = Serialize::IDataContainer::IAssociativeDataContainer::AssociativeType;
                if constexpr (AZStd::IsOrderedSet_v<T>)
                {
                    return AssociativeType::Set;
                }
                else if constexpr (AZStd::IsOrderedMap_v<T>)
                {
                    return AssociativeType::Map;
                }
                else if constexpr (AZStd::IsUnorderedSet_v<T>)
                {
                    return AssociativeType::UnorderedSet;
                }
                else if constexpr (AZStd::IsUnorderedMap_v<T>)
                {
                    return AssociativeType::UnorderedMap;
                }
                else
                {
                    return AssociativeType::Unknown;
                }
            }

            /// Get an element's address by its index (called before the element is loaded).
            void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                // Linear iteration of the container.   If possible, use GetElementByKey, but this is implemented to be compatible
                // with things that present the container as if it is a linear container (such as UIs)
                T* containerPtr = reinterpret_cast<T*>(instance);
                if (index < containerPtr->size())
                {
                    auto elementIterator = containerPtr->begin();
                    AZStd::advance(elementIterator, index);
                    
                    return (elementIterator != containerPtr->end()) ? &(*elementIterator) : nullptr;
                }
                return nullptr;
            }

            /// Get an element's address by its key. Not used for serialization.
            void*   GetElementByKey(void* instance, const SerializeContext::ClassElement* classElement, const void* key) const override
            {
                (void)classElement;
                T* containerPtr = reinterpret_cast<T*>(instance);
                auto elementIterator = containerPtr->find(*reinterpret_cast<const KeyType*>(key));
                return (elementIterator != containerPtr->end()) ? &(*elementIterator) : nullptr;
            }

            /// Get the mapped value's address by its key. If there is no mapped value (like in a set<>) the value returned is
            /// the key itself
            virtual void* GetValueByKey(void* instance, const void* key) const override
            {
                void* value = nullptr;
                T* containerPtr = reinterpret_cast<T*>(instance);
                auto elementIterator = containerPtr->find(*reinterpret_cast<const KeyType*>(key));

                if constexpr (AZStd::AssociativeInternal::IsMapType_v<T>)
                {
                    if (elementIterator != containerPtr->end())
                    {
                        value = &(elementIterator->second);
                    }
                }
                else
                {
                    if (elementIterator != containerPtr->end())
                    {
                        value = &(*elementIterator);
                    }
                }
                return value;
            }

            /// Store element
            void    StoreElement(void* instance, void* element) override
            {
                T* containerPtr = reinterpret_cast<T*>(instance);
                ValueType* valuePtr = reinterpret_cast<ValueType*>(element);
                containerPtr->insert(AZStd::move(*valuePtr));
                valuePtr->~ValueType();
                containerPtr->get_allocator().deallocate(valuePtr, sizeof(ValueType), AZStd::alignment_of<ValueType>::value);
            }

            /// FreeReservedElement
            void    FreeReservedElement(void* instance, void* element, SerializeContext* deletePointerDataContext) override
            {
                if (deletePointerDataContext)
                {
                    DeletePointerData(deletePointerDataContext, &m_classElement, element);
                }

                T* containerPtr = reinterpret_cast<T*>(instance);
                ValueType* valuePtr = reinterpret_cast<ValueType*>(element);
                valuePtr->~ValueType();
                containerPtr->get_allocator().deallocate(valuePtr, sizeof(ValueType), AZStd::alignment_of<ValueType>::value);
            }

            /// Remove element in the container.
            bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                T* containerPtr = reinterpret_cast<T*>(instance);
                // this container can be a multi container so key is NOT enough, but a good start
                const auto& key = T::traits_type::key_from_value(*reinterpret_cast<const ValueType*>(element));
                AZStd::pair<typename T::iterator, typename T::iterator> valueRange = containerPtr->equal_range(key);
                while (valueRange.first != valueRange.second) // in a case of multi key support iterate over all elements with that key until we find the one
                {
                    if (&(*valueRange.first) == element)
                    {
                        // Extracts the node from the associative container without deleting it
                        // The T::node_type destructor takes care of cleaning up the memory of the extracted node
                        typename T::node_type removeNode = containerPtr->extract(valueRange.first);
                        if (deletePointerDataContext)
                        {
                            // The following call will invoke the AZStdPairContainer::ClearElements function which will delete any elements
                            // of pointer type stored by the pair and reset the pair itself back to a default constructed value
                            DeletePointerData(deletePointerDataContext, &m_classElement, element);
                        }
                        return true;
                    }
                    ++valueRange.first;
                }
                return false;
            }

            /// Inserts an entry at key in the container (for keyed containers only). Not used for serialization.
            void    SetElementKey(void* element, void* key) const override
            {
                KeyHelper<ValueType>::SetElementKey(reinterpret_cast<ValueType*>(element), *reinterpret_cast<KeyType*>(key));
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                size_t numRemoved = 0;
                for (size_t i = 0; i < numElements; ++i)
                {
                    if (RemoveElement(instance, elements[i], deletePointerDataContext))
                    {
                        ++numRemoved;
                    }
                }
                return numRemoved;
            }

            /// Clear elements in the instance.
            void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                T* containerPtr = reinterpret_cast<T*>(instance);
                if (deletePointerDataContext)
                {
                    for (typename T::iterator it = containerPtr->begin(); it != containerPtr->end(); ++it)
                    {
                        DeletePointerData(deletePointerDataContext, &m_classElement, &(*it));
                    }
                }
                containerPtr->clear();
            }

            void ElementsUpdated(void* instance) override
            {
                // reinsert all elements, technically for hash containers we can just call rehash
                // which might be faster, but we will not be able to cover other containers;
                T* containerPtr = reinterpret_cast<T*>(instance);
                T tempContainer(AZStd::make_move_iterator(containerPtr->begin()), AZStd::make_move_iterator(containerPtr->end()));
                containerPtr->swap(tempContainer);
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };


        template<class T1, class T2>
        class AZStdPairContainer
            : public SerializeContext::IDataContainer
        {
            typedef typename AZStd::remove_pointer<T1>::type Value1Class;
            typedef typename AZStd::remove_pointer<T2>::type Value2Class;
            typedef typename AZStd::pair<T1, T2> PairType;
        public:
            AZStdPairContainer()
            {
                // FIXME: We should properly fill in all the other fields as well.
                m_value1ClassElement.m_name = "value1";
                m_value1ClassElement.m_nameCrc = AZ_CRC_CE("value1");
                m_value1ClassElement.m_offset = 0;
                SetupClassElementFromType<T1>(m_value1ClassElement);

                m_value2ClassElement.m_name = "value2";
                m_value2ClassElement.m_nameCrc = AZ_CRC_CE("value2");
                m_value2ClassElement.m_offset = sizeof(T1);
                SetupClassElementFromType<T2>(m_value2ClassElement);
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_value1ClassElement.m_nameCrc)
                {
                    return &m_value1ClassElement;
                }
                if (elementNameCrc == m_value2ClassElement.m_nameCrc)
                {
                    return &m_value2ClassElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_value1ClassElement.m_nameCrc)
                {
                    classElement = m_value1ClassElement;
                    return true;
                }
                else if (dataElement.m_nameCrc == m_value2ClassElement.m_nameCrc)
                {
                    classElement = m_value2ClassElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                PairType* pairPtr = reinterpret_cast<PairType*>(instance);

                T1* value1Ptr = &pairPtr->first;
                T2* value2Ptr = &pairPtr->second;

                if (cb(value1Ptr, m_value1ClassElement.m_typeId, m_value1ClassElement.m_genericClassInfo ? m_value1ClassElement.m_genericClassInfo->GetClassData() : nullptr, &m_value1ClassElement))
                {
                    cb(value2Ptr, m_value2ClassElement.m_typeId, m_value2ClassElement.m_genericClassInfo ? m_value2ClassElement.m_genericClassInfo->GetClassData() : nullptr, &m_value2ClassElement);
                }
            }

            void EnumTypes(const ElementTypeCB& cb) override
            {
                cb(m_value1ClassElement.m_typeId, &m_value1ClassElement);
                cb(m_value2ClassElement.m_typeId, &m_value2ClassElement);
            }

            /// Return number of elements in the container.
            size_t  Size(void* instance) const override
            {
                (void)instance;
                return 2;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 2;
            }


            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool    IsStableElements() const override           { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            bool    IsFixedSize() const override                { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            bool    IsFixedCapacity() const override            { return true; }

            /// Returns true if the container is a smart pointer.
            bool    IsSmartPointer() const override             { return false; }

            /// Returns true if elements can be retrieved by index.
            bool    CanAccessElementsByIndex() const override   { return true; }

            /// Reserve element
            void*   ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                PairType* pairPtr = reinterpret_cast<PairType*>(instance);
                if (classElement->m_nameCrc == m_value1ClassElement.m_nameCrc)
                {
                    return &pairPtr->first;
                }
                if (classElement->m_nameCrc == m_value2ClassElement.m_nameCrc)
                {
                    return &pairPtr->second;
                }
                return nullptr; // you can't add any new elements no this container.
            }

            /// Get an element's address by its index (called before the element is loaded).
            void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                PairType* pairPtr = reinterpret_cast<PairType*>(instance);
                if (index == 0)
                {
                    return &pairPtr->first;
                }
                if (index == 1)
                {
                    return &pairPtr->second;
                }
                return nullptr; // you can't add any new elements no this container.
            }

            /// Store element
            void    StoreElement(void* instance, void* element) override         { (void)instance; (void)element; }

            /// Remove element in the container.
            bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                PairType* pairPtr = reinterpret_cast<PairType*>(instance);
                if (&pairPtr->first == element && deletePointerDataContext)
                {
                    DeletePointerData(deletePointerDataContext, &m_value1ClassElement, &pairPtr->first);
                    pairPtr->first = typename PairType::first_type();
                }
                if (&pairPtr->second == element && deletePointerDataContext)
                {
                    DeletePointerData(deletePointerDataContext, &m_value2ClassElement, &pairPtr->second);
                    pairPtr->second = typename PairType::second_type();
                }
                return false; // you can't remove element from this container.
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
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
            void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                if (deletePointerDataContext)
                {
                    PairType* pairPtr = reinterpret_cast<PairType*>(instance);
                    DeletePointerData(deletePointerDataContext, &m_value1ClassElement, &pairPtr->first);
                    DeletePointerData(deletePointerDataContext, &m_value2ClassElement, &pairPtr->second);
                    *pairPtr = PairType();
                }
            }

            SerializeContext::ClassElement m_value1ClassElement;
            SerializeContext::ClassElement m_value2ClassElement;
        };

        // Define our own SafeArrayTupleSize, as we want to avoid including tuple and array
        // Notably, we always have a SafeArrayTupleSize::value of at least 1, to allow arrays to be allocated to these dimensions
        template <class T>
        struct ArraySafeTupleSize : public AZStd::integral_constant<size_t, 1> {};

        template <class TFirst, class TSecond, class... TRest>
        struct ArraySafeTupleSize<AZStd::tuple<TFirst, TSecond, TRest...>> : public AZStd::integral_constant<size_t, 2 + sizeof...(TRest)> {};

        template<typename TupleType, size_t Index>
        struct CreateClassElementHelper
        {
            // First class element: Initializes a class element that starts at index 0
            static void Create(SerializeContext::ClassElement (&classElements)[ArraySafeTupleSize<TupleType>::value])
            {
                using ElementType = AZStd::tuple_element_t<Index, TupleType>;
                classElements[Index].m_name = Internal::IndexToCStr<Index + 1>::value;
                classElements[Index].m_nameCrc = Crc32(classElements[Index].m_name);
                // Use the previous class element data to calculate the next class element offset
                classElements[Index].m_offset = classElements[Index - 1].m_offset + classElements[Index - 1].m_dataSize;
                SetupClassElementFromType<ElementType>(classElements[Index]);
            }
        };

        template<typename TupleType>
        struct CreateClassElementHelper<TupleType, 0>
        {
            static void Create(SerializeContext::ClassElement (&classElements)[ArraySafeTupleSize<TupleType>::value])
            {
                using ElementType = AZStd::tuple_element_t<0, TupleType>;
                classElements[0].m_name = "Value1";
                classElements[0].m_nameCrc = Crc32("Value1");
                classElements[0].m_offset = 0;
                SetupClassElementFromType<ElementType>(classElements[0]);
            }
        };

        template<typename... Types>
        class AZStdTupleContainer
            : public SerializeContext::IDataContainer
        {
            using TupleType = AZStd::tuple<Types...>;
        public:
            static const constexpr size_t s_tupleSize = AZStd::tuple_size<TupleType>::value;

            AZStdTupleContainer()
            {
                CreateClassElementArray(AZStd::make_index_sequence<s_tupleSize>{});
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                return GetElementAddressTuple(elementNameCrc, AZStd::make_index_sequence<s_tupleSize>{});
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                return GetElementTuple(classElement, dataElement.m_nameCrc, AZStd::make_index_sequence<s_tupleSize>{});
            }

            /// Enumerate elements in the array
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                EnumElementsTuple(*tuplePtr, cb, AZStd::make_index_sequence<s_tupleSize>{});
            }

            void EnumTypes(const ElementTypeCB& cb) override
            {
                auto tupleSize = ArraySafeTupleSize<TupleType>::value;
                for (decltype(tupleSize) i = 0; i < tupleSize; ++i)
                {
                    cb(m_valueClassElements[i].m_typeId, &m_valueClassElements[i]);
                }

            }

            /// Return number of elements in the container.
            size_t  Size(void*) const override
            {
                return s_tupleSize;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void*) const override
            {
                return s_tupleSize;
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
             bool CanAccessElementsByIndex() const override { return true; }

            /// Reserve element
             void* ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                return ReserveElementTuple(*tuplePtr, classElement, AZStd::make_index_sequence<s_tupleSize>{});
            }

            /// Get an element's address by its index (called before the element is loaded).
            void* GetElementByIndex(void* instance, const SerializeContext::ClassElement*, size_t index) override
            {
                auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                return GetElementByIndexTuple(*tuplePtr, index, AZStd::make_index_sequence<s_tupleSize>{});
            }

            /// Store element
            void StoreElement(void* instance, void* element) override { (void)instance; (void)element; }

            /// Remove element in the container.
            bool RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                RemoveElementTuple(*tuplePtr, element, deletePointerDataContext, AZStd::make_index_sequence<s_tupleSize>{});
                return false; // you can't remove element from this container.
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
                    auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                    DeletePointerDataTuple(*tuplePtr, deletePointerDataContext, AZStd::make_index_sequence<s_tupleSize>{});
                    *tuplePtr = TupleType();
                }
            }

            SerializeContext::ClassElement m_valueClassElements[ArraySafeTupleSize<TupleType>::value];

            private:

                template<size_t... Indices>
                void CreateClassElementArray(AZStd::index_sequence<Indices...>)
                {
                    int dummy[]{ 0, (CreateClassElementHelper<TupleType, Indices>::Create(m_valueClassElements), 0)... };
                    (void)dummy;
                }

                template<size_t Index>
                bool GetElementAddressTuple(u32 elementNameCrc, const SerializeContext::ClassElement*& classElement) const
                {
                    if (!classElement && elementNameCrc == m_valueClassElements[Index].m_nameCrc)
                    {
                        classElement = &m_valueClassElements[Index];
                        return true;
                    }
                    return false;
                }
                template<size_t... Indices>
                const SerializeContext::ClassElement* GetElementAddressTuple(u32 elementNameCrc, AZStd::index_sequence<Indices...>) const
                {
                    (void)elementNameCrc;
                    const SerializeContext::ClassElement* classElement{};
                    bool dummy[]{ true, (GetElementAddressTuple<Indices>(elementNameCrc, classElement))... };
                    (void)dummy;
                    return classElement;
                }

                template<size_t Index>
                bool GetElementTuple(SerializeContext::ClassElement& classElement, u32 elementNameCrc, bool& elementFound) const
                {
                    if (!elementFound && elementNameCrc == m_valueClassElements[Index].m_nameCrc)
                    {
                        elementFound = true;
                        classElement = m_valueClassElements[Index];
                        return true;
                    }
                    return false;
                }
                template<size_t... Indices>
                bool GetElementTuple(SerializeContext::ClassElement& classElement, u32 elementNameCrc, AZStd::index_sequence<Indices...>) const
                {
                    (void)classElement;
                    (void)elementNameCrc;
                    bool elementFound = false;
                    bool dummy[]{ true, (GetElementTuple<Indices>(classElement, elementNameCrc, elementFound))... };
                    (void)dummy;
                    return elementFound;
                }

                template<size_t Index>
                bool EnumElementsTuple(TupleType& tupleRef, const ElementCB& cb, bool& aggregateCallbackResult)
                {
                    if (aggregateCallbackResult && cb(&AZStd::get<Index>(tupleRef), m_valueClassElements[Index].m_typeId,
                        m_valueClassElements[Index].m_genericClassInfo ? m_valueClassElements[Index].m_genericClassInfo->GetClassData() : nullptr, &m_valueClassElements[Index]))
                    {
                        return true;
                    }

                    aggregateCallbackResult = false;
                    return false;
                }
                template<size_t... Indices>
                void EnumElementsTuple(TupleType& tupleRef, const ElementCB& cb, AZStd::index_sequence<Indices...>)
                {
                    // When the parameter pack is empty the invoked Member functions are not called and therefore the function parameters are unused
                    (void)tupleRef;
                    (void)cb;
                    bool aggregateCBResult = true;
                    (void)aggregateCBResult;
                    bool dummy[]{ true, (EnumElementsTuple<Indices>(tupleRef, cb, aggregateCBResult))... };
                    (void)dummy;
                }

                template<size_t Index>
                bool ReserveElementTuple(TupleType& tupleRef, const SerializeContext::ClassElement* classElement, void*& reserveElement)
                {
                    if (!reserveElement && m_valueClassElements[Index].m_nameCrc == classElement->m_nameCrc)
                    {
                        reserveElement = &AZStd::get<Index>(tupleRef);
                        return true;
                    }
                    return false;
                }

                template<size_t... Indices>
                void* ReserveElementTuple(TupleType& tupleRef, const SerializeContext::ClassElement* classElement, AZStd::index_sequence<Indices...>)
                {
                    (void)tupleRef;
                    (void)classElement;
                    void* reserveElement{};
                    using DummyArray = bool[];
                    [[maybe_unused]] DummyArray dummy = { true, (ReserveElementTuple<Indices>(tupleRef, classElement, reserveElement))... };
                    return reserveElement;
                }

                template<size_t Index>
                bool GetElementByIndexTuple(TupleType& tupleRef, size_t index, void*& resultElement)
                {
                    if (index == Index)
                    {
                        resultElement = &AZStd::get<Index>(tupleRef);
                        return true;
                    }
                    return false;
                }
                template<size_t... Indices>
                void* GetElementByIndexTuple(TupleType& tupleRef, size_t index, AZStd::index_sequence<Indices...>)
                {
                    (void)tupleRef;
                    (void)index;
                    void* resultElement{};
                    bool dummy[]{ true, (GetElementByIndexTuple<Indices>(tupleRef, index, resultElement))... };
                    (void)dummy;
                    return resultElement;
                }

                template<size_t Index>
                bool RemoveElementTuple(TupleType& tupleRef, const void* element, SerializeContext* deletePointerDataContext)
                {
                    if (&AZStd::get<Index>(tupleRef) == element && deletePointerDataContext)
                    {
                        DeletePointerData(deletePointerDataContext, &m_valueClassElements[Index], &AZStd::get<Index>(tupleRef));
                        AZStd::get<Index>(tupleRef) = AZStd::tuple_element_t<Index, TupleType>();
                        return true;
                    }
                    return false;
                }
                template<size_t... Indices>
                bool RemoveElementTuple(TupleType& tupleRef, const void* element, SerializeContext* deletePointerDataContext, AZStd::index_sequence<Indices...>)
                {
                    (void)tupleRef;
                    (void)element;
                    (void)deletePointerDataContext;
                    bool dummy[]{true, (RemoveElementTuple<Indices>(tupleRef, element, deletePointerDataContext))...};
                    (void)dummy;
                    return false; // Elements cannot be removed from tuple container
                }

                template<size_t... Indices>
                void DeletePointerDataTuple(TupleType& tupleRef, SerializeContext* deletePointerDataContext, AZStd::index_sequence<Indices...>)
                {
                    (void)tupleRef;
                    (void)deletePointerDataContext;
                    int dummy[]{0, (DeletePointerData(deletePointerDataContext, &m_valueClassElements[Indices], &AZStd::get<Indices>(tupleRef)), 0)...};
                    (void)dummy;
                }
        };

        template <class T>
        class AZRValueContainer
            : public SerializeContext::IDataContainer
        {
            typedef typename Internal::RValueToLValueWrapper<T> WrapperType;
        public:
            AZRValueContainer()
            {
                m_valueClassElement.m_name = "value";
                m_valueClassElement.m_nameCrc = AZ_CRC_CE("value");
                m_valueClassElement.m_offset = 0;
                SetupClassElementFromType<T>(m_valueClassElement);
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_valueClassElement.m_nameCrc)
                {
                    return &m_valueClassElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_valueClassElement.m_nameCrc)
                {
                    classElement = m_valueClassElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                WrapperType* wrapperPtr = reinterpret_cast<WrapperType*>(instance);
                cb(&wrapperPtr->m_data, m_valueClassElement.m_typeId, m_valueClassElement.m_genericClassInfo ? m_valueClassElement.m_genericClassInfo->GetClassData() : nullptr, &m_valueClassElement);
            }

            void EnumTypes(const ElementTypeCB& cb) override
            {
                cb(m_valueClassElement.m_typeId, &m_valueClassElement);
            }

            /// Return number of elements in the container.
            size_t  Size(void* instance) const override
            {
                (void)instance;
                return 1;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 1;
            }


            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool    IsStableElements() const override
            {
                return true;
            }

            /// Returns true if the container is fixed size, otherwise false.
            bool    IsFixedSize() const override
            {
                return true;
            }

            /// Returns if the container is fixed capacity, otherwise false
            bool    IsFixedCapacity() const override
            {
                return true;
            }

            /// Returns true if the container is a smart pointer.
            bool    IsSmartPointer() const override
            {
                return false;
            }

            /// Returns true if elements can be retrieved by index.
            bool    CanAccessElementsByIndex() const override
            {
                return true;
            }

            /// Reserve element
            void*   ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                WrapperType* wrapperPtr = reinterpret_cast<WrapperType*>(instance);
                if (classElement->m_nameCrc == m_valueClassElement.m_nameCrc)
                {
                    return &wrapperPtr->m_data;
                }
                return nullptr; // you can't add any new elements no this container.
            }

            /// Get an element's address by its index (called before the element is loaded).
            void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                WrapperType* wrapperPtr = reinterpret_cast<WrapperType*>(instance);
                if (index == 0)
                {
                    return &wrapperPtr->m_data;
                }
                return nullptr; // you can't add any new elements no this container.
            }

            /// Store element
            void    StoreElement(void* instance, void* element) override
            {
                (void)instance; (void)element;
            }

            /// Remove element in the container.
            bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                WrapperType* wrapperPtr = reinterpret_cast<WrapperType*>(instance);
                if (&wrapperPtr->m_data == element && deletePointerDataContext)
                {
                    DeletePointerData(deletePointerDataContext, &m_valueClassElement, &wrapperPtr->m_data);
                }
                return false; // you can't remove element from this container.
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
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
            void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                if (deletePointerDataContext)
                {
                    WrapperType* wrapperPtr = reinterpret_cast<WrapperType*>(instance);
                    DeletePointerData(deletePointerDataContext, &m_valueClassElement, &wrapperPtr->m_data);
                }
            }

            SerializeContext::ClassElement m_valueClassElement;
        };

        template <class T>
        struct AZSmartPtrValueType
        {
            using value_type = typename T::value_type;
        };

        template <class E, class Deleter>
        struct AZSmartPtrValueType<std::unique_ptr<E, Deleter>>
        {
            using value_type = typename std::unique_ptr<E, Deleter>::element_type;
        };

        // Smart pointer generic handler
        template<class T>
        class AZStdSmartPtrContainer
            : public SerializeContext::IDataContainer
        {
        public:
            typedef typename AZSmartPtrValueType<T>::value_type ValueType;
            AZStdSmartPtrContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_dataSize = sizeof(ValueType*);
                m_classElement.m_offset = 0;
                m_classElement.m_azRtti = GetRttiHelper<ValueType>();
                m_classElement.m_flags = SerializeContext::ClassElement::FLG_POINTER;
                m_classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueType>::GetGenericInfo();
                m_classElement.m_typeId = SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
                m_classElement.m_editData = nullptr;
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(AZ::u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                T* smartPtr = reinterpret_cast<T*>(instance);
                // HACK HACK HACK!!!
                // We assume that the data pointer is at offset 0 inside the smart pointer!!!
                typename T::element_type * *valuePtr = reinterpret_cast<typename T::element_type**>(smartPtr);
                cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement);
            }

            void EnumTypes(const ElementTypeCB& cb) override
            {
                cb(m_classElement.m_typeId, &m_classElement);
            }

            /// Return number of elements in the container.
            size_t  Size(void* instance) const override
            {
                (void)instance;
                return 1;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 1;
            }


            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool    IsStableElements() const override           { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            bool    IsFixedSize() const override                { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            bool    IsFixedCapacity() const override            { return true; }

            /// Returns true if the container is a smart pointer.
            bool    IsSmartPointer() const override             { return true; }

            /// Returns true if the container elements can be addressesd by index, otherwise false.
            bool    CanAccessElementsByIndex() const override   { return false; }

            /// Reserve element
            void*   ReserveElement(void* instance, const  SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                T* smartPtr = reinterpret_cast<T*>(instance);
                smartPtr->reset();
                return instance; // store the pointer in the first element of the smart pointer (this is hack, but it's true for our pointers and used in the serializer)
            }

            /// Get an element's address by its index (called before the element is loaded).
            void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                (void)index;

                void* ptrToRawPtr = nullptr;
                auto captureValue = [&ptrToRawPtr](void* ptr, const AZ::Uuid&, const AZ::SerializeContext::ClassData*, const AZ::SerializeContext::ClassElement*) -> bool
                {
                    ptrToRawPtr = ptr;
                    return false;
                };
                EnumElements(instance, captureValue);
                typename T::element_type *valuePtr = *reinterpret_cast<typename T::element_type**>(ptrToRawPtr);
                return valuePtr;
            }

            /// Store element
            void    StoreElement(void* instance, void* element) override
            {
                (void)element;
                void* newElementAddress = *reinterpret_cast<void**>(instance);
                *reinterpret_cast<void**>(instance) = nullptr; // reset the smart pointer first element to the value before Reserve element
                T* smartPtr = reinterpret_cast<T*>(instance);
                *smartPtr = T(reinterpret_cast<ValueType*>(newElementAddress)); // store the new element properly, by assignment (so all counters are correct)
            }

            /// Remove element in the container.
            bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                (void)deletePointerDataContext;
                (void)element;
                T* smartPtr = reinterpret_cast<T*>(instance);
                smartPtr->reset();
                return false; // you can't remove element from this container.
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                (void)deletePointerDataContext;
                (void)elements;
                (void)numElements;
                T* smartPtr = reinterpret_cast<T*>(instance);
                smartPtr->reset();
                return 0; // you can't remove elements from this container.
            }

            /// Clear elements in the instance.
            void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                (void)deletePointerDataContext;
                T* smartPtr = reinterpret_cast<T*>(instance);
                smartPtr->reset();
            }

            void    FreeReservedElement(void* instance, void* element, SerializeContext* deletePointerDataContext) override
            {
                // First store the target to properly set all counters required by the smart pointer swap later on.
                StoreElement(instance, element);
                // With all the counters set it's now safe to reset.
                RemoveElement(instance, element, deletePointerDataContext);
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };

        template<class T>
        class AZStdOptionalContainer
            : public SerializeContext::IDataContainer
        {
        public:
            using ValueType = typename AZSmartPtrValueType<T>::value_type;
            AZStdOptionalContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_dataSize = sizeof(ValueType*);
                m_classElement.m_offset = 0;
                m_classElement.m_azRtti = GetRttiHelper<ValueType>();
                m_classElement.m_flags = AZStd::is_pointer<ValueType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                m_classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueType>::GetGenericInfo();
                m_classElement.m_typeId = SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
                m_classElement.m_editData = nullptr;
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(AZ::u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                T* optionalPtr = reinterpret_cast<T*>(instance);
                if (optionalPtr->has_value())
                {
                    // HACK This returns a pointer to to the beginning of the
                    // optional, assuming it is where the optional stores its
                    // typed object
                    typename T::value_type* valuePtr = AZStd::addressof(optionalPtr->value());
                    cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement);
                }
            }

            void EnumTypes(const ElementTypeCB& cb) override
            {
                cb(m_classElement.m_typeId, &m_classElement);
            }

            /// Return number of elements in the container.
            size_t Size(void* instance) const override
            {
                T* optional = reinterpret_cast<T*>(instance);
                return optional->has_value() ? 1 : 0;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 1;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool IsStableElements() const override           { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            bool IsFixedSize() const override                { return false; }

            /// Returns if the container is fixed capacity, otherwise false
            bool IsFixedCapacity() const override            { return true; }

            /// Returns true if the container is a smart pointer.
            bool IsSmartPointer() const override             { return true; }

            /// Returns true if the container elements can be addressed by index, otherwise false.
            bool CanAccessElementsByIndex() const override   { return false; }

            /// Reserve element
            void* ReserveElement(void* instance, const  SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                T* optional = reinterpret_cast<T*>(instance);
                optional->emplace();
                return AZStd::addressof(optional->value());
            }

            /// Get an element's address by its index (called before the element is loaded).
            /// Caller should ensure CanAccessElementsByIndex returns true for this to be valid.
            void* GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                (void)index;

                void* ptrToRawPtr = nullptr;
                auto captureValue = [&ptrToRawPtr](void* ptr, const AZ::Uuid&, const AZ::SerializeContext::ClassData*, const AZ::SerializeContext::ClassElement*) -> bool
                {
                    ptrToRawPtr = ptr;
                    return false;
                };
                EnumElements(instance, captureValue);
                typename T::value_type* valuePtr = *reinterpret_cast<typename T::value_type**>(ptrToRawPtr);
                return valuePtr;
            }

            /// Store element
            void StoreElement(void* instance, void* element) override
            {
                // optional is already stored, was created in ReserveElement()
                AZ_UNUSED(instance);
                AZ_UNUSED(element);
            }

            /// Remove element in the container.
            bool RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                (void)deletePointerDataContext;
                (void)element;
                T* optional = reinterpret_cast<T*>(instance);
                optional->reset();
                return false; // you can't remove element from this container.
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                (void)deletePointerDataContext;
                (void)elements;
                (void)numElements;
                T* optional = reinterpret_cast<T*>(instance);
                optional->reset();
                return 0; // you can't remove elements from this container.
            }

            /// Clear elements in the instance.
            void ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                (void)deletePointerDataContext;
                T* optional = reinterpret_cast<T*>(instance);
                optional->reset();
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };

        // basic string serialization
        template<class T>
        class AZStdString
            : public SerializeContext::IDataSerializer
        {
        public:
            /// Convert binary data to text
            size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;

                size_t dataSize = static_cast<size_t>(in.GetLength());

                AZStd::string outText;
                outText.resize(dataSize);
                in.Read(dataSize, outText.data());

                return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
            }

            /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
            size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
            {
                (void)textVersion;
                (void)isDataBigEndian;

                size_t bytesToWrite = strlen(text);

                return static_cast<size_t>(stream.Write(bytesToWrite, reinterpret_cast<const void*>(text)));
            }

            /// Store the class data into a stream.
            size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian = false) override
            {
                (void)isDataBigEndian;

                const T* string = reinterpret_cast<const T*>(classPtr);
                size_t bytesToWrite = string->size();

                return static_cast<size_t>(stream.Write(bytesToWrite, string->c_str()));
            }

            /// Load the class data from a stream.
            bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
            {
                (void)isDataBigEndian;

                T* string = reinterpret_cast<T*>(classPtr);
                size_t textLen = static_cast<size_t>(stream.GetLength());

                string->resize(textLen);
                stream.Read(textLen, string->data());

                return true;
            }

            bool    CompareValueData(const void* lhs, const void* rhs) override
            {
                return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
            }
        };

        class AZBinaryData
            : public SerializeContext::IDataSerializer
        {
        public:
            size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;
                const size_t dataSize = static_cast<size_t>(in.GetLength()); // each byte is encoded in 2 chars (hex)

                AZStd::string outStr;
                if (dataSize)
                {
                    AZ::u8* buffer = static_cast<AZ::u8*>(azmalloc(dataSize));

                    in.Read(dataSize, reinterpret_cast<void*>(buffer));

                    const AZ::u8* first = buffer;
                    const AZ::u8* last = first + dataSize;
                    if (first == last)
                    {
                        azfree(buffer);
                        outStr.clear();
                        return 1; // for no data (just terminate)
                    }

                    outStr.resize(dataSize * 2);

                    char* data = outStr.data();
                    for (; first != last; ++first, data += 2)
                    {
                        azsnprintf(data, 3, "%02X", *first);
                    }

                    azfree(buffer);
                }

                return static_cast<size_t>(out.Write(outStr.size(), outStr.data()));
            }

            size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
            {
                (void)textVersion;
                (void)isDataBigEndian;
                size_t textLenth = strlen(text);
                size_t minDataSize = textLenth / 2;

                const char* textEnd = text + textLenth;
                char workBuffer[3];
                workBuffer[2] = '\0';
                for (; text != textEnd; text += 2)
                {
                    workBuffer[0] = text[0];
                    workBuffer[1] = text[1];
                    AZ::u8 value = static_cast<AZ::u8>(strtoul(workBuffer, NULL, 16));
                    stream.Write(sizeof(AZ::u8), &value);
                }
                return minDataSize;
            }
        };

        template<class Allocator>
        class AZByteStream
            : public AZBinaryData
        {
            typedef AZStd::vector<AZ::u8, Allocator> ContainerType;

            /// Load the class data from a stream.
            bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
            {
                (void)isDataBigEndian;
                const AZ::u8* dataStart;
                const AZ::u8* dataEnd;
                ContainerType* container = reinterpret_cast<ContainerType*>(classPtr);

                size_t dataSize = static_cast<size_t>(stream.GetLength());
                if (dataSize)
                {
                    container->resize_no_construct(dataSize);
                    size_t bytesRead = static_cast<size_t>(stream.Read(dataSize, container->data()));
                    dataStart = container->data();
                    dataEnd = dataStart + bytesRead;
                }
                else
                {
                    dataStart = nullptr;
                    dataEnd = nullptr;
                    *container = ContainerType(dataStart, dataEnd);
                }

                return true;
            }

            /// Store the class data into a stream.
            size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;
                const ContainerType* container = reinterpret_cast<const ContainerType*>(classPtr);
                size_t dataSize = container->size();
                return static_cast<size_t>(stream.Write(dataSize, container->data()));
            }

            bool    CompareValueData(const void* lhs, const void* rhs) override
            {
                return SerializeContext::EqualityCompareHelper<ContainerType>::CompareValues(lhs, rhs);
            }
        };

        template<AZStd::size_t NumBits>
        class AZBitSet
            : public AZBinaryData
        {
            typedef typename AZStd::bitset<NumBits> ContainerType;

            /// Store the class data into a stream.
            size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;
                const ContainerType* container = reinterpret_cast<const ContainerType*>(classPtr);
                size_t dataSize = container->num_words() * sizeof(typename ContainerType::word_t);
                return static_cast<size_t>(stream.Write(dataSize, container->data()));
            }

            /// Load the class data from a stream.
            bool Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
            {
                (void)isDataBigEndian;
                ContainerType* container = reinterpret_cast<ContainerType*>(classPtr);
                size_t dataSize = static_cast<size_t>(stream.GetLength());
                size_t bytes = container->num_words() * sizeof(typename ContainerType::word_t);
                (void)dataSize;
                AZ_Assert(bytes <= dataSize, "Not enough data provided expected %d bytes and got %d", bytes, dataSize);

                stream.Read(bytes, container->data());
                return true;
            }

            bool    CompareValueData(const void* lhs, const void* rhs) override
            {
                return SerializeContext::EqualityCompareHelper<ContainerType>::CompareValues(lhs, rhs);
            }
        };
    } // namespace Internal

    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_UUID(AZ::Internal::RValueToLValueWrapper, "RValueToLValueWrapper", "{2590807F-5748-4CD0-A475-83EF5FD216CF}", AZ_TYPE_INFO_TYPENAME);

    AZ_INLINE static constexpr AZ::TypeId GetGenericClassInfoVectorTypeId()
    {
        return Uuid("{2BADE35A-6F1B-4698-B2BC-3373D010020C}");
    };

    /// Generic specialization for AZStd::vector
    template<class T, class A>
    struct SerializeGenericTypeInfo< AZStd::vector<T, A> >
    {
        typedef typename AZStd::vector<T, A> ContainerType;

        class GenericClassInfoVector
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoVector, GetGenericClassInfoVectorTypeId());
            GenericClassInfoVector()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            AZ::TypeId GetLegacySpecializedTypeId() const override
            {
                // There are still legacy slice, uicanvas and ScriptCanvas files that serialize
                // an `AZStd::vector<Component*, Alloc>` using the typeid for `AZStd::vector<Component, Alloc>`
                // All of these files were created before 2018
                return AZ::AzTypeInfo<T>::GetPointeeTypeId() + AZ::AzTypeInfo<A>::GetCanonicalTypeId() + AZ::AzGenericTypeInfo::Uuid<AZStd::vector>();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdRandomAccessContainer<ContainerType, false> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassInfoVector;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    AZ_INLINE static constexpr AZ::TypeId GetGenericClassInfoFixedVectorTypeId()
    {
        return Uuid("{6C6751B0-392A-4E71-8BF8-179484D7D22F}");
    };

    /// Generic specialization for AZStd::fixed_vector
    template<class T, size_t Capacity>
    struct SerializeGenericTypeInfo< AZStd::fixed_vector<T, Capacity> >
    {
        typedef typename AZStd::fixed_vector<T, Capacity>    ContainerType;

        class GenericClassInfoFixedVector
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoFixedVector, GetGenericClassInfoFixedVectorTypeId());
            GenericClassInfoFixedVector()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdFixedCapacityRandomAccessContainer<ContainerType, true, Capacity> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassInfoFixedVector;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    /// Generic specialization for AZStd::list
    template<class T, class A>
    struct SerializeGenericTypeInfo< AZStd::list<T, A> >
    {
        typedef typename AZStd::list<T, A>           ContainerType;

        class GenericClassInfoList
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoList, "{B845AD64-B5A0-4ccd-A86B-3477A36779BE}");
            GenericClassInfoList()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdBasicContainer<ContainerType, true> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassInfoList;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    /// Generic specialization for AZStd::forward_list
    template<class T, class A>
    struct SerializeGenericTypeInfo< AZStd::forward_list<T, A> >
    {
        typedef typename AZStd::forward_list<T, A> ContainerType;

        class GenericClassInfoForwardList
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoForwardList, "{D48E20E1-D3A3-46F7-A869-927F5EF50127}");
            GenericClassInfoForwardList()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdBasicContainer<ContainerType, true> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassInfoForwardList;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    AZ_INLINE static constexpr AZ::TypeId GetGenericClassInfoArrayTypeId()
    {
        return Uuid("{286E1198-0867-4198-95D3-6CC569658E07}");
    };

    /// Generic specialization for AZStd::array
    template<class T, size_t Size>
    struct SerializeGenericTypeInfo< AZStd::array<T, Size> >
    {
        typedef typename AZStd::array<T, Size>  ContainerType;

        class GenericClassInfoArray
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoArray, GetGenericClassInfoArrayTypeId());
            GenericClassInfoArray()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
                m_classData.m_eventHandler = &m_eventHandler;
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdArrayContainer<T, Size> m_containerStorage;
            SerializeContext::ClassData m_classData;
            Internal::AZStdArrayEvents m_eventHandler;
        };

        using ClassInfoType = GenericClassInfoArray;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    AZ_INLINE static constexpr AZ::TypeId GetGenericClassSetTypeId()
    {
        return Uuid("{4A64D2A5-7265-4E3D-805C-BA2D0626F542}");
    };

    // Generic specialization for AZStd::set
    template<class K, class C, class A>
    struct SerializeGenericTypeInfo< AZStd::set<K, C, A> >
    {
        typedef typename AZStd::set<K, C, A>        ContainerType;

        class GenericClassSet
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassSet, GetGenericClassSetTypeId());
            GenericClassSet()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t /*element*/) override
            {
                return SerializeGenericTypeInfo<K>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    Internal::AZStdAssociativeContainer<ContainerType>::Reflect(serializeContext);
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassSet;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    AZ_INLINE static constexpr AZ::TypeId GetGenericClassUnorderedSetTypeId()
    {
        return Uuid("{B04E902E-C6F7-4212-A166-1B52F7437D3C}");
    };

    /// Generic specialization for AZStd::unordered_set
    template<class K, class H, class E, class A>
    struct SerializeGenericTypeInfo< AZStd::unordered_set<K, H, E, A> >
    {
        typedef typename AZStd::unordered_set<K, H, E, A>      ContainerType;

        class GenericClassUnorderedSet
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUnorderedSet, GetGenericClassUnorderedSetTypeId());
            GenericClassUnorderedSet()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<K>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    Internal::AZStdAssociativeContainer<ContainerType>::Reflect(serializeContext);
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassUnorderedSet;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    /// Generic specialization for AZStd::unordered_multiset
    template<class K, class H, class E, class A>
    struct SerializeGenericTypeInfo< AZStd::unordered_multiset<K, H, E, A> >
    {
        typedef typename AZStd::unordered_multiset<K, H, E, A>      ContainerType;

        class GenericClassUnorderedMultiSet
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUnorderedMultiSet, "{B619DA0D-F050-41AA-B092-416CACB7C710}");
            GenericClassUnorderedMultiSet()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<K>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    Internal::AZStdAssociativeContainer<ContainerType>::Reflect(serializeContext);
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassUnorderedMultiSet;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    AZ_INLINE static constexpr AZ::TypeId GetGenericOutcomeTypeId()
    {
        return Uuid("{DF6803FE-1C95-4DB8-8C08-6CDA5353ACD7}");
    };

    /// Generic specialization for AZ::Outcome
    template<class t_Success, class t_Failure>
    struct SerializeGenericTypeInfo< AZ::Outcome<t_Success, t_Failure> >
    {
        typedef typename AZ::Outcome<t_Success, t_Failure> OutcomeType;

        class GenericClassOutcome
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassOutcome, GetGenericOutcomeTypeId());
            GenericClassOutcome()
                : m_classData{ SerializeContext::ClassData::Create<OutcomeType>(AZ::AzTypeInfo<OutcomeType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 2;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                if (element == 0)
                {
                    return SerializeGenericTypeInfo<t_Success>::GetClassTypeId();
                }
                else
                {
                    return SerializeGenericTypeInfo<t_Failure>::GetClassTypeId();
                }
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<OutcomeType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<OutcomeType>::CreateAny);
                    if (GenericClassInfo* successGenericClassInfo = SerializeGenericTypeInfo<t_Success>::GetGenericInfo())
                    {
                        successGenericClassInfo->Reflect(serializeContext);
                    }

                    if (GenericClassInfo* failureGenericClassInfo = SerializeGenericTypeInfo<t_Failure>::GetGenericInfo())
                    {
                        failureGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassOutcome;
        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<OutcomeType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    AZ_INLINE constexpr AZ::TypeId GetGenericClassPairTypeId()
    {
        return Uuid("{9F3F5302-3390-407a-A6F7-2E011E3BB686}");
    };

    /// Generic specialization for AZStd::pair
    template<class T1, class T2>
    struct SerializeGenericTypeInfo< AZStd::pair<T1, T2> >
    {
        typedef typename  AZStd::pair<T1, T2> PairType;

        class GenericClassPair
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassPair, GetGenericClassPairTypeId());
            GenericClassPair()
                : m_classData{ SerializeContext::ClassData::Create<PairType>(AZ::AzTypeInfo<PairType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_pairContainer) }
            {
                // Convert a pair to a string, if both types of the pair
                // supports the AZStd::to_string function
                auto PairToString = [](const void* pairInst) -> AZStd::string
                {
                    [[maybe_unused]] auto pairElement = reinterpret_cast<const PairType*>(pairInst);
                    AZStd::string result;
                    if constexpr (AZStd::HasToString_v<T1> && AZStd::HasToString_v<T2>)
                    {
                        AZStd::string value;
                        AZStd::to_string(value, pairElement->first);
                        result += "<";
                        result += value;
                        result += ", ";
                        AZStd::to_string(value, pairElement->second);
                        result += value;
                        result += ">";
                    }

                    return result;
                };
                m_classData.m_attributes.emplace_back(AZ::Crc32("ToString"), CreateModuleAttribute(AZStd::move(PairToString)));
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 2;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                if (element == 0)
                {
                    return SerializeGenericTypeInfo<T1>::GetClassTypeId();
                }
                else
                {
                    return SerializeGenericTypeInfo<T2>::GetClassTypeId();
                }
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<PairType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<PairType>::CreateAny);
                    if (GenericClassInfo* firstGenericClassInfo = m_pairContainer.m_value1ClassElement.m_genericClassInfo)
                    {
                        firstGenericClassInfo->Reflect(serializeContext);
                    }

                    if (GenericClassInfo* secondGenericClassInfo = m_pairContainer.m_value2ClassElement.m_genericClassInfo)
                    {
                        secondGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdPairContainer<T1, T2> m_pairContainer;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassPair;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<PairType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    AZ_INLINE constexpr AZ::TypeId GetGenericClassTupleTypeId()
    {
        return Uuid("{F98DF943-F870-4FE2-B6A9-3E8BC5861782}");
    };

    /// Generic specialization for AZStd::tuple
    template<typename... Types>
    struct SerializeGenericTypeInfo< AZStd::tuple<Types...> >
    {
        using TupleType = AZStd::tuple<Types...>;

        class GenericClassTuple
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassTuple, GetGenericClassTupleTypeId());
            GenericClassTuple()
                : m_classData{ SerializeContext::ClassData::Create<TupleType>(AZ::AzTypeInfo<TupleType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_tupleContainer) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return Internal::AZStdTupleContainer<TupleType>::s_tupleSize;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                if (GenericClassInfo* valueGenericClassInfo = m_tupleContainer.m_valueClassElements[element].m_genericClassInfo)
                {
                    return valueGenericClassInfo->GetSpecializedTypeId();
                }
                return m_tupleContainer.m_valueClassElements[element].m_typeId;
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<TupleType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<TupleType>::CreateAny);
                    for (size_t index = 0; index < AZStd::tuple_size<TupleType>::value; ++index)
                    {
                        if (GenericClassInfo* valueGenericClassInfo = m_tupleContainer.m_valueClassElements[index].m_genericClassInfo)
                        {
                            valueGenericClassInfo->Reflect(serializeContext);
                        }

                    }
                }
            }

            Internal::AZStdTupleContainer<Types...> m_tupleContainer;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassTuple;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<TupleType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    /// Generic specialization for our internal RValue wrapper
    template<class T>
    struct SerializeGenericTypeInfo< Internal::RValueToLValueWrapper<T> >
    {
        typedef typename  Internal::RValueToLValueWrapper<T> WrapperType;

        class GenericClassWrapper
            : public GenericClassInfo
        {
            inline static const AZStd::fixed_string<128> ClassName = AZStd::fixed_string<128>("Internal::RValueToLValueWrapper<")
                + AZ::AzTypeInfo<T>::Name() + ">";
        public:
            GenericClassWrapper()
                : m_classData{ SerializeContext::ClassData::Create<WrapperType>(ClassName.c_str(), AZ::TypeId("{642ABA5E-BB70-40EF-A986-933420D89F85}"), Internal::NullFactory::GetInstance(), nullptr, &m_wrapperContainer) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<WrapperType>();
            }

            void Reflect(SerializeContext* serializeContext) override
            {

                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<WrapperType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_wrapperContainer.m_valueClassElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZRValueContainer<T> m_wrapperContainer;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassWrapper;
        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<WrapperType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    AZ_INLINE constexpr AZ::TypeId GetGenericClassMapTypeId()
    {
        return Uuid("{DB825311-453D-45C8-B07F-B9CD9A32ACB4}");
    };

    /// Generic specialization for AZStd::map
    template<class K, class M, class C, class A>
    struct SerializeGenericTypeInfo< AZStd::map<K, M, C, A> >
    {
        typedef typename AZStd::map<K, M, C, A>        ContainerType;

        class GenericClassMap
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassMap, GetGenericClassMapTypeId());
            GenericClassMap()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 2;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                if (element == 0)
                {
                    return SerializeGenericTypeInfo<K>::GetClassTypeId();
                }
                else
                {
                    return SerializeGenericTypeInfo<M>::GetClassTypeId();
                }
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    Internal::AZStdAssociativeContainer<ContainerType>::Reflect(serializeContext);
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassMap;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    AZ_INLINE static constexpr AZ::TypeId GetGenericClassUnorderedMapTypeId()
    {
        return Uuid("{18456A80-63CC-40c5-BF16-6AF94F9A9ECC}");
    };

    /// Generic specialization for AZStd::unordered_map
    template<class K, class M, class H, class E, class A>
    struct SerializeGenericTypeInfo< AZStd::unordered_map<K, M, H, E, A> >
    {
        typedef typename AZStd::unordered_map<K, M, H, E, A>        ContainerType;

        class GenericClassUnorderedMap
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUnorderedMap, GetGenericClassUnorderedMapTypeId());
            GenericClassUnorderedMap()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 2;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                if (element == 0)
                {
                    return SerializeGenericTypeInfo<K>::GetClassTypeId();
                }
                else
                {
                    return SerializeGenericTypeInfo<M>::GetClassTypeId();
                }
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    Internal::AZStdAssociativeContainer<ContainerType>::Reflect(serializeContext);
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassUnorderedMap;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    /// Generic specialization for AZStd::unordered_multimap
    template<class K, class M, class H, class E, class A>
    struct SerializeGenericTypeInfo< AZStd::unordered_multimap<K, M, H, E, A> >
    {
        typedef typename AZStd::unordered_multimap<K, M, H, E, A>        ContainerType;

        class GenericClassUnorderedMultiMap
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUnorderedMultiMap, "{119669B8-92BF-468F-97D9-9D45F298BCD4}");
            GenericClassUnorderedMultiMap()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 2;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                if (element == 0)
                {
                    return SerializeGenericTypeInfo<K>::GetClassTypeId();
                }
                else
                {
                    return SerializeGenericTypeInfo<M>::GetClassTypeId();
                }
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    Internal::AZStdAssociativeContainer<ContainerType>::Reflect(serializeContext);
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassUnorderedMultiMap;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    /// Generic specialization for AZStd::string
    template<class E, class T, class A>
    struct SerializeGenericTypeInfo< AZStd::basic_string<E, T, A> >
    {
        typedef typename AZStd::basic_string<E, T, A>     ContainerType;

        class GenericClassBasicString
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassBasicString, "{EF8FF807-DDEE-4eb0-B678-4CA3A2C490A4}");
            GenericClassBasicString()
            {
                const char* className = "";
                if constexpr (AZStd::is_same_v<ContainerType, AZStd::string>)
                {
                    className = "AZStd::string";
                }
                else
                {
                    className = AZ::AzTypeInfo<ContainerType>::Name();
                }
                m_classData = SerializeContext::ClassData::Create<ContainerType>(className, GetSpecializedTypeId(),
                    Internal::NullFactory::GetInstance(), &m_stringSerializer, nullptr);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<E>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                }
            }

            Internal::AZStdString<ContainerType> m_stringSerializer;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassBasicString;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    /// Generic specialization for AZStd::vector<AZ::u8,Allocator> byte stream. If this conflics with a normal vector<unsigned char> create a new type.
    template<class A>
    struct SerializeGenericTypeInfo< AZStd::vector<AZ::u8, A> >
    {
        typedef typename AZStd::vector<AZ::u8, A>    ContainerType;

        class GenericClassByteStream
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassByteStream, "{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}");
            GenericClassByteStream()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>("ByteStream", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), &m_dataSerializer, nullptr) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return AzTypeInfo<AZ::u8>::Uuid();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                }
            }

            Internal::AZByteStream<A> m_dataSerializer;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassByteStream;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    template<AZStd::size_t NumBits>
    struct SerializeGenericTypeInfo< AZStd::bitset<NumBits> >
    {
        typedef typename AZStd::bitset<NumBits> ContainerType;

        class GenericClassBitSet
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassBitSet, "{1C0270B7-F5E1-4bd6-B7BC-8D25A74B79B4}");
            GenericClassBitSet()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), &m_dataSerializer, nullptr) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 0;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return AzTypeInfo<AZ::u8>::Uuid();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                }
            }

            Internal::AZBitSet<NumBits> m_dataSerializer;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassBitSet;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    template<class T>
    struct SerializeGenericTypeInfo< AZStd::shared_ptr<T> >
    {
        typedef typename AZStd::shared_ptr<T>   ContainerType;

        class GenericClassSharedPtr
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassSharedPtr, "{D5B5ACA6-A81E-410E-8151-80C97B8CD2A0}");
            GenericClassSharedPtr()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            // AZStdSmartPtrContainer uses the underlying smart_ptr container value_type typedef type id for serialization
            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            bool CanStoreType(const Uuid& typeId) const override
            {
                return GetSpecializedTypeId() == typeId || GetGenericTypeId() == typeId || m_containerStorage.m_classElement.m_typeId == typeId;
            }

            Internal::AZStdSmartPtrContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassSharedPtr;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    template<class T>
    struct SerializeGenericTypeInfo< AZStd::intrusive_ptr<T> >
    {
        typedef typename AZStd::intrusive_ptr<T>    ContainerType;

        class GenericClassIntrusivePtr
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassIntrusivePtr, "{C4B5400B-5CDC-4B14-932E-BFA30BC1DE35}");
            GenericClassIntrusivePtr()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            // AZStdSmartPtrContainer uses the underlying smart_ptr container value_type typedef type id for serialization
            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            bool CanStoreType(const Uuid& typeId) const override
            {
                return GetSpecializedTypeId() == typeId || GetGenericTypeId() == typeId || m_containerStorage.m_classElement.m_typeId == typeId;
            }

            Internal::AZStdSmartPtrContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassIntrusivePtr;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    template<class T, class Deleter>
    struct SerializeGenericTypeInfo< AZStd::unique_ptr<T, Deleter> >
    {
        typedef typename AZStd::unique_ptr<T, Deleter>   ContainerType;

        class GenericClassUniquePtr
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUniquePtr, "{D37DF835-5B18-4F9E-9C8F-03B967483080}");
            GenericClassUniquePtr()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            // AZStdSmartPtrContainer uses the smart_ptr container type id for serialization
            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            bool CanStoreType(const Uuid& typeId) const override
            {
                return GetSpecializedTypeId() == typeId || GetGenericTypeId() == typeId || m_containerStorage.m_classElement.m_typeId == typeId;
            }

            Internal::AZStdSmartPtrContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassUniquePtr;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    template<class T>
    struct SerializeGenericTypeInfo<AZStd::optional<T>>
    {
        using ContainerType = AZStd::optional<T>;

        class GenericClassOptional
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassOptional, "{2C75B779-4661-4102-9D40-B2A680B6DE36}");
            GenericClassOptional()
                : m_classData{ SerializeContext::ClassData::Create<ContainerType>(AZ::AzTypeInfo<ContainerType>::Name(), GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            // AZStdOptionalContainer uses the underlying container value_type typedef type id for serialization
            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            Internal::AZStdOptionalContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassOptional;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ContainerType>();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };
}

#endif // AZCORE_SERIALIZE_AZSTD_CONTAINERS_H
#pragma once
