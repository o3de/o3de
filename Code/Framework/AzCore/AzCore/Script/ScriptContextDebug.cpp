/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZCORE_EXCLUDE_LUA)

#include <AzCore/Script/lua/lua.h>
#include <AzCore/Script/ScriptContextDebug.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Debug/StackTracer.h>  // for CodeStackTrace
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/Locale.h>
#include <AzCore/std/string/tokenize.h>

extern "C" {
#   include <Lua/lualib.h>
#   include <Lua/lauxlib.h>
}

namespace AZ
{
    
    void LuaHook(lua_State* l, lua_Debug* ar);

/**
 * A temp class that will override the current script context error handler and store the error (without any messages)
 * for a later query.
 */
class ScriptErrorCatcher
{
public:
    ScriptErrorCatcher(ScriptContext* context)
        : m_context(context)
        , m_oldHook(context->GetErrorHook())
        , m_numErrors(0)
    {
        using namespace AZStd::placeholders;
        m_context->SetErrorHook([this](ScriptContext* a, ScriptContext::ErrorType b, const char* c) { ErrorCB(a,b,c); });
    }
    ~ScriptErrorCatcher()
    {
        m_context->SetErrorHook(m_oldHook);
    }

    void ErrorCB(ScriptContext*, ScriptContext::ErrorType, const char*)
    {
        ++m_numErrors;
    }

    ScriptContext* m_context;
    ScriptContext::ErrorHook m_oldHook;

    int m_numErrors;
};

//=========================================================================
// ScriptContextDebug
// [6/29/2012]
//=========================================================================
ScriptContextDebug::ScriptContextDebug(ScriptContext& scriptContext, bool isEnableStackRecord)
    : m_luaDebug(nullptr)
    , m_currentStackLevel(-1)
    , m_stepStackLevel(-1)
    , m_isRecordCallstack(isEnableStackRecord)
    , m_isRecordCodeCallstack(AZ_TRAIT_SCRIPT_RECORD_CALLSTACK_DEFAULT)
    , m_context(scriptContext)
{
    ConnectHook();
}

//=========================================================================
// ~ScriptContextDebug
// [6/29/2012]
//=========================================================================
ScriptContextDebug::~ScriptContextDebug()
{
    DisconnectHook();
    DisableBreakpoints();
}


//=========================================================================
// ConnectHook
//=========================================================================
void ScriptContextDebug::ConnectHook()
{
    lua_Hook hook = lua_gethook(m_context.NativeContext());
    if (!hook)
    {
        lua_sethook(m_context.NativeContext(), &LuaHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
    }
}

//=========================================================================
// ConnectHook
//=========================================================================
void ScriptContextDebug::DisconnectHook()
{
    lua_sethook(m_context.NativeContext(), nullptr, 0, 0);
    m_currentStackLevel = -1;
    m_stepStackLevel = -1;
}

//=========================================================================
// EnumRegisteredClasses
// [4/5/2012]
//=========================================================================
void
ScriptContextDebug::EnumRegisteredClasses(EnumClass enumClass, EnumMethod enumMethod, EnumProperty enumProperty, void* userData)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    AZ_Assert(enumClass && enumMethod && enumProperty, "Invalid input!");

    lua_State* l = m_context.NativeContext();
    lua_rawgeti(l, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF); // load the class table

    lua_pushnil(l);

    while (lua_next(l, -2) != 0)
    {
        if (!lua_isuserdata(l, -2))
        {
            lua_pop(l, 1);
            continue;
        }

        if (lua_istable(l, -1))
        {
            lua_rawgeti(l, -1, AZ_LUA_CLASS_METATABLE_BEHAVIOR_CLASS);
            BehaviorClass* behaviorClass = reinterpret_cast<BehaviorClass*>(lua_touserdata(l, -1));

            if (AZ::Attribute* attribute = behaviorClass->FindAttribute(AZ::Script::Attributes::ExcludeFrom))
            {
                auto excludeAttributeData = azdynamic_cast<const AZ::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(attribute);
                AZ::u64 exclusionFlags = AZ::Script::Attributes::ExcludeFlags::List | AZ::Script::Attributes::ExcludeFlags::Documentation;

                bool shouldSkip = (static_cast<AZ::u64>(excludeAttributeData->Get(nullptr)) & exclusionFlags) != 0;
                if (shouldSkip)
                {
                    lua_pop(l, 2);
                    continue;
                }
            }

            lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load class name
            AZ_Assert(lua_isstring(l, -1), "Internal scipt error: class without a classname at index %d", AZ_LUA_CLASS_METATABLE_NAME_INDEX);

            if (!enumClass(lua_tostring(l, -1), behaviorClass->m_typeId, userData))
            {
                lua_pop(l, 5);
                return;
            }
            lua_pop(l, 2); // pop the Class name and behaviorClass

            lua_pushnil(l);

            // iterate over the key/value pairs
            while (lua_next(l, -2) != 0)
            {
                // if key: string value: function
                if (lua_isstring(l, -2) && lua_isfunction(l, -1))
                {
                    const char* name = lua_tostring(l, -2);
                    if (lua_tocfunction(l, -1) == &Internal::LuaPropertyTagHelper) // if it's a property
                    {
                        bool isRead = true;
                        bool isWrite = true;

                        // check if there is a getter provided
                        lua_getupvalue(l, -1, 1);
                        if (lua_isnil(l, -1))
                        {
                            isRead = false;
                        }
                        lua_pop(l, 1);

                        // check if there is a setter provided
                        lua_getupvalue(l, -1, 2);
                        if (lua_isnil(l, -1))
                        {
                            isWrite = false;
                        }
                        lua_pop(l, 1);

                        // enumerate the remaining property
                        if (!enumProperty(&behaviorClass->m_typeId, name, isRead, isWrite, userData))
                        {
                            lua_pop(l, 5);
                            return;
                        }
                    }
                    else
                    {
                        // for any non-built in methods
                        if (strncmp(name, "__", 2) != 0)
                        {
                            const char* dbgParamInfo = nullptr;

                            // attempt to get the name
                            bool popDebugName = lua_getupvalue(l, -1, 2) != nullptr;
                            if (lua_isstring(l, -1))
                            {
                                dbgParamInfo = lua_tostring(l, -1);
                            }

                            // enumerate the method's parameters
                            if (!enumMethod(&behaviorClass->m_typeId, name, dbgParamInfo, userData))
                            {
                                lua_pop(l, 6);
                                return;
                            }

                            // if we were able to get the name, pop it from the stack
                            if (popDebugName)
                            {
                                lua_pop(l, 1);
                            }
                        }
                    }
                }
                lua_pop(l, 1);
            }
        }
        lua_pop(l, 1);
    }
    lua_pop(l, 1); // pop the class table
}

//=========================================================================
// EnumRegisteredGlobals
// [4/5/2012]
//=========================================================================
void
ScriptContextDebug::EnumRegisteredGlobals(EnumMethod enumMethod, EnumProperty enumProperty, void* userData)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    lua_State* l = m_context.NativeContext();
    lua_rawgeti(l, LUA_REGISTRYINDEX, AZ_LUA_GLOBALS_TABLE_REF); // load the class table

