/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_ATTRIBUTE_READER_H
#define AZCORE_ATTRIBUTE_READER_H

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
            if (auto invocable = azrtti_cast<AttributeInvocable<AttrType(AZStd::remove_reference_t<InstType>, Args...)>*>(attr); invocable != nullptr)
            {
                value = static_cast<DestType>(invocable->operator()(AZStd::forward<InstType>(instance), AZStd::forward<Args>(args)...));
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
            if (auto invocable = azrtti_cast<AttributeInvocable<RetType(AZStd::remove_reference_t<InstType>, Args...)>*>(attr); invocable != nullptr)
            {
                invocable->operator()(AZStd::forward<InstType>(instance), AZStd::forward<Args>(args)...);
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
                    if (AttributeRead<bool, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<char, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned char, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<short, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned short, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<int, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned int, Args..., DestType>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<AZ::s64, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<AZ::u64, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<AZ::Crc32, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                }
                if constexpr (AZStd::is_floating_point_v<DestType>)
                {
                    if (AttributeRead<float, DestType>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<double, DestType>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                }
                if constexpr (AZStd::is_same_v<DestType, AZStd::string> && (AZStd::is_same_v<AttrType, AZStd::string>
                    || AZStd::is_same_v<AttrType, AZStd::string_view>
                    || AZStd::is_same_v<AttrType, const char*>))
                {
                    if (AttributeRead<const char*, AZStd::string>(value, attr, instance, args...))
                    {
                        return true;
                    }
                    if (AttributeRead<AZStd::string_view, AZStd::string>(value, attr, instance, args...))
                    {
                        return true;
                    }
                }
                return AttributeRead<AttrType, DestType>(value, attr, instance, args...);
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
        AZ_CLASS_ALLOCATOR(AttributeInvoker, AZ::SystemAllocator, 0);

        // this is the only thing you should need to call!
        // returns false if it is an incompatible type or fails to read it.
        // for example, Read<int>(someInteger);
        // for example, Read<double>(someDouble);
        // for example, Read<MyStruct>(someStruct that has operator=(const other&) defined)
        template <class T, class U, typename... Args>
        bool Read(U& value, Args&&... args)
        {
            return Internal::AttributeReader<T, U, Args...>::Read(value, m_attribute, m_instancePointer, AZStd::forward<Args>(args)...);
        }

        template <typename RetType, typename... Args>
        bool Invoke(Args&&... args)
        {
            return Internal::AttributeInvoke<RetType>(m_attribute, m_instancePointer, AZStd::forward<Args>(args)...);
        }

        // ------------ private implementation ---------------------------------
        AttributeInvoker(InstType* instancePointer, Attribute* attrib)
            : m_instancePointer(instancePointer)
            , m_attribute(attrib)
        {
        }

        Attribute* GetAttribute() const { return m_attribute; }
        InstType* GetInstancePointer() const { return m_instancePointer; }

    private:
        InstType* m_instancePointer;
        Attribute* m_attribute;
    };

    // This class is kept for backwards compatibility with the AttributeReader interface
    // of supplying a void pointer and an attribute
    // For Attributes that stores raw or free function such as AZ::AttributeData and AZ::AttributeFunction
    // this class works fine as the void pointer instance isn't used, but for Attributes that works
    class AttributeReader
        : public AttributeInvoker<void>
    {
    public:
        AZ_CLASS_ALLOCATOR(AttributeReader, AZ::SystemAllocator, 0);

        // Bring in AttributeInvoker<void> constructors into scope
        using AttributeInvoker<void>::AttributeInvoker;
    };

    using AZ::Internal::AttributeRead;

    struct UnsafeAttributeInvoker
    {
        AttributeReader GetAttributeReader()
        {
            return m_reader;
        }
    private:
        UnsafeAttributeInvoker(AZStd::unique_ptr<AZ::Attribute> attribute, AZ::AttributeReader reader)
            : m_callableAttribute(AZStd::move(attribute))
            , m_reader(AZStd::move(reader))
        {}

        AZStd::unique_ptr<AZ::Attribute> m_callableAttribute;
        AttributeReader m_reader;

        friend class ::AZ::Attribute;

        template<class Invocable>
        friend class ::AZ::AttributeInvocable;
    };

    namespace Internal
    {
        template<class RetType, class... Args>
        struct ClassToVoidInvoker;

        // Specialization for function object with no parameters
        template<class RetType>
        struct ClassToVoidInvoker<RetType>
        {
            explicit ClassToVoidInvoker(AZStd::function<RetType()> callable)
                : m_callable(AZStd::move(callable))
            {}

            // In the case of the function object not accepting at least one argument, then the class type is assumed to be non-existent
            RetType operator()() const
            {
                return m_callable();
            }

        private:
            AZStd::function<RetType()> m_callable;
        };

        // Specialization for function object with at least one parameter
        // The first parameter is assumed to be the class type
        // The void pointer instance will be cast to that type
        template<class RetType, class ClassType, class... Args>
        struct ClassToVoidInvoker<RetType, ClassType, Args...>
        {
            explicit ClassToVoidInvoker(AZStd::function<RetType(ClassType, Args...)> callable)
                : m_callable(AZStd::move(callable))
            {}
            RetType operator()(void* instancePtr, Args... args) const
            {
                using RemoveClassCVRef = AZStd::remove_cvref_t<ClassType>;
                if constexpr (AZStd::is_pointer_v<RemoveClassCVRef>)
                {
                    // ClassType is a pointer so cast directly from the void pointer
                    return m_callable(static_cast<RemoveClassCVRef>(instancePtr), static_cast<Args&&>(args)...);
                }
                else
                {
                    // If the ClassType parameter accepts a value type, then cast the void pointer to a class type pointer
                    // and deference
                    return m_callable(*static_cast<RemoveClassCVRef*>(instancePtr), static_cast<Args&&>(args)...);
                }
            }

        private:
            AZStd::function<RetType(ClassType, Args...)> m_callable;
        };

        template<class RetType>
        struct FunctionObjectInvoker
        {
            template<class... Args>
            using ClassToVoidArgsInvoker = ClassToVoidInvoker<RetType, Args...>;
        };
    } // namespace Internal

    inline UnsafeAttributeInvoker Attribute::GetUnsafeAttributeReader(void* instance)
    {
        return UnsafeAttributeInvoker{ nullptr, AttributeReader(instance, this) };
    }

    template<class Invocable>
    inline UnsafeAttributeInvoker AttributeInvocable<Invocable>::GetUnsafeAttributeReader(void* instance)
    {
        // Make a temporary AttributeInvocable attribute that can cast from a void pointer
        // to the class type needed by this AttributeInvocable

        if constexpr (AZStd::function_traits<Callable>::value)
        {
            using CallableReturnType = typename AZStd::function_traits<Callable>::return_type;
            using VoidInstanceCallWrapper = typename AZStd::function_traits<Callable>::template expand_args<
                Internal::FunctionObjectInvoker<CallableReturnType>::template ClassToVoidArgsInvoker>;
            VoidInstanceCallWrapper voidWrapper{ m_callable };
            auto wrappedAttributeInvocable = new AttributeInvocable<typename AZStd::function_traits<VoidInstanceCallWrapper>::function_type>(voidWrapper);
            return UnsafeAttributeInvoker{ AZStd::unique_ptr<AZ::Attribute>(wrappedAttributeInvocable),
                AttributeReader(instance, wrappedAttributeInvocable) };
        }
        else
        {
            // Otherwise if the type being wrapped is not callable then assume it is a regular value type and use
            // Invoke base Attribute class GetUnsafeAttributeReader function to get an AttributeInvoker
            return Attribute::GetUnsafeAttributeReader(instance);
        }
    }
} // namespace AZ

#endif // AZCORE_ATTRIBUTE_READER_H
