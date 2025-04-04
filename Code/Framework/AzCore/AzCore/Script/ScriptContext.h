/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_const.h>
#include <AzCore/std/typetraits/is_function.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_abstract.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/any.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/parallel/thread.h>

#include <AzCore/RTTI/ReflectContext.h>

#ifndef AZ_USE_CUSTOM_SCRIPT_BIND
struct lua_State;
struct lua_Debug;
#endif // AZ_USE_CUSTOM_SCRIPT_BIND

// Uncomment to validate lua stack per function call (WARNING: EXPENSIVE)
//#define AZ_LUA_VALIDATE_STACK

// Uncomment to throw Lua errors on unacceptable number conversions from Lua to C++ numerical targets
//#AZ_LUA_RESTRICT_NUMBER_CONVERSIONS_TO_CPP

// Always validate stacks when running tests
#if defined(AZ_TESTS_ENABLED)
#define AZ_LUA_VALIDATE_STACK
#define AZ_LUA_RESTRICT_NUMBER_CONVERSIONS_TO_CPP
#endif

#if defined(AZ_LUA_VALIDATE_STACK)
// LuaStackValidator
// Used for validating that the stack is left in the expected state after a function call
// Only included in debug builds
namespace AzLsvInternal
{
    // Struct used for keeping track of required return values
    struct LuaStackValidator
    {
        // Constructor for when return count will be set later (variable return count)
        LuaStackValidator(lua_State* lua);

        // Constructor for when return count is known ahead of time
        LuaStackValidator(lua_State* lua, int expectedStackChange);

        // Used for setting return count later
        void SetExpectedStackChange(int expectedReturnCount);

        // Used on function exit to validate stack size
        ~LuaStackValidator();

    private:
        lua_State* m_lua = nullptr;
        bool m_returnCountSet = false;
        int m_expectedStackChange = 0;
        int m_startStackSize = 0;
    };
}

// Call at beginning of function to initialize stack validator
#define LSV_BEGIN(L, expectedStackChange) ::AzLsvInternal::LuaStackValidator _LSV_stackValidator(L, expectedStackChange)

// Call at beginning of function to initialize stack validator (must call LSV_END_VARIABLE before returning!)
#define LSV_BEGIN_VARIABLE(L) ::AzLsvInternal::LuaStackValidator _LSV_stackValidator(L)

// Call at end of function (just before returning) to validate stack changed as expected
#define LSV_END_VARIABLE(expectedStackChange) _LSV_stackValidator.SetExpectedStackChange(expectedStackChange)

#else
#define LSV_BEGIN(L, expectedStackChange) (void)L; (void)expectedStackChange
#define LSV_BEGIN_VARIABLE(L) (void)L
#define LSV_END_VARIABLE(expectedStackChange) (void)(expectedStackChange)
#endif // AZ_LUA_VALIDATE_STACK

namespace AZ
{
    class BehaviorClass;
    class BehaviorContext;
    class ScriptProperty;
    class ScriptContext;
    class ScriptContextDebug;
    struct BehaviorParameter;
    struct BehaviorArgument;

    namespace IO
    {
        class GenericStream;
    }


    enum class AcquisitionOnPush
    {
        None,
        ScriptAcquire
    };

    enum class ObjectToLua
    {
        ByReference,
        ByValue
    };

    // Structure is padded to 16 byes so we can 16 byte alignments (assuming the underlaying allocator is already doing that)
    struct LuaUserData
    {
        u32 magicData;
        u32 storageType;
        u64 m_pad;
        void* value;
        BehaviorClass* behaviorClass;
    };

    /**
     * A generic interface to access the script data. Internally it has 3 modes:
     * During a script call to a C function, it's called 'callee mode' and you are allowed to
     * use the functions in \ref CalleeMode "Callee Mode".
     * When you are about to call a function from C to script \ref CallerMode "caller mode".
     * And finally when you inspect a script (traverse,cache,change or call) from C you can use \ref InspectMode "Inspect Mode" functionality.
     * All of these modes are set internally depending on what you are doing with the context and they will error to prevent you
     * from calling the wrong functions in the wrong mode. You are not allowed to create or destroy the ScriptDataContext, this is why
     * the constructor and destructor are private.
     */
    class ScriptDataContext
    {
        friend ScriptContext;
        friend ScriptContextDebug;
        friend class LuaGenericCaller;
        friend class LuaEBusHandler;

        enum Modes
        {
            MD_INVALID,     // invalid mode
            MD_CALLER,      // allow to push arguments and read results
            MD_CALLEE,      // allow to push results and read arguments
            MD_INSPECT,     // allow to access next element function
            MD_CALLER_EXECUTED, // was in caller mode and executed the call
            MD_READ_STACK   // used to read a value from the stack, and not pop off results.
        };