    //AZ_TracePrintf("Script","\nGlobals\n");
    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        if (lua_isstring(l, -2) && lua_isfunction(l, -1))
        {
            const char* name = lua_tostring(l, -2);
            if (lua_tocfunction(l, -1) == &Internal::LuaPropertyTagHelper) // if it's a property
            {
                bool isRead = true;
                bool isWrite = true;

                lua_getupvalue(l, -1, 1);
                if (lua_isnil(l, -1))
                {
                    isRead = false;
                }
                lua_pop(l, 1);

                lua_getupvalue(l, -1, 2);
                if (lua_isnil(l, -1))
                {
                    isWrite = false;
                }
                lua_pop(l, 1);

                //AZ_TracePrintf("Script"," -- property %s %s\n",name,propType);
                if (!enumProperty(nullptr, name, isRead, isWrite, userData))
                {
                    lua_pop(l, 3);
                    return;
                }
            }
            else
            {
                if (strncmp(name, "__", 2) != 0)
                {
                    const char* dbgParamInfo = nullptr;
                    lua_getupvalue(l, -1, 2);
                    if (lua_isstring(l, -1))
                    {
                        dbgParamInfo = lua_tostring(l, -1);
                    }

                    if (!enumMethod(nullptr, name, dbgParamInfo, userData))
                    {
                        lua_pop(l, 4);
                        return;
                    }
                    lua_pop(l, 1); // pop the dbgParamInfo
                }
            }
        }
        lua_pop(l, 1);
    }

    lua_pop(l, 1); // pop globals table
}

bool EBusCanBroadcast(const AZ::BehaviorEBus* bus)
{
    auto canBroadcast = [](const AZStd::pair<AZStd::string, AZ::BehaviorEBusEventSender>& pair) { return pair.second.m_broadcast != nullptr; };
    return AZStd::find_if(bus->m_events.begin(), bus->m_events.end(), canBroadcast) != bus->m_events.end();
}

bool EBusCanQueue(const AZ::BehaviorEBus* bus)
{
    auto canQueue = [](const AZStd::pair<AZStd::string, AZ::BehaviorEBusEventSender>& pair) { return pair.second.m_queueEvent != nullptr || pair.second.m_queueBroadcast != nullptr; };
    return AZStd::find_if(bus->m_events.begin(), bus->m_events.end(), canQueue) != bus->m_events.end();
}

bool EBusHasHandler(const AZ::BehaviorEBus* bus)
{
    return bus->m_createHandler && bus->m_destroyHandler;
}

void ScriptContextDebug::EnumRegisteredEBuses(EnumEBus enumEBus, EnumEBusSender enumEBusSender, void* userData)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    AZ::BehaviorContext* behaviorContext = m_context.GetBoundContext();
    if (!behaviorContext)
    {
        return;
    }

    for (auto const &ebusPair : behaviorContext->m_ebuses)
    {
        AZ::BehaviorEBus* ebus = ebusPair.second;

        // Do not enum if this bus should not be available in Lua
        if (!AZ::Internal::IsAvailableInLua(ebus->m_attributes))
        {
            continue;
        }

        bool canBroadcast = EBusCanBroadcast(ebus);
        bool canQueue = EBusCanQueue(ebus);
        bool hasHandler = EBusHasHandler(ebus);

        enumEBus(ebus->m_name, canBroadcast, canQueue, hasHandler, userData);

        for (const auto& eventSender : ebus->m_events)
        {
            if (eventSender.second.m_event)
            {
                // Find a func that does this already!
                AZStd::string scriptArgs;
                for (size_t i = 0; i < eventSender.second.m_event->GetNumArguments(); ++i)
                {
                    AZStd::string argName = eventSender.second.m_event->GetArgument(i)->m_name;
                    StripQualifiers(argName);
                    scriptArgs += argName;
                    if (i != eventSender.second.m_event->GetNumArguments() - 1)
                    {
                        scriptArgs += ", ";
                    }
                }

                AZStd::string funcName = eventSender.first;
                StripQualifiers(funcName);

                enumEBusSender(ebus->m_name, funcName, scriptArgs, "Event", userData);
            }
            if (eventSender.second.m_broadcast)
            {
                // Find a func that does this already!
                AZStd::string scriptArgs;
                for (size_t i = 0; i < eventSender.second.m_broadcast->GetNumArguments(); ++i)
                {
                    AZStd::string argName = eventSender.second.m_broadcast->GetArgument(i)->m_name;
                    StripQualifiers(argName);
                    scriptArgs += argName;
                    if (i != eventSender.second.m_broadcast->GetNumArguments() - 1)
                    {
                        scriptArgs += ", ";
                    }
                }

                AZStd::string funcName = eventSender.first;
                StripQualifiers(funcName);

                enumEBusSender(ebus->m_name, funcName, scriptArgs, "Broadcast", userData);
            }
        }

        if (ebus->m_createHandler)
        {
            AZ::BehaviorEBusHandler* handler = nullptr;
            ebus->m_createHandler->InvokeResult(handler);
            if (handler)
            {
                const auto& notifications = handler->GetEvents();
                for (const auto& notification : notifications)
                {
                    AZStd::string scriptArgs;
                    const size_t paramCount = notification.m_parameters.size();
                    for (size_t i = 0; i < notification.m_parameters.size(); ++i)
                    {
                        AZStd::string argName = notification.m_parameters[i].m_name;
                        StripQualifiers(argName);
                        scriptArgs += argName;
                        if (i != paramCount - 1)
                        {
                            scriptArgs += ", ";
                        }
                    }

                    AZStd::string funcName = notification.m_name;
                    StripQualifiers(funcName);

                    enumEBusSender(ebus->m_name, funcName, scriptArgs, "Notification", userData);
                }
                ebus->m_destroyHandler->Invoke(handler);
            }
        }
    }
}


