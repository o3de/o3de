/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Internal
    {
        template<class AttrType, class DestType, class InstType, typename ... Args>
        bool AttributeRead(DestType& value, Attribute* attr, InstType&& instance, Args&& ... args)
        {
            // try a value
            if (auto data = azdynamic_cast<AttributeData<AttrType>*>(attr); data != nullptr)
            {
                value = static_cast<DestType>(data->Get(instance));
                return true;
            }
            // try a function with return type
            if (auto func = azrtti_cast<AttributeFunction<AttrType(Args...)>*>(attr); func != nullptr)
            {
                value = static_cast<DestType>(func->Invoke(AZStd::forward<InstType>(instance), AZStd::forward<Args>(args)...));
                return true;
            }
            // try a type with an invocable function
            if (auto invocable = azrtti_cast<AttributeInvocable<AttrType(InstType, Args...)>*>(attr); invocable != nullptr)
            {
                value = static_cast<DestType>(invocable->operator()(AZStd::forward<InstType>(instance), AZStd::forward<Args>(args)...));
                return true;
            }
            // try a type with an invocable function that takes no "this" instance
            if (auto invocable = azrtti_cast<AttributeInvocable<AttrType(Args...)>*>(attr); invocable != nullptr)
            {
                value = static_cast<DestType>(invocable->operator()(AZStd::forward<Args>(args)...));
                return true;
            }
            // else you are on your own!
            return false;
        }

        template<class RetType, class InstType, typename ... Args>
        bool AttributeInvoke(Attribute* attr, InstType&& instance, Args&& ... args)
        {
            // try a function
            if (auto func = azrtti_cast<AttributeFunction<RetType(Args...)>*>(attr); func != nullptr)
            {
                func->Invoke(AZStd::forward<InstType>(instance), AZStd::forward<Args>(args)...);
                return true;
            }
            // try a type with an invocable function
            if (auto invocable = azrtti_cast<AttributeInvocable<RetType(InstType, Args...)>*>(attr); invocable != nullptr)
            {
                invocable->operator()(AZStd::forward<InstType>(instance), AZStd::forward<Args>(args)...);
                return true;
            }
            // try a type with an invocable function that takes no "this" instance
            if (auto invocable = azrtti_cast<AttributeInvocable<RetType(Args...)>*>(attr); invocable != nullptr)
            {
                invocable->operator()(AZStd::forward<Args>(args)...);
                return true;
            }
            return false;
        }


        /**
        * Integral types (except bool) can be converted char <-> short <-> int <-> int64
        * for bool it requires exact matching (as comparing to zero is ambiguous)
        * The same applied for floating point types,can convert float <-> double, but conversion to integers is not allowed
        */
        template<class AttrType, class DestType, typename ... Args>
        struct AttributeReader
        {
            template<typename InstType>
            static bool Read(DestType& value, Attribute* attr, InstType&& instance, const Args& ... args)
            {
                if constexpr (AZStd::is_integral_v<DestType> && !AZStd::is_same_v<DestType, bool>)
                {
                    if (AttributeRead<bool, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<char, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned char, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<short, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned short, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<int, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned int, Args..., DestType>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<long, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned long, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<AZ::s64, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<AZ::u64, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<AZ::Crc32, DestType, Args...>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                }
                if constexpr (AZStd::is_floating_point_v<DestType>)
                {
                    if (AttributeRead<float, DestType>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<double, DestType>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                }
                if constexpr (AZStd::is_same_v<DestType, AZStd::string> && (AZStd::is_same_v<AttrType, AZStd::string>
                    || AZStd::is_same_v<AttrType, AZStd::string_view>
                    || AZStd::is_same_v<AttrType, const char*>))
                {
                    if (AttributeRead<const char*, AZStd::string>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                    if (AttributeRead<AZStd::string_view, AZStd::string>(value, attr, AZStd::forward<InstType>(instance), args...))
                    {
                        return true;
                    }
                }
                return AttributeRead<AttrType, DestType>(value, attr, AZStd::forward<InstType>(instance), args...);
            }
        };
    } // namespace Internal

    // AttributeInvoker reads values from Attributes based on the type of Attribute being read from
    // Calling Read<ExpectedTypeOfData>(someVariableOfThatType) will for AttributeData and AttributeMemberData that wrap data, read the value
    // For attributes that wrap functions(AttributeFunction, AttributeMemberFunction, AttributeInvocable), it will invoke those functions to
    // attempt to store the result into the destination parameter
    // Special rules
    // integers will automatically convert if the type fits (for example, even if the attribute is a u8, you can read it in as a u64 or u32 and it will work)
    // floats will automatically convert if the type fits.
    // AZStd::string will also accept const char*  (But not the other way around)
    //
    template<class InstType>
    class AttributeInvoker
    {
    public:
        AZ_CLASS_ALLOCATOR(AttributeInvoker, AZ::SystemAllocator);

        // this is the only thing you should need to call!
        // returns false if it is an incompatible type or fails to read it.
        // for example, Read<int>(someInteger);
        // for example, Read<double>(someDouble);
        // for example, Read<MyStruct>(someStruct that has operator=(const other&) defined)
        template <class T, class U, typename... Args>
        bool Read(U& value, Args&&... args)
        {
            using DecayInstType = AZStd::remove_cvref_t<InstType>;
            if constexpr (AZStd::is_pointer_v<DecayInstType> && AZStd::is_void_v<AZStd::remove_pointer_t<DecayInstType>>)
            {
                // Forward the void* as an rvalue so it is not a void*&
                return Internal::AttributeReader<T, U, Args...>::Read(
                    value, m_attribute, static_cast<DecayInstType>(m_instance), AZStd::forward<Args>(args)...);
            }
            else
            {
                return Internal::AttributeReader<T, U, Args...>::Read(value, m_attribute, m_instance, AZStd::forward<Args>(args)...);
            }
        }

        template <typename RetType, typename... Args>
        bool Invoke(Args&&... args)
        {
            using DecayInstType = AZStd::remove_cvref_t<InstType>;
            if constexpr (AZStd::is_pointer_v<DecayInstType> && AZStd::is_void_v<AZStd::remove_pointer_t<DecayInstType>>)
            {
                // Forward the void* as an rvalue so it is not a void*&
                return Internal::AttributeInvoke<RetType>(m_attribute, static_cast<DecayInstType>(m_instance), AZStd::forward<Args>(args)...);
            }
            else
            {
                return Internal::AttributeInvoke<RetType>(m_attribute, m_instance, AZStd::forward<Args>(args)...);
            }
        }

        // ------------ private implementation ---------------------------------
        AttributeInvoker(const InstType& instance, Attribute* attrib)
            : m_instance(instance)
            , m_attribute(attrib)
        {
        }

        AttributeInvoker(InstType&& instance, Attribute* attrib)
            : m_instance(AZStd::move(instance))
            , m_attribute(attrib)
        {
        }

        Attribute* GetAttribute() const { return m_attribute; }
        InstType& GetInstance() { return m_instance; }
        const InstType& GetInstance() const { return m_instance; }

    private:
        InstType m_instance;
        Attribute* m_attribute;
    };

    // This class is kept for backwards compatibility with the AttributeReader interface
    // of supplying a void pointer and an attribute
    // For Attributes that stores raw or free function such as AZ::AttributeData and AZ::AttributeFunction
    // this class works fine as the void pointer instance isn't used, but for Attributes that works
    class AttributeReader
        : public AttributeInvoker<void*>
    {
    public:
        AZ_CLASS_ALLOCATOR(AttributeReader, AZ::SystemAllocator);

        // Bring in AttributeInvoker<void*> constructors into scope
        using AttributeInvoker<void*>::AttributeInvoker;
    };

    using AZ::Internal::AttributeRead;
} // namespace AZ

