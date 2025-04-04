/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Memory/OSAllocator.h>

namespace AZ
{
    namespace Debug
    {
        struct StackFrame;
    }

    /**
     * Debug functionality for a script context (VM).
     * This context will be generated automatically once you enable the Debugging in the \ref ScriptContext.
     * Use this class to obtain registered classes, better callstacks, breakpoints, etc.
     */
    class ScriptContextDebug
    {
        ScriptContextDebug(const ScriptContextDebug&);
        ScriptContextDebug operator=(const ScriptContextDebug&);

    public:
        // TODO: this is suboptimal, ideally we want to use the UUID or a hash
        // however until we support the asset database we need to leverage
        // path information.
        typedef AZStd::string BreakpointId;

        AZ_CLASS_ALLOCATOR(ScriptContextDebug, SystemAllocator);

        ScriptContextDebug(ScriptContext& scriptContext, bool isEnableStackRecord = false);
        ~ScriptContextDebug();

        // print functions return true to continue and false to stop enum.
        typedef bool(*EnumClass)(const char* /* class Name*/, const AZ::Uuid& /* class TypeId */, void* /* userdata */);
        typedef bool(*EnumMethod)(const AZ::Uuid* /* class TypeId or null if global*/, const char* /* method name */, const char* /* debug argument info (can be null) */, void* /* user data */);
        typedef bool(*EnumProperty)(const AZ::Uuid* /* class TypeId or null if global*/, const char* /* property Name*/, bool /* can read property */, bool /* can write property */, void* /* user data */);
        typedef bool(*EnumEBus)(const AZStd::string& /*ebus name*/, bool /* canBroadcast */, bool /* canQueue */, bool /* hasHandler  */, void* /* userdata */);
        typedef bool(*EnumEBusSender)(const AZStd::string& /*ebus*/, const AZStd::string& /*Sender Name*/, const AZStd::string& /*debug argument info*/, const AZStd::string& category, void* /*user data*/);

        // Enum registered classes, this is enabled even in final builds
        void                EnumRegisteredClasses(EnumClass enumClass, EnumMethod enumMethod, EnumProperty enumProperty, void* userData = NULL);
        // Enum registered global properties and methods, this is enabled even in final builds
        void                EnumRegisteredGlobals(EnumMethod enumMethod, EnumProperty enumProperty, void* userData = NULL);
        // Enum registered ebusses including senders and handlers, this is enabled even in final builds
        void                EnumRegisteredEBuses(EnumEBus enumEBus, EnumEBusSender enumEBusSender, void* userData = NULL);
        // Prints a call-stack in a string buffer
        void                StackTrace(char* stackOutput, size_t stackOutputSize);

        struct Breakpoint
        {
            Breakpoint()
                : m_lineNumber(-1)
                , m_count(-1)    {}
            OSString        m_sourceName;
            int             m_lineNumber;
            OSString        m_expression;
            int             m_count;
        };

        typedef AZStd::function<void (ScriptContextDebug* /* debugContext*/, const Breakpoint* /*breakpoint*/)> BreakpointCallback;
        void                EnableBreakpoints(BreakpointCallback& cb);
        void                DisableBreakpoints();
        void                AddBreakpoint(Breakpoint& bp);
        void                RemoveBreakpoint(Breakpoint& bp);

        // A debug context may require a debugging hook to be connected
        void ConnectHook();

        // Disconnect the debugging hook
        void DisconnectHook();

        /**
         *
         */
        typedef AZStd::function<void (ScriptContextDebug* /* debugContext*/)> ProcessDebugCmdCallback;
        /// Register a callback that will be called when "ProcessDebugCommands" is called.
        void                EnableDebugCmdProcess(ProcessDebugCmdCallback& cb)  { m_processDebugCmdCallback = cb; }
        void                DisableDebugCmdProcess()                            { m_processDebugCmdCallback = NULL; }
        /**
         * You MUST can this function from a safe context (when a script is not running) to allow the debugger to execute actions on the script.
         * You are not required to call this, but then you will be able to debug only from within "break points" callback.
         */
        inline void         ProcessDebugCommands()
        {
            if (m_processDebugCmdCallback)
            {
                m_processDebugCmdCallback(this);
            }
        }

        /// Enable detailed stack recording (more detailed than the default Script one), of course this will cause a performance penalty.
        void                EnableStackRecord()     { m_isRecordCallstack = true; }
        void                DisableStackRecord()    { m_isRecordCallstack = false; m_callstack.clear(); }
        /// Enable C++ stack recoding (this can be slow on certain platforms and configurations). It's off by default
        void                EnableCodeStackRecord() { m_isRecordCodeCallstack = true; }
        void                DisableCodeStackRecord(){ m_isRecordCodeCallstack = false; }

        typedef AZStd::function<bool (const char* /*name*/, ScriptDataContext& /*dataContext*/)> EnumLocalCallback;