//=========================================================================
// StackTrace
// [4/9/2012]
//=========================================================================
void
ScriptContextDebug::StackTrace(char* stackOutput, size_t stackOutputSize)
{
    if (m_isRecordCallstack)
    {
        for (int i = static_cast<int>(m_callstack.size()) - 1; i >= 0; --i)
        {
            CallstackLine& cl = m_callstack[i];
            if (cl.m_codeNumStackFrames)
            {
                /// C++ stack trace
                if (cl.m_codeStackFrames)  // full stack frames
                {
                    Debug::SymbolStorage::StackLine stackLine;
                    stackLine[0] = '\0';
                    for (int iFrame = 0; iFrame < cl.m_codeNumStackFrames; ++iFrame)
                    {
                        Debug::SymbolStorage::DecodeFrames(&cl.m_codeStackFrames[iFrame], 1, &stackLine);
                        int numberAdded = azsnprintf(stackOutput, stackOutputSize, "%s\n", stackLine);
                        if (numberAdded < 0)
                        {
                            return;
                        }
                        stackOutputSize -= static_cast<size_t>(numberAdded);
                        stackOutput += numberAdded;
                    }
                }
                else
                {
                    int numberAdded = azsnprintf(stackOutput, stackOutputSize, "[C++] Code tracing not enabled!\n");
                    if (numberAdded < 0)
                    {
                        return;
                    }
                    stackOutputSize -= static_cast<size_t>(numberAdded);
                    stackOutput += numberAdded;
                }
            }
            else
            {
                /// LUA stack trace
                char definedName[128];
                const char* functionName = cl.m_functionName;
                if (functionName == nullptr)
                {
                    // print the defined name
                    if (cl.m_lineDefined != 0)
                    {
                        azsnprintf(definedName, AZ_ARRAY_SIZE(definedName), "FunctionDefinedAtLine[%d]", cl.m_lineDefined);
                        functionName = definedName;
                    }
                    else
                    {
                        functionName = "unknown";
                    }
                }
                int numberAdded = azsnprintf(stackOutput, stackOutputSize, "[%s] %s (%d) : %s(%s)\n", cl.m_functionType, cl.m_sourceName, cl.m_lineCalled, functionName, cl.m_parameters.c_str());
                if (numberAdded < 0)
                {
                    return;
                }
                stackOutputSize -= static_cast<size_t>(numberAdded);
                stackOutput += numberAdded;
            }
            if (stackOutputSize == 0)
            {
                break; // stop if we run out of buffer space
            }
        }
    }
    else
    {
        // get what we can from the call stack trace
        Internal::LuaStackTrace(m_context.NativeContext(), stackOutput, stackOutputSize);
    }
}

//=========================================================================
// PushCodeCallstack
// [8/14/2012]
//=========================================================================
void
ScriptContextDebug::PushCodeCallstack(int stackSuppressCount, int numStackLevels)
{
    // TODO: Move this function to a common ScriptContextDebug.cpp there is no lua dependency here
    if (m_isRecordCallstack && numStackLevels > 0)
    {
        CallstackLine& cl = m_callstack.emplace_back();
        cl.m_codeNumStackFrames = numStackLevels; // set non 0 number to indicate a C++ code on the callstack
        if (m_isRecordCodeCallstack)
        {
            cl.m_codeStackFrames = reinterpret_cast<AZ::Debug::StackFrame*>(azmalloc(sizeof(AZ::Debug::StackFrame) * numStackLevels, 1, OSAllocator));
            if (cl.m_codeStackFrames)
            {
                Debug::StackRecorder::Record(cl.m_codeStackFrames, numStackLevels, stackSuppressCount + 1);
            }
        }
    }
}

//=========================================================================
// PopCallstack
// [8/14/2012]
//=========================================================================
void
ScriptContextDebug::PopCallstack()
{
    if (m_isRecordCallstack && !m_callstack.empty())
    {
        CallstackLine& cl = m_callstack.back();
        if (cl.m_codeNumStackFrames > 0)
        {
            azfree(cl.m_codeStackFrames, OSAllocator);
        }
        m_callstack.pop_back();
    }
}