        lua_State* m_nativeContext;
        int m_numArguments;
        int m_numResults;
        int m_startVariableIndex;
        Modes m_mode;

        ScriptDataContext(lua_State* c, int numArguments, int startStackIndex)
            : m_nativeContext(c)
            , m_numArguments(numArguments)
            , m_numResults(0)
            , m_startVariableIndex(startStackIndex)
            , m_mode(MD_CALLEE)
        {
        }

        void Reset();

        /// Assumes that the called function is on the top of the stack. It will be poped when the context is destroyed or Reset
        void SetCaller(lua_State* c, int startStackIndex)
        {
            m_nativeContext = c;
            m_mode = MD_CALLER;
            m_startVariableIndex = startStackIndex;
        }

        void SetInspect(lua_State* c, int startStackIndex)
        {
            m_nativeContext = c;
            m_mode = MD_INSPECT;
            m_startVariableIndex = startStackIndex;
        }

        void ReadStack(lua_State* c)
        {
            m_nativeContext = c;
            m_mode = MD_READ_STACK;
            m_startVariableIndex = 0;
        }

    public:
        // DesignNote: Would probably add a new public method here called
        // ReadStack(lua_State* c, int startStackIndex)
        // or something similar that will basically mimic inspect, but not destructively pop things

        /// Assumes that the inspected value is on the top of the stack. It will be poped when the context is destroyed or Reset.

        AZ_TYPE_INFO(ScriptDataContext, "{7ec9e09e-6559-49bd-8826-5eef8880d970}");

        ScriptDataContext()
            : m_nativeContext(NULL)
            , m_numArguments(0)
            , m_numResults(0)
            , m_startVariableIndex(0)
            , m_mode(MD_INVALID)
        {}

        ~ScriptDataContext() { Reset(); }

        //! Retrieve a string representing the current version of the interpreter.
        //! Example of use: To signal incompatibility with previously emitted bytecode, to invalidate
        static const char* GetInterpreterVersion();

        ScriptContext* GetScriptContext() const;
        lua_State* GetNativeContext() const { return m_nativeContext; }

        /// returns true for script or C function (bound in script).
        bool    IsFunction(int index) const;
        /// returns true for C functions only.
        bool    IsCFunction(int index) const;
        bool    IsNumber(int index, bool aceptNumberStrings = false) const;
        bool    IsString(int index, bool acceptNumbers = false) const;
        bool    IsBoolean(int index) const;
        bool    IsNil(int index) const;
        bool    IsTable(int index) const;
        /// Check if this is a registered class with the system (context.Class<T>(...)). This is a test if the userdata has metable assigned.
        bool    IsRegisteredClass(int index) const;
        /* Check if we hold registered class instance (we an own it).
         * By registered we mean a type that was exposed/bound to script via script.Class<A...> interface.
         */
        template<class T>
        bool    IsClass(int index) const;

        /// Cached a value, using a weak reference, by index (instead of a hash search)
        int     CacheValue(int index);
        /// Release a cached value (so it can be garbage collected)
        void    ReleaseCached(int cachedIndex);

        /// Script will release ownership and the C++ runtime we will responsible to release the object
        bool    AcquireOwnership(int index, bool isNullPointer) const;
        /// Script will take ownership of the object, C++ runtime will NOT release it
        bool    ReleaseOwnership(int index) const;

        /**
         * \anchor CallerMode
         * \name Caller Mode
         * @{
         */
        /// Push argument for the script call.
        template<class T>
        void    PushArg(const T& value);
        template<class Arg, class... Args>
        void    PushArgs(Arg& value, Args&&... args);

        void    PushArgs() {}
        /// Push value from the lua registery for the script call.
        void    PushArgFromRegistryIndex(int cachedIndex);
        /// Push cached value for the script call.
        void    PushArgCached(int cachedIndex);
        void    PushArgScriptProperty(AZ::ScriptProperty* scriptProperty);
        /// Get number of results from a script call (ExecuteScriptCall must be called)
        int     GetNumResults() const   { return m_numResults; }
        /// Read result from a script call [0,GetNumResults()). Returns true if successful otherwise false.
        template<class T>
        bool    ReadResult(int index, T& valueRef) const;
        //@}

        /**
         * \anchor CalleeMode
         * \name Callee Mode
         * @{
         */
        /// Get number of arguments passed for you.
        int     GetNumArguments() const { return m_numArguments; }
        /// Read argument from a script [0,GetNumArguments). Returns true if successful otherwise false.
        template<class T>
        bool    ReadArg(int index, T& valueRef) const;
        /// Push result for the script.
        template<class T>
        void    PushResult(const T& value);
        /// Push result from a cache value
        void    PushResultCached(int cachedIndex);
        // @}

