/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/ScriptCanvas/ScriptCanvasAttributes.h>
#include <AzCore/ScriptCanvas/ScriptCanvasOnDemandNames.h>
#include <AzCore/RTTI/AzStdOnDemandPrettyName.inl>
#include <AzCore/RTTI/AzStdOnDemandReflectionLuaFunctions.inl>
#include <AzCore/std/optional.h>
#include <AzCore/std/typetraits/has_member_function.h>

#ifndef AZ_USE_CUSTOM_SCRIPT_BIND
struct lua_State;
struct lua_Debug;
#endif // AZ_USE_CUSTOM_SCRIPT_BIND

// forward declare specialized types
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
    template<class Key, class MappedType, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_map;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_set;
    template<AZStd::size_t NumBits>
    class bitset;
    template<class Element, class Traits, class Allocator>
    class basic_string;
    template<class Element>
    struct char_traits;

    template<class T>
    class intrusive_ptr;
    template<class T>
    class shared_ptr;
}

namespace AZ
{
    class BehaviorContext;
    class ScriptDataContext;

    namespace OnDemandLuaFunctions
    {
        inline void AnyToLua(lua_State* lua, BehaviorArgument& param);
    }
    namespace ScriptCanvasOnDemandReflection
    {
        template<typename T>
        struct OnDemandPrettyName;
        template<typename T>
        struct OnDemandToolTip;
        template<typename T>
        struct OnDemandCategoryName;
    }
    namespace CommonOnDemandReflections
    {
        void ReflectCommonString(ReflectContext* context);
        void ReflectCommonStringView(ReflectContext* context);
        void ReflectStdAny(ReflectContext* context);
        void ReflectVoidOutcome(ReflectContext* context);
    }
    /// OnDemand reflection for AZStd::basic_string
    template<class Element, class Traits, class Allocator>
    struct OnDemandReflection< AZStd::basic_string<Element, Traits, Allocator> >
    {
        using ContainerType = AZStd::basic_string<Element, Traits, Allocator>;
        using SizeType = typename ContainerType::size_type;
        using ValueType = typename ContainerType::value_type;

        static void Reflect(ReflectContext* context)
        {
            constexpr bool is_string = AZStd::is_same_v<Element, char> && AZStd::is_same_v<Traits, AZStd::char_traits<char>>
                    && AZStd::is_same_v<Allocator, AZStd::allocator>;
            if constexpr(is_string)
            {
                CommonOnDemandReflections::ReflectCommonString(context);
            }
            static_assert (is_string, "Unspecialized basic_string<> template reflection requested.");
        }
    };

    /// OnDemand reflection for AZStd::basic_string_view
    template<class Element, class Traits>
    struct OnDemandReflection< AZStd::basic_string_view<Element, Traits> >
    {
        using ContainerType = AZStd::basic_string_view<Element, Traits>;
        using SizeType = typename ContainerType::size_type;
        using ValueType = typename ContainerType::value_type;

        static void Reflect(ReflectContext* context)
        {
            constexpr bool is_common = AZStd::is_same_v<Element,char> && AZStd::is_same_v<Traits,AZStd::char_traits<char>>;
            static_assert (is_common, "Unspecialized basic_string_view<> template reflection requested.");
            CommonOnDemandReflections::ReflectCommonStringView(context);
        }
    };

    /// OnDemand reflection for AZStd::intrusive_ptr
    template<class T>
    struct OnDemandReflection< AZStd::intrusive_ptr<T> >
    {
        using ContainerType = AZStd::intrusive_ptr<T>;

        // TODO: Count reflection types for a proper un-reflect