//=========================================================================
// DbgValueToString
// [8/15/2012]
//=========================================================================
OSString
ScriptContextDebug::DbgValueToString(int valueIndex)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    lua_State* l = m_context.NativeContext();
    int type = lua_type(l, valueIndex);
    switch (type)
    {
    case LUA_TSTRING:
        return lua_tostring(l, valueIndex);
    case LUA_TNUMBER:
        return OSString::format("%f", lua_tonumber(l, valueIndex));
    case LUA_TTABLE:
    case LUA_TFUNCTION:
    case LUA_TLIGHTUSERDATA:
    case LUA_TTHREAD:
        return OSString::format("%p", lua_topointer(l, valueIndex));
    case LUA_TUSERDATA: // get the pointer to our class
    {
        void* objectAddress;
        if (Internal::LuaGetClassInfo(l, -1, &objectAddress, nullptr))
        {
            return OSString::format("%p", objectAddress);
        }
        else
        {
            return "0";
        }
    }
    case LUA_TNIL:
        return "nil";
    case LUA_TNONE:
    default:
        return "none";
    }
}

//=========================================================================
// MakeBreakpointId
// [6/29/2012]
//=========================================================================
static ScriptContextDebug::BreakpointId MakeBreakpointId(const char* sourceName, int lineNumber)
{
    char buffer[1024];
    azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%d %s", lineNumber, sourceName);
    return AZStd::string(buffer);
}

//=========================================================================
// LuaHook
// [6/28/2012]
//=========================================================================
void LuaHook(lua_State* l, lua_Debug* ar)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    // Read contexts
    lua_rawgeti(l, LUA_REGISTRYINDEX, AZ_LUA_SCRIPT_CONTEXT_REF);
    ScriptContext* sc = reinterpret_cast<ScriptContext*>(lua_touserdata(l, -1));
    ScriptContextDebug* context = sc->GetDebugContext();
    lua_pop(l, 1);
    //
    bool doBreak = false;
    ScriptContextDebug::Breakpoint* bp = nullptr;
    ScriptContextDebug::Breakpoint  localBreakPoint;

    lua_getinfo(l, "Sunl", ar);

    if (ar->event == LUA_HOOKCALL)
    {
        // add to callstack
        if (context->m_isRecordCallstack)
        {
            ScriptContextDebug::CallstackLine& cl = context->m_callstack.emplace_back();
            cl.m_sourceName = ar->source;
            cl.m_functionType = ar->what;
            if (strcmp(ar->what, "main") != 0)
            {
                cl.m_lineCalled = ar->currentline;
#if LUA_VERSION_NUM >= 502
                for (int i = 0; i < ar->nparams; ++i)
                {
                    cl.m_parameters += context->DbgValueToString(1 + i);
                    if (i < ar->nparams - 1)
                    {
                        cl.m_parameters += ", ";
                    }
                }
#endif // LUA_VERSION_NUM
                cl.m_functionName = ar->name;
                cl.m_lineDefined = ar->linedefined;
            }
            else
            {
                cl.m_functionName = "[ScriptStart]";
            }
        }
        context->m_currentStackLevel++;
    }
    else if (ar->event == LUA_HOOKRET)
    {
        // remove from callstack
        if (context->m_isRecordCallstack)
        {
            context->PopCallstack();
        }
        context->m_currentStackLevel--;

        if (context->m_currentStackLevel == -1)
        {
            context->m_stepStackLevel = -1;
        }
    }
    else if (ar->event == LUA_HOOKLINE)
    {
        if (context->m_stepStackLevel != -1 && context->m_currentStackLevel <= context->m_stepStackLevel)
        {
            doBreak = true;
            context->m_stepStackLevel = -1;
            localBreakPoint.m_sourceName = ar->source;
            localBreakPoint.m_lineNumber = ar->currentline;
            bp = &localBreakPoint;
        }
        else if (!context->m_breakpoints.empty() && ar->source && ar->currentline != -1)
        {
            for (auto& breakpoint : context->m_breakpoints)
            {
                int line = 0;

                const char* breakpointID = breakpoint.first.c_str();
                azsscanf(breakpointID, "%d", &line);

                const char* substr = strstr(breakpointID, ar->source);
                if (substr && line == ar->currentline)
                {
                    bp = &breakpoint.second;

                    if (!bp->m_expression.empty())
                    {
                        //doBreak = true; // break by default
                        int oldStackPos = lua_gettop(l);
                        int res = luaL_dostring(l, bp->m_expression.c_str());
                        if (res == 0)
                        {
                            int numResults = lua_gettop(l) - oldStackPos;
                            if (numResults > 0)
                            {
                                doBreak = lua_toboolean(l, -1) == 1;
                            }
                            lua_pop(l, numResults);
                        }
                    }
                    else if (bp->m_count != -1)
                    {
                        --bp->m_count;
                        if (bp->m_count == 0)
                        {
                            doBreak = true;
                            bp->m_count = -1;
                        }
                    }
                    else
                    {
                        doBreak = true;
                    }
                }
            }
        }

        if (context->m_isRecordCallstack /*&& !context->m_callstack.empty()*/)
        {
            ScriptContextDebug::CallstackLine& cl = context->m_callstack.back();
            cl.m_lineCalled = ar->currentline;
        }

        //printf("\nLua Hook!\n");
        //printf("Event %d name %s nameWhat %s what %s\n",ar->event,ar->name ? ar->name : "NoName", ar->namewhat ? ar->namewhat : "NoName",ar->what ? ar->what : "NoName");
        //printf("Source %s cur_line %d line_def %d line_last_def %d\n",ar->source ? ar->source : "NoName",ar->currentline,ar->linedefined,ar->lastlinedefined);
        //printf("Ups %d Params %d IsVarArg %d\n",ar->nups,ar->nparams,ar->isvararg);
        //printf("ShortSrc %s\n",ar->short_src ? ar->short_src : "NoName");

        //int local = 1;
        //const char* name;
        //while((name = lua_getlocal(l,ar,local)) != NULL)
        //{
        //  if(name[0]!='(')
        //      printf("    Local[%d]: %s : %d\n",local,name,lua_tointeger(l,-1));
        //  lua_pop(l,1);
        //  ++local;
        //}
    }

    if (doBreak && bp->m_lineNumber > 0)
    {
        context->m_luaDebug = ar;
        context->m_breakCallback(context, bp);
        context->m_luaDebug = nullptr;
    }
}