        /**
         * \anchor InspectMode
         * \name Inspect Mode
         * @{
         */
        template<class T>
        bool    ReadValue(int index, T& valueRef) const;
        template<class T>
        // Read table element with 'name'. Returns true of successful and fast if tableIndex is not a table, name doesn't exists or type doesn't match.
        bool    ReadTableElement(int tableIndex, const char* name, T& value);
        template<class T>
        void    AddReplaceTableElement(int tableIndex, const char* name, const T& value);
        /// Check if table element with 'name' exists.
        bool    CheckTableElement(int tableIndex, const char* name);
        /// Check if table element with 'name' exists, assming the context points to a table.
        bool    CheckTableElement(const char* name);
        /// Load table element on the stack if 'name' exists.
        bool    PushTableElement(int tableIndex, const char* name, int* valueIndex = nullptr);
        /// Load table element on the stack if 'name' exists, assming the context points to a table.
        bool    PushTableElement(const char* name, int* valueIndex = nullptr);

        /// Construct a table element as a ScriptProperty, assuming the context points to a table
        ScriptProperty* ConstructTableScriptProperty(const char* name, bool restrictToPropertyArrays = false);

        /// Load a table element

        /// Inspect a table and return true if the table is a table and data context is set, otherwise false.
        bool    InspectTable(int tableIndex, ScriptDataContext& dc);
        /// Inspects a metatable to the value on the stack. If the value has no metatable we return false, otherwise true.
        bool    InspectMetaTable(int index, ScriptDataContext& dc);

        /**
         * Inspect next table element.
         * There are 2 types of table elements: 1. Elements with name (hashed named elements) and 2. Index elements.
         * When the function return true either "name" will be != NULL or index will be != -1.
         * \param name - name of the element, valid of we have a named element otherwise NULL.
         * \param index - index of the element, valid for indexed elements otherwise -1.
         * \param isProtectedElements - true if you want to inspect protected elements (in LUA for example they start with "__")
         * \return true if we loaded the next element on the stack, otherwise false if we are done traversing.
         */
        bool    InspectNextElement(int& valueIndex, const char*& name, int& index, bool isProtectedElements = false);

        /// Return context start index. For example if this context is get from "InspectTable" the start index is the address of the the table.
        int     GetStartIndex() const       { return m_startVariableIndex; }

        //// call function while inspecting a table, code MUST be consistent Start/Execute/End
        bool    Call(int functionIndex, ScriptDataContext& dc);
        bool    Call(int tableIndex, const char* functionName, ScriptDataContext& dc);
        bool    CallExecute();
        ////@}

        /// Converts the value at a given index into a ScriptProperty object.
        /// If restrictToPropertyArrays is true. Will only parse out to property arrays(i.e. an array of all numbers)
        /// If restrictToPropertyArarys is false. May return a table that can be of mixed values, instead of an array.
        AZ::ScriptProperty* ConstructScriptProperty(int index, const char* name=nullptr, bool restrictToPropertyArrays = false);
    };

    using ScriptTypeId = Uuid;
    using ScriptShortTypeId = size_t;

    namespace Internal
    {
        // Cache the value at the given index using a weak reference. Returns an index that can be used to access the value quicker.
        int LuaCacheValue(lua_State* lua, int index);
        int LuaCacheRegisteredClass(lua_State* lua, void* value, const AZ::Uuid& typeId);

        // Loads the value at the given cached index.
        void LuaLoadCached(lua_State* lua, int cachedIndex);

        // Released the value at the given cached index
        void LuaReleaseCached(lua_State* lua, int cachedIndex);

        bool LuaIsClass(lua_State* lua, int stackIndex, const AZ::Uuid* typeId = nullptr);
        bool LuaGetClassInfo(lua_State* lua, int stackIndex, void** valueAddress, const BehaviorClass** behaviorClass);
        bool LuaReleaseClassOwnership(lua_State* lua, int stackIndex, bool nullThePointer);
        bool LuaAcquireClassOwnership(lua_State* lua, int stackIndex);
        /// Returns the pointer to class if the typeId matches to the typeId of the object on the stack
        void* LuaClassFromStack(lua_State* lua, int stackIndex, const AZ::Uuid& typeId);
        /// Returns the pointer to the class and the type (optional) in typeId. It will return null if the element on the class is not a reflected class.
        void* LuaAnyClassFromStack(lua_State* lua, int stackIndex, AZ::Uuid* typeId);
        void LuaClassToStack(lua_State* lua, void* valueAddress, const AZ::Uuid& typeId, ObjectToLua toLua = ObjectToLua::ByReference, AcquisitionOnPush acquitionOnPush = AcquisitionOnPush::None);

