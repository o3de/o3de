/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ::Internal
{
    /// Internal structure to maintain class information while we are describing a class.
    struct ClassBuilderBase : public AZ::Internal::GenericAttributes<ClassBuilderBase>
    {
        using Base = AZ::Internal::GenericAttributes<ClassBuilderBase>;

        //////////////////////////////////////////////////////////////////////////
        /// Internal implementation
        ClassBuilderBase(BehaviorContext* context, BehaviorClass* behaviorClass);
        ~ClassBuilderBase();
        ClassBuilderBase* operator->();

        /**
        * Sets custom allocator for a class, this function will error if this not inside a class.
        * This is only for very specific cases when you want to override AZ_CLASS_ALLOCATOR or you are dealing with 3rd party classes, otherwise
        * you should use AZ_CLASS_ALLOCATOR to control which allocator the class uses.
        */
        ClassBuilderBase* Allocator(BehaviorClass::AllocateType allocate, BehaviorClass::DeallocateType deallocate);

        /// Attaches different constructor signatures to the class.
        template<class C, class... Params>
        ClassBuilderBase* ConstructorWithClass();


        /// Provide a function to unwrap this class to an underlying member address
        /// Such as retrieving the raw pointer from a smart_ptr or a const char* from a string type
        template<class WrappedType, class Callable>
        ClassBuilderBase* WrappingMember(Callable callableFunction);

        /// Sets userdata to a class.
        ClassBuilderBase* UserData(void* userData);

        /// Setup methods
        ///< \deprecated Use "Method(const char* name, Function f, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc = nullptr)" instead
        ///< This method does not support passing in argument names and tooltips nor does it support overriding specific parameter Behavior traits
        template<class Function>
        ClassBuilderBase* Method(const char* name, Function, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        ///< \deprecated Use "Method(const char* name, Function f, const char* deprecatedName, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc = nullptr)" instead
        ///< This method does not support passing in argument names and tooltips nor does it support overriding specific parameter Behavior traits
        template<class Function>
        ClassBuilderBase* Method(const char* name, Function f, const char* deprecatedName, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        template<class Function>
        ClassBuilderBase* Method(const char* name, Function f, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc = nullptr);

        template<class Function>
        ClassBuilderBase* Method(
            const char* name,
            Function f,
            const BehaviorParameterOverrides& classMetadata,
            const BehaviorParameterOverridesArray<Function>& argsMetadata,
            const char* dbgDesc = nullptr);

        template<class Function>
        ClassBuilderBase* Method(const char* name, Function f, const char* deprecatedName, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc = nullptr);

        template<class Getter, class Setter>
        ClassBuilderBase* Property(const char* name, Getter getter, Setter setter);

        /// All enums are treated as the enum type
        template<auto Value>
        ClassBuilderBase* Enum(const char* name);

        template<class Getter>
        ClassBuilderBase* Constant(const char* name, Getter getter);

        /**
         * You can describe buses that this class uses to communicate. Those buses will be used by tools when
         * you need to give developers hints as to what buses this class interacts with.
         * You don't need to reflect all buses that your class uses, just the ones related to
         * class behavior. Please refer to component documentation for more information on
         * the pattern of Request and Notification buses.
         * {@
         */
        ClassBuilderBase* RequestBus(const char* busName);
        ClassBuilderBase* NotificationBus(const char* busName);
        // @}

        BehaviorClass* m_class;
    };
}

namespace AZ::Internal
{
    template<class C, class... Params>
    auto ClassBuilderBase::ConstructorWithClass() -> ClassBuilderBase*
    {
        if (!Base::m_context->IsRemovingReflection())
        {
            AZ_Error("BehaviorContext", m_class, "You can set constructors only on valid classes!");
        }
        if (m_class)
        {
            auto Construct = [](C* address, Params... params) { new (address) C(params...); };
            BehaviorMethod* constructor = aznew BehaviorMethodImpl
            (static_cast<void(*)(C*, Params...)>(Construct),
                Base::m_context, AZStd::string::format("%s::Constructor", m_class->m_name.c_str()));
            m_class->m_constructors.push_back(constructor);
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class WrappedType, class Callable>
    auto ClassBuilderBase::WrappingMember(Callable callableFunction) -> ClassBuilderBase*
    {
        auto Unwrap = [](void* classPtr, BehaviorObject& unwrappedObject, const UnwrapperUserData& userData)
        {
            const Callable* callablePtr{};
            callablePtr = static_cast<const Callable*>(userData.m_unwrapperPtr.get());

            using CallableTraits = AZStd::function_traits<Callable>;
            if constexpr (!AZStd::is_same_v<typename CallableTraits::class_type, AZStd::Internal::error_type>)
            {
                using ClassTypePtr = typename CallableTraits::class_type*;
                unwrappedObject.m_address = const_cast<void*>(reinterpret_cast<const void*>(AZStd::invoke(*callablePtr, reinterpret_cast<ClassTypePtr>(classPtr))));
            }
            else
            {
                static_assert(CallableTraits::arity > 0, "Non member Wrapping function must accept at least 1 argument");
                using ClassTypePtr = typename CallableTraits::template get_arg_t<0>;
                unwrappedObject.m_address = const_cast<void*>(reinterpret_cast<const void*>(AZStd::invoke(*callablePtr, reinterpret_cast<ClassTypePtr>(classPtr))));
            }
            unwrappedObject.m_typeId = AZ::AzTypeInfo<WrappedType>::Uuid();
            unwrappedObject.m_rttiHelper = AZ::GetRttiHelper<WrappedType>();
        };

        UnwrapperUserData userData;
        auto deleteCallable = [](void* ptr)
        {
            if (ptr != nullptr)
            {
                delete static_cast<Callable*>(ptr);
            }
        };
        userData.m_unwrapperPtr = UnwrapperPtr(new Callable(AZStd::move(callableFunction)),
            UnwrapperFuncDeleter{ AZStd::move(deleteCallable) });

        if (!Base::m_context->IsRemovingReflection())
        {
            AZ_Error("BehaviorContext", m_class, "You can wrap only valid classes!");
        }
        if (m_class)
        {
            const AZ::Uuid wrappedTypeId = AzTypeInfo<WrappedType>::Uuid();
            AZ_Assert(m_class->m_typeId != wrappedTypeId, "A Wrapping member cannot unwrap to the same type as itself."
                " As wrapped types are implicitly reflected by the ScriptContext, this prevents a recursive loop");
            m_class->m_wrappedTypeId = wrappedTypeId;
            m_class->m_unwrapper = Unwrap;
            m_class->m_unwrapperUserData = AZStd::move(userData);
        }
        return this;
    }

    template<class Function>
    auto ClassBuilderBase::Method(const char* name, Function f, BehaviorValues* defaultValues, const char* dbgDesc) -> ClassBuilderBase*
    {
        return Method(name, f, nullptr, defaultValues, dbgDesc);
    }

    template<class Function>
    auto ClassBuilderBase::Method(const char* name, Function f, const char* deprecatedName, BehaviorValues* defaultValues, const char* dbgDesc) -> ClassBuilderBase*
    {
        BehaviorParameterOverridesArray<Function> parameterOverrides;
        if (defaultValues)
        {
            AZ_Assert(defaultValues->GetNumValues() <= parameterOverrides.size(), "You can't have more default values than the number of function arguments");
            // Copy default values to parameter override structure
            size_t startArgumentIndex = parameterOverrides.size() - defaultValues->GetNumValues();
            for (size_t i = 0; i < defaultValues->GetNumValues(); ++i)
            {
                parameterOverrides[startArgumentIndex + i].m_defaultValue = defaultValues->GetDefaultValue(i);
            }
            delete defaultValues;
        }

        return Method(name, f, deprecatedName, parameterOverrides, dbgDesc);
    }

    template<class Function>
    auto ClassBuilderBase::Method(const char* name, Function f, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc) -> ClassBuilderBase*
    {
        return Method(name, f, nullptr, args, dbgDesc);
    }

    template<class Function>
    auto ClassBuilderBase::Method(
        const char* name,
        Function f,
        const BehaviorParameterOverrides& classMetadata,
        const BehaviorParameterOverridesArray<Function>& argsMetadata,
        const char* dbgDesc) -> ClassBuilderBase*
    {
        if (m_class)
        {
            using FunctionTraits = AZStd::function_traits<Function>;
            using FunctionCastType = AZStd::conditional_t<
                !AZStd::is_same_v<typename FunctionTraits::class_type, AZStd::Internal::error_type>,
                typename FunctionTraits::type,
                typename FunctionTraits::function_object_signature*>;
            BehaviorMethod* method =
                aznew BehaviorMethodImpl(static_cast<FunctionCastType>(f),
                    Base::m_context, AZStd::string::format("%s::%s", m_class->m_name.c_str(), name));
            method->m_debugDescription = dbgDesc;

            auto methodIter = m_class->m_methods.find(name);
            if (methodIter != m_class->m_methods.end())
            {
                if (!methodIter->second->AddOverload(method))
                {
                    AZ_Error("BehaviorContext", false, "Method incorrectly reflected as C++ overload");
                    delete method;
                    return this;
                }
            }
            else
            {
                m_class->m_methods.insert(AZStd::make_pair(name, method));
            }

            size_t classPtrIndex = 0;
            if (method->IsMember())
            {
                method->SetArgumentName(classPtrIndex, classMetadata.m_name);
                method->SetArgumentToolTip(classPtrIndex, classMetadata.m_toolTip);
                method->SetDefaultValue(classPtrIndex, classMetadata.m_defaultValue);
                method->OverrideParameterTraits(classPtrIndex, classMetadata.m_addTraits, classMetadata.m_removeTraits);
                classPtrIndex++;
            }

            for (size_t i = 0; i < argsMetadata.size(); ++i)
            {
                method->SetArgumentName(i + classPtrIndex, argsMetadata[i].m_name);
                method->SetArgumentToolTip(i + classPtrIndex, argsMetadata[i].m_toolTip);
                method->SetDefaultValue(i + classPtrIndex, argsMetadata[i].m_defaultValue);
                method->OverrideParameterTraits(i + classPtrIndex, argsMetadata[i].m_addTraits, argsMetadata[i].m_removeTraits);
            }

            // \note we can start returning a context so we can maintain the scope
            Base::m_currentAttributes = &method->m_attributes;
        }

        return this;
    }

    template<class Function>
    auto ClassBuilderBase::Method(const char* name, Function f, const char* deprecatedName, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc)
        -> ClassBuilderBase*
    {
        if (m_class)
        {
            using FunctionTraits = AZStd::function_traits<Function>;
            using FunctionCastType = AZStd::conditional_t<
                !AZStd::is_same_v<typename FunctionTraits::class_type, AZStd::Internal::error_type>,
                typename FunctionTraits::type,
                typename FunctionTraits::function_object_signature*>;
            BehaviorMethod* method = aznew BehaviorMethodImpl(static_cast<FunctionCastType>(f),
                Base::m_context, AZStd::string::format("%s::%s", m_class->m_name.c_str(), name));
            method->m_debugDescription = dbgDesc;

            /*
            ** check to see if the deprecated name is used, and ensure its not duplicated.
            */

            if (deprecatedName != nullptr)
            {
                auto itr = m_class->m_methods.find(name);
                if (itr != m_class->m_methods.end())
                {
                    // now check to make sure that the deprecated name is not being used as a identical deprecated name for another method.
                    bool isDuplicate = false;
                    for (const auto& i : m_class->m_methods)
                    {
                        if (i.second->GetDeprecatedName() == deprecatedName)
                        {
                            AZ_Warning("BehaviorContext", false, "Method %s is attempting to use a deprecated name of %s which is already in use for method %s! Deprecated name is ignored!", name, deprecatedName, i.first.c_str());
                            isDuplicate = true;
                            break;
                        }
                    }

                    if (!isDuplicate)
                    {
                        itr->second->SetDeprecatedName(deprecatedName);
                    }
                }
                else
                {
                    AZ_Warning("BehaviorContext", false, "Method %s does not exist, so the deprecated name is ignored!", name, deprecatedName);
                }
            }

            auto methodIter = m_class->m_methods.find(name);
            if (methodIter != m_class->m_methods.end())
            {
                if (!methodIter->second->AddOverload(method))
                {
                    AZ_Error("BehaviorContext", false, "Method incorrectly reflected as C++ overload");
                    delete method;
                    return this;
                }
            }
            else
            {
                m_class->m_methods.insert(AZStd::make_pair(name, method));
            }

            size_t classPtrIndex = method->IsMember() ? 1 : 0;
            for (size_t i = 0; i < args.size(); ++i)
            {
                method->SetArgumentName(i + classPtrIndex, args[i].m_name);
                method->SetArgumentToolTip(i + classPtrIndex, args[i].m_toolTip);
                method->SetDefaultValue(i + classPtrIndex, args[i].m_defaultValue);
                method->OverrideParameterTraits(i + classPtrIndex, args[i].m_addTraits, args[i].m_removeTraits);
            }

            // \note we can start returning a context so we can maintain the scope
            Base::m_currentAttributes = &method->m_attributes;
        }

        return this;
    }


    //////////////////////////////////////////////////////////////////////////
    template<class Getter, class Setter>
    auto ClassBuilderBase::Property(const char* name, Getter getter, Setter setter) -> ClassBuilderBase*
    {
        if (m_class)
        {
            BehaviorProperty* prop = aznew BehaviorProperty(Base::m_context);
            prop->m_name = name;
            if (!prop->Set(getter, setter, m_class, Base::m_context))
            {
                delete prop;
                return this;
            }

            m_class->m_properties.insert(AZStd::make_pair(name, prop));

            // \note we can start returning a context so we can maintain the scope
            Base::m_currentAttributes = &prop->m_attributes;
        }

        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<auto Value>
    auto ClassBuilderBase::Enum(const char* name) -> ClassBuilderBase*
    {
        Property(name, []() { return Value; }, nullptr);
        ClassBuilderBase::Attribute(AZ::Script::Attributes::ClassConstantValue, true);
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Getter>
    auto ClassBuilderBase::Constant(const char* name, Getter getter) -> ClassBuilderBase*
    {
        Property(name, getter, nullptr);
        return this;
    };
} // namespace AZ::Internal


namespace AZ
{
    // ClassBuilder implementation containing template functions
    // that rely on the class T parameter that cannot be implemented
    // in the base class
    template <class T>
    struct BehaviorContext::ClassBuilder : public Internal::ClassBuilderBase
    {
        using Internal::ClassBuilderBase::ClassBuilderBase;

        // Add an arrow operator for the derived template class
        ClassBuilder* operator->();

        // Backward Compatibility functions
        // As the majority of BehaviorContext class reflection uses operator->
        // for reflection, definitions for the ClassBuilderBase member functions
        // have been added to this template class, where it calls the base class function

        template<class U>
        ClassBuilder* Attribute(const char* id, U value);
        template<class U>
        ClassBuilder* Attribute(AZ::Crc32 id, U value);

        //! Attaches different constructor signatures to the class.
        template<class... Params>
        ClassBuilder* Constructor();

        template<class WrappedType, class Callable>
        ClassBuilder* WrappingMember(Callable callableFunction);

        ClassBuilder* UserData(void* userData);

        template<class Function>
        ClassBuilder* Method(const char* name, Function,
            BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        template<class Function>
        ClassBuilder* Method(const char* name, Function f, const char* deprecatedName,
            BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        template<class Function>
        ClassBuilder* Method(const char* name, Function f,
            const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc = nullptr);

        template<class Function>
        ClassBuilder* Method(
            const char* name,
            Function f,
            const BehaviorParameterOverrides& classMetadata,
            const BehaviorParameterOverridesArray<Function>& argsMetadata,
            const char* dbgDesc = nullptr);

        template<class Function>
        ClassBuilder* Method(const char* name, Function f, const char* deprecatedName,
            const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc = nullptr);

        template<class Getter, class Setter>
        ClassBuilder* Property(const char* name, Getter getter, Setter setter);

        template<auto Value>
        ClassBuilder* Enum(const char* name);

        template<class Getter>
        ClassBuilder* Constant(const char* name, Getter getter);

        ClassBuilder* RequestBus(const char* busName);
        ClassBuilder* NotificationBus(const char* busName);

    };

    template<class T>
    auto BehaviorContext::ClassBuilder<T>::operator->() -> ClassBuilder*
    {
        return this;
    }


    template<class T>
    template<class U>
    auto BehaviorContext::ClassBuilder<T>::Attribute(const char* id, U value)
        -> ClassBuilder*
    {
        ClassBuilderBase::Attribute(id, AZStd::move(value));
        return this;
    }

    template<class T>
    template<class U>
    auto BehaviorContext::ClassBuilder<T>::Attribute(AZ::Crc32 id, U value)
        -> ClassBuilder*
    {
        ClassBuilderBase::Attribute(id, AZStd::move(value));
        return this;
    }

    template<class T>
    template<class... Params>
    auto BehaviorContext::ClassBuilder<T>::Constructor() -> ClassBuilder*
    {
        ClassBuilderBase::ConstructorWithClass<T, Params...>();
        return this;
    }

    template<class T>
    template<class WrappedType, class Callable>
    auto BehaviorContext::ClassBuilder<T>::WrappingMember(Callable callableFunction)
        -> ClassBuilder*
    {
        ClassBuilderBase::WrappingMember<WrappedType>(AZStd::move(callableFunction));
        return this;
    }

    template<class T>
    auto BehaviorContext::ClassBuilder<T>::UserData(void* userData)
        -> ClassBuilder*
    {
        ClassBuilderBase::UserData(userData);
        return this;
    }

    template<class T>
    template<class Function>
    auto BehaviorContext::ClassBuilder<T>::Method(const char* name, Function f,
        BehaviorValues* defaultValues, const char* dbgDesc)
        -> ClassBuilder*
    {
        ClassBuilderBase::Method(name, AZStd::move(f), defaultValues, dbgDesc);
        return this;
    }

    template<class T>
    template<class Function>
    auto BehaviorContext::ClassBuilder<T>::Method(const char* name, Function f, const char* deprecatedName,
        BehaviorValues* defaultValues, const char* dbgDesc)
        -> ClassBuilder*
    {
        ClassBuilderBase::Method(name, AZStd::move(f), deprecatedName, defaultValues, dbgDesc);
        return this;
    }

    template<class T>
    template<class Function>
    auto BehaviorContext::ClassBuilder<T>::Method(const char* name, Function f,
        const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc)
        -> ClassBuilder*
    {
        ClassBuilderBase::Method(name, AZStd::move(f), args, dbgDesc);
        return this;
    }

    template<class T>
    template<class Function>
    auto BehaviorContext::ClassBuilder<T>::Method(
        const char* name,
        Function f,
        const BehaviorParameterOverrides& classMetadata,
        const BehaviorParameterOverridesArray<Function>& argsMetadata,
        const char* dbgDesc)
        -> ClassBuilder*
    {
        ClassBuilderBase::Method(name, AZStd::move(f), classMetadata, argsMetadata, dbgDesc);
        return this;
    }

    template<class T>
    template<class Function>
    auto BehaviorContext::ClassBuilder<T>::Method(const char* name, Function f, const char* deprecatedName,
        const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc)
        -> ClassBuilder*
    {
        ClassBuilderBase::Method(name, AZStd::move(f), deprecatedName, args, dbgDesc);
        return this;
    }

    template<class T>
    template<class Getter, class Setter>
    auto BehaviorContext::ClassBuilder<T>::Property(const char* name, Getter getter, Setter setter)
        -> ClassBuilder*
    {
        ClassBuilderBase::Property(name, AZStd::move(getter), AZStd::move(setter));
        return this;
    }

    template<class T>
    template<auto Value>
    auto BehaviorContext::ClassBuilder<T>::Enum(const char* name)
        -> ClassBuilder*
    {
        ClassBuilderBase::Enum<Value>(name);
        return this;
    }

    template<class T>
    template<class Getter>
    auto BehaviorContext::ClassBuilder<T>::Constant(const char* name, Getter getter)
        -> ClassBuilder*
    {
        ClassBuilderBase::Constant(name, AZStd::move(getter));
        return this;
    }

    template<class T>
    auto BehaviorContext::ClassBuilder<T>::RequestBus(const char* busName)
        -> ClassBuilder*
    {
        ClassBuilderBase::RequestBus(busName);
        return this;
    }
    template<class T>
    auto BehaviorContext::ClassBuilder<T>::NotificationBus(const char* busName)
        -> ClassBuilder*
    {
        ClassBuilderBase::NotificationBus(busName);
        return this;
    }
} // namespace AZ

namespace AZ
{
    // Returns a new ClassBuilder instance that can be used to configure a BehaviorClass
    // representing the type T being reflected
    template<class T>
    auto BehaviorContext::Class(const char* name) -> ClassBuilder<T>
    {
        if (name == nullptr)
        {
            name = AzTypeInfo<T>::Name();
        }

        AZ::Uuid typeUuid = AzTypeInfo<T>::Uuid();
        AZ_Assert(!typeUuid.IsNull(), "Type %s has no AZ_TYPE_INFO or AZ_RTTI.  Please use an AZ_RTTI or AZ_TYPE_INFO declaration before trying to use it in reflection contexts.", name ? name : "<Unknown class>");
        if (typeUuid.IsNull())
        {
            return ClassBuilder<T>(this, static_cast<BehaviorClass*>(nullptr));
        }

        auto classTypeIt = m_typeToClassMap.find(typeUuid);
        if (IsRemovingReflection())
        {
            if (classTypeIt != m_typeToClassMap.end())
            {
                // find it in the name category
                auto nameIt = m_classes.find(name);
                while (nameIt != m_classes.end())
                {
                    if (nameIt->second == classTypeIt->second)
                    {
                        m_classes.erase(nameIt);
                        break;
                    }
                }
                BehaviorContextBus::Event(this, &BehaviorContextBus::Events::OnRemoveClass, name, classTypeIt->second);
                delete classTypeIt->second;
                m_typeToClassMap.erase(classTypeIt);
            }
            return ClassBuilder<T>(this, static_cast<BehaviorClass*>(nullptr));
        }
        else
        {
            if (classTypeIt != m_typeToClassMap.end())
            {
                AZ_Error("Reflection", false, "Class '%s' is already registered using Uuid: %s!", name, classTypeIt->first.ToFixedString().c_str());
                return ClassBuilder<T>(this, static_cast<BehaviorClass*>(nullptr));
            }

            // TODO: make it a set and use the name inside the class
            if (m_classes.find(name) != m_classes.end())
            {
                AZ_Error("Reflection", false, "A class with name '%s' is already registered!", name);
                return ClassBuilder<T>(this, static_cast<BehaviorClass*>(nullptr));
            }

            BehaviorClass* behaviorClass = aznew BehaviorClass();
            behaviorClass->m_typeId = AzTypeInfo<T>::Uuid();
            behaviorClass->m_azRtti = GetRttiHelper<T>();
            behaviorClass->m_alignment = AZStd::alignment_of<T>::value;
            behaviorClass->m_size = sizeof(T);
            behaviorClass->m_name = name;

            // enumerate all base classes (RTTI), we store only the IDs to allow for our of order reflection
            // At runtime it will be more efficient to have the pointers to the classes. Analyze in practice and cache them if needed.
            AZ::RttiEnumHierarchy<T>(
                [](const AZ::Uuid& typeId, void* userData)
                {
                    BehaviorClass* bc = reinterpret_cast<BehaviorClass*>(userData);
                    AZ_Assert(bc, "behavior class is invalid for typeId: %s", typeId.ToString<AZStd::string>().c_str());
                    if (bc && typeId != bc->m_typeId)
                    {
                        bc->m_baseClasses.push_back(typeId);
                    }
                }
                , behaviorClass
                    );

            SetClassHasher<T>(behaviorClass);
            SetClassDefaultAllocator<T>(behaviorClass, AZStd::bool_constant<HasAZClassAllocator_v<T>>{});
            SetClassDefaultConstructor<T>(behaviorClass, typename AZStd::conditional< AZStd::is_constructible<T>::value && !AZStd::is_abstract<T>::value, AZStd::true_type, AZStd::false_type>::type());
            SetClassDefaultDestructor<T>(behaviorClass, typename AZStd::is_destructible<T>::type());
            SetClassDefaultCopyConstructor<T>(behaviorClass, typename AZStd::conditional< AZStd::is_copy_constructible<T>::value && !AZStd::is_abstract<T>::value, AZStd::true_type, AZStd::false_type>::type());
            SetClassDefaultMoveConstructor<T>(behaviorClass, typename AZStd::conditional< AZStd::is_move_constructible<T>::value && !AZStd::is_abstract<T>::value, AZStd::true_type, AZStd::false_type>::type());

            // Switch to Set (we store the name in the class)
            m_classes.insert(AZStd::make_pair(behaviorClass->m_name, behaviorClass));
            m_typeToClassMap.insert(AZStd::make_pair(behaviorClass->m_typeId, behaviorClass));
            return ClassBuilder<T>(this, behaviorClass);
        }
    };
} // namespace AZ