//=========================================================================
// EnumLocals
// [6/29/2012]
//=========================================================================
void
ScriptContextDebug::EnumLocals(EnumLocalCallback& cb)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    if (cb && m_luaDebug)
    {
        lua_State* l = m_context.NativeContext();
        int local = 1;
        const char* name;
        ScriptDataContext dc;
        while ((name = lua_getlocal(l, m_luaDebug, local)) != nullptr)
        {
            if (name[0] != '(') // skip temporary variables
            {
                dc.SetInspect(l, lua_gettop(l));
                if (!cb(name, dc))
                {
                    break;
                }
            }
            else
            {
                lua_pop(l, 1);
            }
            ++local;
        }
    }
}

//=========================================================================
// StepInto
// [7/2/2012]
//=========================================================================
void
ScriptContextDebug::StepInto()
{
    if (m_luaDebug)
    {
        m_stepStackLevel = 10000; // some high number so any line (next line) will hit
    }
    else
    {
        AZ_Warning("Script", false, "You can call StepInto only from Breakpoint callback!");
    }
}

//=========================================================================
// StepOver
// [7/2/2012]
//=========================================================================
void
ScriptContextDebug::StepOver()
{
    if (m_luaDebug)
    {
        m_stepStackLevel = m_currentStackLevel;
    }
    else
    {
        AZ_Warning("Script", false, "You can call StepOver only from Breakpoint callback!");
    }
}

//=========================================================================
// StepOut
// [7/2/2012]
//=========================================================================
void
ScriptContextDebug::StepOut()
{
    if (m_luaDebug)
    {
        if (m_currentStackLevel > 0)
        {
            m_stepStackLevel = m_currentStackLevel - 1;
        }
        else
        {
            // -1 forces to exit the current function at the top of the callstack.
            // This is important, because if set to 0, StepOut would behave like a
            // StepOver event and that's not the right user experience when using
            // debuggers.
            m_stepStackLevel = -1; 
        }
    }
    else
    {
        AZ_Warning("Script", false, "You can call StepOut only from Breakpoint callback!");
    }
}

//=========================================================================
// EnableBreakpoints
// [6/29/2012]
//=========================================================================
void
ScriptContextDebug::EnableBreakpoints(BreakpointCallback& cb)
{
    m_breakCallback = cb;
}

//=========================================================================
// DisableBreakpoints
// [6/29/2012]
//=========================================================================
void
ScriptContextDebug::DisableBreakpoints()
{
    m_breakCallback = nullptr;
}

//=========================================================================
// AddBreakpoint
// [6/29/2012]
//=========================================================================
void
ScriptContextDebug::AddBreakpoint(Breakpoint& bp)
{
    AZ_Assert(!bp.m_sourceName.empty() && bp.m_lineNumber != -1, "Invalid breakpoint!");
    BreakpointId id = MakeBreakpointId(bp.m_sourceName.c_str(), bp.m_lineNumber);
    m_breakpoints.insert(AZStd::make_pair(id, bp));
}

//=========================================================================
// RemoveBreakpoint
// [6/29/2012]
//=========================================================================
void
ScriptContextDebug::RemoveBreakpoint(Breakpoint& bp)
{
    AZ_Assert(!bp.m_sourceName.empty() && bp.m_lineNumber != -1, "Invalid breakpoint!");
    BreakpointId id = MakeBreakpointId(bp.m_sourceName.c_str(), bp.m_lineNumber);
    m_breakpoints.erase(id);
}