        static void CustomConstructor(ContainerType* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<T>(0))
                {
                    T* value = nullptr;
                    dc.ReadArg(0, value);
                    dc.AcquireOwnership(0, false); // the smart pointer will be owner by the object
                    new(thisPtr) ContainerType(value);
                }
                else if (dc.IsClass<ContainerType>(0))
                {
                    dc.ReadArg(0, thisPtr);
                }
            }
        }

        static void Reflect(ReflectContext* context)
        {
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Category, ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructibleFromNil, true)
                    ->template Constructor<typename ContainerType::value_type*>()
                        ->Attribute(AZ::Script::Attributes::ConstructorOverride, &CustomConstructor)
                    ->template WrappingMember<typename ContainerType::value_type>(&ContainerType::get)
                    ->Method("get", &ContainerType::get)
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::shared_ptr
    template<class T>
    struct OnDemandReflection< AZStd::shared_ptr<T> >
    {
        using ContainerType = AZStd::shared_ptr<T>;

        // TODO: Count reflection types for a proper un-reflect

        static void CustomConstructor(ContainerType* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<T>(0))
                {
                    T* value = nullptr;
                    dc.ReadArg(0, value);
                    dc.AcquireOwnership(0, false); // the smart pointer will be owner by the object
                    new(thisPtr) ContainerType(value);
                }
                else if (dc.IsClass<ContainerType>(0))
                {
                    dc.ReadArg(0, thisPtr);
                }
            }
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Category, ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructibleFromNil, true)
                    ->template Constructor<typename ContainerType::value_type*>()
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &CustomConstructor)
                    ->template WrappingMember<typename ContainerType::value_type>(&ContainerType::get)
                    ->Method("get", &ContainerType::get)
                    ;
            }
        }
    };

    template<class T>
    struct Iterator_VM
    {
        void NoSpecializationFunction();
    };

    AZ_HAS_MEMBER(Iterator_VM, NoSpecializationFunction, void, ())

    AZ_TYPE_INFO_TEMPLATE(Iterator_VM, "{55E371F4-4051-4E20-B205-8F11DBCE0907}", AZ_TYPE_INFO_CLASS);

    template< class t_Value, class t_Allocator>
    class Iterator_VM<AZStd::vector<t_Value, t_Allocator> >
    {
    public:
        using ContainerType = AZStd::vector<t_Value, t_Allocator>;

        Iterator_VM(ContainerType& container)
            : m_iterator(container.begin())
            , m_end(container.end())
        {}

        bool IsNotAtEnd() const
        {
            return m_iterator != m_end;
        }

        t_Value& ModValueUnchecked()
        {
            return *m_iterator;
        }

        void Next()
        {
            ++m_iterator;
        }

    private:
        typename ContainerType::iterator m_iterator;
        typename ContainerType::iterator m_end;
    };

    template<typename T>
    using decay_array = AZStd::conditional_t<AZStd::is_array_v<AZStd::remove_reference_t<T>>, std::remove_extent_t<AZStd::remove_reference_t<T>>*, T&&>;

    template<typename... T>
    BehaviorObject CreateConnectedAZEventHandler(void* voidPtr, BehaviorFunction&& function)
    {
        auto behaviorForwardingFunction = [function](T... args)
        {
            AZStd::tuple<decay_array<T>...> lvalueWrapper(AZStd::forward<T>(args)...);
            using BVPReserveArray = AZStd::array<AZ::BehaviorArgument, sizeof...(args)>;
            auto MakeBVPArrayFunction = [](auto&&... element)
            {
                return BVPReserveArray{ {AZ::BehaviorArgument{&element}...} };
            };
            BVPReserveArray argsBVPs = AZStd::apply(MakeBVPArrayFunction, lvalueWrapper);
            function(nullptr, argsBVPs.data(), sizeof...(T));
        };

        auto result = aznew AZ::EventHandler<T...>(AZStd::move(behaviorForwardingFunction));
        auto eventPtr = reinterpret_cast<AZ::Event<T...>*>(voidPtr);
        result->Connect(*eventPtr);
        return { reinterpret_cast<void*>(result), azrtti_typeid<AZ::EventHandler<T...>>() };
    }

    template<typename... T>
    struct OnDemandReflection<AZ::Event<T...>>
    {
        template<typename U>
        static AZ::BehaviorParameter CreateBehaviorEventParameter()
        {
            AZ::BehaviorParameter param;
            AZ::Internal::SetParametersStripped<U>(&param, nullptr);
            return param;
        }
        static void Reflect(ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                AZ::EventHandlerCreationFunctionHolder createHandlerHolder;
                createHandlerHolder.m_function = &CreateConnectedAZEventHandler<T...>;

                AZStd::vector<AZ::BehaviorParameter> eventParamsTypes{ AZStd::initializer_list<AZ::BehaviorParameter>{
                    CreateBehaviorEventParameter<decay_array<T>>()... } };
                behaviorContext->Class<AZ::Event<T...>>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ->Attribute(AZ::Script::Attributes::EventHandlerCreationFunction, createHandlerHolder)
                    ->Attribute(AZ::Script::Attributes::EventParameterTypes, eventParamsTypes)
                    ->Method("HasHandlerConnected", &AZ::Event<T...>::HasHandlerConnected)
                    ;

                behaviorContext->Class<AZ::EventHandler<T...>>()
                    ->Method("Disconnect", &AZ::EventHandler<T...>::Disconnect)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::vector
    template<class T, class A>
    struct OnDemandReflection< AZStd::vector<T, A> >
    {
        using ContainerType = AZStd::vector<T, A>;

        using ValueIteratorType = Iterator_VM<ContainerType>;

        // resize the container to the appropriate size if needed and set the element
        static void AssignAt(ContainerType& thisPtr, AZ::u64 index, T& value)
        {
            size_t uindex = static_cast<size_t>(index);
            if (thisPtr.size() <= uindex)
            {
                thisPtr.resize(uindex + 1);
            }
            thisPtr[uindex] = value;
        }


        static bool EraseCheck_VM(ContainerType& thisPtr, AZ::u64 index)
        {
            if (index < thisPtr.size())
            {
                thisPtr.erase(thisPtr.begin() + index);
                return true;
            }
            else
            {
                return false;
            }
        }

        static ContainerType& ErasePost_VM(ContainerType& thisPtr, AZ::u64 /*index*/)
        {
            return thisPtr;
        }

        static ValueIteratorType Iterate_VM(ContainerType& thisContainer)
        {
            return ValueIteratorType(thisContainer);
        }

        static bool HasKey(ContainerType& thisPtr, AZ::u64 index)
        {
            return index < thisPtr.size();
        }

        static ContainerType& Insert(ContainerType& thisPtr, AZ::u64 index, const T& value)
        {
            if (index >= thisPtr.size())
            {
                thisPtr.resize(index + 1);
                thisPtr[index] = value;
            }
            else
            {
                thisPtr.insert(thisPtr.begin() + index, value);
            }

            return thisPtr;
        }

        static bool IsScriptEventType()
        {
            return AZStd::is_same<T, AZ::EntityId>::value;
        }

        static ContainerType& PushBack_VM(ContainerType& thisType, const T& value)
        {
            thisType.push_back(value);
            return thisType;
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                BranchOnResultInfo emptyBranchInfo;
                emptyBranchInfo.m_returnResultInBranches = true;
                emptyBranchInfo.m_trueToolTip = "The container is empty";
                emptyBranchInfo.m_falseToolTip = "The container is not empty";

                BranchOnResultInfo hasElementsBranchInfo;
                hasElementsBranchInfo.m_returnResultInBranches = true;
                hasElementsBranchInfo.m_trueToolTip = "The container has elements";
                hasElementsBranchInfo.m_falseToolTip = "The container has no elements";

                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Category, ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->Attribute(AZ::Script::Attributes::EnableAsScriptEventParamType, &IsScriptEventType)
                    ->Method("AssignAt", &AssignAt, { { {}, { "Index", "The index at which to assign the element to, resizes the container if necessary", nullptr, BehaviorParameter::Traits::TR_INDEX } } })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexWrite)
                    ->Method("Erase_VM", &ErasePost_VM,
                        { { { "Container", "The container from which to delete", nullptr, {} },
                            { "Key", "The key to delete", nullptr, {} } } })
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Erase", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::CheckedOperation, CheckedOperationInfo("EraseCheck_VM", {}, "Out", "Key Not Found", true))
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup", "" }, { "ContainerGroup" }))
                    ->Method("EraseCheck_VM", &EraseCheck_VM)
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->template Method<void(ContainerType::*)(typename ContainerType::const_reference)>("push_back", &ContainerType::push_back)
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Method("PushBack_VM", &PushBack_VM,
                        { { { "Container", "The container into which to add an element to", nullptr, {} },
                            { "Value", "The value to be added", nullptr, {} } } })
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Add Element at End", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup", "" }, { "ContainerGroup" }))
                    ->Method("pop_back", &ContainerType::pop_back)
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->template Method<typename ContainerType::reference(ContainerType::*)(typename ContainerType::size_type)>
                        ("at", &ContainerType::at, {{ { "Index", "The index to read from", nullptr, BehaviorParameter::Traits::TR_INDEX } }})
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexRead)
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->template Method<typename ContainerType::reference (ContainerType::*)(typename ContainerType::size_type)>(
                        k_accessElementNameUnchecked, &ContainerType::at,
                        { "Container", "The container to get element from", nullptr, {} },
                        { { { "Index", "The index to read from", nullptr, {} } } })
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Get Element", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::CheckedOperation, CheckedOperationInfo("Has Key", {}, "Out", "Key Not Found"))
                    ->Method("size", [](ContainerType& thisPtr) { return aznumeric_cast<int>(thisPtr.size()); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                    ->Method("GetSize", [](ContainerType& thisPtr) { return aznumeric_cast<int>(thisPtr.size()); },
                        { { { "Container", "The container to get the size of", nullptr, {} } } })
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Get Size", "Containers"))
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                    ->Method("clear", &ContainerType::clear)
                    ->template Method<typename ContainerType::reference(ContainerType::*)(typename ContainerType::size_type)>(k_accessElementName, &ContainerType::at, { { { "Index", "The index to read from", nullptr, BehaviorParameter::Traits::TR_INDEX } } })
                    ->Method("Capacity", &ContainerType::capacity)
                    ->Method("Clear", [](ContainerType& thisContainer)->ContainerType& { thisContainer.clear(); return thisContainer; },
                        { { { "Container", "The container to clear", nullptr, {} } } })
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Clear All Elements", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup" }, { "ContainerGroup" }))
                    ->Method("Empty", &ContainerType::empty,
                        { "Container", "The container to check if it is empty", nullptr, {} }, {})
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Is Empty", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::BranchOnResult, emptyBranchInfo)
                    ->Method("NotEmpty", [](ContainerType& thisPtr) { return !thisPtr.empty(); },
                        { { { "Container", "The container to check if it is not empty", nullptr, {} } } })
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Has Elements", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::BranchOnResult, hasElementsBranchInfo)
                    ->template Method<typename ContainerType::reference(ContainerType::*)()>("Back", &ContainerType::back,
                        { "Container", "The container to get the last element from", nullptr, {} }, {})
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Get Last Element", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::CheckedOperation, CheckedOperationInfo("NotEmpty", {}, "Out", "Empty"))
                    ->template Method<typename ContainerType::reference(ContainerType::*)()>("Front", &ContainerType::front,
                        { "Container", "The container to get the first element from", nullptr, {} }, {})
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Get First Element", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::CheckedOperation, CheckedOperationInfo("NotEmpty", {}, "Out", "Empty"))
                    ->Method("Has Key", &HasKey,
                        { { { "Container", "The container into which to check if the given key exists", nullptr, {} },
                            { "Key", "The key to check for", nullptr, {} } } })
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Has Key", "Containers"))
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                    ->Method("Insert", &Insert,
                        { { { "Container", "The container into which to insert the value", nullptr, {} },
                            { "Index", "The index at which to insert the value", nullptr, {} },
                            { "Value", "The value that is to be inserted", nullptr, {} } } })
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Insert", "Containers"))
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup", "", "" }, { "ContainerGroup" }))
                    ->Method("PushBack", static_cast<void(ContainerType::*)(typename ContainerType::const_reference)>(&ContainerType::push_back))
                    ->Method("Reserve", &ContainerType::reserve)
                    ->Method("Resize", static_cast<void(ContainerType::*)(size_t)>(&ContainerType::resize))
                    ->Method(k_sizeName, [](ContainerType& thisPtr) { return aznumeric_cast<int>(thisPtr.size()); })
                    ->Method("Swap", static_cast<void(ContainerType::*)(ContainerType&)>(&ContainerType::swap))
                    ->Method(k_iteratorConstructorName, &Iterate_VM)
                    ;

                behaviorContext->Class<ValueIteratorType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->Attribute(AZ::Script::Attributes::EnableAsScriptEventParamType, &IsScriptEventType)
                    ->Method(k_iteratorIsNotAtEndName, &ValueIteratorType::IsNotAtEnd)
                    ->Method(k_iteratorModValueName, &ValueIteratorType::ModValueUnchecked)
                    ->Method(k_iteratorNextName, &ValueIteratorType::Next)
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::array
    template<class T, AZStd::size_t N>
    struct OnDemandReflection< AZStd::array<T, N> >
    {
        using ContainerType = AZStd::array<T, N>;

        static AZ::Outcome<T, AZStd::string> At(ContainerType& thisContainer, size_t index)
        {
            if (index >= 0 && index < thisContainer.size())
            {
                return AZ::Success(thisContainer.at(index));
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Index out of bounds: %zu (size: %zu)", index, thisContainer.size()));
            }
        }

        static AZ::Outcome<void, void> Replace(ContainerType& thisContainer, size_t index, T& value)
        {
            if (index >= 0 && index < thisContainer.size())
            {
                thisContainer[index] = value;
                return AZ::Success();
            }

            return AZ::Failure();
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Category, ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->template Method<typename ContainerType::reference(ContainerType::*)(typename ContainerType::size_type)>("at", &ContainerType::at, {{ { "Index", "The index to read from", nullptr, BehaviorParameter::Traits::TR_INDEX } }})
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexRead)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Method("size", [](ContainerType*) { return aznumeric_cast<int>(N); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)

                    ->Method(k_accessElementName, &At, {{ {}, { "Index", "The index to read from", nullptr, BehaviorParameter::Traits::TR_INDEX }}})
                    ->Method(k_sizeName, [](ContainerType*) { return aznumeric_cast<int>(N); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                    ->template Method<typename ContainerType::reference(ContainerType::*)()>("Back", &ContainerType::back)
                    ->Method("Fill", &ContainerType::fill)
                    ->template Method<typename ContainerType::reference(ContainerType::*)()>("Front", &ContainerType::front)
                    ->Method("Replace", &Replace, { { {}, { "Index", "The index to replace", nullptr, BehaviorParameter::Traits::TR_INDEX }, {} } })
                    ->template Method<void(ContainerType::*)(ContainerType&)>("Swap", &ContainerType::swap)
                    ;
            }
        }
    };

    template<typename ValueT, typename ErrorT>
    struct OnDemandReflection<AZ::Outcome<ValueT, ErrorT>>
    {
        using OutcomeType = AZ::Outcome<ValueT, ErrorT>;

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                auto getValueFunc = [](OutcomeType* outcomePtr) { return outcomePtr->GetValue(); };
                auto getErrorFunc = [](OutcomeType* outcomePtr) { return outcomePtr->GetError(); };
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<OutcomeType>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<OutcomeType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Method("Failure", [](const ErrorT failure) -> OutcomeType { return AZ::Failure(failure); })
                    ->Method("Success", [](const ValueT success) -> OutcomeType { return AZ::Success(success); })
                    ->Method("GetValue", getValueFunc)
                    ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 0)
                    ->Method("GetError", getErrorFunc)
                    ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 1)
                    ->Method("IsSuccess", &OutcomeType::IsSuccess)
                    ;
            }
        }
    };

    template<typename ValueT>
    struct OnDemandReflection<AZ::Outcome<ValueT, void>>
    {
        using OutcomeType = AZ::Outcome<ValueT, void>;

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                auto getValueFunc = [](OutcomeType* outcomePtr) { return outcomePtr->GetValue(); };
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<OutcomeType>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, true)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<OutcomeType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Method("Failure", []() -> OutcomeType { return AZ::Failure(); })
                    ->Method("Success", [](const ValueT& success) -> OutcomeType { return AZ::Success(success); })
                    ->Method("GetValue", getValueFunc)
                    ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 0)
                    ->Method("IsSuccess", &OutcomeType::IsSuccess)
                    ;
            }
        }
    };

    template<typename ErrorT>
    struct OnDemandReflection<AZ::Outcome<void, ErrorT>>
    {
        using OutcomeType = AZ::Outcome<void, ErrorT>;

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                auto getErrorFunc = [](OutcomeType* outcomePtr) { return outcomePtr->GetError(); };
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<OutcomeType>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, true)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<OutcomeType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Method("Failure", [](const ErrorT& failure) -> OutcomeType { return AZ::Failure(failure); })
                    ->Method("Success", []() -> OutcomeType { return AZ::Success(); })
                    ->Method("GetError", getErrorFunc)
                    ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 1)
                    ->Method("IsSuccess", &OutcomeType::IsSuccess)
                    ;
            }
        }
    };

    template<> // in case someone has an issue with bool
    struct OnDemandReflection<AZ::Outcome<void, void>>
    {
        static void Reflect(ReflectContext* context) {
            CommonOnDemandReflections::ReflectVoidOutcome(context);
        }
    };

    template<>
    struct OnDemandReflection<AZStd::unexpect_t>
    {
        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<AZStd::unexpect_t>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "std")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ;
            }
        }
    };

    template<typename T1, typename T2>
    struct OnDemandReflection<AZStd::pair<T1, T2>>
    {
        using ContainerType = AZStd::pair<T1, T2>;
        static T1& GetFirst(ContainerType& pair) { return pair.first; };
        static T2& GetSecond(ContainerType& pair) { return pair.second; };

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "std")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Category, ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->template Constructor<const T1&, const T2&>()
                    ->Property("first", [](ContainerType& thisPtr) { return thisPtr.first; }, [](ContainerType& thisPtr, const T1& value) { thisPtr.first = value; })
                        ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 0)
                    ->Property("second", [](ContainerType& thisPtr) { return thisPtr.second; }, [](ContainerType& thisPtr, const T2& value) { thisPtr.second = value; })
                        ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 1)
                    ->Method("ConstructTuple", [](const T1& first, const T2& second) { return AZStd::make_pair(first, second); })
                    ;
            }
        }
    };

    template<typename...T>
    AZStd::vector<AZ::TypeId> ToTypeIds()
    {
        AZStd::vector<AZ::TypeId> typeIds;
        (typeIds.push_back(azrtti_typeid<T>()), ...);
        return typeIds;
    }

    template<typename... T>
    struct OnDemandReflection<AZStd::tuple<T...>>
    {
        using ContainerType = AZStd::tuple<T...>;

        template<typename Targ, size_t Index>
        static void ReflectUnpackMethodFold(BehaviorContext::ClassBuilder<ContainerType>& builder, const AZStd::vector<AZStd::string>& typeNames)
        {
            const AZStd::string methodName = AZStd::string::format("Get%zu", Index);
            builder->Method(methodName.data(), [](ContainerType& thisPointer) { return AZStd::get<Index>(thisPointer); })
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, Index)
                ;

            builder->Property
                ( AZStd::string::format("element_%zu_%s", Index, typeNames[Index].c_str()).c_str()
                , [](ContainerType& thisPointer) { return AZStd::get<Index>(thisPointer); }
                , [](ContainerType& thisPointer, const Targ& element) { AZStd::get<Index>(thisPointer) = element; });
        }

        template<typename... Targ, size_t... Indices>
        static void ReflectUnpackMethods(BehaviorContext::ClassBuilder<ContainerType>& builder, AZStd::index_sequence<Indices...>)
        {
            AZStd::vector<AZStd::string> typeNames;
            ScriptCanvasOnDemandReflection::GetTypeNames<T...>(typeNames, *builder.m_context);
            (ReflectUnpackMethodFold<Targ, Indices>(builder, typeNames), ...);
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                AZ::ScriptCanvasAttributes::GetUnpackedReturnValueTypesHolder unpackFunctionHolder;
                unpackFunctionHolder.m_function = [](){ return ToTypeIds<T...>(); };

                AZ::ScriptCanvasAttributes::TupleConstructorHolder constructorHolder;
                constructorHolder.m_function = []()->void*
                {
                    return new ContainerType{};
                };

                BehaviorContext::ClassBuilder<ContainerType> builder = behaviorContext->Class<ContainerType>();
                builder->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "std")
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::ScriptCanvasAttributes::ReturnValueTypesFunction, unpackFunctionHolder)
                    ->Attribute(AZ::ScriptCanvasAttributes::TupleConstructorFunction, constructorHolder)
                    ->template Constructor<T...>()
                ;

                ReflectUnpackMethods<T...>(builder, AZStd::make_index_sequence<sizeof...(T)>{});

                builder->Method("GetSize", []() { return AZStd::tuple_size<ContainerType>::value; })
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                    ;
            }
        }
    };

    template<class t_Key, class t_Value, class t_Hasher, class t_EqualKey, class t_Allocator>
    class Iterator_VM<AZStd::unordered_map<t_Key, t_Value, t_Hasher, t_EqualKey, t_Allocator>>
    {
    public:
        using ContainerType = AZStd::unordered_map<t_Key, t_Value, t_Hasher, t_EqualKey, t_Allocator>;
        using IteratorType = typename ContainerType::iterator;
        Iterator_VM(ContainerType& container)
            : m_iterator(container.begin())
            , m_end(container.end())
        {}

        const t_Key& GetKeyUnchecked() const
        {
            return m_iterator->first;
        }

        bool IsNotAtEnd() const
        {
            return m_iterator != m_end;
        }

        t_Value& ModValueUnchecked()
        {
            return m_iterator->second;
        }

        void Next()
        {
            ++m_iterator;
        }

    private:
        IteratorType m_iterator;
        IteratorType m_end;
    };

    /// OnDemand reflection for AZStd::unordered_map
    template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
    struct OnDemandReflection< AZStd::unordered_map<Key, MappedType, Hasher, EqualKey, Allocator> >
    {
        using ContainerType = AZStd::unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>;
        using KeyListType = AZStd::vector<Key, Allocator>;

        using ValueIteratorType = Iterator_VM<ContainerType>;

        static ValueIteratorType Iterate_VM(ContainerType& thisContainer)
        {
            return ValueIteratorType(thisContainer);
        }

        static MappedType At(ContainerType& thisMap, Key& key)
        {
            auto it = thisMap.find(key);
            AZ_Assert(it != thisMap.end(), "unchecked map access, key not in map");
            return it->second;
        }

        static bool EraseCheck_VM(ContainerType& thisMap, Key& key)
        {
            return thisMap.erase(key) != 0;
        }

        static ContainerType& ErasePost_VM(ContainerType& thisMap, Key& /*key*/)
        {
            return thisMap;
        }

        static KeyListType GetKeys(ContainerType& thisMap)
        {
            KeyListType keys;
            for (auto entry : thisMap)
            {
                keys.push_back(entry.first);
            }
            return keys;
        }

        static ContainerType& Insert(ContainerType& thisMap, Key& key, MappedType& value)
        {
            thisMap.insert(AZStd::make_pair(key, value));
            return thisMap;
        }

        static void Swap(ContainerType& thisMap, ContainerType& otherMap)
        {
            thisMap.swap(otherMap);
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                BranchOnResultInfo emptyBranchInfo;
                emptyBranchInfo.m_returnResultInBranches = true;
                emptyBranchInfo.m_trueToolTip = "The container is empty";
                emptyBranchInfo.m_falseToolTip = "The container is not empty";

                auto ContainsTransparent = [](const ContainerType& containerType, typename ContainerType::key_type& key)->bool
                {
                    return containerType.contains(key);
                };

                ExplicitOverloadInfo explicitOverloadInfo;
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Category, ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->Method(k_accessElementName, &At)
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::CheckedOperation, CheckedOperationInfo("contains", {}, "Out", "Key Not Found"))
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Get Element", "Containers"))
                    ->Method("BucketCount", static_cast<typename ContainerType::size_type(ContainerType::*)() const>(&ContainerType::bucket_count))
                    ->Method("Empty", static_cast<bool(ContainerType::*)() const>(&ContainerType::empty), { { { "Container", "The container to check if it is empty", nullptr, {} } } })
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Is Empty", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::BranchOnResult, emptyBranchInfo)
                    ->Method("Erase", &ErasePost_VM)
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Erase", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::CheckedOperation, CheckedOperationInfo("EraseCheck_VM", {}, "Out", "Key Not Found", true))
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup", "" }, { "ContainerGroup" }))
                    ->Method("EraseCheck_VM", &EraseCheck_VM)
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Method("GetKeys", &GetKeys)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Method("contains", ContainsTransparent, { { { "Key", "The key to check for", nullptr, {} } } })
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Has Key", "Containers"))
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                    ->Method("Insert", &Insert, { { {}, { "Index", "The index at which to insert the value", nullptr, {} }, {} } })
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Insert", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup", "", "" }, { "ContainerGroup" }))
                    ->Method("Reserve", static_cast<void(ContainerType::*)(typename ContainerType::size_type)>(&ContainerType::reserve))
                    ->Method(k_sizeName, [](ContainerType& thisPtr) { return aznumeric_cast<int>(thisPtr.size()); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                    ->Method("GetSize", [](ContainerType& thisPtr) { return aznumeric_cast<int>(thisPtr.size()); })
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Get Size", "Containers"))
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                    ->Method("Swap", &Swap)
                    ->Method("Clear", [](ContainerType& thisContainer)->ContainerType& { thisContainer.clear(); return thisContainer; })
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Clear All Elements", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup" }, { "ContainerGroup" }))
                    ->Method(k_iteratorConstructorName, &Iterate_VM)
                    ;

                behaviorContext->Class<ValueIteratorType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->Method(k_iteratorGetKeyName, &ValueIteratorType::GetKeyUnchecked)
                    ->Method(k_iteratorModValueName, &ValueIteratorType::ModValueUnchecked)
                    ->Method(k_iteratorIsNotAtEndName, &ValueIteratorType::IsNotAtEnd)
                    ->Method(k_iteratorNextName, &ValueIteratorType::Next)
                    ;
            }
        }
    };


    template<class t_Key, class t_Hasher, class t_EqualKey, class t_Allocator>
    class Iterator_VM<AZStd::unordered_set<t_Key, t_Hasher, t_EqualKey, t_Allocator>>
    {
    public:
        using ContainerType = AZStd::unordered_set<t_Key, t_Hasher, t_EqualKey, t_Allocator>;
        using IteratorType = typename ContainerType::iterator;
        Iterator_VM(ContainerType& container)
            : m_iterator(container.begin())
            , m_end(container.end())
        {}

        const t_Key& GetKeyUnchecked() const
        {
            return *m_iterator;
        }

        bool IsNotAtEnd() const
        {
            return m_iterator != m_end;
        }

        t_Key& ModValueUnchecked()
        {
            return *m_iterator;
        }

        void Next()
        {
            ++m_iterator;
        }

    private:
        IteratorType m_iterator;
        IteratorType m_end;
    };

    /// OnDemand reflection for AZStd::unordered_set
    template<class Key, class Hasher, class EqualKey, class Allocator>
    struct OnDemandReflection< AZStd::unordered_set<Key, Hasher, EqualKey, Allocator> >
    {
        using ContainerType = AZStd::unordered_set<Key, Hasher, EqualKey, Allocator>;
        using KeyListType = AZStd::vector<Key, Allocator>;

        using ValueIteratorType = Iterator_VM<ContainerType>;

        static bool EraseCheck_VM(ContainerType& thisSet, Key& key)
        {
            return thisSet.erase(key) != 0;
        }

        static ContainerType& ErasePost_VM(ContainerType& thisSet, [[maybe_unused]] Key&)
        {
            return thisSet;
        }

        static KeyListType GetKeys(ContainerType& thisSet)
        {
            KeyListType keys;
            for (auto entry : thisSet)
            {
                keys.push_back(entry);
            }
            return keys;
        }

        static ContainerType& Insert(ContainerType& thisSet, Key& key)
        {
            thisSet.insert(key);
            return thisSet;
        }

        static ValueIteratorType Iterate_VM(ContainerType& thisContainer)
        {
            return ValueIteratorType(thisContainer);
        }

        static void Swap(ContainerType& thisSet, ContainerType& otherSet)
        {
            thisSet.swap(otherSet);
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                BranchOnResultInfo emptyBranchInfo;
                emptyBranchInfo.m_returnResultInBranches = true;
                emptyBranchInfo.m_trueToolTip = "The container is empty";
                emptyBranchInfo.m_falseToolTip = "The container is not empty";

                auto ContainsTransparent = [](const ContainerType& containerType, typename ContainerType::key_type& key)->bool
                {
                    return containerType.contains(key);
                };

                ExplicitOverloadInfo explicitOverloadInfo;
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Category, ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get(*behaviorContext))
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->Method("BucketCount", static_cast<typename ContainerType::size_type(ContainerType::*)() const>(&ContainerType::bucket_count))
                    ->Method("Empty", static_cast<bool(ContainerType::*)() const>(&ContainerType::empty), { { { "Container", "The container to check if it is empty", nullptr, {} } } })
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Is Empty", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::BranchOnResult, emptyBranchInfo)
                    ->Method("EraseCheck_VM", &EraseCheck_VM)
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Method("Erase", &ErasePost_VM)
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Erase", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::CheckedOperation, CheckedOperationInfo("EraseCheck_VM", {}, "Out", "Key Not Found", true))
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup", "" }, { "ContainerGroup" }))
                    ->Method("contains", ContainsTransparent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Has Key", "Containers"))
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                    ->Method("Insert", &Insert)
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Insert", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup", "", "" }, { "ContainerGroup" }))
                    ->Method(k_sizeName, [](ContainerType* thisPtr) { return aznumeric_cast<int>(thisPtr->size()); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                    ->Method("GetKeys", &GetKeys)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Method("GetSize", [](ContainerType& thisPtr) { return aznumeric_cast<int>(thisPtr.size()); })
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Get Size", "Containers"))
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                    ->Method("Reserve", static_cast<void(ContainerType::*)(typename ContainerType::size_type)>(&ContainerType::reserve))
                    ->Method("Swap", &Swap)
                    ->Method("Clear", [](ContainerType& thisContainer)->ContainerType& { thisContainer.clear(); return thisContainer; })
                        ->Attribute(AZ::Script::Attributes::TreatAsMemberFunction, AZ::AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Clear All Elements", "Containers"))
                        ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "ContainerGroup" }, { "ContainerGroup" }))
                    ->Method(k_iteratorConstructorName, &Iterate_VM)
                        ;

                behaviorContext->Class<ValueIteratorType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->Method(k_iteratorGetKeyName, &ValueIteratorType::GetKeyUnchecked)
                    ->Method(k_iteratorModValueName, &ValueIteratorType::ModValueUnchecked)
                    ->Method(k_iteratorIsNotAtEndName, &ValueIteratorType::IsNotAtEnd)
                    ->Method(k_iteratorNextName, &ValueIteratorType::Next)
                    ;
            }
        }
    };

    template <>
    struct OnDemandReflection<AZStd::any>
    {
        static void Reflect(ReflectContext* context) {
            CommonOnDemandReflections::ReflectStdAny(context);
        }
    };

    template <typename T>
    struct OnDemandReflection<AZStd::optional<T>>
    {
        using OptionalType = AZStd::optional<T>;

        static void Reflect(ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                auto hasValueFunc = [](OptionalType* optionalInst) { return optionalInst->has_value(); };
                auto valueFunc = [](OptionalType* optionalInst) -> typename OptionalType::value_type
                {
                    if (optionalInst->has_value())
                    {
                        return optionalInst->value();
                    }

                    AZ_Assert(false, "Optional does not have a value, a default constructed value will be returned instead");
                    return typename OptionalType::value_type{};
                };
                auto valueOrFunc = [](OptionalType* optionalInst, const typename OptionalType::value_type& defaultValue) -> typename OptionalType::value_type
                {
                    return optionalInst->has_value() ? optionalInst->value() : defaultValue;
                };

                // Attribute functions for optional
                auto nameAttrFunc = [](AZ::BehaviorContext& context) -> AZStd::string
                {
                    AZStd::string valueName = AZ::ScriptCanvasOnDemandReflection::GetPrettyNameForAZTypeId(context, AZ::AzTypeInfo<T>::Uuid());
                    if (!valueName.empty())
                    {
                        return AZStd::string::format("optional<%.*s>",
                            aznumeric_cast<int>(valueName.size()), valueName.data());
                    }
                    return "optional<T>";
                };
                auto toolTipAttrFunc = [](AZ::BehaviorContext& context) -> AZStd::string
                {
                    AZStd::string valueName = AZ::ScriptCanvasOnDemandReflection::GetPrettyNameForAZTypeId(context, AZ::AzTypeInfo<T>::Uuid());
                    if (!valueName.empty())
                    {
                        return AZStd::string::format("Wraps an optional around type %.*s",
                            aznumeric_cast<int>(valueName.size()), valueName.data());
                    }
                    return "Wraps an optional around type T";
                };
                auto categoryAttrFunc = [](AZ::BehaviorContext&) -> AZStd::string_view
                {
                    return "AZStd";
                };

                behaviorContext->Class<OptionalType>()
                    ->template Constructor<const T&>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, static_cast<AZStd::string(*)(BehaviorContext&)>(nameAttrFunc))
                    ->Attribute(AZ::Script::Attributes::ToolTip, static_cast<AZStd::string(*)(BehaviorContext&)>(toolTipAttrFunc))
                    ->Attribute(AZ::Script::Attributes::Category, static_cast<AZStd::string_view(*)(BehaviorContext&)>(categoryAttrFunc))
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Method("has_value", hasValueFunc)
                    ->Method("__bool__", hasValueFunc)
                    ->Method("value", valueFunc)
                    ->Method("value_or", valueOrFunc)
                    ;
            }
        }
    };
}