        void LuaStackTrace(lua_State* l, char* stackOutput = nullptr, size_t stackOutputSize = 0);
        int LuaPropertyTagHelper(lua_State*);
        int LuaMethodTagHelper(lua_State*);

        // A drop-in replacement for lua_pcall that supplies an error handler.
        // Removes numParams + 1 (the function called) from the stack. If the call is successful, places numResults elements on the stack.
        bool LuaSafeCall(lua_State* lua, int numParams, int numResults);

        //////////////////////////////////////////////////////////////////////////
        // AzLUA wrappers so we don't need to pull lua directly, keep in mind this only
        // so we don't pull lua into script context (as lots code includes it)
        // If you want to work with LUA directly in specific places feel free to do so!

        /// raw setglobal function (no metamethods called)
        void azlua_setglobal(lua_State* _lua, const char* _name);
        ///  raw getglobal function (no metamethods called)
        void azlua_getglobal(lua_State* _lua, const char* _name);
        //
        void azlua_pushinteger(lua_State* lua, int value);
        int azlua_tointeger(lua_State* lua, int stackIndex);

        //
        bool azlua_istable(lua_State* lua, int stackIndex);
        bool azlua_isfunction(lua_State* lua, int stackIndex);
        bool azlua_isnil(lua_State* lua, int stackIndex);
        void azlua_pop(lua_State* lua, int numElements);
        void azlua_getfield(lua_State* lua, int stackIndex, const char* name);
        int azlua_gettop(lua_State* lua);
        void azlua_pushstring(lua_State* lua, const char* string);
        void azlua_pushvalue(lua_State* lua, int stackIndex);
        const char* azlua_tostring(lua_State* lua, int stackIndex);
        void azlua_rawset(lua_State* lua, int stackIndex);
        void LuaRegistrySet(lua_State* lua, int valueIndex);
        void LuaRegistryGet(lua_State* lua, int valueIndex);

        // type ID can be null for unregistered types, that way we will push it as a lighuserdata (pointer only)
        void LuaScriptValueStackPush(lua_State* lua, void* dataPointer, const AZ::Uuid typeId, AZ::ObjectToLua toLua = AZ::ObjectToLua::ByReference);
        void* LuaScriptValueStackRead(lua_State* lua, int stackIndex, const AZ::Uuid typeId);
    }

    //////////////////////////////////////////////////////////////////////////
    // LUA can process some native types double/float,int/short/char/unsigned...
    // and string as internal types. Those types are passed by VALUE.
    // C data types (user data) is always stored as userdata.
    template<class T, bool isEnum = AZStd::is_enum<T>::value>
    struct ScriptValueGeneric
    {
        typedef typename AZStd::remove_const< typename AZStd::remove_reference< typename AZStd::remove_pointer<T>::type >::type >::type ValueType;
        static const bool isNativeValueType = false;  //< This is a value type, but non native (it's a user class), we have specializations for native types
        static inline void StackPush(lua_State* l, const ValueType& value);
        static inline T StackRead(lua_State* l, int stackIndex);
    };

    template<class T>
    struct ScriptValueGeneric<T, true>
    {
        typedef typename AZStd::remove_const< typename AZStd::remove_reference< typename AZStd::remove_pointer<T>::type >::type >::type ValueType;
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static inline void StackPush(lua_State* l, T value)
        {
            AZ::Internal::azlua_tointeger(l, static_cast<int>(value)); // treat enums as ints
        }
        static inline T  StackRead(lua_State* l, int stackIndex)
        {
            return static_cast<T>(AZ::Internal::azlua_tointeger(l, stackIndex));
        }
    };

    /// Script value implementation for all value and reference types. The push Function will always create an object COPY, this will be slower than just using pointers.
    template<class T>
    struct ScriptValue
        : public ScriptValueGeneric<T>
    {
        typedef typename AZStd::remove_const< typename AZStd::remove_reference< typename AZStd::remove_pointer<T>::type >::type >::type ValueType;
    };

