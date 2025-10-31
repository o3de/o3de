/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Script/ScriptContext.h>

namespace AZStd
{
    template<class Element, class Traits, class Allocator>
    class basic_string;

    template<class Element, class Traits>
    class basic_string_view;
}

namespace AZ
{
    class BehaviorContext;
    class ScriptDataContext;

    namespace OnDemandLuaFunctions
    {
        template<class Element, class Traits>
        inline void ConstructStringView(AZStd::basic_string_view<Element, Traits>* thisPtr, ScriptDataContext& dc)
        {
            using ContainerType = AZStd::basic_string_view<Element, Traits>;

            // TODO: Count reflection types for a proper un-reflect
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsString(0))
                {
                    // this is the part that has to be custom the rest can be shared
                    AZStd::basic_string<Element, Traits, AZStd::allocator> str;
                    dc.ReadArg(0, str);
                    new(thisPtr) ContainerType(str);
                }
                else if (dc.IsClass<ContainerType>(0))
                {
                    dc.ReadArg(0, thisPtr);
                }
            }
        }

        template<class ContainerType>
        inline void ConstructBasicString(ContainerType* thisPtr, ScriptDataContext& dc)
        {
            // TODO: Count reflection types for a proper un-reflect
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsString(0))
                {
                    ContainerType str;
                    dc.ReadArg(0, str);
                    new(thisPtr) ContainerType(AZStd::move(str));
                }
                else if (dc.IsClass<ContainerType>(0))
                {
                    dc.ReadArg(0, thisPtr);
                }
            }
        }

        // Read a Lua native string as a C++ string class
        template<typename StringType>
        inline bool StringTypeFromLua(lua_State* lua, int stackIndex, BehaviorArgument& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator* stackTempAllocator)
        {
            const char* str = ScriptValue<const char*>::StackRead(lua, stackIndex);
            if (stackTempAllocator)
            {
                // Allocate space for the string.
                // If passing by reference, we need to construct it ahead of time (in this allocated space)
                // If not, StoreResult will construct it for us, and we can pass in a temp
                value.m_value = stackTempAllocator->allocate(sizeof(StringType), AZStd::alignment_of<StringType>::value, 0);

                if (valueClass->m_defaultConstructor)
                {
                    valueClass->m_defaultConstructor(value.m_value, valueClass->m_userData);
                }

            }
            else if (value.m_value)
            {
                // If there's no allocator, it's possible the destination has already been set-up
            }
            else
            {
                AZ_Assert(false, "Invalid call to FromLua! Either a stack allocator must be passed, or value.m_value must be a valid storage location.");
            }

            // Can't construct string from nullptr, so replace it with empty string
            if (str == nullptr)
            {
                str = "";
            }

            if (value.m_traits & BehaviorParameter::TR_REFERENCE)
            {
                new(value.m_value) StringType(str);
            }
            else
            {
                // Otherwise, we can construct the string as we pass it, but need to allocate value.m_value ahead of time
                value.StoreResult<StringType>(StringType(str));
            }

            return true;
        }

        // Store any string as a Lua native string
        template<typename StringType>
        inline void StringTypeToLua(lua_State * lua, BehaviorArgument & value)
        {
            ScriptValue<const char*>::StackPush(lua, reinterpret_cast<StringType*>(value.GetValueAddress())->data());
        }

        // Reads the value in on top of the stack into an any
        inline bool AnyFromLua(lua_State* lua, int stackIndex, BehaviorArgument& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator* stackTempAllocator)
        {
            if (stackTempAllocator)
            {
                // Note, this is safe to do in this case, even if we are reading an any pointer type
                // The BehaviorArgument remains in scope for the duration of AZ::LuaScriptCaller::Call, so
                // StoreResult, even though it's using temporarily allocated memory, should remain in scope for the lifetime of the CustomReaderWriter invocation
                value.m_value = stackTempAllocator->allocate(sizeof(AZStd::any), AZStd::alignment_of<AZStd::any>::value);

                // if a reference type, the StoreResult call will point to returned, temporary memory, so a value copy is necessary
                // this value was created by the stackTempAllocator, so it is acceptable to modify the BVP.
                value.m_traits = 0;

                if (valueClass->m_defaultConstructor)
                {
                    valueClass->m_defaultConstructor(value.m_value, valueClass->m_userData);
                }
            }

            AZ_Assert(value.m_value, "Invalid call to FromLua! Either a stack allocator must be passed, or value.m_value must be a valid storage location.");
            return value.StoreResult(AZStd::move(ScriptValue<AZStd::any>::StackRead(lua, stackIndex)));
        }

        // Pushes an any onto the Lua stack
        inline void AnyToLua(lua_State* lua, BehaviorArgument& param)
        {
            AZStd::any* value = param.GetAsUnsafe<AZStd::any>();
            if (value)
            {
                ScriptValue<AZStd::any>::StackPush(lua, *value);
            }
        }

    } // namespace OnDemandLuaFunctions

} // namespace AZ