//=========================================================================
// ReadValue
// [8/15/2012]
//=========================================================================
void
ScriptContextDebug::ReadValue(DebugValue& value, VoidPtrArray& tablesVisited, bool isReadOnly)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    lua_State* l = m_context.NativeContext();
    int valueType = lua_type(l, -1);
    value.m_type = static_cast<char>(valueType);
    value.m_flags = isReadOnly ? DebugValue::FLAG_READ_ONLY : 0;

    bool showMetatable = true;
    switch (valueType)
    {
    case LUA_TSTRING:
    {
        value.m_value = lua_tostring(l, -1);
        value.m_flags |= isReadOnly ? 0 : DebugValue::FLAG_ALLOW_TYPE_CHANGE;
        showMetatable = false;     // don't show the metatable
    } break;
    case LUA_TNUMBER:
    {
        value.m_value = OSString::format("%f", lua_tonumber(l, -1));
        value.m_flags |= isReadOnly ? 0 : DebugValue::FLAG_ALLOW_TYPE_CHANGE;
    } break;
    case LUA_TBOOLEAN:
    {
        value.m_value = (lua_toboolean(l, -1) == 1) ? "true" : "false";
        value.m_flags |= isReadOnly ? 0 :  DebugValue::FLAG_ALLOW_TYPE_CHANGE;
    } break;
    case LUA_TTABLE:
    {
        value.m_value = "{...}";
        value.m_flags |= isReadOnly ? 0 : DebugValue::FLAG_ALLOW_TYPE_CHANGE;
        value.m_flags |= DebugValue::FLAG_READ_ONLY;     // we can't modify the table itself
        const void* tableId = lua_topointer(l, -1);
        if (AZStd::find(tablesVisited.begin(), tablesVisited.end(), tableId) == tablesVisited.end())
        {
            tablesVisited.push_back(tableId);
            // traverse table elements
            lua_pushnil(l);
            while (lua_next(l, -2))
            {
                DebugValue& subValue = value.m_elements.emplace_back();
                if (lua_type(l, -2) == LUA_TSTRING)
                {
                    subValue.m_name = lua_tostring(l, -2);
                }
                else
                {
                    subValue.m_name = OSString::format("[%lld]", lua_tointeger(l, -2));
                }
                ReadValue(subValue, tablesVisited, isReadOnly);
                lua_pop(l, 1);    // pop the value and leave the key for next iteration
            }
        }
    } break;
    case LUA_TLIGHTUSERDATA:
    case LUA_TTHREAD:
    case LUA_TFUNCTION:
    {
        value.m_value = OSString::format("%p", lua_topointer(l, -1));
        value.m_flags |= DebugValue::FLAG_READ_ONLY;
    } break;
    case LUA_TUSERDATA: // get the pointer to our class
    {
        // Skip if this is not our object
        bool hasClassType = false;
        if (lua_getmetatable(l, -1) != 0)
        {
            lua_rawgeti(l, -1, AZ_LUA_CLASS_METATABLE_TYPE_INDEX);
            if (lua_islightuserdata(l, -1))  // check if we have a type for this class
            {
                hasClassType = true;
            }
            lua_pop(l, 2); // pop the type id and the metatable
        }
        if (!hasClassType)
        {
            return;
        }

        void* objectAddress;
        const BehaviorClass* behaviorClass;
        value.m_flags |= DebugValue::FLAG_READ_ONLY;

        if (Internal::LuaGetClassInfo(l,-1,&objectAddress,&behaviorClass) )
        {
            showMetatable = false;     // don't show the metatable

            // Attempt to call tostring if possible
            if (luaL_callmeta(l, -1, "__tostring"))
            {
                value.m_value = lua_tostring(l, -1);
                lua_pop(l, 1);
            }
            else
            {
                value.m_value = OSString::format("%p", objectAddress);
            }

            value.m_typeId = behaviorClass->m_typeId;
            // find the table and traverse all elements
            lua_rawgeti(l, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF);   // load the class table
            lua_pushlightuserdata(l, (void*)value.m_typeId.GetHash());     // add the key for the look up
            lua_rawget(l, -2);    // load the value for this key
            if (!lua_istable(l, -1))
            {
                lua_pop(l, 2);    // pop the unknown value and the class table
                break;
            }
            lua_pushnil(l);
            while (lua_next(l, -2))
            {
                if (lua_isstring(l, -2) && lua_isfunction(l, -1))
                {
                    const char* subValueName = lua_tostring(l, -2);
                    if (lua_tocfunction(l, -1) == &Internal::LuaPropertyTagHelper)     // if it's a property
                    {
                        DebugValue& subValue = value.m_elements.emplace_back();
                        subValue.m_name = subValueName;

                        //bool isRead = true;
                        bool isWrite = true;
                        lua_getupvalue(l, -1, 1);
                        if (lua_isnil(l, -1))
                        {
                            //isRead = false;
                            // write only value
                            value.m_elements.pop_back();
                        }
                        else if (lua_iscfunction(l, -1))
                        {
                            // call the function
                            lua_pushvalue(l, -6);    // copy the user data to be passed as a this pointer.
                            lua_call(l, 1, 1);   // call a function with one argument (this pointer) and 1 result
                            ReadValue(subValue, tablesVisited, isReadOnly);
                        }
                        lua_pop(l, 1);    // pop the up value (function) or the function call result

                        lua_getupvalue(l, -1, 2);
                        if (lua_isnil(l, -1))
                        {
                            isWrite = false;
                        }
                        lua_pop(l, 1);
                        if (!isWrite)
                        {
                            subValue.m_flags |= DebugValue::FLAG_READ_ONLY;     // it's ready only
                        }
                    }
                }
                lua_pop(l, 1);    // pop the value and leave the key for next iteration
            }

            lua_pop(l, 2);    // type table and the class table
        }
        else
        {
            value.m_value = OSString::format("%p", lua_topointer(l, -1));
        }
    } break;
    case LUA_TNIL:
    {
        value.m_value = "nil";
        //value.m_flags |= DebugValue::FLAG_ALLOW_TYPE_CHANGE;
    } break;
    case LUA_TNONE:
    default:
        value.m_value = "none";
        value.m_flags |= DebugValue::FLAG_READ_ONLY;
    }

    // add the metatable if we have one
    if (showMetatable && lua_getmetatable(l, -1))
    {
        DebugValue& subValue = value.m_elements.emplace_back();
        subValue.m_flags = DebugValue::FLAG_READ_ONLY; // we should NOT allow any modification of the metatable field.
        subValue.m_name = "__metatable__";
        ReadValue(subValue, tablesVisited, true);
        lua_pop(l, 1); // pop the metatable
    }
}

