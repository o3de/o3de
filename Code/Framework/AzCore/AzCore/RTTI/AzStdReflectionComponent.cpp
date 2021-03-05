/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#include <AzCore/RTTI/AzStdReflectionComponent.h>

#include <AzCore/std/any.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/Math/VectorFloat.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Internal
    {
        // Pushes an any onto the Lua stack
        void AnyToLua(lua_State* lua, BehaviorValueParameter& param)
        {
            AZStd::any* value = param.GetAsUnsafe<AZStd::any>();

#define CHECK_BUILTIN(T)                                            \
    if (value->is<T>()) {                                           \
        ScriptValue<T>::StackPush(lua, *AZStd::any_cast<T>(value)); \
        return;                                                     \
    }
            if (value->empty())
            {
                lua_pushnil(lua);
            }
            else CHECK_BUILTIN(bool)
            else CHECK_BUILTIN(char)
            else CHECK_BUILTIN(u8)
            else CHECK_BUILTIN(s16)
            else CHECK_BUILTIN(u16)
            else CHECK_BUILTIN(s32)
            else CHECK_BUILTIN(u32)
            else CHECK_BUILTIN(s64)
            else CHECK_BUILTIN(u64)
            else CHECK_BUILTIN(float)
            else CHECK_BUILTIN(double)
            else CHECK_BUILTIN(VectorFloat)
            else CHECK_BUILTIN(char*)
            else CHECK_BUILTIN(AZStd::string)
            else
            {
                Internal::LuaClassToStack(lua, AZStd::any_cast<void>(value), value->type());
            }

#undef CHECK_BUILTIN
        }

        // Reads the value in on top of the stack into an any
        bool AnyFromLua(lua_State* lua, int stackIndex, BehaviorValueParameter& value, BehaviorClass* /*ignore, it's any*/, ScriptContext::StackVariableAllocator* stackTempAllocator)
        {
            void* anyPtr = stackTempAllocator->allocate(sizeof(AZStd::any), AZStd::alignment_of<AZStd::any>::value);

            switch (lua_type(lua, stackIndex))
            {
            case LUA_TNIL:
                {
                    // Create empty any
                    value.Set(new(anyPtr) AZStd::any());
                }
                break;

            case LUA_TNUMBER:
                {
                    lua_Number number = lua_tonumber(lua, stackIndex);
                    value.Set(new(anyPtr) AZStd::any(number));
                }
                break;

            case LUA_TBOOLEAN:
                {
                    value.Set(new(anyPtr) AZStd::any(lua_toboolean(lua, stackIndex) == 1));
                }
                break;

            case LUA_TSTRING:
                {
                    value.Set(new(anyPtr) AZStd::any(AZStd::string(lua_tostring(lua, stackIndex))));
                }
                break;

            case LUA_TUSERDATA:
            case LUA_TLIGHTUSERDATA:
                {
                    void* userData = nullptr;
                    const BehaviorClass* sourceClass = nullptr;
                    Internal::LuaGetClassInfo(lua, stackIndex, &userData, &sourceClass);

                    // Can't copy value
                    Script::Attributes::StorageType storage = Script::Attributes::StorageType::ScriptOwn;
                    if (auto attribute = FindAttribute(Script::Attributes::Storage, sourceClass->m_attributes))
                    {
                        AZ::AttributeReader reader(nullptr, attribute);
                        reader.Read<Script::Attributes::StorageType>(storage);
                    }

                    AZStd::any::type_info type;
                    type.m_id = sourceClass->m_typeId;
                    type.m_isPointer = false;
                    type.m_useHeap = true;

                    if (storage == Script::Attributes::StorageType::Value)
                    {
                        if (!sourceClass->m_allocate || !sourceClass->m_cloner || !sourceClass->m_mover || !sourceClass->m_destructor || !sourceClass->m_deallocate)
                        {
                            return false;
                        }

                        // If it's a value type, copy/move it around
                        type.m_handler = [sourceClass](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
                        {
                            switch (action)
                            {
                            case AZStd::any::Action::Reserve:
                                {
                                    *reinterpret_cast<void**>(dest) = sourceClass->Allocate();
                                    break;
                                }
                            case AZStd::any::Action::Copy:
                                {
                                    sourceClass->m_cloner(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(source), sourceClass->m_userData);
                                    break;
                                }
                            case AZStd::any::Action::Move:
                                {
                                    sourceClass->m_mover(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(const_cast<AZStd::any*>(source)), sourceClass->m_userData);
                                    break;
                                }
                            case AZStd::any::Action::Destroy:
                                {
                                    sourceClass->Destroy(BehaviorObject(AZStd::any_cast<void>(dest), sourceClass->m_typeId));
                                    break;
                                }
                            }
                        };
                    }
                    else
                    {
                        // If not a value type, just move the pointer around
                        type.m_handler = [](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
                        {
                            switch (action)
                            {
                            case AZStd::any::Action::Reserve:
                                {
                                    // No-op
                                    break;
                                }
                            case AZStd::any::Action::Copy:
                            case AZStd::any::Action::Move:
                                {
                                    *reinterpret_cast<void**>(dest) = AZStd::any_cast<void>(const_cast<AZStd::any*>(source));
                                    break;
                                }
                            case AZStd::any::Action::Destroy:
                                {
                                    *reinterpret_cast<void**>(dest) = nullptr;
                                    break;
                                }
                            }
                        };
                    }

                    value.Set(new(anyPtr) AZStd::any(userData, type));
                }
                break;

            case LUA_TTABLE:
            case LUA_TFUNCTION:
            case LUA_TTHREAD:
            default:
                // Tables, functions, and threads will never be convertible, as we have no structure to convert it to
                stackTempAllocator->deallocate(anyPtr, sizeof(AZStd::any), AZStd::alignment_of<AZStd::any>::value);
                return false;
            }

            return true;
        }
    }

    void AzStdReflectionComponent::Reflect(ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AzStdReflectionComponent, AZ::Component>()
                ->Version(1);
        }
        else if (BehaviorContext* behavior = azrtti_cast<BehaviorContext*>(context))
        {
            behavior->Class<AZStd::any>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(Script::Attributes::Ignore, true)   // Don't reflect any type to script (there should never be an any instance in script)
                ->Attribute(Script::Attributes::ReaderWriterOverride, ScriptContext::CustomReaderWriter(&Internal::AnyToLua, &Internal::AnyFromLua))
                ;
        }
    }
}