    /// Script value implementation for generic pointer types, always prefer pointer types.
    template<class T>
    struct ScriptValue<T*>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, T* pointer, const AZStd::true_type& /* AZ::Internal::HasAzTypeInfo<T>::value() */)
        {
            AZ::Internal::LuaScriptValueStackPush(l, pointer, AzTypeInfo<T>::Uuid());
        }
        static T*   StackRead(lua_State* l, int stackIndex, const AZStd::true_type& /* AZ::Internal::HasAzTypeInfo<T>::value() */)
        {
            return reinterpret_cast<T*>(AZ::Internal::LuaScriptValueStackRead(l, stackIndex, AzTypeInfo<T>::Uuid()));
        }
        static void StackPush(lua_State* l, T* pointer, const AZStd::false_type& /* AZ::Internal::HasAzTypeInfo<T>::value() */)
        {
            AZ::Internal::LuaScriptValueStackPush(l, *pointer, nullptr);
        }
        static T*   StackRead(lua_State* l, int stackIndex, const AZStd::false_type& /* AZ::Internal::HasAzTypeInfo<T>::value() */)
        {
            return reinterpret_cast<T*>(AZ::Internal::LuaScriptValueStackRead(l, stackIndex, AZ::Uuid::CreateNull()));
        }
        static void StackPush(lua_State* l, T* pointer)
        {
            StackPush(l, pointer, typename AZ::Internal::HasAZTypeInfo<T>::type());
        }
        static T*   StackRead(lua_State* l, int stackIndex)
        {
            return StackRead(l, stackIndex, typename AZ::Internal::HasAZTypeInfo<T>::type());
        }
    };

    // TODO make sure we handle all integral types and their pointers
    // integral types are support by value only, otherwise we need an object.
    template<>
    struct ScriptValue<int**>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void     StackPush(lua_State*, int**)                   { AZ_Assert(false, "We can't use pointer to intergral objects in script!"); }
        static int**    StackRead(lua_State*, int)                     { AZ_Assert(false, "We can't use pointer to intergral objects in script!"); return nullptr; }
    };
    template<>
    struct ScriptValue<char>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, int value);
        static char StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<short>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void     StackPush(lua_State* l, int value);
        static short    StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<int>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, int value);
        static int  StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<AZ::s64>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, AZ::s64 value);
        static AZ::s64 StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<long>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, long value);
        static long StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<unsigned char>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, unsigned int value);
        static unsigned char StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<unsigned short>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, unsigned int value);
        static unsigned short StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<unsigned int>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, unsigned int value);
        static unsigned int StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<AZ::u64>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, AZ::u64 value);
        static AZ::u64 StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<unsigned long>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, unsigned long value);
        static unsigned long StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<float>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, float value);
        static float StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<double>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, double value);
        static double StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<bool>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, bool value);
        static bool StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<const char*>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, const char* value);
        static const char* StackRead(lua_State* l, int stackIndex);
    };
    template<>
    struct ScriptValue<void*>
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, void* value);
        static void* StackRead(lua_State* l, int stackIndex);
    };

    template<>
    struct ScriptValue<AZStd::any>
    {
        static const bool isNativeValueType = false;  // We use native type for internal representation
        static void StackPush(lua_State* l, const AZStd::any& value);
        static AZStd::any StackRead(lua_State* lua, int stackIndex);
    };

    template<>
    struct ScriptValue<const AZStd::any&>
        : public ScriptValue<AZStd::any> {};

    template<>
    struct ScriptValue<const AZStd::any>
        : public ScriptValue<AZStd::any> {};

    template<class Element, class Traits, class Allocator>
    struct ScriptValue<const AZStd::basic_string< Element, Traits, Allocator> >
    {
        static const bool isNativeValueType = true;  // We use native type for internal representation
        static void StackPush(lua_State* l, const AZStd::basic_string< Element, Traits, Allocator>& value)
        {
            AZ::Internal::azlua_pushstring(l, value.c_str());
        }
        static AZStd::basic_string< Element, Traits, Allocator> StackRead(lua_State* l, int stackIndex)
        {
            return AZ::Internal::azlua_tostring(l, stackIndex);
        }
    };
    template<class Element, class Traits, class Allocator>
    struct ScriptValue<AZStd::basic_string< Element, Traits, Allocator> >
        : public ScriptValue< const AZStd::basic_string< Element, Traits, Allocator> >{};
    template<class Element, class Traits, class Allocator>
    struct ScriptValue<const AZStd::basic_string< Element, Traits, Allocator>&>
        : public ScriptValue< const AZStd::basic_string< Element, Traits, Allocator> >{};

    //////////////////////////////////////////////////////////////////////////
    // Script data context

    template<class T>
    inline bool ScriptDataContext::IsClass(int index) const
    {
        const auto uuidValue{ AzTypeInfo<T>::Uuid() };
        return AZ::Internal::LuaIsClass(m_nativeContext, m_startVariableIndex + index, &uuidValue);
    }
    template<class T>
    inline bool ScriptDataContext::ReadArg(int index, T& valueRef) const
    {
        if (m_mode == MD_CALLEE)
        {
            valueRef = ScriptValue<T>::StackRead(m_nativeContext, m_startVariableIndex + index);
            return true;
        }
        else
        {
            AZ_Warning("Script", false, "You are allowed to read arguments only when you are in a function called from script!");
            return false;
        }
    }

    template<class T>
    inline bool ScriptDataContext::ReadResult(int index, T& valueRef) const
    {
        if (m_mode == MD_CALLER_EXECUTED)
        {
            valueRef = ScriptValue<T>::StackRead(m_nativeContext, m_startVariableIndex + index);
            return true;
        }
        else
        {
            AZ_Warning("Script", false, "You are allowed to read results only after you have called a script function!");
            return false;
        }
    }
    template<class T>
    inline bool ScriptDataContext::ReadValue(int index, T& valueRef) const
    {
        if (m_mode == MD_INSPECT || m_mode == MD_READ_STACK)
        {
            valueRef = ScriptValue<T>::StackRead(m_nativeContext, m_startVariableIndex + index);
            return true;
        }
        else
        {
            AZ_Warning("Script", false, "You are allowed to read values only in inspect or read stack mode!");
            return false;
        }
    }
    template<class T>
    inline void ScriptDataContext::PushResult(const T& value)
    {
        if (m_mode == MD_CALLEE)
        {
            ScriptValue<T>::StackPush(m_nativeContext, value);
            m_numResults++;
        }
        else
        {
            AZ_Warning("Script", false, "Results can be pused only when you are in a function called from script!");
        }
    }
    template<class T>
    inline void ScriptDataContext::PushArg(const T& value)
    {
        if (m_mode == MD_CALLER)
        {
            ScriptValue<T>::StackPush(m_nativeContext, value);
            m_numArguments++;
        }
        else
        {
            AZ_Warning("Script", false, "Arguments can be pused only when you are about to call a script function from C++!");
        }
    }

    template<class Arg, class... Args>
    inline void ScriptDataContext::PushArgs(Arg& value, Args&&... args)
    {
        PushArg(value);
        PushArgs(AZStd::forward(args)...);
    }

    template<class T>
    bool ScriptDataContext::ReadTableElement(int tableIndex, const char* name, T& value)
    {
        if (m_mode == MD_INSPECT)
        {
            bool result = false;
            AZ::Internal::azlua_getfield(m_nativeContext, m_startVariableIndex + tableIndex, name);
            if (!AZ::Internal::azlua_isnil(m_nativeContext, -1))
            {
                // TODO make a type check
                value = ScriptValue<T>::StackRead(m_nativeContext, -1);
                result = true;
            }
            AZ::Internal::azlua_pop(m_nativeContext, 1);
            return result;
        }
        else
        {
            AZ_Warning("Script", false, "You can read table elements only in Inspect mode!");
            return false;
        }
    }
    template<class T>
    void ScriptDataContext::AddReplaceTableElement(int tableIndex, const char* name, const T& value)
    {
        if (m_mode == MD_INSPECT)
        {
            AZ::Internal::azlua_pushstring(m_nativeContext, name);
            ScriptValue<T>::StackPush(m_nativeContext, value);
            AZ::Internal::azlua_rawset(m_nativeContext, m_startVariableIndex + tableIndex);
        }
        else
        {
            AZ_Warning("Script", false, "You can add/replace table elements only in Inspect mode!");
        }
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //
    // Generic script value implementation
    template<class T, bool isEnum>
    inline void ScriptValueGeneric<T, isEnum>::StackPush(lua_State* l, const ValueType& value)
    {
        AZ::Internal::LuaClassToStack(l, (void*)&value, AzTypeInfo<T>::Uuid());
    }

    template<class T, bool isEnum>
    inline T ScriptValueGeneric<T, isEnum>::StackRead(lua_State* l, int stackIndex)
    {
        return *reinterpret_cast<T*>(AZ::Internal::LuaClassFromStack(l, stackIndex, AzTypeInfo<T>::Uuid()));
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    class ScriptContextDebug;

    typedef AZ::u32 ScriptContextId;
    enum ScriptContextIds : ScriptContextId
    {
        DefaultScriptContextId = 0,
        CryScriptContextId = 1
    };

    using StackVariableAllocator = AZStd::static_buffer_allocator<256, 16>;

    /**
    * Lua VM context.
    * \note move all LUA code to CPP and remove including it by default, most operations happen via ScriptDataContext.
    */
    class ScriptContext
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptContext, AZ::SystemAllocator);

        static const int m_maxDbgName = 512; ///< Max debug name length.

        enum class ErrorType
        {
            Log,
            Warning,
            Error,
        };
        using ErrorHook = AZStd::function<void(ScriptContext* context, ErrorType error, const char* message)>;
        // Note: Always use l over context->NativeContext(), as require may be called from a thread.
        using RequireHook = AZStd::function<int(lua_State* lua, ScriptContext* context, const char* module)>;

        using StackVariableAllocator = AZ::StackVariableAllocator;
        /// Stack temporary memory

        /**
         * Class that provides custom reader/write to the Lua VM
         */
        struct CustomReaderWriter
        {
            AZ_TYPE_INFO(CustomReaderWriter,"{ea18a980-de83-4f1c-bf5b-1658584ebe2d}");
            // This function is REQUIRED to push 1 value on the stack (push nil if you can't push a valid value)
            using CustomToLua = void(*)(lua_State* lua, BehaviorArgument& value);
            // temporary allocation on the stack,if you need it by CustomFromLua to read a value
            using TempStackAllocate = void*(*)(size_t byteSize, size_t alignment, int flags);
            // reads value from lua stack, returns true if read is successful otherwise false
            using CustomFromLua = bool(*)(lua_State* lua, int statckIndex, BehaviorArgument& value, BehaviorClass* valueClass, StackVariableAllocator* stackTempAllocator);

            CustomReaderWriter(CustomToLua write, CustomFromLua read)
                : m_toLua(write)
                , m_fromLua(read)
            {}

            CustomToLua m_toLua;
            CustomFromLua m_fromLua;
        };

        ScriptContext(ScriptContextId id = ScriptContextIds::DefaultScriptContextId, IAllocator* allocator = nullptr, lua_State* nativeContext = nullptr);
        ~ScriptContext();

        /// Bind LUA context (VM) a specific behaviorContext
        void BindTo(BehaviorContext* behaviorContext);

        BehaviorContext* GetBoundContext() const;

        ScriptContextId GetId() const
        {
            return m_id;
        }

        /// execute all script loaded though load function, plus the one you supply as string
        bool Execute(const char* script = nullptr, const char* dbgName = nullptr, size_t dataSize = 0);

        /**
         * Load a lua buffer from the stream provided. Can be either text or binary blob.
         * If load succeeds (true is returned), a function will be on top of the stack.
         * If load fails (false is returned), a string (error message) will be on top of the stack.
         * If not passing the result directly to lua (i.e. as you would for require), it is the caller's responsibility to pop the result.
         *
         * \param stream    stream to read from.
         * \param debugName the debug name for the module
         * \param thread    the thread to execute the load on (MUST be a child of NativeContext()). Defaults to NativeContext().
         * \param mode      the type of the stream, "b" for compiled Lua binary, "t" for raw Lua text, "bt" to have Lua check for you
         * \returns whether or not the load succeeded.
         */
        bool LoadFromStream(IO::GenericStream* stream, const char* debugName, const char* mode, lua_State* thread = nullptr);

        /// Perform a full garbage collect step, this can be slow prefer GargabeCollectStep for runtime garbage collect
        void GarbageCollect();

        /// Return the amount of memory used, in bytes
        size_t GetMemoryUsage() const;

        /**
         *  Step the garbage collector. There is no exact number that works in all cases, tune this number for optimal
         * performance in your app.
         */
        void GarbageCollectStep(int numberOfSteps = 2);

        lua_State* NativeContext();

        //////////////////////////////////////////////////////////////////////////
        // @{
        bool ExistsGlobal(const char* name);
        bool FindGlobal(const char* name, ScriptDataContext& dc);
        bool FindGlobal(int cachedIndex, ScriptDataContext& dc);
        template<class T>
        bool ReadGlobal(const char* name, T& value);
        template<class T>
        bool ReadGlobal(int cacheIndex, T& value);
        template<class T>
        void AddReplaceGlobal(const char* name, const T& value);
        void RemoveGlobal(const char* name);
        template<class T>
        void AddReplace(int cacheIndex, const T& value);

        int CacheGlobal(const char* name);
        /// Release any cached resource (global or local)
        void ReleaseCached(int cacheIndex);
        bool Call(const char* functionName, ScriptDataContext& dc);
        bool CallCached(int cachedIndex, ScriptDataContext& dc);

        bool InspectTable(const char* tableName, ScriptDataContext& dc);
        bool InspectTableCached(int cachedIndex, ScriptDataContext& dc);
        bool ReadStack(ScriptDataContext& dc);
        // @}

        /// Get Native context
        static ScriptContext*   FromNativeContext(lua_State* nativeContext);

        //////////////////////////////////////////////////////////////////////////
        // Debugging
        void EnableDebug();   ///< Creates debug context (you can obtain vie GetDebugContext()). Depending on the implementation this can require more memory.
        void DisableDebug();  ///< Destroys debug context
        ScriptContextDebug* GetDebugContext();

        /**
         * Make sure that the Lua EBus handlers are not called from background threads.
         * By default the thread that creates the script context is the owner.
         * This method allows to override this default behaviour.
        */
        void DebugSetOwnerThread(AZStd::thread::id ownerThreadId);

        /**
         * Make sure that the Lua EBus handlers are not called from background threads.
         * Use this to make sure the calling thread is the thread that owns the script context.
        */
        bool DebugIsCallingThreadTheOwner() const;

        void SetErrorHook(ErrorHook cb);
        ErrorHook GetErrorHook() const;

        void Error(ErrorType error, bool doStackTrace, const char* format, ...);
        //////////////////////////////////////////////////////////////////////////

        /**
         * Set function to be invoked when require() is called, but module is not cached.
         */
        void SetRequireHook(RequireHook hook);

        // Used to convert the value specified at the specified index in the datacontext to a ScriptProperty.
        AZ::ScriptProperty* ConstructScriptProperty(ScriptDataContext& dc, int valueIndex, const char* name = nullptr, bool restrictToPropertyArrays = false);

    protected:
        class ScriptContextImpl* m_impl;
        ScriptContextId m_id;

#if defined(LUA_USERDATA_TRACKING)
    public:
        void AddAllocation(LuaUserData* userdata);
        void RemoveAllocation(LuaUserData* userdata);

        AZStd::unordered_map<LuaUserData*, void*> m_valueByUserData;
        AZStd::unordered_map<void*, LuaUserData*> m_userDataByValue;
#endif//defined(LUA_USERDATA_TRACKING)

    };

    using LuaLoadFromStack = bool(*)(lua_State*, int, AZ::BehaviorArgument&, AZ::BehaviorClass*, AZ::StackVariableAllocator*);
    using LuaPushToStack = void(*)(lua_State*, AZ::BehaviorArgument&);
    using LuaPrepareValue = bool(*)(AZ::BehaviorArgument&, AZ::BehaviorClass*, AZ::StackVariableAllocator&, AZStd::allocator* backupAllocator);

    // returns a function that allows the caller to push the parameter into the
    LuaLoadFromStack FromLuaStack(AZ::BehaviorContext* context, const AZ::BehaviorParameter* param, AZ::BehaviorClass*& behaviorClass);
    LuaPushToStack ToLuaStack(AZ::BehaviorContext* context, const AZ::BehaviorParameter* param, LuaPrepareValue* prepareParamOut, AZ::BehaviorClass*& behaviorClass);

    void StackPush(lua_State* lua, AZ::BehaviorContext* context, AZ::BehaviorArgument& param);
    void StackPush(lua_State* lua, AZ::BehaviorArgument& param);
    bool StackRead(lua_State* lua, int index, AZ::BehaviorContext* context,  AZ::BehaviorArgument& param, AZ::StackVariableAllocator*);
    bool StackRead(lua_State* lua, int index, AZ::BehaviorArgument& param, AZ::StackVariableAllocator* = nullptr);

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    bool ScriptContext::ReadGlobal(const char* name, T& value)
    {
        bool result = false;
        lua_State* nativeContext = NativeContext();
        AZ::Internal::azlua_getglobal(nativeContext, name);
        if (!AZ::Internal::azlua_isnil(nativeContext, -1))
        {
            value = ScriptValue<T>::StackRead(nativeContext, -1);
            result = true;
        }
        AZ::Internal::azlua_pop(nativeContext, 1);
        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    bool ScriptContext::ReadGlobal(int cacheIndex, T& value)
    {
        bool result = false;
        lua_State* nativeContext = NativeContext();
        AZ::Internal::LuaRegistryGet(nativeContext, cacheIndex);
        if (!AZ::Internal::azlua_isnil(nativeContext, -1))
        {
            value = ScriptValue<T>::StackRead(nativeContext, -1);
            result = true;
        }
        AZ::Internal::azlua_pop(nativeContext, 1);
        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    inline void ScriptContext::AddReplaceGlobal(const char* name, const T& value)
    {
        lua_State* nativeContext = NativeContext();
        ScriptValue<T>::StackPush(nativeContext, value);
        AZ::Internal::azlua_setglobal(nativeContext, name);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    inline void ScriptContext::AddReplace(int cacheIndex, const T& value)
    {
        lua_State* nativeContext = NativeContext();
        ScriptValue<T>::StackPush(nativeContext, value);
        AZ::Internal::LuaRegistrySet(nativeContext, cacheIndex);
    }
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    /**
     * RAII method of creating and managing Lua threads.
     * Because of implicit cast to lua_State*, this object can be passed to lua[L]_* functions
     */
    struct LuaNativeThread
    {
        LuaNativeThread() = delete;
        LuaNativeThread(lua_State* rootState);
        ~LuaNativeThread();

        operator lua_State*() { return m_thread; }

    private:
        lua_State* m_thread = nullptr;
        int m_registryIndex = 0;
    };

    namespace Internal
    {
        bool IsAvailableInLua(const AttributeArray& attributes);
    }

} // namespace AZ