//=========================================================================
// WriteValue
// [8/16/2012]
//=========================================================================
void
ScriptContextDebug::WriteValue(const DebugValue& value, const char* valueName, int localIndex, int tableIndex, int userDataStackIndex)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    lua_State* l = m_context.NativeContext();
    int valueTableIndex = -1;
    if (valueName[0] == '[')
    {
        valueTableIndex = static_cast<int>(strtol(valueName + 1, nullptr, 10));
    }
    if (strcmp(valueName, "__metatable__") == 0) // metatable are read only
    {
        return;
    }
    if (strcmp(valueName, "__mode") == 0) // we can't change the mode of tables
    {
        return;
    }

    switch (value.m_type)
    {
    case LUA_TSTRING:
    {
        lua_pushstring(l, value.m_value.c_str());
        if (localIndex != -1)
        {
            lua_setlocal(l, m_luaDebug, localIndex);
        }
        else if (valueTableIndex != -1)
        {
            lua_rawseti(l, tableIndex, valueTableIndex);
        }
        else if (tableIndex != -1)
        {
            lua_setfield(l, tableIndex, valueName);
        }
        else
        {
            lua_setglobal(l, valueName);
        }
    } break;
    case LUA_TNUMBER:
    {
        lua_pushnumber(l, static_cast<lua_Number>(strtod(value.m_value.c_str(), nullptr)));
        if (localIndex != -1)
        {
            lua_setlocal(l, m_luaDebug, localIndex);
        }
        else if (valueTableIndex != -1)
        {
            lua_rawseti(l, tableIndex, valueTableIndex);
        }
        else if (tableIndex != -1)
        {
            lua_setfield(l, tableIndex, valueName);
        }
        else
        {
            lua_setglobal(l, valueName);
        }
    } break;
    case LUA_TBOOLEAN:
    {
        lua_pushboolean(l, azstricmp(value.m_value.c_str(), "true") == 0 ? 1 : 0);
        if (localIndex != -1)
        {
            lua_setlocal(l, m_luaDebug, localIndex);
        }
        else if (valueTableIndex != -1)
        {
            lua_rawseti(l, tableIndex, valueTableIndex);
        }
        else if (tableIndex != -1)
        {
            lua_setfield(l, tableIndex, valueName);
        }
        else
        {
            lua_setglobal(l, valueName);
        }
    } break;
    case LUA_TTABLE:
    {
        // load the table
        if (localIndex != -1)
        {
            lua_getlocal(l, m_luaDebug, localIndex);
        }
        else if (valueTableIndex != -1)
        {
            lua_rawgeti(l, tableIndex, valueTableIndex);
        }
        else if (tableIndex != -1)
        {
            lua_getfield(l, tableIndex, valueName);
        }
        else
        {
            lua_getglobal(l, valueName);
        }
        AZ_Assert(userDataStackIndex == -1, "We should not have tables and user data mixed. If we need we can add a case for that!");

        // should be check if the value is a table or we should just allow reassign
        int newTableIndex = lua_gettop(l);

        for (size_t i = 0; i < value.m_elements.size(); ++i)
        {
            WriteValue(value.m_elements[i], value.m_elements[i].m_name.c_str(), -1, newTableIndex, -1);
        }
        lua_pop(l, 1);    // pop the table value
    } break;
    case LUA_TLIGHTUSERDATA:
    case LUA_TTHREAD:
    case LUA_TFUNCTION:
    {
        // if we set a table it's ok to try to set a read only value.
        AZ_Warning("Script", tableIndex != -1, "We can't set read only values!");
    } break;
    case LUA_TUSERDATA: // get the pointer to our class
    {
        // user data
        if (localIndex != -1)
        {
            lua_getlocal(l, m_luaDebug, localIndex);
        }
        else if (valueTableIndex != -1)
        {
            lua_rawgeti(l, tableIndex, valueTableIndex);
        }
        else if (tableIndex != -1)
        {
            lua_getfield(l, tableIndex, valueName);
        }
        else if (userDataStackIndex != -1)
        {
            lua_pushvalue(l, userDataStackIndex);                                    // user data is on the stack already
        }
        else
        {
            lua_getglobal(l, valueName);
        }

        if (lua_isuserdata(l, -1))
        {
            if (Internal::LuaIsClass(l, -1))
            {
                lua_rawgeti(l, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF);   // load the class table
                lua_pushlightuserdata(l, (void*)value.m_typeId.GetHash());    // add the key for the look up
                lua_rawget(l, -2);    // load the value for this key
                if (lua_istable(l, -1))
                {
                    //lua_getfield(l,-1,)
                    for (size_t i = 0; i < value.m_elements.size(); ++i)
                    {
                        const DebugValue& subElement = value.m_elements[i];
                        lua_pushstring(l, subElement.m_name.c_str());
                        lua_rawget(l, -2);    // get element

                        if (lua_isfunction(l, -1) && lua_tocfunction(l, -1) == &Internal::LuaPropertyTagHelper)
                        {
                            // call the setter with the value
                            switch (subElement.m_type)
                            {
                            case LUA_TSTRING:
                                lua_getupvalue(l, -1, 2);
                                if (lua_isnil(l, -1))
                                {
                                    lua_pop(l, 1);
                                }
                                else
                                {
                                    lua_pushvalue(l, -5);    // copy the user data (this pointer)
                                    lua_pushstring(l, subElement.m_value.c_str());
                                    lua_call(l, 2, 0);   // call the setter
                                }
                                break;
                            case LUA_TNUMBER:
                                {
                                    lua_getupvalue(l, -1, 2);
                                    if (lua_isnil(l, -1))
                                    {
                                        lua_pop(l, 1);
                                    }
                                    else
                                    {
                                        lua_pushvalue(l, -5);    // copy the user data (this pointer)
                                        lua_pushnumber(l, static_cast<lua_Number>(strtod(subElement.m_value.c_str(), nullptr)));
                                        lua_call(l, 2, 0);   // call the setter
                                    }
                                }
                                break;
                            case LUA_TBOOLEAN:
                                lua_getupvalue(l, -1, 2);
                                if (lua_isnil(l, -1))
                                {
                                    lua_pop(l, 1);
                                }
                                else
                                {
                                    lua_pushvalue(l, -5);    // copy the user data (this pointer)
                                    lua_pushboolean(l, azstricmp(subElement.m_value.c_str(), "true") == 0 ? 1 : 0);
                                    lua_call(l, 2, 0);   // call the setter
                                }
                                break;
                            case LUA_TUSERDATA:
                                // call the getter
                                lua_getupvalue(l, -1, 1);     // load the getter
                                if (lua_isnil(l, -1))
                                {
                                    lua_pop(l, 1);
                                }
                                else
                                {
                                    lua_pushvalue(l, -5);    // copy the user data (this pointer)
                                    lua_call(l, 1, 1);   // call the getter
                                    if (!lua_isnil(l, -1))
                                    {
                                        WriteValue(subElement, subElement.m_name.c_str(), -1, -1, lua_gettop(l));
                                    }
                                    lua_pop(l, 1);
                                }
                                break;
                            default:
                                AZ_Warning("Script", false, "This type is NOT supported for user types!");
                            }
                        }

                        lua_pop(l, 1);    // pop the element
                    }
                }
                lua_pop(l, 2);    // pop the value and the class table
            }
            else
            {
                AZ_Warning("Script", false, "We can't set read only values!");
            }
        }
        else
        {
            AZ_Warning("Script", false, "We change this type %d to user data!", lua_type(l, -1));
        }
        lua_pop(l, 1);    // userdata
    } break;
    case LUA_TNIL:
    {
        lua_pushnil(l);
        if (localIndex != -1)
        {
            lua_setlocal(l, m_luaDebug, localIndex);
        }
        else if (valueTableIndex != -1)
        {
            lua_rawseti(l, tableIndex, valueTableIndex);
        }
        else if (tableIndex != -1)
        {
            lua_setfield(l, tableIndex, valueName);
        }
        else
        {
            lua_setglobal(l, valueName);
        }
    } break;
    default:
        // if we set a table it's ok to try to set a read only value.
        AZ_Warning("Script", tableIndex != -1, "Can't store this value type!");
    }
}