        /// Enumerate locals variables. This can be called only from a breakpoint callback.
        void                EnumLocals(EnumLocalCallback& cb);
        /// Step into current instruction. This can be called only from a breakpoint callback and you must return from it (until in breaks again)
        void                StepInto();
        /// Step over current instruction. This can be called only from a breakpoint callback and you must return from it (until in breaks again)
        void                StepOver();
        /// Step out current instruction. This can be called only from a breakpoint callback and you must return from it (until in breaks again)
        void                StepOut();
        // @}

        // @{ Detailed callstack information - available only when debugging is enabled
        struct CallstackLine
        {
            CallstackLine()
                : m_sourceName(0)
                , m_functionName(0)
                , m_functionType(0)
                , m_lineCalled(0)
                , m_lineDefined(0)
                , m_codeStackFrames(0)
                , m_codeNumStackFrames(0) {}
            const char*             m_sourceName;   ///< Source name, if it starts with @ it's a file name.
            OSString                m_parameters;   ///< Function parameters as strings.
            const char*             m_functionName; ///< Function name or NULL if nothing suitable is found.
            const char*             m_functionType; ///< "Lua", "C", "main" or "tail".
            int                     m_lineCalled;   ///< Line where we are currently executing.
            int                     m_lineDefined;  ///< Line where the current function is defined.
            AZ::Debug::StackFrame*  m_codeStackFrames;
            int                     m_codeNumStackFrames;
        };
        typedef AZStd::vector<CallstackLine, OSStdAllocator> CallstackInfo;

        const CallstackInfo& GetCallstack() const   { return m_callstack; }
        // @}

        struct DebugValue
        {
            AZ_TYPE_INFO(DebugValue, "{c32d1e88-2b8b-432c-91bc-d0b4b135279d}");
            enum Flags
            {
                FLAG_READ_ONLY          = (1 << 0),
                FLAG_ALLOW_TYPE_CHANGE  = (1 << 1),
            };

            DebugValue()
                : m_type(-1)
                , m_flags(0) {}
            OSString        m_name;
            OSString        m_value;
            char            m_type;                 ///< Script internal type.
            //AZStd::type_id    m_typeId;               ///< Valid for AZ classes only.
            ScriptTypeId       m_typeId;
            unsigned char   m_flags;
            typedef AZStd::vector<DebugValue, OSStdAllocator> DebugValueArray;
            DebugValueArray m_elements;
        };

        /**
         * Get a DebugValue (if found) for a specific script value. You can use that information to display in debugger and
         * edit if possible (not all types are editable).
         * When the function is called we will check for a local variable with that name, then a global name. You can prefix with
         * a special symbol ':' to check global only.
         * value should have m_name set to the name we are trying to get
         * \returns true if value structure was filled with information, false value can't be found.
         */
        bool        GetValue(DebugValue& value);
        /**
         * Allows you to change a value in the script context. Function will return true is the change was successful otherwise false.
         * Not all types can be set, not all type can be changed at runtime (it depends on the script language and script binding).
         */
        bool        SetValue(const DebugValue& value);

    protected:
        friend ScriptContextImpl;
        void        PushCodeCallstack(int stackSuppressCount = 1, int numStackLevels = 10);
        void        PopCallstack();

    private:
        typedef AZStd::unordered_map<BreakpointId, Breakpoint, AZStd::hash<BreakpointId>, AZStd::equal_to<BreakpointId>, OSStdAllocator> BreakpointsMap;
        typedef AZStd::vector<const void*, OSStdAllocator> VoidPtrArray;

        /// Converts a value (in the current value list - LUA stack in lua's case).
        OSString            DbgValueToString(int valueIndex);

        void                ReadValue(DebugValue& value, VoidPtrArray& tablesVisited, bool isReadOnly = false);
        void                WriteValue(const DebugValue& value, const char* valueName, int localIndex, int tableIndex, int userDataStackIndex);

#ifndef AZ_USE_CUSTOM_SCRIPT_BIND
        // TODO derive from the base class
        friend void LuaHook(lua_State*, lua_Debug*);
        lua_Debug*          m_luaDebug;
#endif
        int                     m_currentStackLevel;    ///< Current stack level 0..N
        int                     m_stepStackLevel;       ///< -1 if no step event, otherwise a stack level where we should stop.
        BreakpointsMap          m_breakpoints;
        BreakpointCallback      m_breakCallback;
        ProcessDebugCmdCallback m_processDebugCmdCallback;
        CallstackInfo           m_callstack;
        bool                    m_isRecordCallstack;    ///< True if we want to record the detailed callstack, otherwise false.
        bool                    m_isRecordCodeCallstack;///< True if we want to record the code callstack, otherwise false. (default is false as it can be slow on many platforms)
        ScriptContext&       m_context;
    };
}