//=========================================================================
// GetValue
// [8/15/2012]
//=========================================================================
bool
ScriptContextDebug::GetValue(DebugValue& value)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    value.m_elements.clear();

    AZStd::vector<AZ::OSString> tokens;
    AZStd::tokenize<AZ::OSString>(value.m_name, ".[] ", tokens);

    if (tokens.empty())
    {
        return false;
    }

    const char* name = tokens.front().c_str();

    bool isCheckLocals = true;
    bool isFoundLocal = false;
    if (name[0] == ':')
    {
        name++;
        isCheckLocals = false;
        if (name[0] == 0)
        {
            return false;
        }
    }

    lua_State* l = m_context.NativeContext();

    if (isCheckLocals && m_luaDebug)
    {
        int iLocal = 1;
        const char* localName;
        while ((localName = lua_getlocal(l, m_luaDebug, iLocal)) != nullptr)
        {
            if (localName[0] != '(' && strcmp(name, localName) == 0)
            {
                isFoundLocal = true;
                break; // leave the local on the stack
            }
            else
            {
                lua_pop(l, 1);
            }
            ++iLocal;
        }
    }

    if (!isFoundLocal)
    {
        // if we found the value call the full get function (with metamethods so we can load properties)
        // since we return true or false overwrite the error to catch if variable doesn't exist
        ScriptErrorCatcher errorHandler(&m_context);
        lua_getglobal(l, name);
        if (errorHandler.m_numErrors)
        {
            lua_pop(l, 1);
            return false;
        }
    }

    // Erase first element, so that all that's left is sub-elements
    tokens.erase(tokens.begin());

    for (const auto& token : tokens)
    {
        // Attempt to convert token to a number
        long number = strtol(token.c_str(), nullptr, 10);
        if (number != 0 || strncmp("0", token.c_str(), 1) == 0)
        {
            lua_pushnumber(l, static_cast<lua_Number>(number));
        }
        else
        {
            lua_pushlstring(l, token.c_str(), token.length());
        }

        // Attempt to read sub-element from table/userdata
        lua_gettable(l, -2); // Specifically not lua_rawget so that __index methods still work
        if (lua_isnil(l, -1))
        {
            lua_pop(l, 2);
            return false;
        }
        else
        {
            // Remove original value, ensuring the only value on the stack is the current element
            lua_remove(l, -2);
        }
    }

    VoidPtrArray tablesVisited; ///< Keep a list of visited tables to allow circular table dependencies

    ReadValue(value, tablesVisited);

    lua_pop(l, 1); // pop the value from the stack

    return true;
}

//=========================================================================
// SetValue
// [8/15/2012]
//=========================================================================
bool
ScriptContextDebug::SetValue(const DebugValue& sourceValue)
{
    // LUA Must execute in the "C" Locale.
    AZ::Locale::ScopedSerializationLocale scopedLocale;

    AZStd::vector<AZ::OSString> tokens;
    AZStd::tokenize<AZ::OSString>(sourceValue.m_name, ".[] ", tokens);

    if (tokens.empty())
    {
        return false;
    }

    // create hierarchy from tokens
    const DebugValue* value = &sourceValue;
    DebugValue untokenizedValue;

    if (tokens.size() > 1)
    {
        untokenizedValue.m_name = tokens[0];
        untokenizedValue.m_type = LUA_TTABLE;

        // Erase first element, so that all that's left is sub-elements
        tokens.erase(tokens.begin());

        // expand the tokens into the hierarchy
        DebugValue* current = &untokenizedValue;

        for (AZ::OSString& token : tokens)
        {
            current = &(current->m_elements.emplace_back());
            current->m_name = token;
            current->m_type = LUA_TTABLE;
        }

        current->m_type = sourceValue.m_type;
        current->m_value = sourceValue.m_value;

        // If any hierarchy was passed in with the sourceValue, make sure it is accounted for
        // in the last element of our new hierarchy
        if (sourceValue.m_elements.size() > 0)
        {
            current->m_elements = sourceValue.m_elements;
        }

        value = &untokenizedValue;
    }

    const char* name = value->m_name.c_str();
    size_t nameLen = value->m_name.length();
    if (nameLen == 0)
    {
        return false;
    }

    bool isCheckLocals = true;
    int localIndex = -1;
    if (name[0] == ':')
    {
        name++;
        nameLen--;
        isCheckLocals = false;
        if (nameLen == 0)
        {
            return false;
        }
    }

    lua_State* l = m_context.NativeContext();

    if (isCheckLocals && m_luaDebug)
    {
        int iLocal = 1;
        const char* localName;
        while ((localName = lua_getlocal(l, m_luaDebug, iLocal)) != nullptr)
        {
            lua_pop(l, 1);
            if (localName[0] != '(' && strcmp(name, localName) == 0)
            {
                localIndex = iLocal;
                break; // leave the local on the stack
            }
            ++iLocal;
        }
    }

    WriteValue(*value, name, localIndex, -1, -1);

    return true;
}

} // namespace AZ

#endif // #if !defined(AZCORE_EXCLUDE_LUA)
