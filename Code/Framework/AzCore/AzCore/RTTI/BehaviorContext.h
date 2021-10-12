/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Preprocessor/Sequences.h> // for AZ_EBUS_BEHAVIOR_BINDER
#include <AzCore/RTTI/BehaviorObjectSignals.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/typetraits/is_abstract.h>
#include <AzCore/std/utils.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Script/ScriptContextAttributes.h>

namespace AZStd
{
    template <typename T>
    struct hash;
}

namespace AZ
{
    const static AZ::Crc32 RuntimeEBusAttribute = AZ_CRC("RuntimeEBus", 0x466b899b); ///< Signals that this reflected ebus should only be available at runtime, helps tools filter out data driven ebuses

    constexpr const char* k_PropertyNameGetterSuffix = "::Getter";
    constexpr const char* k_PropertyNameSetterSuffix = "::Setter";

    /// Typedef for class unwrapping callback (i.e. used for things like smart_ptr<T> to unwrap for T)
    using BehaviorClassUnwrapperFunction = void(*)(void* /*classPtr*/, void*& /*unwrappedClass*/, AZ::Uuid& /*unwrappedClassTypeId*/, void* /*userData*/);

    class BehaviorContext;
    class BehaviorClass;
    class BehaviorProperty;
    class BehaviorEBusHandler;
    class BehaviorDefaultValue;

    using BehaviorDefaultValuePtr = AZStd::intrusive_ptr<BehaviorDefaultValue>;

    enum class AttributeIsValid : AZ::u8
    {
        IfPresent,
    };

    struct BehaviorObject // same as DynamicSerializableField, make sure we merge them... so we can store the object easily
    {
        AZ_TYPE_INFO(BehaviorObject, "{2813cdfb-0a4a-411c-9216-72a7b644d1dd}");

        BehaviorObject();
        BehaviorObject(void* address, const Uuid& typeId);
        BehaviorObject(void* address, IRttiHelper* rttiHelper);
        bool IsValid() const;

        void* m_address;
        Uuid m_typeId;
        IRttiHelper* m_rttiHelper{};
    };

    /**
     * Stores information about a function parameter (no instance). During calls we use \ref BehaviorValueParameter which in addition
     * offers value storage and functions to interact with the data
     */
    struct BehaviorParameter
    {
        AZ_TYPE_INFO(BehaviorParameter, "{BD7B664E-5B8C-4B51-84F3-DE89B271E075}")
        /// Temporary POD buffer when we convert parameters on the stack.
        typedef AZStd::static_buffer_allocator<32, 32> TempValueParameterAllocator;

        /**
         * Function parameters traits
         */
        enum Traits : u32
        {
            TR_POINTER = (1 << 0),
            TR_CONST = (1 << 1),
            TR_REFERENCE = (1 << 2),
            TR_THIS_PTR = (1 << 3), // set if the parameter is a this pointer to a method
            TR_STRING = (1 << 4),
            TR_ARRAY_BEGIN = (1 << 5), // Parameter specifies the begin address of the array inclusive
            TR_ARRAY_END = (1 << 6),  // Parameter specifies the end address of the array exclusive (conflicts with TR_ARRAY_SIZE)
            TR_ARRAY_SIZE = (1 << 7), // Parameter specifies the number of elements in an array parameter (conflicts with TR_ARRAY_END)
            TR_INDEX = (1 << 8), // Parameter specifies an index into a collection, and should be offset by 1 when transferring to Lua
            TR_NONE = 0U,
        };

        const char* m_name;
        Uuid m_typeId;
        IRttiHelper* m_azRtti;
        u32 m_traits;
    };

    /*
     * struct to help with Additional data to be associated with parameters(Argument names and tooltips, type traits and default values), these are usually in arrays
     */
    struct BehaviorParameterMetadata
    {
        BehaviorParameterMetadata(AZStd::string_view name = {}, AZStd::string_view toolTip = {}, BehaviorDefaultValuePtr defaultValue = {})
            : m_name(name)
            , m_toolTip(toolTip)
            , m_defaultValue(defaultValue)
        {}
        AZStd::string m_name;
        AZStd::string m_toolTip;
        BehaviorDefaultValuePtr m_defaultValue;
    };

    /*
     * Struct to use to pass per parameter overrides when reflecting a Method or Event
     * \param addTraits - OR'ed against the traits of the BehaviorParameter being overridden. This specifies which flags to add.
     * \param keepTraits - AND'ed against the traits of the BehaviorParameter being overridden. This specifies which flags to keep.
     */
    struct BehaviorParameterOverrides
    {
        BehaviorParameterOverrides(AZStd::string_view name = {}, AZStd::string_view toolTip = {}, BehaviorDefaultValuePtr defaultValue = {},
            u32 addTraits = BehaviorParameter::Traits::TR_NONE, u32 removeTraits = BehaviorParameter::Traits::TR_NONE)
            : m_name(name)
            , m_toolTip(toolTip)
            , m_defaultValue(defaultValue)
            , m_addTraits(addTraits)
            , m_removeTraits(removeTraits)
        {}

        AZStd::string m_name;
        AZStd::string m_toolTip;
        BehaviorDefaultValuePtr m_defaultValue;
        AZ::u32 m_addTraits;
        AZ::u32 m_removeTraits;
    };

    /**
     * BehaviorValueParameter is used for calls on the stack. It should not be reused or stored as we might store temp data
     * during conversion in the class on the stack. For storing type info use \ref BehaviorParameter
     *
     */
    struct BehaviorValueParameter : public BehaviorParameter
    {
        BehaviorValueParameter();
        BehaviorValueParameter(const BehaviorValueParameter&) = default;
        BehaviorValueParameter(BehaviorValueParameter&&);
        template<class T>
        BehaviorValueParameter(T* value);

        /// Special handling for the generic object holder.
        BehaviorValueParameter(BehaviorObject* value);

        template<class T>
        void Set(T* value);
        void Set(BehaviorObject* value);
        void Set(const BehaviorParameter& param);
        void Set(const BehaviorValueParameter& param);

        void* GetValueAddress() const;

        /// Convert to BehaviorObject implicitly for passing generic parameters (usually not known at compile time)
        operator BehaviorObject() const;

        /// Converts internally the value to a specific type known at compile time. \returns true if conversion was successful.
        template<class T>
        bool ConvertTo();

        /// Converts a value to a specific one by typeID (usually when the type is not known at compile time)
        bool ConvertTo(const AZ::Uuid& typeId);

        /// This function is Unsafe, because it assumes that you have called ConvertTo<T> prior to called it and it returned true (basically mean the BehaviorValueParameter is converted to T)
        template<typename T>
        AZStd::decay_t<T>* GetAsUnsafe() const;

        BehaviorValueParameter& operator=(const BehaviorValueParameter&) = default;
        BehaviorValueParameter& operator=(BehaviorValueParameter&&);
        template<typename T>
        BehaviorValueParameter& operator=(T&& result);

        /// Stores a value (usually return value of a function).
        template<typename T>
        bool StoreResult(T&& result);

        /// Used internally to store values in the temp data
        template<typename T>
        void StoreInTempData(T&& value);

        void* m_value;  ///< Pointer to value, keep it mind to check the traits as if the value is pointer, this will be pointer to pointer and use \ref GetValueAddress to get the actual value address
        AZStd::function<void()> m_onAssignedResult;
        BehaviorParameter::TempValueParameterAllocator m_tempData; ///< Temp data for conversion, etc. while preparing the parameter for a call (POD only)
    };

    AZ_TYPE_INFO_SPECIALIZE(AZ::BehaviorValueParameter, "{B1680AE9-4DBE-4803-B12F-1E99A32990B7}")

    /**
    * Class that handles a single default value. The Value type is verified to match parameter signature
    */
    class BehaviorDefaultValue
        : public AZStd::intrusive_base
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorDefaultValue, AZ::SystemAllocator, 0);

        /**
        * Create a default value for a specific method parameter. The Default values is stored by value
        * in a temp storage, so currently there is limit the \ref BehaviorValueParameter temp storage, we
        * can easily change that if it became a problem.
        */
        template<typename Value>
        BehaviorDefaultValue(Value&& value)
        {
            m_value.StoreInTempData(AZStd::forward<Value>(value));
        }

        const BehaviorValueParameter& GetValue() const
        {
            return m_value;
        }

        BehaviorValueParameter m_value;
    };


    /**
     * Base class that handles default values. Values types are verified to match exactly the function signature.
     * the order of the default values is the same as in C++ (going in reverse the back).
     */
    class BehaviorValues
    {
    public:
        virtual ~BehaviorValues() {}

        virtual size_t GetNumValues() const = 0;
        virtual BehaviorDefaultValuePtr GetDefaultValue(size_t i) const = 0;
    };

    /**
     * Stores the name of an AZ::Event<Params...> and names for each of it's parameters
     * For use in scripting to annotate functions and nodes with user-friendly names
     */
    struct BehaviorAzEventDescription
    {
        AZ_TYPE_INFO(BehaviorAzEventDescription, "{B5D95E87-FA17-41C7-AC90-7258A520FE82}");
        AZStd::string m_eventName;
        AZStd::vector<AZStd::string> m_parameterNames;
    };

    //! Checks if the supplied BehaviorMethod returns AZ::Event by either pointer or reference
    bool MethodReturnsAzEventByReferenceOrPointer(const AZ::BehaviorMethod& method);

    //! Validates that a method that returns an AZ::Event fulfills the following conditions.
    //! 1. It has an AzEventDescription that stores a BehaviorAzEventDescription instance
    //! 2. The number of parameters that the method accepts, matches the number of elements
    //!    in the parameter names array
    //! 3. Neither the AZ::Event name nor any of it's parameters are an empty string
    bool ValidateAzEventDescription(const AZ::BehaviorContext& context, const AZ::BehaviorMethod& method);

    /**
     * Use behavior method to get type information and invoke reflected methods.
     */
    class BehaviorMethod
        : public OnDemandReflectionOwner
    {
    public:
        BehaviorMethod(BehaviorContext* context);
        ~BehaviorMethod() override;

        template<class... Args>
        bool Invoke(Args&&... args) const;
        bool Invoke() const;

        template<class R, class... Args>
        bool InvokeResult(R& r, Args&&... args) const;
        template<class R>
        bool InvokeResult(R& r) const;
        void SetDeprecatedName(const AZStd::string& name) { m_deprecatedName = name; }
        const AZStd::string& GetDeprecatedName() const { return m_deprecatedName; }

        virtual bool Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result = nullptr) const = 0;
        virtual bool HasResult() const = 0;
        /// Returns true if the method is a class member method. If true the first argument should always be the "this"/ClassType pointer.
        virtual bool IsMember() const = 0;
        /// Returns true if the method is an ebus method with a bus id argument.
        virtual bool HasBusId() const = 0;
        /// Returns the BehaviorParameter corresponding to the the ebus BusId argument if the ebus method is addressed by Id
        virtual const BehaviorParameter* GetBusIdArgument() const = 0;

        virtual void OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits) = 0;

        virtual size_t GetNumArguments() const = 0;
        /// Return the minimum number of arguments needed (considering default arguments)
        virtual size_t GetMinNumberOfArguments() const = 0;
        virtual const BehaviorParameter* GetArgument(size_t index) const = 0;
        virtual const AZStd::string* GetArgumentName(size_t index) const = 0;
        virtual void SetArgumentName(size_t index, const AZStd::string& name) = 0;
        virtual const AZStd::string* GetArgumentToolTip(size_t index) const = 0;
        virtual void SetArgumentToolTip(size_t index, const AZStd::string& name) = 0;
        virtual void SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue) = 0;
        virtual BehaviorDefaultValuePtr GetDefaultValue(size_t index) const = 0;
        virtual const BehaviorParameter* GetResult() const = 0;

        bool AddOverload(BehaviorMethod* method);
        bool IsAnOverload(BehaviorMethod* candidate) const;

        BehaviorMethod* m_overload = nullptr;

        AZStd::string m_name;   ///< Debug friendly behavior method name
        AZStd::string m_deprecatedName;     ///<this is the deprecated name of this method
        const char* m_debugDescription;
        bool m_isConst = false; ///< Is member function const (false if not a member function)
        AttributeArray m_attributes;    ///< Attributes for the method
    };

    using InputIndices = AZStd::vector<AZ::u8>;

    struct InputRestriction
    {
        AZ_TYPE_INFO(InputRestriction, "{9DF4DDBE-63BE-4749-9921-52C82BF5E307}");
        AZ_CLASS_ALLOCATOR(InputRestriction, AZ::SystemAllocator, 0);

        bool m_listExcludes = true;
        InputIndices m_indices;

        InputRestriction() = default;
        InputRestriction(bool listExcludes, const InputIndices& indices)
            : m_listExcludes(listExcludes)
            , m_indices(indices)
        {}
    };

    struct BranchOnResultInfo
    {
        AZ_TYPE_INFO(BranchOnResultInfo, "{C063AB6F-462F-485F-A911-DE3A8946A019}");
        AZ_CLASS_ALLOCATOR(BranchOnResultInfo, AZ::SystemAllocator, 0);

        AZStd::string m_trueName = "True";
        AZStd::string m_falseName = "False";
        AZStd::string m_trueToolTip;
        AZStd::string m_falseToolTip;
        bool m_returnResultInBranches = false;
        AZStd::string m_nonBooleanResultCheckName;

        BranchOnResultInfo() = default;
    };

    struct CheckedOperationInfo
    {
        AZ_TYPE_INFO(CheckedOperationInfo, "{9CE9560F-ECAB-46EF-B341-3A86973E71CD}");
        AZ_CLASS_ALLOCATOR(CheckedOperationInfo, AZ::SystemAllocator, 0);

        AZStd::string m_safetyCheckName;
        InputRestriction m_inputRestriction;
        AZStd::string m_successCaseName;
        AZStd::string m_failureCaseName;
        bool m_callCheckedFunctionInBothCases = false;

        CheckedOperationInfo() = default;

        CheckedOperationInfo
            ( AZStd::string_view safetyCheckName
            , const InputRestriction& inputRestriction
            , AZStd::string_view successName
            , AZStd::string_view failureName
            , bool callCheckedFunctionInBothCases = false);

        // \todo replace in hash operations with custom equality check
        bool operator==(const CheckedOperationInfo& other) const;
        bool operator!=(const CheckedOperationInfo& other) const;
    };

    struct OverloadArgumentGroupInfo
    {
        AZ_TYPE_INFO(OverloadArgumentGroupInfo, "{AEFEFC42-3ED8-43A9-AE1F-6D8F32A280D2}");
        AZ_CLASS_ALLOCATOR(OverloadArgumentGroupInfo, AZ::SystemAllocator, 0);

        AZStd::vector<AZStd::string> m_parameterGroupNames;
        AZStd::vector<AZStd::string> m_resultGroupNames;

        OverloadArgumentGroupInfo() = default;

        OverloadArgumentGroupInfo(const AZStd::vector<AZStd::string>&& parameterGroupNames, const AZStd::vector<AZStd::string>&& resultGroupName);
    };

    struct ExplicitOverloadInfo
    {
        AZ_TYPE_INFO(ExplicitOverloadInfo, "{AEFEFC42-3ED8-43A9-AE1F-6D8F32A280D2}");
        AZ_CLASS_ALLOCATOR(ExplicitOverloadInfo, AZ::SystemAllocator, 0);

        AZStd::string m_name;
        AZStd::string m_categoryPath;
        AZStd::vector<AZStd::pair<AZ::BehaviorMethod*, AZ::BehaviorClass*>> m_overloads;

        ExplicitOverloadInfo() = default;

        ExplicitOverloadInfo(AZStd::string_view name, AZStd::string_view categoryPath);

        // \todo replace in hash operations with custom equality check
        bool operator==(const ExplicitOverloadInfo& other) const;
        bool operator!=(const ExplicitOverloadInfo& other) const;
    };
}

namespace AZStd
{
    template<>
    struct hash<AZ::ExplicitOverloadInfo>
    {
        using argument_type = AZ::ExplicitOverloadInfo;
        using result_type = size_t;

        AZ_INLINE result_type operator() (const argument_type& info) const
        {
            size_t h = 0;
            hash_combine(h, info.m_name);
            return h;
        }
    };

    template<>
    struct hash<AZ::CheckedOperationInfo>
    {
        using argument_type = AZ::CheckedOperationInfo;
        using result_type = size_t;

        AZ_INLINE result_type operator() (const argument_type& info) const
        {
            size_t h = 0;
            hash_combine(h, info.m_safetyCheckName);
            return h;
        }
    };
} // namespace AZStd

namespace AZ
{
    // AZ::Event support
    using BehaviorFunction = AZStd::function<void(BehaviorValueParameter* result, BehaviorValueParameter* arguments, int numArguments)>;
    using EventHandlerCreationFunction = AZStd::function<BehaviorObject(void* , BehaviorFunction&&)>;

    struct EventHandlerCreationFunctionHolder
    {
        AZ_TYPE_INFO(EventHandlerCreationFunctionHolder, "{40F7C5D8-8DA0-4979-BC8C-0A52EDA80633}");
        AZ_CLASS_ALLOCATOR(EventHandlerCreationFunctionHolder, AZ::SystemAllocator, 0);

        EventHandlerCreationFunction m_function;
    };

    namespace Internal
    {
        const AZ::TypeId& GetUnderlyingTypeId(const IRttiHelper& enumRttiHelper);

        // Converts sourceAddress to targetType
        inline bool ConvertValueTo(void* sourceAddress, const IRttiHelper* sourceRtti, const AZ::Uuid& targetType, void*& targetAddress, BehaviorParameter::TempValueParameterAllocator& tempAllocator)
        {
            // Check see if the underlying typeid is an enum whose typeIds match
            if (GetUnderlyingTypeId(*sourceRtti) == targetType)
            {
                return true;
            }
            // convert
            void* convertedAddress = sourceRtti->Cast(sourceAddress, targetType);
            if (convertedAddress && convertedAddress != sourceAddress) // if we converted as we have a different address
            {
                // allocate temp storage and store it
                targetAddress = tempAllocator.allocate(sizeof(void*), AZStd::alignment_of<void*>::value, 0);
                *reinterpret_cast<void**>(targetAddress) = convertedAddress;
            }
            return convertedAddress != nullptr;
        }

        // Assumes parameters array is big enough to store all parameters
        template<class... Args>
        void SetParameters(BehaviorParameter* parameters, OnDemandReflectionOwner* onDemandReflection = nullptr);

        // call helper
        template<class R, class... Args>
        struct CallFunction
        {
            template<AZStd::size_t... Is, class Function>
            static inline void Global(Function functionPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>);
            template<AZStd::size_t... Is, class C, class Function>
            static inline void Member(Function functionPtr, C thisPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>);
        };

        template<class... Args>
        struct CallFunction<void, Args...>
        {
            template<AZStd::size_t... Is, class Function>
            static inline void Global(Function functionPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result; (void)arguments;
                functionPtr(*arguments[Is].GetAsUnsafe<Args>()...);
            };

            template<AZStd::size_t... Is, class C, class Function>
            static inline void Member(Function functionPtr, C thisPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result; (void)arguments;
                (thisPtr->*functionPtr)(*arguments[Is].GetAsUnsafe<Args>()...);
            };
        };

        template<class Function>
        class BehaviorMethodImpl;

        template<class R, class... Args>
        class BehaviorMethodImpl<R(Args...)> : public BehaviorMethod
        {
        public:
            using FunctionPointer = R(*)(Args...);
            typedef void ClassType;

            AZ_CLASS_ALLOCATOR(BehaviorMethodImpl, AZ::SystemAllocator, 0);

            static const int s_startArgumentIndex = 1; // +1 for result type
            static const int s_startNamedArgumentIndex = s_startArgumentIndex; // +1 for result type

            BehaviorMethodImpl(FunctionPointer functionPointer, BehaviorContext* context, const AZStd::string& name = AZStd::string());

            bool Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result) const override;

            bool HasResult() const override;
            bool IsMember() const override;
            bool HasBusId() const override;

            const BehaviorParameter* GetBusIdArgument() const override;

            size_t GetNumArguments() const override;
            size_t GetMinNumberOfArguments() const override;

            const BehaviorParameter* GetArgument(size_t index) const override;
            const AZStd::string* GetArgumentName(size_t index) const override;
            void SetArgumentName(size_t index, const AZStd::string& name) override;
            const AZStd::string* GetArgumentToolTip(size_t index) const override;
            void SetArgumentToolTip(size_t index, const AZStd::string& name) override;
            void SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue) override;
            BehaviorDefaultValuePtr GetDefaultValue(size_t index) const override;
            const BehaviorParameter* GetResult() const override;

            void OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits) override;

            FunctionPointer m_functionPtr;

            BehaviorParameter m_parameters[sizeof...(Args)+s_startNamedArgumentIndex];
            AZStd::array<BehaviorParameterMetadata, sizeof...(Args)+s_startNamedArgumentIndex> m_metadataParameters; ///< Stores the per parameter metadata which is used to add names, tooltips, trait, default values, etc... to the parameters
        };

#if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must be distinguished from non-noexcept overloads
        template<class R, class... Args>
        class BehaviorMethodImpl<R(Args...) noexcept>
            : public BehaviorMethodImpl<R(Args...)>
        {
            using base_type = BehaviorMethodImpl<R(Args...)>;
        public:
            using base_type::base_type;
            using FunctionPointer = R(*)(Args...) noexcept;
        };
#endif

        template<class R, class C, class... Args>
        class BehaviorMethodImpl<R(C::*)(Args...)> : public BehaviorMethod
        {
        public:
            using FunctionPointer = R(C::*)(Args...);
            using FunctionPointerConst = R(C::*)(Args...) const;
            typedef C ClassType;

            AZ_CLASS_ALLOCATOR(BehaviorMethodImpl<R(C::*)(Args...)>, AZ::SystemAllocator, 0);

            static const int s_startArgumentIndex = 1; // +1 for result type
            static const int s_startNamedArgumentIndex = s_startArgumentIndex + 1; // +1 for result type, +1 for class Type (this ptr)

            BehaviorMethodImpl(FunctionPointer functionPointer, BehaviorContext* context, const AZStd::string& name = AZStd::string());
            BehaviorMethodImpl(FunctionPointerConst functionPointer, BehaviorContext* context, const AZStd::string& name = AZStd::string());

            bool Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result) const override;

            bool HasResult() const override;
            bool IsMember() const override;
            bool HasBusId() const override;

            const BehaviorParameter* GetBusIdArgument() const override;

            size_t GetNumArguments() const override;
            size_t GetMinNumberOfArguments() const override;
            const BehaviorParameter* GetArgument(size_t index) const override;
            const AZStd::string* GetArgumentName(size_t index) const override;
            void SetArgumentName(size_t index, const AZStd::string& name) override;
            const AZStd::string* GetArgumentToolTip(size_t index) const override;
            void SetArgumentToolTip(size_t index, const AZStd::string& name) override;
            void SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue) override;
            BehaviorDefaultValuePtr GetDefaultValue(size_t index) const override;
            const BehaviorParameter* GetResult() const override;

            void OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits) override;

            FunctionPointer m_functionPtr;
            BehaviorParameter m_parameters[sizeof...(Args)+s_startNamedArgumentIndex];
            AZStd::array<BehaviorParameterMetadata, sizeof...(Args)+s_startNamedArgumentIndex> m_metadataParameters; ///< Stores the per parameter metadata which is used to add names, tooltips, trait, default values, etc... to the parameters
        };

 #if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must be distinguished from non-noexcept overloads
        template<class R, class C, class... Args>
        class BehaviorMethodImpl<R(C::*)(Args...) noexcept>
            : public BehaviorMethodImpl<R(C::*)(Args...)>
        {
            using base_type = BehaviorMethodImpl<R(C::*)(Args...)>;
        public:
            using base_type::base_type;
            using FunctionPointer = R(C::*)(Args...) noexcept;
            using FunctionPointerConst = R(C::*)(Args...) const noexcept;
        };
#endif

        enum BehaviorEventType
        {
            BE_BROADCAST,
            BE_EVENT_ID,
            BE_QUEUE_BROADCAST,
            BE_QUEUE_EVENT_ID,
        };

        template<BehaviorEventType EventType, class EBus, class R, class... Args>
        struct EBusCaller;

        template<class EBus, class... Args>
        struct EBusCaller<BE_BROADCAST, EBus, void, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result; (void)arguments;
                EBus::Broadcast(e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        template<class EBus, class R, class... Args>
        struct EBusCaller<BE_BROADCAST, EBus, R, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)arguments;
                if (result)
                {
                    EBus::BroadcastResult(*result, e, *arguments[Is].GetAsUnsafe<Args>()...);
                }
                else
                {
                    EBus::Broadcast(e, *arguments[Is].GetAsUnsafe<Args>()...);
                }
            }
        };

        template<class EBus, class... Args>
        struct EBusCaller<BE_EVENT_ID, EBus, void, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result;
                BehaviorValueParameter& id = arguments[0];
                ++arguments;
                EBus::Event(*id.GetAsUnsafe<typename EBus::BusIdType>(), e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        template<class EBus, class R, class... Args>
        struct EBusCaller<BE_EVENT_ID, EBus, R, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                BehaviorValueParameter& id = *arguments++;
                if (result)
                {
                    EBus::EventResult(*result, *id.GetAsUnsafe<typename EBus::BusIdType>(), e, *arguments[Is].GetAsUnsafe<Args>()...);
                }
                else
                {
                    EBus::Event(*id.GetAsUnsafe<typename EBus::BusIdType>(), e, *arguments[Is].GetAsUnsafe<Args>()...);
                }
            }
        };

        template<class EBus, class R, class... Args>
        struct EBusCaller<BE_QUEUE_BROADCAST, EBus, R, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result; (void)arguments;
                EBus::QueueBroadcast(e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        template<class EBus, class R, class... Args>
        struct EBusCaller<BE_QUEUE_EVENT_ID, EBus, R, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result;
                BehaviorValueParameter& id = *arguments++;
                EBus::QueueEvent(*id.GetAsUnsafe<typename EBus::BusIdType>(), e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        template<class EBus, BehaviorEventType EventType, class Function>
        class BehaviorEBusEvent;

        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        class BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)> : public BehaviorMethod
        {
        public:
            using FunctionPointer = R(C::*)(Args...);
            using FunctionPointerConst = R(C::*)(Args...) const;

            static constexpr int s_isBusIdParameter = (EventType == BE_EVENT_ID || EventType == BE_QUEUE_EVENT_ID) ? 1 : 0;
            static const int s_startArgumentIndex = 1; // +1 for result type
            static const int s_startNamedArgumentIndex = s_startArgumentIndex + s_isBusIdParameter; // +1 for result type, +1 (optional for busID)

            AZ_CLASS_ALLOCATOR(BehaviorEBusEvent, AZ::SystemAllocator, 0);

            BehaviorEBusEvent(FunctionPointer functionPointer, BehaviorContext* context);
            BehaviorEBusEvent(FunctionPointerConst functionPointer, BehaviorContext* context);

            template<bool IsBusId>
            inline AZStd::enable_if_t<IsBusId> SetBusIdType();

            template<bool IsBusId>
            inline AZStd::enable_if_t<!IsBusId> SetBusIdType();

            bool Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result) const override;
            bool HasResult() const override;
            bool IsMember() const override;
            bool HasBusId() const override;

            const BehaviorParameter* GetBusIdArgument() const override;

            size_t GetNumArguments() const override;
            size_t GetMinNumberOfArguments() const override;

            const BehaviorParameter* GetArgument(size_t index) const override;
            const AZStd::string* GetArgumentName(size_t index) const override;
            void SetArgumentName(size_t index, const AZStd::string& name) override;
            const AZStd::string* GetArgumentToolTip(size_t index) const override;
            void SetArgumentToolTip(size_t index, const AZStd::string& name) override;
            void SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue) override;
            BehaviorDefaultValuePtr GetDefaultValue(size_t index) const override;
            const BehaviorParameter* GetResult() const override;

            void OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits) override;

            FunctionPointer m_functionPtr;
            BehaviorParameter m_parameters[sizeof...(Args)+s_startNamedArgumentIndex];
            AZStd::array<BehaviorParameterMetadata, sizeof...(Args)+s_startNamedArgumentIndex> m_metadataParameters; ///< Stores the per parameter metadata which is used to add names, tooltips, trait, default values, etc... to the parameters
        };

#if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must be distinguished from non-noexcept overloads
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        class BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...) noexcept>
            : public BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>
        {
            using base_type = BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>;
        public:
            using base_type::base_type;
            using FunctionPointer = R(C::*)(Args...) noexcept;
            using FunctionPointerConst = R(C::*)(Args...) const noexcept;
        };
#endif

        template<class F>
        struct SetFunctionParameters;

        template<class R, class... Args>
        struct SetFunctionParameters<R(Args...)>
        {
            static void Set(AZStd::vector<BehaviorParameter>& params);
            static bool Check(AZStd::vector<BehaviorParameter>& source);
        };

#if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must be distinguished from non-noexcept overloads
        template<class R, class... Args>
        struct SetFunctionParameters<R(Args...) noexcept>
            : SetFunctionParameters<R(Args...)>
        {};
#endif
        template<class R, class C, class... Args>
        struct SetFunctionParameters<R(C::*)(Args...)>
        {
            static void Set(AZStd::vector<BehaviorParameter>& params);
            static bool Check(AZStd::vector<BehaviorParameter>& source);
        };

#if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must be distinguished from non-noexcept overloads
        template<class R, class C, class... Args>
        struct SetFunctionParameters<R(C::*)(Args...) noexcept>
            : SetFunctionParameters<R(C::*)(Args...)>
        {};
#endif

        template<class FunctionType>
        struct BehaviorOnDemandReflectHelper;
        template<class R, class... Args>
        struct BehaviorOnDemandReflectHelper<R(*)(Args...)>
        {
            static void QueueReflect(OnDemandReflectionOwner* onDemandReflection)
            {
                if (onDemandReflection)
                {
                    static constexpr size_t c_functionParameterAndReturnSize = sizeof...(Args) + 1;
                    // deal with OnDemand reflection
                    AZ::Uuid argumentTypes[c_functionParameterAndReturnSize] = { AzTypeInfo<R>::Uuid(), (AzTypeInfo<Args>::Uuid())... };
                    StaticReflectionFunctionPtr reflectHooks[c_functionParameterAndReturnSize] = { OnDemandReflectHook<AZStd::remove_pointer_t<AZStd::decay_t<R>>>::Get(),
                        (OnDemandReflectHook<AZStd::remove_pointer_t<AZStd::decay_t<Args>>>::Get())... };
                    for (size_t i = 0; i < c_functionParameterAndReturnSize; ++i)
                    {
                        if (reflectHooks[i])
                        {
                            onDemandReflection->AddReflectFunction(argumentTypes[i], reflectHooks[i]);
                        }
                    }
                }
            }
        };

#if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must be distinguished from non-noexcept overloads
        template<class R, class... Args>
        struct BehaviorOnDemandReflectHelper<R(*)(Args...) noexcept>
            : BehaviorOnDemandReflectHelper<R(*)(Args...)>
        {};
#endif

        template<class... Functions>
        void OnDemandReflectFunctions(OnDemandReflectionOwner* onDemandReflection, AZStd::Internal::pack_traits_arg_sequence<Functions...>);

        template<class T>
        struct BahaviorDefaultFactory
        {
            static void* Create(void* inplaceAddress, void* userData);
            static void Destroy(void* objectPtr, bool isFreeMemory, void* userData);
            static void* Clone(void* targetAddress, void* sourceAddress, void* userData);
        };

        template<class Handler>
        struct BehaviorEBusHandlerFactory
        {
            static BehaviorEBusHandler* Create();
            static void Destroy(BehaviorEBusHandler* handler);
        };

        template<class... Values>
        class BehaviorValuesSpecialization : public BehaviorValues
        {
        public:
            AZ_CLASS_ALLOCATOR(BehaviorValuesSpecialization, AZ::SystemAllocator, 0);

            template<class LastValue>
            inline void SetValues(LastValue&& value)
            {
                m_values[sizeof...(Values)-1] = aznew BehaviorDefaultValue(AZStd::forward<LastValue>(value));
            }

            template<class CurrentValue, class... RestValues>
            inline void SetValues(CurrentValue&& value, RestValues&&... values)
            {
                m_values[sizeof...(Values)-sizeof...(RestValues)-1] = aznew BehaviorDefaultValue(AZStd::forward<CurrentValue>(value));
                SetValues(AZStd::forward<RestValues>(values)...);
            };

            BehaviorValuesSpecialization(Values&&... values)
            {
                SetValues(AZStd::forward<Values>(values)...);
            }

            size_t GetNumValues() const override
            {
                return sizeof...(Values);
            }

            BehaviorDefaultValuePtr GetDefaultValue(size_t i) const override
            {
                AZ_Assert(i < sizeof...(Values), "Invalid value index!");
                return m_values[i];
            }

            BehaviorDefaultValuePtr m_values[sizeof...(Values)];
        };

        template<class Owner>
        class GenericAttributes
        {
        protected:
            template<class T>
            void SetAttributeContextData(T value, AZ::Attribute* attribute, const AZStd::false_type& /* is_function<remove_pointer<T>::type> && is_member_function_pointer<T>*/)
            {
                (void)value; (void)attribute;
            }

            template<class T>
            void SetAttributeContextData(T value, AZ::Attribute* attribute, const AZStd::true_type& /* is_function<remove_pointer<T>::type> && is_member_function_pointer<T>*/);

            GenericAttributes(BehaviorContext* context)
                : m_currentAttributes(nullptr)
                , m_context(context)
            {
            }
        public:

            /**
            * All T (attribute value) MUST be copy constructible as they are stored in internal
            * AttributeContainer<T>, which can be accessed by azrtti and AttributeData.
            * Attributes can be assigned to either classes or DataElements.
            */
            template<class T>
            Owner* Attribute(const char* id, T value)
            {
                return Attribute(Crc32(id), value);
            }

            /**
            * All T (attribute value) MUST be copy constructible as they are stored in internal
            * AttributeContainer<T>, which can be accessed by azrtti and AttributeData.
            * Attributes can be assigned to either classes or DataElements.
            */
            template<class T>
            Owner* Attribute(Crc32 idCrc, T value);

            AttributeArray* m_currentAttributes;
            BehaviorContext* m_context;
        };

        template<typename Bus>
        class EBusAttributes;

    } // namespace Internal

    /**
     * Internal helpers to bind fields to properties.
     * See: \ref BehaviorValueGetter, \ref BehaviorValueSetter, and \ref BehaviorValueProperty
     */
    namespace Internal
    {
        template<class T>
        struct BehaviorValuePropertyHelper;

        template<class T>
        struct BehaviorValuePropertyHelper<T*>
        {
            template<T* Value>
            static T& Get()
            {
                return *Value;
            }

            template<T* Value>
            static void Set(T v)
            {
                *Value = v;
            }
        };

        template<class T, class C>
        struct BehaviorValuePropertyHelper<T C::*>
        {
            template<T C::* Member>
            static T& Get(C* thisPtr)
            {
                AZ_Assert(thisPtr, "null thisPtr in BehaviorValuePropertyHelper::Get, clients must check for must check for TR_THIS_PTR trait and a valid value address before member functions");
                return thisPtr->*Member;
            }

            template<T C::* Member>
            static void Set(C* thisPtr, const T& v)
            {
                AZ_Assert(thisPtr, "null thisPtr in BehaviorValuePropertyHelper::Set, clients must check for TR_THIS_PTR trait and a valid value address before member functions");
                thisPtr->*Member = v;
            }
        };
    } // namespace Internal

    /**
     * Behavior representation of reflected class.
     */
    class BehaviorClass
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorClass, SystemAllocator, 0);

        BehaviorClass();
        ~BehaviorClass();

        /// Hooks to override default memory allocation for the class (AZ_CLASS_ALLOCATOR is used by default)
        using AllocateType = void*(*)(void* userData);
        using DeallocateType = void(*)(void* address, void* userData);
        /// Default constructor and destructor custom function
        using DefaultConstructorType = void(*)(void* address, void* userData);
        using DestructorType = void(*)(void* objectPtr, void* userData);
        /// Clone object over an existing address
        using CopyContructorType = void(*)(void* address, const void* sourceObjectPtr, void* userData);
        /// Move object over an existing address
        using MoveContructorType = void(*)(void* address, void* sourceObjectPtr, void* userData);
        /// Hash a value of a class
        using ValueHasherType = AZStd::function<size_t(void*)>;
        /// Compare values
        using EqualityComparisonType = bool(*)(const void* lhs, const void* rhs, void* userData);

        // Create the object with default constructor if possible otherwise returns an invalid object
        BehaviorObject Create() const;

        // Create the object with default constructor in the provided memory if possible otherwise returns an invalid object
        BehaviorObject Create(void* address) const;

        // TODO: Should we do that or just ask users to Allocate and invoke the constructor manually ?
        //template<class... Args>
        //BehaviorObject Create();

        BehaviorObject Clone(const BehaviorObject& object) const;
        BehaviorObject Move(BehaviorObject&& object) const;

        void Destroy(const BehaviorObject& object) const;

        /// Allocate a class, NO CONSTRUCTOR is called, only memory is allocated, call \ref Construct or use Create to allocate and create the object
        void* Allocate() const;
        /// Deallocate a class, NO DESTRUCTOR is called, only memory is free, call \ref Destruct or use Destroy to destroy and free the object
        void  Deallocate(void* address) const;

        AZ::Attribute* FindAttribute(const AttributeId& attributeId) const;
        bool HasAttribute(const AttributeId& attributeId) const;

        AllocateType m_allocate;
        DeallocateType m_deallocate;
        DefaultConstructorType m_defaultConstructor;
        AZStd::vector<BehaviorMethod*> m_constructors; // signatures are (address, Params...)
        DestructorType m_destructor;
        CopyContructorType m_cloner;
        MoveContructorType m_mover;
        EqualityComparisonType m_equalityComparer;


        const BehaviorMethod* FindMethodByReflectedName(const AZStd::string& reflectedName) const;
        bool IsMethodOverloaded(const AZStd::string& string) const;
        bool IsMethodOverloaded(BehaviorMethod* method) const;
        AZStd::vector<BehaviorMethod*> GetOverloads(const AZStd::string& string) const;
        AZStd::vector<BehaviorMethod*> GetOverloadsIncludeMethod(BehaviorMethod* method) const;
        AZStd::vector<BehaviorMethod*> GetOverloadsExcludeMethod(BehaviorMethod* method) const;
        void PostProcessMethod(BehaviorContext* context, BehaviorMethod& method);

        void* m_userData;
        AZStd::string m_name;
        AZStd::vector<AZ::Uuid> m_baseClasses;
        AZStd::unordered_map<AZStd::string, BehaviorMethod*> m_methods;
        AZStd::unordered_map<AZStd::string, BehaviorProperty*> m_properties;
        AttributeArray m_attributes;
        AZStd::unordered_set<AZStd::string> m_requestBuses;
        AZStd::unordered_set<AZStd::string> m_notificationBuses;
        AZ::Uuid m_typeId;
        IRttiHelper* m_azRtti;
        size_t m_alignment;
        size_t m_size;
        BehaviorClassUnwrapperFunction m_unwrapper;
        ValueHasherType m_valueHasher;
        void* m_unwrapperUserData;
        AZ::Uuid m_wrappedTypeId;
        // Store all owned instances for unload verification?
    };

    // Helper macros to generate getter and setter function from a pointer to value or member value
    // Syntax BehaviorValueGetter(&globalValue) BehaviorValueGetter(&Class::MemberValue)
#   define BehaviorValueGetter(valueAddress) &AZ::Internal::BehaviorValuePropertyHelper<decltype(valueAddress)>::Get<valueAddress>
#   define BehaviorValueSetter(valueAddress) &AZ::Internal::BehaviorValuePropertyHelper<decltype(valueAddress)>::Set<valueAddress>
#   define BehaviorValueProperty(valueAddress) BehaviorValueGetter(valueAddress),BehaviorValueSetter(valueAddress)

    // Constant helper
#   define BehaviorConstant(value) []() { return value; }

    /**
     * Property representation, a property has getter and setter. A read only property will have a "nullptr" for a setter.
     * You can use lambdas, global of member function. If you want to just expose a variable (not write the function and handle changes)
     * you can use \ref BehaviorValueProperty macros (or BehaviorValueGetter/Setter to control read/write functionality)
     * Member constants are a property too, use \ref BehaviorConstant for it. Everything is either a property or a method, the main reason
     * why we "push" people to use functions is that in most cases when we manipulate an object, you will need to do more than just set a value
     * to a new value.
     */
    class BehaviorProperty
        : public OnDemandReflectionOwner
    {
        template<class Getter>
        bool SetGetter(Getter, BehaviorClass* /*currentClass*/, BehaviorContext* context, const AZStd::true_type& /* is AZStd::is_same<Getter,nullptr_t>::type() */);

        template<class Getter>
        bool SetGetter(Getter getter, BehaviorClass* currentClass, BehaviorContext* context, const AZStd::false_type& /* is AZStd::is_same<Getter,nullptr_t>::type( */);

        template<class Setter>
        bool SetSetter(Setter, BehaviorClass*, BehaviorContext*, const AZStd::true_type& /* is AZStd::is_same<Getter,nullptr_t>::type() */);

        template<class Setter>
        bool SetSetter(Setter setter, BehaviorClass* currentClass, BehaviorContext* context, const AZStd::false_type& /* is AZStd::is_same<Getter,nullptr_t>::type( */);

    public:
        AZ_CLASS_ALLOCATOR(BehaviorProperty, AZ::SystemAllocator, 0);

        BehaviorProperty(BehaviorContext* context);
        ~BehaviorProperty() override;

        template<class Getter, class Setter>
        bool Set(Getter getter, Setter setter, BehaviorClass* currentClass, BehaviorContext* context);

        const AZ::Uuid& GetTypeId() const;

        AZStd::string m_name;
        BehaviorMethod* m_getter;
        BehaviorMethod* m_setter;
        AttributeArray m_attributes;
    };

    struct BehaviorEBusEventSender
    {
        template<class EBus, class Event>
        void Set(Event e, const char* eventName, BehaviorContext* context);

        template<class EBus, class Event>
        void SetEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::true_type& /*is NullBusId*/);

        template<class EBus, class Event>
        void SetEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/);

        template<class EBus, class Event>
        void SetQueueBroadcast(Event e, const char* eventName, BehaviorContext* context, const AZStd::true_type& /*is NullBusId*/);

        template<class EBus, class Event>
        void SetQueueBroadcast(Event e, const char* eventName, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/);

        template<class EBus, class Event>
        void SetQueueEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::true_type& /* is Queue and is BusId valid*/);

        template<class EBus, class Event>
        void SetQueueEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::false_type& /* is Queue and is BusId valid*/);

        BehaviorMethod* m_broadcast = nullptr;
        BehaviorMethod* m_event = nullptr;
        BehaviorMethod* m_queueBroadcast = nullptr;
        BehaviorMethod* m_queueEvent = nullptr;
        AZStd::string m_deprecatedName;
        AttributeArray m_attributes;
    };

    /**
     * RAII class which keeps track of functions reflected to the BehaviorContext
     * when it is supplied as an OnDemandReflectionOwner
     */
    class ScopedBehaviorOnDemandReflector
        : public OnDemandReflectionOwner
    {
    public:
        ScopedBehaviorOnDemandReflector(BehaviorContext& behaviorContext);
    };

    /**
     * EBus behavior wrapper.
     */
    class BehaviorEBus
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorEBus, SystemAllocator, 0);

        typedef void(*QueueFunctionType)(void* /*userData1*/, void* /*userData2*/);

        struct VirtualProperty
        {
            VirtualProperty(BehaviorEBusEventSender* getter, BehaviorEBusEventSender* setter)
                : m_getter(getter)
                , m_setter(setter)
            {}

            BehaviorEBusEventSender* m_getter;
            BehaviorEBusEventSender* m_setter;
        };


        BehaviorEBus();
        ~BehaviorEBus();

        BehaviorMethod* m_createHandler;
        BehaviorMethod* m_destroyHandler;

        AZStd::string m_name;
        AZStd::string m_deprecatedName;
        AZStd::string m_toolTip;
        BehaviorMethod* m_queueFunction;
        BehaviorParameter m_idParam; /// Invalid if bus doesn't have ID (you can check the typeId for invalid)
        BehaviorMethod* m_getCurrentId; ///< Method that returns current ID of the message, null if this EBus has not ID.
        AZStd::unordered_map<AZStd::string, BehaviorEBusEventSender> m_events;
        AZStd::unordered_map<AZStd::string, VirtualProperty> m_virtualProperties;
        AttributeArray m_attributes;

        AZStd::unique_ptr<ScopedBehaviorOnDemandReflector> m_ebusHandlerOnDemandReflector; /// Keep track of OnDemandReflections for EBusHandler functions
    };

    enum eBehaviorBusForwarderEventIndices
    {
        Result,
        UserData,
        ParameterFirst,
        Count
    };

    class BehaviorEBusHandler
    {
    public:
        AZ_RTTI(BehaviorEBusHandler, "{10fbcb9d-8a0d-47e9-8a51-cbd9bfbbf60d}");

        // Since we can share hooks we should probably pass the event name
        typedef void(*GenericHookType)(void* /*userData*/, const char* /*eventName*/, int /*eventIndex*/, BehaviorValueParameter* /*result*/, int /*numParameters*/, BehaviorValueParameter* /*parameters*/);

        struct BusForwarderEvent
        {
            BusForwarderEvent()
                : m_name(nullptr)
                , m_function(nullptr)
                , m_isFunctionGeneric(false)
                , m_userData(nullptr)
            {
            }

            const char* m_name;
            AZ::Crc32   m_eventId;
            void* m_function; ///< Pointer user handler R Function(userData, Args...)
            bool m_isFunctionGeneric;
            void* m_userData;

            /// \note, even if this function returns no result, the first parameter is STILL space for the result
            bool HasResult() const;

            AZStd::vector<BehaviorParameter> m_parameters; // result, userdata, arguments...
            AZStd::vector<BehaviorParameterMetadata> m_metadataParameters; //< Custom Metadata for the parameter. Contains Names and Tooltips for each parameter.
                                                           // Not consolidated with the above vector, because existing internal functions expect BehaviorParameters to be laid out contiguously
        };

        typedef AZStd::vector<BusForwarderEvent> EventArray;

        BehaviorEBusHandler() {}

        virtual ~BehaviorEBusHandler() {}

        virtual int GetFunctionIndex(const char* name) const = 0;

        template<class BusId>
        bool Connect(BusId id)
        {
            BehaviorValueParameter p(&id);
            return Connect(&p);
        }

        virtual bool Connect(BehaviorValueParameter* id = nullptr) = 0;

        virtual void Disconnect() = 0;

        virtual bool IsConnected() = 0;
        virtual bool IsConnectedId(BehaviorValueParameter* id) = 0;

        //const AZ::Uuid* GetIdTypeId() const = 0;

        template<class Hook>
        bool InstallHook(int index, Hook h, void* userData = nullptr);

        template<class Hook>
        bool InstallHook(const char* name, Hook h, void* userData = nullptr);

        bool InstallGenericHook(int index, GenericHookType hook, void* userData = nullptr);

        bool InstallGenericHook(const char* name, GenericHookType hook, void* userData = nullptr);

        const EventArray& GetEvents() const;

#if !defined(_RELEASE) // m_scriptPath is only available in non-Release mode
        AZStd::string m_scriptPath;
#endif

        AZStd::string GetScriptPath() const
        {
#if !defined(_RELEASE) // m_scriptPath is only available in non-Release mode
            return m_scriptPath;
#else
            return{};
#endif
        }

        void SetScriptPath(const char* scriptPath)
        {
#if !defined(_RELEASE) // m_scriptPath is only available in non-Release mode
            m_scriptPath = scriptPath;
#else
            AZ_UNUSED(scriptPath);
#endif
        }

    protected:
        template<class Event>
        void SetEvent(Event e, const char* name);

        template<class Event>
        void SetEvent(Event e, AZStd::string_view name, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Event>::num_args>& args);

        template<class... Args>
        void Call(int index, Args&&... args) const;

        template<class R, class... Args>
        void CallResult(R& result, int index, Args&&... args) const;

        EventArray m_events;
    };

    // Behavior context events you can listen for
    class BehaviorContextEvents : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusInterface settings
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef BehaviorContext* BusIdType;
        //////////////////////////////////////////////////////////////////////////

        /// Called when a new global method is reflected in behavior context or removed from it
        virtual void OnAddGlobalMethod(const char* methodName, BehaviorMethod* method)      { (void)methodName; (void)method; }
        virtual void OnRemoveGlobalMethod(const char* methodName, BehaviorMethod* method)   { (void)methodName; (void)method; }

        /// Called when a new global property is reflected in behavior context or remove from it
        virtual void OnAddGlobalProperty(const char* propertyName, BehaviorProperty* prop)  { (void)propertyName; (void)prop; }
        virtual void OnRemoveGlobalProperty(const char* propertyName, BehaviorProperty* prop) { (void)propertyName; (void)prop; }

        /// Called when a class is added or removed
        virtual void OnAddClass(const char* className, BehaviorClass* behaviorClass)    { (void)className; (void)behaviorClass; }
        virtual void OnRemoveClass(const char* className, BehaviorClass* behaviorClass) { (void)className; (void)behaviorClass; }

        /// Called when a ebus is added or removed
        virtual void OnAddEBus(const char* ebusName, BehaviorEBus* ebus)    { (void)ebusName; (void)ebus; }
        virtual void OnRemoveEBus(const char* ebusName, BehaviorEBus* ebus) { (void)ebusName; (void)ebus; }
    };

    using BehaviorContextBus = AZ::EBus<BehaviorContextEvents>;

    /**
     * BehaviorContext is used to reflect classes, methods and EBuses for runtime interaction. A typical consumer of this context and different
     * scripting systems (i.e. Lua, Visual Script, etc.). Even though (as designed) there are overlaps between some context they have very different
     * purpose and set of rules. For example SerializeContext, doesn't reflect any methods, it just reflects data fields that will be stored for initial object
     * setup, it handles version conversion and so thing, this related to storing the object to a persistent storage. Behavior context, doesn't need to deal with versions as
     * no data is stored, just methods for manipulating the object state.
     */
    class BehaviorContext : public ReflectContext
    {
        template<class Bus, typename AZStd::enable_if<!AZStd::is_same<typename Bus::BusIdType, AZ::NullBusId>::value>::type* = nullptr>
        void EBusSetIdFeatures(BehaviorEBus* ebus)
        {
            AZ::Internal::SetParameters<typename Bus::BusIdType>(&ebus->m_idParam);
            ebus->m_getCurrentId = aznew AZ::Internal::BehaviorMethodImpl<const typename Bus::BusIdType*()>(&Bus::GetCurrentBusId, this, AZStd::string(Bus::GetName()) + "::GetCurrentBusId");
        }

        template<class Bus, typename AZStd::enable_if<AZStd::is_same<typename Bus::BusIdType, AZ::NullBusId>::value>::type* = nullptr>
        void EBusSetIdFeatures(BehaviorEBus*)
        {
        }

        template<class Bus>
        static void QueueFunction(BehaviorEBus::QueueFunctionType f, void* userData1, void* userData2)
        {
            Bus::QueueFunction(f, userData1, userData2);
        }

        template<class Bus, typename AZStd::enable_if<Bus::Traits::EnableEventQueue>::type* = nullptr>
        BehaviorMethod* QueueFunctionMethod()
        {
            return aznew AZ::Internal::BehaviorMethodImpl<void(BehaviorEBus::QueueFunctionType, void*, void*)>(&QueueFunction<Bus>, this, AZStd::string(Bus::GetName()) + "::QueueFunction");
        }

        template<class Bus, typename AZStd::enable_if<!Bus::Traits::EnableEventQueue>::type* = nullptr>
        BehaviorMethod* QueueFunctionMethod()
        {
            return nullptr;
        }

        /// Helper struct to call the default class allocator \ref AZ_CLASS_ALLOCATOR
        template<class T>
        struct DefaultAllocator
        {
            static void* Allocate(void* userData)
            {
                (void)userData;
                return T::AZ_CLASS_ALLOCATOR_Allocate();
            }

            static void DeAllocate(void* address, void* userData)
            {
                (void)userData;
                T::AZ_CLASS_ALLOCATOR_DeAllocate(address);
            }
        };

        /// Helper struct to pipe allocation to the system allocator
        template<class T>
        struct DefaultSystemAllocator
        {
            static void* Allocate(void* userData)
            {
                (void)userData;
                return azmalloc(sizeof(T), AZStd::alignment_of<T>::value, AZ::SystemAllocator, AZ::AzTypeInfo<T>::Name());
            }

            static void DeAllocate(void* address, void* userData)
            {
                (void)userData;
                azfree(address, AZ::SystemAllocator, sizeof(T), AZStd::alignment_of<T>::value);
            }
        };

        /// Helper function to call default constructor
        template<class T>
        static void DefaultConstruct(void* address, void* userData)
        {
            (void)userData;
            new(address) T();
        }

        /// Helper function to call generic constructor
        template<class T, class... Params>
        static void Construct(T* address, Params... params)
        {
            new(address) T(params...);
        }

        /// Helper function to destroy an object
        template<class T>
        static void DefaultDestruct(void* object, void* userData)
        {
            (void)userData; (void)object;
            reinterpret_cast<T*>(object)->~T();
        }

        /// Helper functor to default copy construct
        template<class T>
        static void DefaultCopyConstruct(void* address, const void* sourceObject, void* userData)
        {
            (void)userData;
            new(address) T(*reinterpret_cast<const T*>(sourceObject));
        }
        /// Helper functor to default copy construct
        template<class T>
        static bool DefaultEqualityComparer(const void* lhs, const void* rhs, void* userData)
        {
            (void)userData;
            if (lhs && rhs)
            {
                return *reinterpret_cast<const T*>(lhs) == *reinterpret_cast<const T*>(rhs);
            }
            else
            {
                return lhs == rhs;
            }
        }

        /// Helper functor to default move construct
        template<class T>
        static void DefaultMoveConstruct(void* address, void* sourceObject, void* userData)
        {
            (void)userData;
            new(address) T(AZStd::move(*reinterpret_cast<T*>(sourceObject)));
        }

        template<class T>
        static void SetClassDefaultAllocator(BehaviorClass* behaviorClass, const AZStd::false_type& /*HasAZClassAllocator<T>*/)
        {
            behaviorClass->m_allocate = &DefaultSystemAllocator<T>::Allocate;
            behaviorClass->m_deallocate = &DefaultSystemAllocator<T>::DeAllocate;
        }

        template<class T>
        static void SetClassDefaultConstructor(BehaviorClass* behaviorClass, const AZStd::false_type& /*AZStd::is_constructible<T>*/) { (void)behaviorClass; }

        template<class T>
        static void SetClassDefaultDestructor(BehaviorClass* behaviorClass, const AZStd::false_type& /*AZStd::is_destructible<T>*/) { (void)behaviorClass; }

        template<class T>
        static void SetClassDefaultCopyConstructor(BehaviorClass* behaviorClass, const AZStd::false_type& /*AZStd::is_copy_constructible<T>*/) { (void)behaviorClass; }

        template<class T>
        static void SetClassDefaultMoveConstructor(BehaviorClass* behaviorClass, const AZStd::false_type& /*AZStd::is_move_constructible<T>*/) { (void)behaviorClass; }

        template<class T>
        static void SetClassDefaultAllocator(BehaviorClass* behaviorClass, const AZStd::true_type&  /*HasAZClassAllocator<T>*/)
        {
            behaviorClass->m_allocate = &DefaultAllocator<T>::Allocate;
            behaviorClass->m_deallocate = &DefaultAllocator<T>::DeAllocate;
        }

        template <class T>
        static void SetClassHasher(BehaviorClass* behaviorClass)
        {
            if constexpr (AZStd::HasherInvocable_v<T>)
            {
                behaviorClass->m_valueHasher = [](void* value) -> AZStd::size_t
                {
                    return AZStd::hash<T>()(*static_cast<T*>(value));
                };
            }
        }

        template<class T>
        static void SetClassDefaultConstructor(BehaviorClass* behaviorClass, const AZStd::true_type& /*AZStd::is_constructible<T>*/)
        {
            behaviorClass->m_defaultConstructor = &DefaultConstruct<T>;
        }

        template<class T>
        static void SetClassDefaultDestructor(BehaviorClass* behaviorClass, const AZStd::true_type& /*AZStd::is_destructible<T>*/)
            {
                behaviorClass->m_destructor = &DefaultDestruct<T>;
            }

        template<class T>
        static void SetClassDefaultCopyConstructor(BehaviorClass* behaviorClass, const AZStd::true_type& /*AZStd::is_copy_constructible<T>*/)
        {
            behaviorClass->m_cloner = &DefaultCopyConstruct<T>;
        }

        template<class T>
        static bool SetClassEqualityComparer(BehaviorClass* behaviorClass, const T*)
        {
            behaviorClass->m_equalityComparer = &DefaultEqualityComparer<T>;
        }

        template<class T>
        static void SetClassDefaultMoveConstructor(BehaviorClass* behaviorClass, const AZStd::true_type& /*AZStd::is_move_constructible<T>*/)
        {
            behaviorClass->m_mover = &DefaultMoveConstruct<T>;
        }

        template<class ClassType, class WrappedType, class Callable>
        struct WrappedClassCaller
        {
            static void Unwrap(void* classPtr, void*& unwrappedClassPtr, AZ::Uuid& unwrappedClassTypeId, void* userData)
            {
                union
                {
                    void* userData;
                    Callable callablePtr;
                } u;
                u.callablePtr = nullptr;

                u.userData = userData;
                unwrappedClassPtr = (void*)AZStd::invoke(u.callablePtr, reinterpret_cast<ClassType*>(classPtr));
                unwrappedClassTypeId = AzTypeInfo<WrappedType>::Uuid();
            }
        };

    public:
        AZ_CLASS_ALLOCATOR(BehaviorContext, SystemAllocator, 0);
        AZ_RTTI(BehaviorContext, "{ED75FE05-9196-4F69-A3E5-1BDF5FF034CF}", ReflectContext);

        bool IsTypeReflected(AZ::Uuid typeId) const override;

        struct GlobalMethodBuilder : public AZ::Internal::GenericAttributes<GlobalMethodBuilder>
        {
            typedef AZ::Internal::GenericAttributes<GlobalMethodBuilder> Base;
            GlobalMethodBuilder(BehaviorContext* context, const char* methodName, BehaviorMethod* method);
            ~GlobalMethodBuilder();
            GlobalMethodBuilder* operator->();

            const char* m_name;
            BehaviorMethod* m_method;
        };

        struct GlobalPropertyBuilder : public AZ::Internal::GenericAttributes<GlobalPropertyBuilder>
        {
            typedef AZ::Internal::GenericAttributes<GlobalPropertyBuilder> Base;

            GlobalPropertyBuilder(BehaviorContext* context, BehaviorProperty* prop);
            ~GlobalPropertyBuilder();
            GlobalPropertyBuilder* operator->();

            BehaviorProperty* m_prop;
        };

        /// Internal structure to maintain class information while we are describing a class.
        template<class C>
        struct ClassBuilder : public AZ::Internal::GenericAttributes<ClassBuilder<C>>
        {
            typedef AZ::Internal::GenericAttributes<ClassBuilder<C>> Base;

            //////////////////////////////////////////////////////////////////////////
            /// Internal implementation
            ClassBuilder(BehaviorContext* context, BehaviorClass* behaviorClass);
            ~ClassBuilder();
            ClassBuilder* operator->();

            /**
            * Sets custom allocator for a class, this function will error if this not inside a class.
            * This is only for very specific cases when you want to override AZ_CLASS_ALLOCATOR or you are dealing with 3rd party classes, otherwise
            * you should use AZ_CLASS_ALLOCATOR to control which allocator the class uses.
            */
            ClassBuilder* Allocator(BehaviorClass::AllocateType allocate, BehaviorClass::DeallocateType deallocate);

            /// Attaches different constructor signatures to the class.
            template<class... Params>
            ClassBuilder* Constructor();

            /// When your class is a wrapper, like smart pointers, you should use this to describe how to unwrap the class.
            template<class WrappedType>
            ClassBuilder* Wrapping(BehaviorClassUnwrapperFunction unwrapper, void* userData);

            /// Provide a function to unwrap this class (use an underlaying class)
            template<class WrappedType, class Callable>
            ClassBuilder* WrappingMember(Callable callableFunction);

            /// Sets userdata to a class.
            ClassBuilder* UserData(void *userData);

            /// Setup methods
            ///< \deprecated Use "Method(const char* name, Function f, const AZStd::array<ParameterOverrides, AZStd::function_traits<Function>::num_args>& args, const char* dbgDesc = nullptr)" instead
            ///< This method does not support passing in argument names and tooltips nor does it support overriding specific parameter Behavior traits
            template<class Function>
            ClassBuilder* Method(const char* name, Function, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

            ///< \deprecated Use "Method(const char* name, Function f, const char* deprecatedName, const AZStd::array<ParameterOverrides, AZStd::function_traits<Function>::num_args>& args, const char* dbgDesc = nullptr)" instead
            ///< This method does not support passing in argument names and tooltips nor does it support overriding specific parameter Behavior traits
            template<class Function>
            ClassBuilder* Method(const char* name, Function f, const char* deprecatedName, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

            template<class Function>
            ClassBuilder* Method(const char* name, Function f, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args, const char* dbgDesc = nullptr);

            template<class Function>
            ClassBuilder* Method(const char* name, Function f, const char* deprecatedName, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args, const char* dbgDesc = nullptr);

            template<class Getter, class Setter>
            ClassBuilder* Property(const char* name, Getter getter, Setter setter);

            /// All enums are treated as the enum type
            template<auto Value, typename T = decltype(Value)>
            ClassBuilder* Enum(const char* name);

            template<class Getter>
            ClassBuilder* Constant(const char* name, Getter getter);

            /**
             * You can describe buses that this class uses to communicate. Those buses will be used by tools when
             * you need to give developers hints as to what buses this class interacts with.
             * You don't need to reflect all buses that your class uses, just the ones related to
             * class behavior. Please refer to component documentation for more information on
             * the pattern of Request and Notification buses.
             * {@
             */
            ClassBuilder* RequestBus(const char* busName);
            ClassBuilder* NotificationBus(const char* busName);
            // @}


            BehaviorClass* m_class;
        };

        /// Internal structure to maintain EBus information while describing it.
        template<typename Bus>
        struct EBusBuilder : public AZ::Internal::EBusAttributes<Bus>
        {
            typedef AZ::Internal::EBusAttributes<Bus> Base;

            EBusBuilder(BehaviorContext* context, BehaviorEBus* behaviorEBus);
            ~EBusBuilder();
            EBusBuilder* operator->();

            /**
            * Reflects an EBus event, valid only when used with in the context of an EBus reflection.
            * We will automatically add all possible variations (Broadcast,Event,QueueBroadcast and QueueEvent)
            */
            template<class Function>
            EBusBuilder* Event(const char* name, Function f, const char* deprecatedName = nullptr);

            template<class Function>
            EBusBuilder* Event(const char* name, Function f, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args);

            template<class Function>
            EBusBuilder* Event(const char* name, Function f, const char* deprecatedName, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args);

            /**
            * Every \ref EBus has two components, sending and receiving. Sending is reflected via \ref BehaviorContext::Event calls and Handler/Received
            * use handled via the receiver class. This class will receive EBus events and forward them to the behavior context functions.
            * Since we can't write a class (without using a codegen) while reflecting, you will need to implement that class
            * with the help of \ref AZ_EBUS_BEHAVIOR_BINDER. This class in mandated because you can't hook to individual events at the moment.
            * In this version of Handler you can provide a custom function to create and destroy that handler (usually where aznew/delete is not
            * applicable or you have a better pooling schema that our memory allocators already have). For most cases use the function \ref
            * Handler()
            */
            template<typename HandlerType, typename HandlerCreator, typename HandlerDestructor>
            EBusBuilder* Handler(HandlerCreator creator, HandlerDestructor destructor);

            /** Set the Handler/Receiver for ebus evens that will be forwarded to BehaviorFunctions. This is a helper implementation where aznew/delete
            * is ready to called on the handler.
            */
            template<class H>
            EBusBuilder* Handler();

            /**
             * With request buses (please refer to component communication patterns documentation) we ofter have EBus events
             * that represent a getter and a setter for a value. To allow our tools to take advantage of it, you can reflect
             * VirtualProperty to indicate which event is the getter and which is the setter.
             * This function validates that getter event has no argument and a result and setter function has no results and only
             * one argument which is the same type as the result of the getter.
             * \note Make sure you call this function after you have reflected the getter and setter events as it will report an error
             * if we can't find the function
             */
            EBusBuilder* VirtualProperty(const char* name, const char* getterEvent, const char* setterEvent);

            BehaviorEBus* m_ebus;
        };

         BehaviorContext();
        ~BehaviorContext();

        ///< \deprecated Use "Method(const char*, Function, const AZStd::array<ParameterOverrides, AZStd::function_traits<Function>::num_args>&, const char*)" instead
        ///< This method does not support passing in argument names and tooltips nor does it support overriding specific parameter Behavior traits
        template<class Function>
        GlobalMethodBuilder Method(const char* name, Function f, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        ///< \deprecated Use "Method(const char*, Function, const char*, const AZStd::array<ParameterOverrides, AZStd::function_traits<Function>::num_args>&, const char*)" instead
        ///< This method does not support passing in argument names and tooltips nor does it support overriding specific parameter Behavior traits
        template<class Function>
        GlobalMethodBuilder Method(const char* name, Function f, const char* deprecatedName, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        template<class Function>
        GlobalMethodBuilder Method(const char* name, Function f, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args, const char* dbgDesc = nullptr);

        template<class Function>
        GlobalMethodBuilder Method(const char* name, Function f, const char* deprecatedName, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args, const char* dbgDesc = nullptr);

        template<class Getter, class Setter>
        GlobalPropertyBuilder Property(const char* name, Getter getter, Setter setter);

        /// All enums are treated as the enum type
        template<auto Value, typename T = decltype(Value)>
        BehaviorContext* Enum(const char* name);

        template<auto Value, typename T = decltype(Value)>
        GlobalPropertyBuilder EnumProperty(const char* name);

        template<class Getter>
        BehaviorContext* Constant(const char* name, Getter getter);

        template<class Getter>
        GlobalPropertyBuilder ConstantProperty(const char* name, Getter getter);

        template<class T>
        ClassBuilder<T> Class(const char* name = nullptr);

        template<class T>
        EBusBuilder<T> EBus(const char* name, const char *deprecatedName = nullptr, const char *toolTip = nullptr);

        /**
        * Create a default value to be stored with the parameter metadata. Default value are stored by value
        * in a temp storage, so currently there is limit the \ref BehaviorValueParameter temp storage, we
        * can easily change that if it became a problem.
        */
        template<class Value>
        BehaviorDefaultValuePtr MakeDefaultValue(Value&& defaultValue);

        /**
        * Create a container of default values to be used with methods. Default values are stored by value
        * in a temp storage, so currently there is limit the \ref BehaviorValueParameter temp storage, we
        * can easily change that if it became a problem.
        */
        template<class... Values>
        BehaviorValues* MakeDefaultValues(Values&&... values);

        static AZ::Uuid GetVoidTypeId()
        {
            return azrtti_typeid<void>();
        }

        static const AZStd::pair< AZ::Uuid, AZStd::string >& GetVoidTypeNamePair()
        {
            static AZStd::pair< AZ::Uuid, AZStd::string > k_voidTypeNamePair = { azrtti_typeid<void>(), "Void" };
            return k_voidTypeNamePair;
        }

        static bool IsVoidType(const AZ::Uuid& uuid)
        {
            return uuid == GetVoidTypeId();
        }


        AZStd::unordered_map<AZStd::string, BehaviorMethod*> m_methods; // TODO: make it a set and use the name inside method
        AZStd::unordered_map<AZStd::string, BehaviorProperty*> m_properties; // TODO: make it a set and use the name inside property
        AZStd::unordered_map<AZStd::string, BehaviorClass*> m_classes; // TODO: make it a set and use the name inside class
        AZStd::unordered_map<AZ::Uuid, BehaviorClass*> m_typeToClassMap; // TODO: make it a set and use the UUID inside the class
        AZStd::unordered_map<AZStd::string, BehaviorEBus*> m_ebuses; // TODO: make it a set and use the name inside EBus

        AZStd::unordered_set<ExplicitOverloadInfo> m_explicitOverloads;
        AZStd::unordered_map< const BehaviorMethod*, AZStd::pair<const BehaviorMethod*, const BehaviorClass*>> m_checksByOperations;
    };

    //////////////////////////////////////////////////////////////////////////

    namespace BehaviorContextHelper
    {
        template <typename T>
        AZ::BehaviorClass* GetClass(BehaviorContext* behaviorContext)
        {
            return GetClass(behaviorContext, AZ::AzTypeInfo<T>::Uuid());
        }

        bool IsBehaviorClass(BehaviorContext* behaviorContext, const AZ::TypeId& typeID);
        AZ::BehaviorClass* GetClass(BehaviorContext* behaviorContext, const AZ::TypeId& typeID);
        const BehaviorClass* GetClass(const AZStd::string& classNameString);
        const BehaviorClass* GetClass(const AZ::TypeId& typeID);
        AZStd::pair<const BehaviorClass*, BehaviorContext*> GetClassAndContext(const AZ::TypeId& typeID);
        AZ::TypeId GetClassType(const AZStd::string& classNameString);
        bool IsStringParameter(const BehaviorParameter& parameter);
    }

    //////////////////////////////////////////////////////////////////////////

/**
 * Helper MACRO to help you write the EBus handler that you want to reflect to behavior. This is not required, but generally we recommend reflecting all useful
 * buses as this enable people to "script" complex behaviors.
 * You don't have to use this macro to write a Handler, but some people find it useful
 * Here is an example how to use it:
 * class MyEBusBehaviorHandler : public MyEBus::Handler, public AZ::BehaviorEBusHandler
 * {
 * public:
 *      AZ_EBUS_BEHAVIOR_BINDER(MyEBusBehaviorHandler, "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXXXX}",Allocator, OnEvent1, OnEvent2 and so on);
 *      // now you need implementations for those event
 *
 *
 *      int OnEvent1(int data) override
 *      {
 *          // do any conversion of caching of the "data" here and forward this to behavior (often the reason for this is that you can't pass everything to behavior
 *          // plus behavior can't really handle all constructs pointer to pointer, rvalues, etc. as they don't make sense for most script environments
 *          int result = 0; // set the default value for your result if the behavior if there is no implementation
 *          // The AZ_EBUS_BEHAVIOR_BINDER defines FN_EventName for each index. You can also cache it yourself (but it's slower), static int cacheIndex = GetFunctionIndex("OnEvent1"); and use that .
 *          CallResult(result, FN_OnEvent1, data);  // forward to the binding (there can be none, this is why we need to always have properly set result, when there is one)
 *          return result; // return the result like you will in any normal EBus even with result
 *      } *
 *      // handle the other events here
 * };
 *
 */
#define AZ_EBUS_BEHAVIOR_BINDER(_Handler,_Uuid,_Allocator,...)\
    AZ_CLASS_ALLOCATOR(_Handler,_Allocator,0)\
    AZ_RTTI(_Handler,_Uuid,AZ::BehaviorEBusHandler)\
    typedef _Handler ThisType;\
    using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence<AZ_BEHAVIOR_EBUS_1ARG_UNPACKER(AZ_BEHAVIOR_EBUS_EVENT_FUNCTION_TYPE, __VA_ARGS__)>;\
    enum {\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_ENUM, AZ_EBUS_SEQ(__VA_ARGS__))\
        FN_MAX\
    };\
    int GetFunctionIndex(const char* functionName) const override {\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_INDEX, AZ_EBUS_SEQ(__VA_ARGS__))\
        return -1;\
    }\
    void Disconnect() override {\
        _Handler::BusDisconnect();\
    }\
    _Handler(){\
        m_events.resize(FN_MAX);\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_REG_EVENT, AZ_EBUS_SEQ(__VA_ARGS__))\
    }\
    bool Connect(AZ::BehaviorValueParameter* id = nullptr) override {\
        return AZ::Internal::EBusConnector<_Handler>::Connect(this, id);\
    }\
    bool IsConnected() override {\
        return AZ::Internal::EBusConnector<_Handler>::IsConnected(this);\
    }\
    bool IsConnectedId(AZ::BehaviorValueParameter* id) override {\
        return AZ::Internal::EBusConnector<_Handler>::IsConnectedId(this, id);\
    }

#define AZ_EBUS_BEHAVIOR_BINDER_TEMPLATE(_Handler,_TemplateUuid,_Allocator,...)\
    AZ_CLASS_ALLOCATOR(_Handler,_Allocator,0)\
    AZ_RTTI(_TemplateUuid, AZ::BehaviorEBusHandler)\
    typedef _Handler ThisType;\
    using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence<AZ_BEHAVIOR_EBUS_1ARG_UNPACKER(AZ_BEHAVIOR_EBUS_EVENT_FUNCTION_TYPE, __VA_ARGS__)>;\
    enum {\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_ENUM, AZ_EBUS_SEQ(__VA_ARGS__))\
        FN_MAX\
    };\
    int GetFunctionIndex(const char* functionName) const override {\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_INDEX, AZ_EBUS_SEQ(__VA_ARGS__))\
        return -1;\
    }\
    void Disconnect() override {\
        _Handler::BusDisconnect();\
    }\
    _Handler(){\
        m_events.resize(FN_MAX);\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_REG_EVENT, AZ_EBUS_SEQ(__VA_ARGS__))\
    }\
    bool Connect(AZ::BehaviorValueParameter* id = nullptr) override {\
        return AZ::Internal::EBusConnector<_Handler>::Connect(this, id);\
    }\
    bool IsConnected() override {\
        return AZ::Internal::EBusConnector<_Handler>::IsConnected(this);\
    }\
    bool IsConnectedId(AZ::BehaviorValueParameter* id) override {\
        return AZ::Internal::EBusConnector<_Handler>::IsConnectedId(this, id);\
    }

 /**
 * Provides the same functionality of the AZ_EBUS_BEHAVIOR_BINDER macro above with the additional ability to specify the names and a tooltips of handler methods
 * after listing the handler method in the macro.
 * An example Usage is
 * class MyEBusBehaviorHandler : public MyEBus::Handler, public AZ::BehaviorEBusHandler
 * {
 * public:
 *      AZ_EBUS_BEHAVIOR_BINDER_WITH_DOC(MyEBusBehaviorHandler, "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXXXX}",Allocator, OnEvent1, ({#OnEvent1 first parameter name(int), #OnEvent1 first parameter tooltip(int)}),
 *            OnEvent2, ({#OnEvent2 first parameter name(float), #OnEvent2 first parameter tooltip(float)}, {#OnEvent2 second parameter name(bool), {#OnEvent2 second parameter tooltip(bool)}),
 *            OnEvent3, ());
 *      // The reason for needing parenthesis around the parameter name and tooltip object(AZ::BehaviorParameterOverrides) is to prevent the macro from parsing the comma in the intializer as seperate parameters
 *      // When using this macro, the BehaviorParameterOverrides objects must be placed after every listing a function as a handler. Furthermore the number of BehaviorParameterOverrides objects for each function must match the number of parameters
 *      // to that function
 *      // Ex. for a function called HugeEvent with a signature of void HugeEvent(int, float, double, char, short), two arguments must be supplied to the macro.
 *      // 1. HugeEvent
 *      // 2. ({<BehaviorParameterOverrides>}, {<BehaviorParameterOverrides>}, {<BehaviorParameterOverrides>}, {<BehaviorParameterOverrides>}, {<BehaviorParameterOverrides>}) - Note Parenthesis here. Also note that because HugeEvent takes 5 arguments, 5 pairs of name and tooltips are required
 *      // If a function accepts no arguments such as void NoArgEvent(), the two arguments are
 *      // 1. NoArgEvent
 *      // 2. () - Parenthesis are required for the empty argument to parsed by the macro
 *
 *
 *      int OnEvent1(int data) override
 *      {
 *          // Handle event and return
 *      }
 *      int OnEvent2(float, bool) override
 *      {
 *          // Handle event and return
 *      }
 *      void OnEvent3() override
 *      {
 *          // Handle event and return
 *      }
 *      // handle the other events here
 * };
 */
#define AZ_EBUS_BEHAVIOR_BINDER_WITH_DOC(_Handler,_Uuid,_Allocator,...)\
    AZ_CLASS_ALLOCATOR(_Handler,_Allocator,0)\
    AZ_RTTI(_Handler,_Uuid,AZ::BehaviorEBusHandler)\
    typedef _Handler ThisType;\
    using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence<AZ_BEHAVIOR_EBUS_2ARG_UNPACKER(AZ_BEHAVIOR_EBUS_EVENT_FUNCTION_TYPE, __VA_ARGS__)>;\
    enum {\
        AZ_BEHAVIOR_EBUS_MACRO_CALLER(AZ_BEHAVIOR_EBUS_FUNC_ENUM, __VA_ARGS__)\
        FN_MAX\
    };\
    int GetFunctionIndex(const char* functionName) const override {\
        AZ_BEHAVIOR_EBUS_MACRO_CALLER(AZ_BEHAVIOR_EBUS_FUNC_INDEX, __VA_ARGS__)\
        return -1;\
    }\
    void Disconnect() override {\
        BusDisconnect();\
    }\
    _Handler(){\
        m_events.resize(FN_MAX);\
        AZ_BEHAVIOR_EBUS_MACRO_CALLER(AZ_BEHAVIOR_EBUS_REG_EVENT, __VA_ARGS__)\
    }\
    bool Connect(AZ::BehaviorValueParameter* id = nullptr) override {\
        return AZ::Internal::EBusConnector<_Handler>::Connect(this, id);\
    }\
    bool IsConnected() override {\
        return AZ::Internal::EBusConnector<_Handler>::IsConnected(this);\
    }\
    bool IsConnectedId(AZ::BehaviorValueParameter* id) override {\
        return AZ::Internal::EBusConnector<_Handler>::IsConnectedId(this, id);\
    }

#define AZ_EBUS_SEQ_1(_1) (_1)
#define AZ_EBUS_SEQ_2(_1,_2) AZ_EBUS_SEQ_1(_1) (_2)
#define AZ_EBUS_SEQ_3(_1,_2,_3) AZ_EBUS_SEQ_2(_1,_2) (_3)
#define AZ_EBUS_SEQ_4(_1,_2,_3,_4) AZ_EBUS_SEQ_3(_1,_2,_3) (_4)
#define AZ_EBUS_SEQ_5(_1,_2,_3,_4,_5) AZ_EBUS_SEQ_4(_1,_2,_3,_4) (_5)
#define AZ_EBUS_SEQ_6(_1,_2,_3,_4,_5,_6) AZ_EBUS_SEQ_5(_1,_2,_3,_4,_5) (_6)
#define AZ_EBUS_SEQ_7(_1,_2,_3,_4,_5,_6,_7) AZ_EBUS_SEQ_6(_1,_2,_3,_4,_5,_6) (_7)
#define AZ_EBUS_SEQ_8(_1,_2,_3,_4,_5,_6,_7,_8) AZ_EBUS_SEQ_7(_1,_2,_3,_4,_5,_6,_7) (_8)
#define AZ_EBUS_SEQ_9(_1,_2,_3,_4,_5,_6,_7,_8,_9) AZ_EBUS_SEQ_8(_1,_2,_3,_4,_5,_6,_7,_8) (_9)
#define AZ_EBUS_SEQ_10(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10) AZ_EBUS_SEQ_9(_1,_2,_3,_4,_5,_6,_7,_8,_9) (_10)
#define AZ_EBUS_SEQ_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) AZ_EBUS_SEQ_10(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10) (_11)
#define AZ_EBUS_SEQ_12(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) AZ_EBUS_SEQ_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) (_12)
#define AZ_EBUS_SEQ_13(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) AZ_EBUS_SEQ_12(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) (_13)
#define AZ_EBUS_SEQ_14(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) AZ_EBUS_SEQ_13(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) (_14)
#define AZ_EBUS_SEQ_15(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) AZ_EBUS_SEQ_14(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) (_15)
#define AZ_EBUS_SEQ_16(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) AZ_EBUS_SEQ_15(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) (_16)
#define AZ_EBUS_SEQ_17(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17) AZ_EBUS_SEQ_16(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) (_17)
#define AZ_EBUS_SEQ_18(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18) AZ_EBUS_SEQ_17(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17) (_18)
#define AZ_EBUS_SEQ_19(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19) AZ_EBUS_SEQ_18(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18) (_19)
#define AZ_EBUS_SEQ_20(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20) AZ_EBUS_SEQ_19(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19) (_20)
#define AZ_EBUS_SEQ_21(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21) AZ_EBUS_SEQ_20(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20) (_21)
#define AZ_EBUS_SEQ_22(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22) AZ_EBUS_SEQ_21(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21) (_22)
#define AZ_EBUS_SEQ_23(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23) AZ_EBUS_SEQ_22(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22) (_23)
#define AZ_EBUS_SEQ_24(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24) AZ_EBUS_SEQ_23(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23) (_24)
#define AZ_EBUS_SEQ_25(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25) AZ_EBUS_SEQ_24(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24) (_25)
#define AZ_EBUS_SEQ_26(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26) AZ_EBUS_SEQ_25(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25) (_26)
#define AZ_EBUS_SEQ_27(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27) AZ_EBUS_SEQ_26(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26) (_27)
#define AZ_EBUS_SEQ_28(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28) AZ_EBUS_SEQ_27(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27) (_28)
#define AZ_EBUS_SEQ_29(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29) AZ_EBUS_SEQ_28(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28) (_29)
#define AZ_EBUS_SEQ_30(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30) AZ_EBUS_SEQ_29(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29) (_30)
#define AZ_EBUS_SEQ_31(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31) AZ_EBUS_SEQ_30(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30) (_31)
#define AZ_EBUS_SEQ_32(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32) AZ_EBUS_SEQ_31(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31) (_32)
#define AZ_EBUS_SEQ_33(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33) AZ_EBUS_SEQ_32(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32) (_33)
#define AZ_EBUS_SEQ_34(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34) AZ_EBUS_SEQ_33(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33) (_34)
#define AZ_EBUS_SEQ_35(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35) AZ_EBUS_SEQ_34(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34) (_35)
#define AZ_EBUS_SEQ_36(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36) AZ_EBUS_SEQ_35(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35) (_36)
#define AZ_EBUS_SEQ_37(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37) AZ_EBUS_SEQ_36(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36) (_37)
#define AZ_EBUS_SEQ_38(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38) AZ_EBUS_SEQ_37(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37) (_38)
#define AZ_EBUS_SEQ_39(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39) AZ_EBUS_SEQ_38(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38) (_39)
#define AZ_EBUS_SEQ_40(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40) AZ_EBUS_SEQ_39(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39) (_40)
#define AZ_EBUS_SEQ_41(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41) AZ_EBUS_SEQ_40(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40) (_41)
#define AZ_EBUS_SEQ_42(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42) AZ_EBUS_SEQ_41(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41) (_42)
#define AZ_EBUS_SEQ_43(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43) AZ_EBUS_SEQ_42(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42) (_43)
#define AZ_EBUS_SEQ_44(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44) AZ_EBUS_SEQ_43(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43) (_44)
#define AZ_EBUS_SEQ_45(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45) AZ_EBUS_SEQ_44(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44) (_45)
#define AZ_EBUS_SEQ_46(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46) AZ_EBUS_SEQ_45(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45) (_46)
#define AZ_EBUS_SEQ_47(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47) AZ_EBUS_SEQ_46(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46) (_47)
#define AZ_EBUS_SEQ_48(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48) AZ_EBUS_SEQ_47(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47) (_48)
#define AZ_EBUS_SEQ_49(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49) AZ_EBUS_SEQ_48(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48) (_49)
#define AZ_EBUS_SEQ_50(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50) AZ_EBUS_SEQ_49(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49) (_50)
#define AZ_EBUS_SEQ_51(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51) AZ_EBUS_SEQ_50(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50) (_51)
#define AZ_EBUS_SEQ_52(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52) AZ_EBUS_SEQ_51(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51) (_52)
#define AZ_EBUS_SEQ_53(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53) AZ_EBUS_SEQ_52(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52) (_53)
#define AZ_EBUS_SEQ_54(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54) AZ_EBUS_SEQ_53(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53) (_54)
#define AZ_EBUS_SEQ_55(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55) AZ_EBUS_SEQ_54(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54) (_55)
#define AZ_EBUS_SEQ_56(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56) AZ_EBUS_SEQ_55(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55) (_56)
#define AZ_EBUS_SEQ_57(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57) AZ_EBUS_SEQ_56(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56) (_57)
#define AZ_EBUS_SEQ_58(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58) AZ_EBUS_SEQ_57(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57) (_58)
#define AZ_EBUS_SEQ_59(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59) AZ_EBUS_SEQ_58(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58) (_59)
#define AZ_EBUS_SEQ_60(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60) AZ_EBUS_SEQ_59(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59) (_60)
#define AZ_EBUS_SEQ_61(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61) AZ_EBUS_SEQ_60(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60) (_61)
#define AZ_EBUS_SEQ_62(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62) AZ_EBUS_SEQ_61(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61) (_62)
#define AZ_EBUS_SEQ_63(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63) AZ_EBUS_SEQ_62(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62) (_63)

#define AZ_EBUS_SEQ(...) AZ_MACRO_SPECIALIZE(AZ_EBUS_SEQ_,AZ_VA_NUM_ARGS(__VA_ARGS__),(__VA_ARGS__))

#define AZ_BEHAVIOR_EBUS_FUNC_ENUM(name)              AZ_BEHAVIOR_EBUS_FUNC_ENUM_I(name),
#define AZ_BEHAVIOR_EBUS_FUNC_ENUM_I(name)            FN_##name

#define AZ_BEHAVIOR_EBUS_FUNC_INDEX(name)             AZ_BEHAVIOR_EBUS_FUNC_INDEX_I(name);
#define AZ_BEHAVIOR_EBUS_FUNC_INDEX_I(name)           if(strcmp(functionName,#name)==0) return FN_##name

#define AZ_BEHAVIOR_EBUS_REG_EVENT(name)             AZ_BEHAVIOR_EBUS_REG_EVENT_I(name);
#define AZ_BEHAVIOR_EBUS_REG_EVENT_I(name)           SetEvent(&ThisType::name,#name);

#define AZ_BEHAVIOR_EBUS_FUNC_ENUM_WITH_DOC(name, args)             AZ_BEHAVIOR_EBUS_FUNC_ENUM_I(name),

#define AZ_BEHAVIOR_EBUS_FUNC_INDEX_WITH_DOC(name, args)             AZ_BEHAVIOR_EBUS_FUNC_INDEX_I(name);

#define AZ_BEHAVIOR_EBUS_REG_EVENT_WITH_DOC(name, args)             AZ_BEHAVIOR_EBUS_REG_EVENT_WITH_DOC_1(name, args)
#define AZ_BEHAVIOR_EBUS_REG_EVENT_WITH_DOC_1(name, args)           SetEvent(&ThisType::name, #name, {{AZ_BEHAVIOR_REMOVE_PARENTHESIS(args)}});

// Macro Helpers
#define AZ_BEHAVIOR_EXPAND(...)  __VA_ARGS__
#define AZ_BEHAVIOR_NOTHING_AZ_INTERNAL_EXTRACT
#define AZ_BEHAVIOR_EVALUATING_PASTE(_x, ...) AZ_INTERNAL_PASTE(_x, __VA_ARGS__)
#define AZ_BEHAVIOR_REMOVE_PARENTHESIS(_x) AZ_INTERNAL_EVALUATING_PASTE(AZ_INTERNAL_NOTHING_, AZ_INTERNAL_EXTRACT _x)

#define AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) macro## _WITH_DOC(name, args)
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(...) AZ_SEQ_JOIN(AZ_BEHAVIOR_EBUS_MACRO_CALLER_, AZ_VA_NUM_ARGS(__VA_ARGS__))

// Supports Handler functions with up to 20 parameters
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_0(macro)
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_2(macro, name, args)       AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args)
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_4(macro, name, args, ...)  AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_6(macro, name, args, ...)  AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_8(macro, name, args, ...)  AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_10(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_12(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_14(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_16(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_18(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_20(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_22(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_24(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_26(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_28(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_30(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_32(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_34(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_36(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_38(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_MACRO_CALLER_40(macro, name, args, ...) AZ_BEHAVIOR_EBUS_MACRO_INVOKE(macro, name, args) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))

#define AZ_BEHAVIOR_EBUS_MACRO_CALLER(macro, ...) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_MACRO_CALLER_NEXT(__VA_ARGS__) (macro, __VA_ARGS__))

// Supports Handler functions with up to 40 parameters
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_0(unpacker_macro, macro)
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_1(unpacker_macro, macro, name) macro(name)
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_2(unpacker_macro, macro, name, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_3(unpacker_macro, macro, name, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_4(unpacker_macro, macro, name, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_5(unpacker_macro, macro, name, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_6(unpacker_macro, macro, name, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_7(unpacker_macro, macro, name, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_8(unpacker_macro, macro, name, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_9(unpacker_macro, macro, name, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_10(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_11(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_12(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_13(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_14(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_15(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_16(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_17(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_18(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_19(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_20(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_21(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_22(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_23(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_24(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_25(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_26(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_27(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_28(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_29(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_30(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_31(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_32(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_33(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_34(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_35(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_36(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_37(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_38(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_39(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER_40(unpacker_macro, macro, name, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))

// Supports Handler functions with up to 20 parameters
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_0(unpacker_macro, macro)
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_2(unpacker_macro, macro, name, variableOverrides, ...) macro(name)
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_4(unpacker_macro, macro, name, variableOverrides, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro,macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_6(unpacker_macro, macro, name, variableOverrides, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro,macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_8(unpacker_macro, macro, name, variableOverrides, ...) macro(name),  AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_10(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_12(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_14(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_16(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_18(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_20(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_22(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_24(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_26(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_28(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_30(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_32(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_34(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_36(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_38(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER_40(unpacker_macro, macro, name, variableOverrides, ...) macro(name), AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, __VA_ARGS__) (unpacker_macro, macro, __VA_ARGS__))

#define AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(unpacker_macro, ...) AZ_SEQ_JOIN(unpacker_macro ## _, AZ_VA_NUM_ARGS(__VA_ARGS__))

// Recursive macros which passes in itself to AZ_BEHAVIOR_EBUS_UNPACKER_NEXT which concatenates the number of variadic arguments with the AZ_BEHAVIOR_EBUS_1ARG_UNPACKER or AZ_BEHAVIOR_EBUS_2ARG_UNPACKER macro
#define AZ_BEHAVIOR_EBUS_1ARG_UNPACKER(macro, ...) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(AZ_BEHAVIOR_EBUS_1ARG_UNPACKER, __VA_ARGS__) (AZ_BEHAVIOR_EBUS_1ARG_UNPACKER, macro, __VA_ARGS__))
#define AZ_BEHAVIOR_EBUS_2ARG_UNPACKER(macro, ...) AZ_BEHAVIOR_EXPAND(AZ_BEHAVIOR_EBUS_UNPACKER_NEXT(AZ_BEHAVIOR_EBUS_2ARG_UNPACKER, __VA_ARGS__) (AZ_BEHAVIOR_EBUS_2ARG_UNPACKER, macro, __VA_ARGS__))

#define AZ_BEHAVIOR_EBUS_EVENT_FUNCTION_TYPE(memberName) AZ_BEHAVIOR_EBUS_EVENT_FUNCTION_TYPE_I(memberName)
#define AZ_BEHAVIOR_EBUS_EVENT_FUNCTION_TYPE_I(memberName) decltype(&ThisType::memberName)

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Template implementations
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    inline BehaviorObject::BehaviorObject()
        : m_address(nullptr)
        , m_typeId(AZ::Uuid::CreateNull())
    {
    }

    //////////////////////////////////////////////////////////////////////////
    inline BehaviorObject::BehaviorObject(void* address, const Uuid& typeId)
        : m_address(address)
        , m_typeId(typeId)
    {
    }

    inline BehaviorObject::BehaviorObject(void* address, IRttiHelper* rttiHelper)
        : m_address(address)
        , m_rttiHelper(rttiHelper)
    {
        m_typeId = rttiHelper ? rttiHelper->GetTypeId() : AZ::Uuid::CreateNull();
    }

    //////////////////////////////////////////////////////////////////////////
    inline bool BehaviorObject::IsValid() const
    {
        return m_address && !m_typeId.IsNull();
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    inline BehaviorValueParameter::BehaviorValueParameter()
        : m_value(nullptr)
    {
        m_name = nullptr;
        m_typeId = Uuid::CreateNull();
        m_azRtti = nullptr;
        m_traits = 0;
    }

    inline BehaviorValueParameter::BehaviorValueParameter(BehaviorValueParameter&& other)
        : BehaviorParameter(AZStd::move(other))
        , m_value(AZStd::move(other.m_value))
        , m_onAssignedResult(AZStd::move(other.m_onAssignedResult))
        , m_tempData(AZStd::move(other.m_tempData))
    {
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    inline BehaviorValueParameter::BehaviorValueParameter(T* value)
    {
        Set<T>(value);
    }

    //////////////////////////////////////////////////////////////////////////
    inline BehaviorValueParameter::BehaviorValueParameter(BehaviorObject* value)
    {
        Set(value);
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void BehaviorValueParameter::StoreInTempData(T&& value)
    {
        AZ::Internal::SetParameters<T>(this);
        m_value = m_tempData.allocate(sizeof(T), AZStd::alignment_of<T>::value, 0);
        AZStd::construct_at(reinterpret_cast<AZStd::decay_t<T>*>(m_value), AZStd::forward<T>(value));
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    AZ_FORCE_INLINE void BehaviorValueParameter::Set(T* value)
    {
        AZ::Internal::SetParameters<AZStd::decay_t<T>>(this);
        m_value = (void*)value;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE void BehaviorValueParameter::Set(BehaviorObject* value)
    {
        m_value = &value->m_address;
        m_typeId = value->m_typeId;
        m_traits = BehaviorParameter::TR_POINTER;
        m_name = value->m_rttiHelper ? value->m_rttiHelper->GetActualTypeName(value->m_address) : nullptr;
        m_azRtti = value->m_rttiHelper;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE void BehaviorValueParameter::Set(const BehaviorParameter& param)
    {
        *static_cast<BehaviorParameter*>(this) = param;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE void BehaviorValueParameter::Set(const BehaviorValueParameter& param)
    {
        *static_cast<BehaviorParameter*>(this) = static_cast<const BehaviorParameter&>(param);
        m_value = param.m_value;
        m_onAssignedResult = param.m_onAssignedResult;
        m_tempData = param.m_tempData;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE void* BehaviorValueParameter::GetValueAddress() const
    {
        void* valueAddress = m_value;
        if (m_traits & BehaviorParameter::TR_POINTER)
        {
            valueAddress = *reinterpret_cast<void**>(valueAddress); // pointer to a pointer
        }
        return valueAddress;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE BehaviorValueParameter::operator BehaviorObject() const
    {
        return BehaviorObject(m_value, m_azRtti);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    AZ_FORCE_INLINE bool BehaviorValueParameter::ConvertTo()
    {
        return ConvertTo(AzTypeInfo<AZStd::decay_t<T>>::Uuid());
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE bool BehaviorValueParameter::ConvertTo(const AZ::Uuid& typeId)
    {
        if (m_azRtti)
        {
            void* valueAddress = GetValueAddress();
            if (valueAddress) // should we make null value to convert to anything?
            {
                return AZ::Internal::ConvertValueTo(valueAddress, m_azRtti, typeId, m_value, m_tempData);
            }
        }
        return m_typeId == typeId;
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    AZ_FORCE_INLINE AZStd::decay_t<T>*  BehaviorValueParameter::GetAsUnsafe() const
    {
        return reinterpret_cast<AZStd::decay_t<T>*>(m_value);
    }

    //////////////////////////////////////////////////////////////////////////

    struct SetResult
    {
        // MSVC does not allow an incomplete type to be used in a compiler intrinsic which is the reason why
        // std::is_assignable_v is not being used here
        // For some reason the Script.cpp test validates that an incomplete type can be used with the SetResult struct
        template<typename T, typename U, typename = void>
        static constexpr bool IsCopyAssignable = false;
        template<typename T, typename U>
        static constexpr bool IsCopyAssignable<T, U, AZStd::void_t<decltype(AZStd::declval<T>() = AZStd::declval<U>())>> = true;

        template<class T>
        static bool Set(BehaviorValueParameter& param, T&& result, bool IsValueCopy)
        {
            using Type = AZStd::decay_t<T>;
            if (param.m_traits & BehaviorParameter::TR_POINTER)
            {
                if constexpr (AZStd::is_pointer_v<Type>)
                {
                    using ValueType = AZStd::remove_cvref_t<AZStd::remove_pointer_t<Type>>;
                    *reinterpret_cast<void**>(param.m_value) = const_cast<ValueType*>(result);
                }
                else
                {
                    *reinterpret_cast<void**>(param.m_value) = &const_cast<Type&>(result);
                }
                return true;
            }
            else if (param.m_traits & BehaviorParameter::TR_REFERENCE)
            {
                if constexpr (AZStd::is_pointer_v<Type>)
                {
                    using ValueType = AZStd::remove_cvref_t<AZStd::remove_pointer_t<Type>>;
                    param.m_value = const_cast<ValueType*>(result);
                }
                else
                {
                    param.m_value = &const_cast<Type&>(result);
                }
                return true;
            }
            else if (IsValueCopy)
            {
                if constexpr (AZStd::is_pointer_v<Type>)
                {
                    using ValueType = AZStd::remove_cvref_t<AZStd::remove_pointer_t<Type>>;
                    if constexpr (IsCopyAssignable<ValueType&, AZStd::add_lvalue_reference_t<AZStd::remove_pointer_t<T>>>)
                    {
                        // copy if result is non-nullptr
                        if (result != nullptr)
                        {
                            *reinterpret_cast<ValueType*>(param.m_value) = *result;
                        }
                    }
                }
                else
                {
                    // value copy
                    if constexpr (IsCopyAssignable<Type&, T>)
                    {
                        *reinterpret_cast<Type*>(param.m_value) = AZStd::forward<T>(result);
                    }
                }
                return true;
            }
            return false;
        }
    };

    AZ_FORCE_INLINE BehaviorValueParameter& BehaviorValueParameter::operator=(BehaviorValueParameter&& other)
    {
        *static_cast<BehaviorParameter*>(this) = AZStd::move(static_cast<BehaviorParameter&&>(other));
        m_value = AZStd::move(other.m_value);
        m_onAssignedResult = AZStd::move(other.m_onAssignedResult);
        m_tempData = AZStd::move(other.m_tempData);
        return *this;
    }

    template<typename T>
    AZ_FORCE_INLINE BehaviorValueParameter& BehaviorValueParameter::operator=(T&& result)
    {
        StoreResult(AZStd::forward<T>(result));
        return *this;
    }

    template<typename T>
    AZ_FORCE_INLINE bool BehaviorValueParameter::StoreResult(T&& result)
    {
        using Type = AZStd::RemoveEnumT<AZStd::decay_t<T>>;

        const AZ::Uuid& typeId = AzTypeInfo<Type>::Uuid();

        bool isResult = false;

        if (m_typeId == typeId)
        {
            isResult = SetResult::Set(*this, AZStd::forward<T>(result), true);
        }
        else if (GetRttiHelper<Type>())
        {
            // try casting
            void* valueAddress = (void*)&result;
            if (m_traits & BehaviorParameter::TR_POINTER)
            {
                valueAddress = *reinterpret_cast<void**>(valueAddress); // pointer to a pointer
            }
            isResult = AZ::Internal::ConvertValueTo(valueAddress, GetRttiHelper<Type>(), m_typeId, m_value, m_tempData);
        }
        else if (m_typeId.IsNull()) // if nullptr we can accept any type, by pointer or reference
        {
            m_typeId = typeId;
            isResult = SetResult::Set(*this, AZStd::forward<T>(result), false);
        }

        if (isResult && m_onAssignedResult)
        {
            m_onAssignedResult();
        }

        return isResult;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    template<class... Args>
    bool BehaviorMethod::Invoke(Args&&... args) const
    {
        BehaviorValueParameter arguments[] = { &args... };
        return Call(arguments, sizeof...(Args), nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE bool BehaviorMethod::Invoke() const
    {
        return Call(nullptr, 0, nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class R, class... Args>
    bool BehaviorMethod::InvokeResult(R& r, Args&&... args) const
    {
        if (!HasResult())
        {
            return false;
        }
        BehaviorValueParameter arguments[sizeof...(args)] = { &args... };
        BehaviorValueParameter result(&r);
        return Call(arguments, sizeof...(Args), &result);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class R>
    bool BehaviorMethod::InvokeResult(R& r) const
    {
        if (!HasResult())
        {
            return false;
        }

        BehaviorValueParameter result(&r);
        return Call(nullptr, 0, &result);
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    template<class Getter>
    bool BehaviorProperty::SetGetter(Getter, BehaviorClass* /*currentClass*/, BehaviorContext* /*context*/, const AZStd::true_type&)
    {
        m_getter = nullptr;
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Getter>
    bool BehaviorProperty::SetGetter(Getter getter, BehaviorClass* currentClass, BehaviorContext* context, const AZStd::false_type&)
    {
        using GetterType = AZ::Internal::BehaviorMethodImpl<typename AZStd::RemoveFunctionConst<AZStd::remove_pointer_t<Getter>>::type>;
        AZStd::string getterPropertyName = currentClass ? currentClass->m_name : AZStd::string{};
        if (!getterPropertyName.empty())
        {
            getterPropertyName += "::";
        }
        getterPropertyName += m_name;
        getterPropertyName += k_PropertyNameGetterSuffix;
        m_getter = aznew GetterType(getter, context, getterPropertyName);

        if (AZStd::is_class<typename GetterType::ClassType>::value)
        {
            AZ_Assert(currentClass, "We should declare class property with in the class!");

            // check getter to have only return value (and this pointer)
            if (m_getter->GetNumArguments() != 1 || m_getter->GetArgument(0)->m_typeId != currentClass->m_typeId)
            {
                AZ_Assert(false, "Member Getter can't have any argument but thisPointer and just return type!");
                delete m_getter;
                m_getter = nullptr;
                return false;
            }

            // assure that TR_THIS_PTR is set on the first parameter
            m_getter->OverrideParameterTraits(0, AZ::BehaviorParameter::TR_THIS_PTR, 0);
        }
        else
        {
            // check getter to have only return value
            if (m_getter->GetNumArguments() > 0)
            {
                bool isValidSignature = false;
                if (currentClass && m_getter->GetNumArguments() == 1)
                {
                    AZ::TypeId thisPtrType = m_getter->GetArgument(0)->m_typeId;
                    // Check that the class is either the same as the first argument, or they are convertible
                    if (currentClass->m_azRtti)
                    {
                        isValidSignature = currentClass->m_azRtti->IsTypeOf(thisPtrType);
                    }
                    else
                    {
                        // No rtti, need to ensure types are the same
                        isValidSignature = thisPtrType == currentClass->m_typeId;
                    }
                }

                // assure that TR_THIS_PTR is set on the first parameter
                m_getter->OverrideParameterTraits(0, AZ::BehaviorParameter::TR_THIS_PTR, 0);

                if (!isValidSignature)
                {
                    AZ_Assert(false, "Getter can't have any argument just return type: %s!", currentClass->m_name.c_str());
                    delete m_getter;
                    m_getter = nullptr;
                    return false;
                }

                // assure that TR_THIS_PTR is set on the first parameter
                m_getter->OverrideParameterTraits(0, AZ::BehaviorParameter::TR_THIS_PTR, 0);
            }
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Setter>
    bool BehaviorProperty::SetSetter(Setter, BehaviorClass*, BehaviorContext*, const AZStd::true_type&)
    {
        m_setter = nullptr;
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Setter>
    bool BehaviorProperty::SetSetter(Setter setter, BehaviorClass* currentClass, BehaviorContext* context, const AZStd::false_type&)
    {
        using SetterType = AZ::Internal::BehaviorMethodImpl<typename AZStd::RemoveFunctionConst<AZStd::remove_pointer_t<Setter>>::type>;
        AZStd::string setterPropertyName = currentClass ? currentClass->m_name : AZStd::string{};
        if (!setterPropertyName.empty())
        {
            setterPropertyName += "::";
        }
        setterPropertyName += m_name;
        setterPropertyName += k_PropertyNameSetterSuffix;
        m_setter = aznew SetterType(setter, context, setterPropertyName);
        if (AZStd::is_class<typename SetterType::ClassType>::value)
        {
            AZ_Assert(currentClass, "We should declare class property with in the class!");

            // check setter have only 1 argument + 1 this pointer
            if (m_setter->GetNumArguments() != 2 || m_setter->GetArgument(0)->m_typeId != currentClass->m_typeId)
            {
                AZ_Assert(false, "Member Setter should have 2 arguments, thisPointer and dataValue to be set!");
                delete m_setter;
                m_setter = nullptr;
                return false;
            }
            // check getter result type is equal to setter input type
            if (m_getter && m_getter->GetResult()->m_typeId != m_setter->GetArgument(1)->m_typeId)
            {
                AZStd::string getterType, setterType;
                m_getter->GetResult()->m_typeId.ToString(getterType);
                m_setter->GetArgument(1)->m_typeId.ToString(setterType);
                AZ_Assert(false, "Getter return type and Setter input argument should be the same type! (getter: %s, setter: %s)", getterType.c_str(), setterType.c_str());
                delete m_setter;
                m_setter = nullptr;
                return false;
            }

            // assure that TR_THIS_PTR is set on the first parameter
            m_setter->OverrideParameterTraits(0, AZ::BehaviorParameter::TR_THIS_PTR, 0);
        }
        else
        {
            size_t valueIndex = 0;
            // check setter have only 1 argument
            if (m_setter->GetNumArguments() != 1)
            {
                bool isValidSignature = false;
                if (currentClass && m_setter->GetNumArguments() == 2)
                {
                    AZ::TypeId thisPtrType = m_setter->GetArgument(0)->m_typeId;
                    // Check that the class is either the same as the first argument, or they are convertible
                    if (currentClass->m_azRtti)
                    {
                        isValidSignature = currentClass->m_azRtti->IsTypeOf(thisPtrType);
                    }
                    else
                    {
                        // No rtti, need to ensure types are the same
                        isValidSignature = thisPtrType == currentClass->m_typeId;
                    }
                }

                if (!isValidSignature)
                {
                    AZ_Assert(false, "Setter should have 1 argument, data value to be set!");
                    delete m_setter;
                    m_setter = nullptr;
                    return false;
                }

                // it's ok as this is a different way to represent a member function
                valueIndex = 1; // since this pointer is at 0

                // assure that TR_THIS_PTR is set on the first parameter
                m_setter->OverrideParameterTraits(0, AZ::BehaviorParameter::TR_THIS_PTR, 0);
            }

            // check getter result type is equal to setter input type
            if (m_getter && m_getter->GetResult()->m_typeId != m_setter->GetArgument(valueIndex)->m_typeId)
            {
                AZStd::string getterType, setterType;
                m_getter->GetResult()->m_typeId.ToString(getterType);
                m_setter->GetArgument(valueIndex)->m_typeId.ToString(setterType);
                AZ_Assert(false, "Getter return type and Setter input argument should be the same type! (getter: %s, setter: %s)", getterType.c_str(), setterType.c_str());
                delete m_setter;
                m_setter = nullptr;
                return false;
            }
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Getter, class Setter>
    bool BehaviorProperty::Set(Getter getter, Setter setter, BehaviorClass* currentClass, BehaviorContext* context)
    {
        if (!SetGetter(getter, currentClass, context, typename AZStd::is_same<Getter, AZStd::nullptr_t>::type()))
        {
            return false;
        }

        if (!SetSetter(setter, currentClass, context, typename AZStd::is_same<Setter, AZStd::nullptr_t>::type()))
        {
            return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::Set(Event e, const char* eventName, BehaviorContext* context)
    {
        m_broadcast = aznew AZ::Internal::BehaviorEBusEvent<EBus, AZ::Internal::BE_BROADCAST, typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Event>::type>::type>(e, context);
        m_broadcast->m_name = eventName;

        SetEvent<EBus>(e, eventName, context, typename AZStd::is_same<typename EBus::BusIdType, NullBusId>::type());
        SetQueueBroadcast<EBus>(e, eventName, context, typename AZStd::is_same<typename EBus::QueuePolicy::BusMessageCall, typename AZ::Internal::NullBusMessageCall>::type());
        SetQueueEvent<EBus>(e, eventName, context, typename AZStd::conditional<AZStd::is_same<typename EBus::BusIdType, NullBusId>::value || AZStd::is_same<typename EBus::QueuePolicy::BusMessageCall, typename AZ::Internal::NullBusMessageCall>::value, AZStd::true_type, AZStd::false_type>::type());
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::true_type& /*is NullBusId*/)
    {
        AZ_UNUSED(e); AZ_UNUSED(context); AZ_UNUSED(eventName);
        m_event = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/)
    {
        m_event = aznew AZ::Internal::BehaviorEBusEvent<EBus, AZ::Internal::BE_EVENT_ID, typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Event>::type>::type>(e, context);
        m_event->m_name = eventName;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueBroadcast(Event e, const char* eventName, BehaviorContext* context, const AZStd::true_type& /*is NullBusId*/)
    {
        AZ_UNUSED(e); AZ_UNUSED(context); AZ_UNUSED(eventName);
        m_queueBroadcast = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueBroadcast(Event e, const char* eventName, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/)
    {
        m_queueBroadcast = aznew AZ::Internal::BehaviorEBusEvent<EBus, AZ::Internal::BE_QUEUE_BROADCAST, typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Event>::type>::type>(e, context);
        m_queueBroadcast->m_name = eventName;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::true_type& /* is Queue and is BusId valid*/)
    {
        AZ_UNUSED(e); AZ_UNUSED(context); AZ_UNUSED(eventName);
        m_queueEvent = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::false_type& /* is Queue and is BusId valid*/)
    {
        m_queueEvent = aznew AZ::Internal::BehaviorEBusEvent<EBus, AZ::Internal::BE_QUEUE_EVENT_ID, typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Event>::type>::type>(e, context);
        m_queueEvent->m_name = eventName;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    template<class Hook>
    bool BehaviorEBusHandler::InstallHook(int index, Hook h, void* userData)
    {
        if (index != -1)
        {
            // Check parameters
            if (!Internal::SetFunctionParameters<typename AZStd::remove_pointer<Hook>::type>::Check(m_events[index].m_parameters))
            {
                return false;
            }

            m_events[index].m_isFunctionGeneric = false;
            m_events[index].m_function = reinterpret_cast<void*>(h);
            m_events[index].m_userData = userData;
            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Hook>
    bool BehaviorEBusHandler::InstallHook(const char* name, Hook h, void* userData)
    {
        return InstallHook(GetFunctionIndex(name), h, userData);
    };

    //////////////////////////////////////////////////////////////////////////
    template<class Event>
    void BehaviorEBusHandler::SetEvent(Event e, const char* name)
    {
        (void)e;
        int i = GetFunctionIndex(name);
        if (i != -1)
        {
            m_events.resize(i + 1);
            m_events[i].m_name = name;
            m_events[i].m_eventId = AZ::Crc32(name);
            m_events[i].m_function = nullptr;
            AZ::Internal::SetFunctionParameters<typename AZStd::remove_pointer<Event>::type>::Set(m_events[i].m_parameters);
            m_events[i].m_metadataParameters.resize(m_events[i].m_parameters.size());
        }
    }

    template<class Event>
    void BehaviorEBusHandler::SetEvent(Event e, AZStd::string_view name, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Event>::num_args>& args)
    {
        (void)e;
        int i = GetFunctionIndex(name.data());
        if (i != -1)
        {
            m_events.resize(i + 1);
            m_events[i].m_name = name.data();
            m_events[i].m_eventId = AZ::Crc32(name.data());
            m_events[i].m_function = nullptr;
            AZ::Internal::SetFunctionParameters<typename AZStd::remove_pointer<Event>::type>::Set(m_events[i].m_parameters);
            m_events[i].m_metadataParameters.resize(m_events[i].m_parameters.size());
            for (size_t argIndex = 0; argIndex < args.size(); ++argIndex)
            {
                m_events[i].m_metadataParameters[AZ::eBehaviorBusForwarderEventIndices::ParameterFirst + argIndex] = { args[argIndex].m_name, args[argIndex].m_toolTip };
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class... Args>
    void BehaviorEBusHandler::Call(int index, Args&&... args) const
    {
        const BusForwarderEvent& e = m_events[index];
        if (e.m_function)
        {
            if (e.m_isFunctionGeneric)
            {
                BehaviorValueParameter arguments[sizeof...(args)+1] = { &args... };
                reinterpret_cast<GenericHookType>(e.m_function)(const_cast<void*>(e.m_userData), e.m_name, index, nullptr, AZ_ARRAY_SIZE(arguments)-1, arguments);
            }
            else
            {
                typedef void(*FunctionType)(void*, Args...);
                reinterpret_cast<FunctionType>(e.m_function)(const_cast<void*>(e.m_userData), args...);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    template<class R, class... Args>
    void BehaviorEBusHandler::CallResult(R& result, int index, Args&&... args) const
    {
        const BusForwarderEvent& e = m_events[index];
        if (e.m_function)
        {
            if (e.m_isFunctionGeneric)
            {
                BehaviorValueParameter arguments[sizeof...(args)+1] = { &args... };
                BehaviorValueParameter r(&result);
                reinterpret_cast<GenericHookType>(e.m_function)(const_cast<void*>(e.m_userData), e.m_name, index, &r, AZ_ARRAY_SIZE(arguments) - 1, arguments);
                // Assign on top of the the value if the param isn't a pointer
                // (otherwise the pointer just gets overridden and no value is returned).
                if ((r.m_traits & BehaviorParameter::TR_POINTER) == 0 && r.GetAsUnsafe<R>())
                {
                    result = *r.GetAsUnsafe<R>();
                }
            }
            else
            {
                typedef R(*FunctionType)(void*, Args...);
                result = reinterpret_cast<FunctionType>(e.m_function)(const_cast<void*>(e.m_userData), args...);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    template<class Value>
    BehaviorDefaultValuePtr BehaviorContext::MakeDefaultValue(Value&& defaultValue)
    {
        return !IsRemovingReflection() ? aznew BehaviorDefaultValue(AZStd::forward<Value>(defaultValue)) : nullptr;
    }

    template<class... Values>
    BehaviorValues* BehaviorContext::MakeDefaultValues(Values&&... values)
    {
        return !IsRemovingReflection() ? aznew AZ::Internal::BehaviorValuesSpecialization<Values...>(AZStd::forward<Values>(values)...) : nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::ClassBuilder<T> BehaviorContext::Class(const char* name)
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
                // class already reflected, display name and uuid
                char uuidName[AZ::Uuid::MaxStringBuffer];
                classTypeIt->first.ToString(uuidName, AZ::Uuid::MaxStringBuffer);

                AZ_Error("Reflection", false, "Class '%s' is already registered using Uuid: %s!", name, uuidName);
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
            SetClassDefaultAllocator<T>(behaviorClass, typename HasAZClassAllocator<T>::type());
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

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassBuilder<C>::ClassBuilder(BehaviorContext* context, BehaviorClass* behaviorClass)
        : AZ::Internal::GenericAttributes<ClassBuilder<C>>(context)
        , m_class(behaviorClass)
    {
        if (behaviorClass)
        {
            Base::m_currentAttributes = &behaviorClass->m_attributes;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassBuilder<C>::~ClassBuilder()
    {
        // process all on demand queued reflections
        Base::m_context->ExecuteQueuedOnDemandReflections();

        if (m_class && (!Base::m_context->IsRemovingReflection()))
        {
            for (const auto &method : m_class->m_methods)
            {
                m_class->PostProcessMethod(Base::m_context, *method.second);
                if (MethodReturnsAzEventByReferenceOrPointer(*method.second))
                {
                    ValidateAzEventDescription(*Base::m_context, *method.second);
                }
            }

            // Validate the AzEvent description of the class property getter's
            for (auto&& [propertyName, propertyInst]: m_class->m_properties)
            {
                if (propertyInst->m_getter && MethodReturnsAzEventByReferenceOrPointer(*propertyInst->m_getter))
                {
                    ValidateAzEventDescription(*Base::m_context, *propertyInst->m_getter);
                }
            }

            BehaviorContextBus::Event(Base::m_context, &BehaviorContextBus::Events::OnAddClass, m_class->m_name.c_str(), m_class);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::operator->()
    {
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<class... Params>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Constructor()
    {
        if (!Base::m_context->IsRemovingReflection())
        {
        AZ_Error("BehaviorContext", m_class, "You can set constructors only on valid classes!");
        }
        if (m_class)
        {
            BehaviorMethod* constructor = aznew AZ::Internal::BehaviorMethodImpl<void(C* address, Params...)>(&Construct<C, Params...>, Base::m_context, AZStd::string::format("%s::Constructor", m_class->m_name.c_str()));
            m_class->m_constructors.push_back(constructor);
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<class WrappedType>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Wrapping(BehaviorClassUnwrapperFunction unwrapper, void* userData)
    {
        static_assert(!AZStd::is_same<AZStd::decay_t<C>, AZStd::decay_t<WrappedType>>::value, "A Wrapping member cannot unwrap to the same type as itself."
            " As wrapped types are implicitly reflected by the ScriptContext, this prevents a recursive loop");
        if (!Base::m_context->IsRemovingReflection())
        {
        AZ_Error("BehaviorContext", m_class, "You can wrap only valid classes!");
        }
        if (m_class)
        {
            m_class->m_wrappedTypeId = AzTypeInfo<WrappedType>::Uuid();
            m_class->m_unwrapper = unwrapper;
            m_class->m_unwrapperUserData = userData;
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<class WrappedType, class Callable>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::WrappingMember(Callable callableFunction)
    {
        union
        {
            Callable callableFunction;
            void* userData;
        } u;
        u.callableFunction = callableFunction;
        return Wrapping<WrappedType>(&WrappedClassCaller<C, WrappedType, Callable>::Unwrap, u.userData);
    }

    template<class Function>
    BehaviorContext::GlobalMethodBuilder BehaviorContext::Method(const char* name, Function f, BehaviorValues* defaultValues, const char* dbgDesc)
    {
        return Method(name, f, nullptr, defaultValues, dbgDesc);
    }


    //////////////////////////////////////////////////////////////////////////
    ///< \deprecated Use "Method(const char*, Function, const AZStd::array<ParameterOverrides, AZStd::function_traits<Function>::num_args>&, const char*)" instead
    template<class Function>
    BehaviorContext::GlobalMethodBuilder BehaviorContext::Method(const char* name, Function f, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args> & args, const char* dbgDesc)
    {
        return Method(name, f, nullptr, args, dbgDesc);
    }

    //////////////////////////////////////////////////////////////////////////
    ///< \deprecated Use "Method(const char*, Function, const char*, const AZStd::array<ParameterOverrides, AZStd::function_traits<Function>::num_args>&, const char*)" instead
    template<class Function>
    BehaviorContext::GlobalMethodBuilder BehaviorContext::Method(const char* name, Function f, const char* deprecatedName, BehaviorValues* defaultValues, const char* dbgDesc)
    {
        AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args> parameterOverrides;
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
    BehaviorContext::GlobalMethodBuilder BehaviorContext::Method(const char* name, Function f, const char* deprecatedName, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args, const char* dbgDesc)
    {
        if (IsRemovingReflection())
        {
            auto globalMethodIt = m_methods.find(name);
            if (globalMethodIt != m_methods.end())
            {
                BehaviorContextBus::Event(this, &BehaviorContextBus::Events::OnRemoveGlobalMethod, name, globalMethodIt->second);
                delete globalMethodIt->second;
                m_methods.erase(globalMethodIt);
            }
            return GlobalMethodBuilder(this, nullptr, nullptr);
        }

        AZ_Assert(!AZStd::is_member_function_pointer<Function>::value, "This is a member %s method declared as global! use script.Class<Type>(Name)->Method()->Value()!\n", name);
        typedef AZ::Internal::BehaviorMethodImpl<typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Function>::type>::type> BehaviorMethodType;
        BehaviorMethod* method = aznew BehaviorMethodType(f, this, name);
        method->m_debugDescription = dbgDesc;

        /*
        ** check to see if the deprecated name is used, and ensure its not duplicated.
        */

        if (deprecatedName != nullptr)
        {
            auto itr = m_methods.find(deprecatedName);
            if (itr != m_methods.end())
            {
                // now check to make sure that the deprecated name is not being used as a identical deprecated name for another method.
                bool isDuplicate = false;
                for (const auto & i : m_methods)
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
                AZ_Warning("BehaviorContext", false, "Method %s is attempting to use a deprecated name of %s which is already in use! Deprecated name is ignored!", name, deprecatedName);
            }
        }

        // global method
        if (!m_methods.insert(AZStd::make_pair(name, method)).second)
        {
            AZ_Error("Reflection", false, "Method '%s' is already registered in the global context!", name);
            delete method;
            return GlobalMethodBuilder(this, nullptr, nullptr);
        }

        size_t classPtrIndex = method->IsMember() ? 1 : 0;
        for (size_t i = 0; i < args.size(); ++i)
        {
            method->SetArgumentName(i + classPtrIndex, args[i].m_name);
            method->SetArgumentToolTip(i + classPtrIndex, args[i].m_toolTip);
            method->SetDefaultValue(i + classPtrIndex, args[i].m_defaultValue);
            method->OverrideParameterTraits(i + classPtrIndex, args[i].m_addTraits, args[i].m_removeTraits);
        }

        return GlobalMethodBuilder(this, name, method);
    }

    template<class C>
    template<class Function>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Method(const char* name, Function f, BehaviorValues* defaultValues, const char* dbgDesc)
    {
        return Method(name, f, nullptr, defaultValues, dbgDesc);
    }

    template<class C>
    template<class Function>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Method(const char* name, Function f, const char* deprecatedName, BehaviorValues* defaultValues, const char* dbgDesc)
    {
        AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args> parameterOverrides;
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

    template<class C>
    template<class Function>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Method(const char* name, Function f, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args, const char* dbgDesc)
    {
        return Method(name, f, nullptr, args, dbgDesc);
    }

    template<class C>
    template<class Function>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Method(const char* name, Function f, const char* deprecatedName, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args, const char* dbgDesc)
    {
        if (m_class)
        {
            typedef AZ::Internal::BehaviorMethodImpl<typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Function>::type>::type> BehaviorMethodType;
            BehaviorMethod* method = aznew BehaviorMethodType(f, Base::m_context, AZStd::string::format("%s::%s", m_class->m_name.c_str(), name));
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
                    for (const auto & i : m_class->m_methods)
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
    template<class C>
    template<class Getter, class Setter>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Property(const char* name, Getter getter, Setter setter)
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
    template<class C>
    template<auto Value, typename T>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Enum(const char* name)
    {
        Property(name, []() { return Value; }, nullptr);
        ClassBuilder::Attribute(AZ::Script::Attributes::ClassConstantValue, true);
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<class Getter>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Constant(const char* name, Getter getter)
    {
        Property(name, getter, nullptr);
        return this;
    };


    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::RequestBus(const char* name)
    {
        if (m_class)
        {
            m_class->m_requestBuses.insert(name);
        }
        return this;
    }


    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::NotificationBus(const char* name)
    {
        if (m_class)
        {
            m_class->m_notificationBuses.insert(name);
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::Allocator(BehaviorClass::AllocateType allocate, BehaviorClass::DeallocateType deallocate)
    {
        AZ_Error("BehaviorContext", m_class, "Allocator can be set on valid classes only!");
        if (m_class)
        {
            m_class->m_allocate = allocate;
            m_class->m_deallocate = deallocate;
        }
        return this;
    }

    template<class C>
    BehaviorContext::ClassBuilder<C>* BehaviorContext::ClassBuilder<C>::UserData(void *userData)
    {
        AZ_Error("BehaviorContext", m_class, "UserData can be set on valid classes only!");
        if (m_class)
        {
            m_class->m_userData = userData;
        }
        return this;
    }

    template<class Getter, class Setter>
    BehaviorContext::GlobalPropertyBuilder BehaviorContext::Property(const char* name, Getter getter, Setter setter)
    {
        if (IsRemovingReflection())
        {
            auto globalPropIt = m_properties.find(name);
            if (globalPropIt != m_properties.end())
            {
                BehaviorContextBus::Event(this, &BehaviorContextBus::Events::OnRemoveGlobalProperty, name, globalPropIt->second);
                delete globalPropIt->second;
                m_properties.erase(globalPropIt);
            }
            return GlobalPropertyBuilder(this, nullptr);
        }

        AZ_Assert(!AZStd::is_member_function_pointer<Getter>::value, "Getter for %s is a member method! script.Class<Type>(Name)->Property()!\n", name);
        AZ_Assert(!AZStd::is_member_function_pointer<Setter>::value, "Setter for %s is a member method! script.Class<Type>(Name)->Property()!\n", name);

        BehaviorProperty* prop = aznew BehaviorProperty(this);
        prop->m_name = name;
        if (!prop->Set(getter, setter, nullptr, this))
        {
            delete prop;
            return GlobalPropertyBuilder(this, nullptr);
        }

        m_properties.insert(AZStd::make_pair(name, prop));

        return GlobalPropertyBuilder(this, prop);
    }

    //////////////////////////////////////////////////////////////////////////
    template<auto Value, typename T>
    BehaviorContext* BehaviorContext::Enum(const char* name)
    {
        Property(name, []() { return Value; }, nullptr);
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<auto Value, typename T>
    BehaviorContext::GlobalPropertyBuilder BehaviorContext::EnumProperty(const char* name)
    {
        return Property(name, []() { return Value; }, nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Getter>
    BehaviorContext* BehaviorContext::Constant(const char* name, Getter getter)
    {
        Property(name, getter, nullptr);
        return this;
    };

    //////////////////////////////////////////////////////////////////////////
    template<class Getter>
    BehaviorContext::GlobalPropertyBuilder BehaviorContext::ConstantProperty(const char* name, Getter getter)
    {
        return Property(name, getter, nullptr);
    };

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::EBusBuilder<T>::EBusBuilder(BehaviorContext* context, BehaviorEBus* ebus)
        : Base(context)
        , m_ebus(ebus)
    {
        Base::m_currentAttributes = &ebus->m_attributes;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::EBusBuilder<T>::~EBusBuilder()
    {
        // process all on demand queued reflections
        Base::m_context->ExecuteQueuedOnDemandReflections();

        if (!Base::m_context->IsRemovingReflection())
        {
            for (auto&& [eventName, eventSender] : m_ebus->m_events)
            {
                if (MethodReturnsAzEventByReferenceOrPointer(*eventSender.m_broadcast))
                {
                    ValidateAzEventDescription(*Base::m_context, *eventSender.m_broadcast);
                }
            }
            BehaviorContextBus::Event(Base::m_context, &BehaviorContextBus::Events::OnAddEBus, m_ebus->m_name.c_str(), m_ebus);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::EBusBuilder<T>* BehaviorContext::EBusBuilder<T>::operator->()
    {
        return this;
    }


    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::EBusBuilder<T> BehaviorContext::EBus(const char* name, const char* deprecatedName /*=nullptr*/, const char* toolTip /*=nullptr*/)
    {
        // should we require AzTypeInfo for EBus, technically we should if we want to work around the compiler issue that made us to do it in first place
        if (IsRemovingReflection())
        {
            auto ebusIt = m_ebuses.find(name);
            if (ebusIt != m_ebuses.end())
            {
                BehaviorContextBus::Event(this, &BehaviorContextBus::Events::OnRemoveEBus, name, ebusIt->second);

                // Erase the deprecated name as well
                auto deprecatedIt = m_ebuses.find(ebusIt->second->m_deprecatedName);
                if (deprecatedIt != m_ebuses.end())
                {
                    m_ebuses.erase(deprecatedIt);
                }

                delete ebusIt->second;
                m_ebuses.erase(ebusIt);
            }

            return EBusBuilder<T>(this, nullptr);
        }
        else
        {
            AZ_Error("BehaviorContext", m_ebuses.find(name) == m_ebuses.end(), "You shouldn't reflect an EBus multiple times (%s), subsequent reflections will not be registered!", name);

            BehaviorEBus* behaviorEBus = aznew BehaviorEBus();
            behaviorEBus->m_name = name;

            if(toolTip != nullptr)
            {
                behaviorEBus->m_toolTip = toolTip;
            }

            /*
            ** If we have a deprecated name, lets make sure the its not in use as an existing bus.
            */

            if (deprecatedName != nullptr)
            {
                if(*deprecatedName == '\0')
                {
                    AZ_Warning("BehaviorContext", false, "Deprecated name can't be a empty string!", deprecatedName);
                }
                else if (m_ebuses.find(deprecatedName) != m_ebuses.end())
                {
                    AZ_Warning("BehaviorContext", false, "EBus %s is attempting to use the deprecated name (%s) that is already used! Ignored!", name, deprecatedName);
                }
                else
                {
                    behaviorEBus->m_deprecatedName = deprecatedName;
                }
            }

            EBusSetIdFeatures<T>(behaviorEBus);
            behaviorEBus->m_queueFunction = QueueFunctionMethod<T>();

            // Switch to Set (we store the name in the class)
            m_ebuses.insert(AZStd::make_pair(behaviorEBus->m_name, behaviorEBus));
            if (!behaviorEBus->m_deprecatedName.empty())
            {
                m_ebuses.insert(AZStd::make_pair(behaviorEBus->m_deprecatedName, behaviorEBus));
            }

            return EBusBuilder<T>(this, behaviorEBus);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    template<class Function>
    BehaviorContext::EBusBuilder<Bus>* BehaviorContext::EBusBuilder<Bus>::Event(const char* name, Function e, const char *deprecatedName /*=nullptr*/)
    {
        AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args> parameterOverrides;
        return Event(name, e, deprecatedName, parameterOverrides);
    }

    template<class Bus>
    template<class Function>
    BehaviorContext::EBusBuilder<Bus>* BehaviorContext::EBusBuilder<Bus>::Event(const char* name, Function e, const char* deprecatedName, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args)
    {
        if (m_ebus)
        {
            BehaviorEBusEventSender ebusEvent;

            ebusEvent.Set<Bus>(e, name, Base::m_context);

            auto insertIt = m_ebus->m_events.insert(AZStd::make_pair(name, ebusEvent));

            if (!insertIt.second)
            {
                AZ_Error("BehaviorContext", false, "Reflection inserted a duplicate event: '%s' for bus '%s' - please check that you are not reflecting the same event repeatedly. This will cause memory leaks.", name, m_ebus->m_name.c_str());
            }
            else
            {
                // do we have a deprecated name for this event?
                if (deprecatedName != nullptr)
                {
                    // ensure deprecated name is not in use as a existing name
                    auto itr = m_ebus->m_events.find(deprecatedName);

                    if (itr != m_ebus->m_events.end())
                    {
                        AZ_Warning("BehaviorContext", false, "Event %s is attempting to use %s as a deprecated name, but the deprecated name is already in use! The deprecated name is ignored!", name, deprecatedName);
                    }
                    else
                    {
                        // ensure that we won't have a duplicate deprecated name
                        bool isDuplicated = false;
                        for (const auto & i : m_ebus->m_events)
                        {
                            if (i.second.m_deprecatedName == deprecatedName)
                            {
                                isDuplicated = true;
                                AZ_Warning("BehaviorContext", false, "Event %s is attempting to use %s as a deprecated name, but the deprecated name is already used as a deprecated name for the Event %s! The deprecated name is ignored!", name, deprecatedName, i.first.c_str());
                                break;
                            }
                        }

                        if (!isDuplicated)
                        {
                            insertIt.first->second.m_deprecatedName = deprecatedName;
                        }
                    }
                }

                for (BehaviorMethod* method : { ebusEvent.m_event, ebusEvent.m_broadcast })
                {
                    if (method)
                    {
                        size_t busIdParameterIndex = method->HasBusId() ? 1 : 0;
                        for (size_t i = 0; i < args.size(); ++i)
                        {
                            method->SetArgumentName(i + busIdParameterIndex, args[i].m_name);
                            method->SetArgumentToolTip(i + busIdParameterIndex, args[i].m_toolTip);
                            method->SetDefaultValue(i + busIdParameterIndex, args[i].m_defaultValue);
                            method->OverrideParameterTraits(i + busIdParameterIndex, args[i].m_addTraits, args[i].m_removeTraits);
                        }
                    }
                }

                Base::m_currentAttributes = &insertIt.first->second.m_attributes;
                Base::SetEBusEventSender(&insertIt.first->second);
            }
        }

        return this;
    }

    template<class Bus>
    template<class Function>
    BehaviorContext::EBusBuilder<Bus>* BehaviorContext::EBusBuilder<Bus>::Event(const char* name, Function e, const AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>& args)
    {
        return Event(name, e, nullptr, args);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    template<typename HandlerType, typename HandlerCreator, typename HandlerDestructor >
    BehaviorContext::EBusBuilder<Bus>* BehaviorContext::EBusBuilder<Bus>::Handler(HandlerCreator creator, HandlerDestructor destructor)
    {
        if (m_ebus)
        {
            AZ_Assert(creator != nullptr && destructor != nullptr, "Both creator and destructor should be provided!");

            BehaviorMethod* createHandler = aznew AZ::Internal::BehaviorMethodImpl<typename AZStd::remove_pointer<HandlerCreator>::type>(creator, Base::m_context, m_ebus->m_name + "::CreateHandler");
            BehaviorMethod* destroyHandler = aznew AZ::Internal::BehaviorMethodImpl<typename AZStd::remove_pointer<HandlerDestructor>::type>(destructor, Base::m_context, m_ebus->m_name + "::DestroyHandler");
            // OnDemandReflect the types in all the handler Event functions
            m_ebus->m_ebusHandlerOnDemandReflector = AZStd::make_unique<ScopedBehaviorOnDemandReflector>(*Base::m_context);
            AZ::Internal::OnDemandReflectFunctions(m_ebus->m_ebusHandlerOnDemandReflector.get(), typename HandlerType::EventFunctionsParameterPack{});

            // check than the handler returns the expected type
            if (createHandler->GetResult()->m_typeId != AzTypeInfo<BehaviorEBusHandler>::Uuid() || destroyHandler->GetArgument(0)->m_typeId != AzTypeInfo<BehaviorEBusHandler>::Uuid())
            {
                AZ_Assert(false, "HandlerCreator my return a BehaviorEBusHandler* object and HandlerDestrcutor should have an argument that can handle BehaviorEBusHandler!");
                delete createHandler;
                delete destroyHandler;
                createHandler = nullptr;
                destroyHandler = nullptr;
            }
            else
            {
                Base::m_currentAttributes = &createHandler->m_attributes;
                Base::SetEBusEventSender(nullptr);
            }
            m_ebus->m_createHandler = createHandler;
            m_ebus->m_destroyHandler = destroyHandler;
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    template<class H>
    BehaviorContext::EBusBuilder<Bus>* BehaviorContext::EBusBuilder<Bus>::Handler()
    {
        Handler<H>(&AZ::Internal::BehaviorEBusHandlerFactory<H>::Create, &AZ::Internal::BehaviorEBusHandlerFactory<H>::Destroy);
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    BehaviorContext::EBusBuilder<Bus>* BehaviorContext::EBusBuilder<Bus>::VirtualProperty(const char* name, const char* getterEvent, const char* setterEvent)
    {
        if (m_ebus)
        {
            BehaviorEBusEventSender* getter = nullptr;
            BehaviorEBusEventSender* setter = nullptr;
            if (getterEvent)
            {
                auto getterIt = m_ebus->m_events.find(getterEvent);
                getter = &getterIt->second;
                if (getterIt == m_ebus->m_events.end())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter event %s is not reflected. Make sure VirtualProperty is reflected after the Event!", m_ebus->m_name.c_str(), name, getterEvent);
                    return this;
                }

                // we should always have the broadcast present, so use it for our checks
                if (!getter->m_broadcast->HasResult())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s should return result", m_ebus->m_name.c_str(), name, getterEvent);
                    return this;
                }
                if (getter->m_broadcast->GetNumArguments() != 0)
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s can not have arguments only result", m_ebus->m_name.c_str(), name, getterEvent);
                    return this;
                }
            }
            if (setterEvent)
            {
                auto setterIt = m_ebus->m_events.find(setterEvent);
                if (setterIt == m_ebus->m_events.end())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s setter event %s is not reflected. Make sure VirtualProperty is reflected after the Event!", m_ebus->m_name.c_str(), name, setterEvent);
                    return this;
                }
                setter = &setterIt->second;
                if (setter->m_broadcast->HasResult())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s setter %s should not return result", m_ebus->m_name.c_str(), name, setterEvent);
                    return this;
                }
                if (setter->m_broadcast->GetNumArguments() != 1)
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s setter %s can have only one argument", m_ebus->m_name.c_str(), name, setterEvent);
                    return this;
                }
            }

            if (getter && setter)
            {
                if (getter->m_broadcast->GetResult()->m_typeId != setter->m_broadcast->GetArgument(0)->m_typeId)
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s return [%s] and setter %s input argument [%s] types don't match ", m_ebus->m_name.c_str(), name, setterEvent, getter->m_broadcast->GetResult()->m_typeId.ToString<AZStd::string>().c_str(), setter->m_broadcast->GetArgument(0)->m_typeId.ToString<AZStd::string>().c_str());
                    return this;
                }
            }

            m_ebus->m_virtualProperties.insert(AZStd::make_pair(name, BehaviorEBus::VirtualProperty(getter, setter)));
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    namespace Internal
    {
        template<class T>
        struct SizeOfSafe
        {
            static size_t Get() { return sizeof(T); }
        };

        template<>
        struct SizeOfSafe<void>
        {
            static size_t Get() { return 0; }
        };

        template<class... Functions>
        inline void OnDemandReflectFunctions(OnDemandReflectionOwner* onDemandReflection, AZStd::Internal::pack_traits_arg_sequence<Functions...>)
        {
            (BehaviorOnDemandReflectHelper<typename AZStd::function_traits<Functions>::raw_fp_type>::QueueReflect(onDemandReflection), ...);
        }

        // Assumes parameters array is big enough to store all parameters
        template<class... Args>
        inline void SetParametersStripped(BehaviorParameter* parameters, OnDemandReflectionOwner* onDemandReflection)
        {
            // +1 to avoid zero array size
            Uuid argumentTypes[sizeof...(Args)+1] = { AzTypeInfo<AZStd::remove_pointer_t<AZStd::remove_cvref_t<Args>>>::Uuid()... };
            const char* argumentNames[sizeof...(Args)+1] = { AzTypeInfo<Args>::Name()... };
            bool argumentIsPointer[sizeof...(Args)+1] = { AZStd::is_pointer<typename AZStd::remove_reference<Args>::type>::value... };
            bool argumentIsConst[sizeof...(Args)+1] = { AZStd::is_const<typename AZStd::remove_pointer<Args>::type>::value... };
            bool argumentIsReference[sizeof...(Args)+1] = { AZStd::is_reference<Args>::value... };
            IRttiHelper* rttiHelper[sizeof...(Args)+1] = { GetRttiHelper<typename AZStd::remove_pointer<typename AZStd::decay<Args>::type>::type>()... };
            (void)argumentIsPointer; (void)argumentIsConst; (void)argumentIsReference;
            // function / member function pointer ?
            for (size_t i = 0; i < sizeof...(Args); ++i)
            {
                parameters[i].m_typeId = argumentTypes[i];
                parameters[i].m_name = argumentNames[i];
                parameters[i].m_azRtti = rttiHelper[i];
                parameters[i].m_traits = (argumentIsPointer[i] ? BehaviorParameter::TR_POINTER : 0) |
                    (argumentIsConst[i] ? BehaviorParameter::TR_CONST : 0) |
                    (argumentIsReference[i] ? BehaviorParameter::TR_REFERENCE : 0);

                // String parameter detection
                if ((parameters[i].m_typeId == azrtti_typeid<char>() && (parameters[i].m_traits & (BehaviorParameter::TR_POINTER | BehaviorParameter::TR_CONST))) // const char* detection
                    || parameters[i].m_typeId == azrtti_typeid<AZStd::string>() || parameters[i].m_typeId == azrtti_typeid<AZStd::string_view>()) // AZStd::string and AZStd::string_view detection
                {
                    parameters[i].m_traits |= BehaviorParameter::TR_STRING;
                }
            }

            if (onDemandReflection)
            {
                // deal with OnDemand reflection
                StaticReflectionFunctionPtr reflectHooks[sizeof...(Args)+1] = { OnDemandReflectHook<typename AZStd::remove_pointer<typename AZStd::decay<Args>::type>::type>::Get()... };
                for (size_t i = 0; i < sizeof...(Args); ++i)
                {
                    if (reflectHooks[i])
                    {
                        onDemandReflection->AddReflectFunction(argumentTypes[i], reflectHooks[i]);
                    }
                }
            }
        }
        template<class... Args>
        inline void SetParameters(BehaviorParameter* parameters, OnDemandReflectionOwner* onDemandReflection)
        {
            SetParametersStripped<AZStd::RemoveEnumT<Args>...>(parameters, onDemandReflection);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        template<AZStd::size_t... Is, class Function>
        inline void CallFunction<R, Args...>::Global(Function functionPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
        {
            (void)arguments;
            if (result)
            {
                result->StoreResult<R>(functionPtr(*arguments[Is].GetAsUnsafe<Args>()...));
            }
            else
            {
                functionPtr(*arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        template<AZStd::size_t... Is, class C, class Function>
        inline void CallFunction<R, Args...>::Member(Function functionPtr, C thisPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
        {
            (void)arguments;
            if (result)
            {
                result->StoreResult<R>((thisPtr->*functionPtr)(*arguments[Is].GetAsUnsafe<Args>()...));
            }
            else
            {
                (thisPtr->*functionPtr)(*arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        BehaviorMethodImpl<R(Args...)>::BehaviorMethodImpl(FunctionPointer functionPointer, BehaviorContext* context, const AZStd::string& name)
            : BehaviorMethod(context)
            , m_functionPtr(functionPointer)
        {
            m_name = name;
            SetParameters<R>(m_parameters, this);
            SetParameters<Args...>(&m_parameters[s_startNamedArgumentIndex], this);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        bool BehaviorMethodImpl<R(Args...)>::Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result) const
        {
            size_t totalArguments = GetNumArguments();
            if (numArguments < totalArguments)
            {
                // We are cloning all arguments on the stack, since Call is called only from Invoke we can reserve bigger "arguments" array
                // that can always handle all parameters. So far the don't use default values that ofter, so we will optimize for the common case first.
                BehaviorValueParameter* newArguments = reinterpret_cast<BehaviorValueParameter*>(alloca(sizeof(BehaviorValueParameter)*  totalArguments));
                // clone the input parameters (we don't need to clone temp buffers, etc. as they will be still on the stack)
                size_t argIndex = 0;
                for (; argIndex < numArguments; ++argIndex)
                {
                    new(&newArguments[argIndex]) BehaviorValueParameter(arguments[argIndex]);
                }

                // clone the default parameters if they exist
                for (; argIndex < totalArguments; ++argIndex)
                {
                    BehaviorDefaultValuePtr defaultValue = GetDefaultValue(argIndex);
                    if (!defaultValue)
                    {
                        AZ_Warning("Behavior", false, "Not enough arguments to make a call! %d needed %d", numArguments, totalArguments);
                        return false;
                    }
                    new(&newArguments[argIndex]) BehaviorValueParameter(defaultValue->GetValue());
                }

                arguments = newArguments;
            }

            for (size_t i = s_startArgumentIndex; i < AZ_ARRAY_SIZE(m_parameters); ++i)
            {
                if (!arguments[i - 1].ConvertTo(m_parameters[i].m_typeId))
                {
                    AZ_Warning("Behavior", false, "Invalid parameter type for method '%s'! Can not convert method parameter %d from %s(%s) to %s(%s)", m_name.c_str(), i - 1, arguments[i - 1].m_name, arguments[i - 1].m_typeId.template ToString<AZStd::string>().c_str(), m_parameters[i].m_name, m_parameters[i].m_typeId.template ToString<AZStd::string>().c_str());
                    return false;
                }
            }

            CallFunction<R, Args...>::Global(m_functionPtr, arguments, result, AZStd::make_index_sequence<sizeof...(Args)>());

            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        bool BehaviorMethodImpl<R(Args...)>::HasResult() const
        {
            return !AZStd::is_same<R, void>::value;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        bool BehaviorMethodImpl<R(Args...)>::IsMember() const
        {
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        bool BehaviorMethodImpl<R(Args...)>::HasBusId() const
        {
            return false;
        }

        template<class R, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(Args...)>::GetBusIdArgument() const
        {
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        size_t BehaviorMethodImpl<R(Args...)>::GetNumArguments() const
        {
            return AZ_ARRAY_SIZE(m_parameters) - s_startArgumentIndex;
        }

        template<class R, class... Args>
        size_t BehaviorMethodImpl<R(Args...)>::GetMinNumberOfArguments() const
        {
            // Iterate from end of MetadataParameters and count the number of consecutive valid BehaviorValue objects
            size_t numDefaultArguments = 0;
            for (size_t i = GetNumArguments() - 1; i >= 0 && GetDefaultValue(i); --i, ++numDefaultArguments)
            {
            }
            return GetNumArguments() - numDefaultArguments;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(Args...)>::GetArgument(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return &m_parameters[index + s_startArgumentIndex];
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        void BehaviorMethodImpl<R(Args...)>::OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits)
        {
            if (index < GetNumArguments())
            {
                m_parameters[index + s_startArgumentIndex].m_traits = (m_parameters[index + s_startArgumentIndex].m_traits & ~removeTraits) | addTraits;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        const AZStd::string* BehaviorMethodImpl<R(Args...)>::GetArgumentName(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return &m_metadataParameters[index + s_startArgumentIndex].m_name;
            }
            return nullptr;
        }

        template<class R, class... Args>
        void BehaviorMethodImpl<R(Args...)>::SetArgumentName(size_t index, const AZStd::string& name)
        {
            if (index < GetNumArguments())
            {
                m_metadataParameters[index + s_startArgumentIndex].m_name = name;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        const AZStd::string* BehaviorMethodImpl<R(Args...)>::GetArgumentToolTip(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return &m_metadataParameters[index + s_startArgumentIndex].m_toolTip;
            }
            return nullptr;
        }

        template<class R, class... Args>
        void BehaviorMethodImpl<R(Args...)>::SetArgumentToolTip(size_t index, const AZStd::string& toolTip)
        {
            if (index < GetNumArguments())
            {
                m_metadataParameters[index + s_startArgumentIndex].m_toolTip = toolTip;
            }
        }

        template<class R, class... Args>
        void BehaviorMethodImpl<R(Args...)>::SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue)
        {
            if (index < GetNumArguments())
            {
                if (defaultValue && defaultValue->GetValue().m_typeId != GetArgument(index)->m_typeId)
                {
                    AZ_Assert(false, "Argument %zu default value type, doesn't match! Default value should be the same type! Current type %s!", index, defaultValue->GetValue().m_name);
                    return;
                }
                m_metadataParameters[index + s_startArgumentIndex].m_defaultValue = defaultValue;
            }
        }

        template<class R, class... Args>
        BehaviorDefaultValuePtr BehaviorMethodImpl<R(Args...)>::GetDefaultValue(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return m_metadataParameters[index + s_startArgumentIndex].m_defaultValue;
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(Args...)>::GetResult() const
        {
            return &m_parameters[0];
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        BehaviorMethodImpl<R(C::*)(Args...)>::BehaviorMethodImpl(FunctionPointer functionPointer, BehaviorContext* context, const AZStd::string& name)
            : BehaviorMethod(context)
            , m_functionPtr(functionPointer)
        {
            m_name = name;
            SetParameters<R>(m_parameters, this);
            SetParameters<C*>(&m_parameters[s_startArgumentIndex], this);
            m_parameters[s_startArgumentIndex].m_traits |= BehaviorParameter::TR_THIS_PTR;
            SetParameters<Args...>(&m_parameters[s_startNamedArgumentIndex], this);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        BehaviorMethodImpl<R(C::*)(Args...)>::BehaviorMethodImpl(FunctionPointerConst functionPointer, BehaviorContext* context, const AZStd::string& name)
            : BehaviorMethodImpl(reinterpret_cast<FunctionPointer>(functionPointer), context, name)
        {
            m_isConst = true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        bool BehaviorMethodImpl<R(C::*)(Args...)>::Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result) const
        {
            size_t totalArguments = GetNumArguments();
            if (numArguments < totalArguments)
            {
                // We are cloning all arguments on the stack, since Call is called only from Invoke we can reserve bigger "arguments" array
                // that can always handle all parameters. So far the don't use default values that ofter, so we will optimize for the common case first.
                BehaviorValueParameter* newArguments = reinterpret_cast<BehaviorValueParameter*>(alloca(sizeof(BehaviorValueParameter)*  totalArguments));
                // clone the input parameters (we don't need to clone temp buffers, etc. as they will be still on the stack)
                size_t argIndex = 0;
                for (; argIndex < numArguments; ++argIndex)
                {
                    new(&newArguments[argIndex]) BehaviorValueParameter(arguments[argIndex]);
                }

                // clone the default parameters if they exist
                for (; argIndex < totalArguments; ++argIndex)
                {
                    BehaviorDefaultValuePtr defaultValue = GetDefaultValue(argIndex);
                    if (!defaultValue)
                    {
                        AZ_Warning("Behavior", false, "Not enough arguments to make a call! %d needed %d", numArguments, totalArguments);
                        return false;
                    }
                    new(&newArguments[argIndex]) BehaviorValueParameter(defaultValue->GetValue());
                }

                arguments = newArguments;
            }

            if (!arguments[0].ConvertTo(AzTypeInfo<C>::Uuid()))
            {
                // this pointer is invalid
                AZ_Warning("Behavior", false, "First parameter should be the 'this' pointer for the member function! %s", m_name.c_str());
                return false;
            }

            for (size_t i = s_startNamedArgumentIndex; i < AZ_ARRAY_SIZE(m_parameters); ++i)
            {
                if (!arguments[i - 1].ConvertTo(m_parameters[i].m_typeId))
                {
                    AZ_Warning("Behavior", false, "Invalid parameter type for method '%s'! Can not convert method parameter %d from %s(%s) to %s(%s)", m_name.c_str(), i - 1, arguments[i - 1].m_name, arguments[i - 1].m_typeId.template ToString<AZStd::string>().c_str(), m_parameters[i].m_name, m_parameters[i].m_typeId.template ToString<AZStd::string>().c_str());
                    return false;
                }
            }

            CallFunction<R, Args...>::Member(m_functionPtr, *arguments[0].GetAsUnsafe<C*>(), &arguments[1], result, AZStd::make_index_sequence<sizeof...(Args)>());

            EBUS_EVENT_ID(((void*)(*arguments[0].GetAsUnsafe<C*>())), BehaviorObjectSignals, OnMemberMethodCalled, this);

            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        bool BehaviorMethodImpl<R(C::*)(Args...)>::HasResult() const
        {
            return !AZStd::is_same<R, void>::value;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        bool BehaviorMethodImpl<R(C::*)(Args...)>::IsMember() const
        {
            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        bool BehaviorMethodImpl<R(C::*)(Args...)>::HasBusId() const
        {
            return false;
        }

        template<class R, class C, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(C::*)(Args...)>::GetBusIdArgument() const
        {
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        size_t BehaviorMethodImpl<R(C::*)(Args...)>::GetNumArguments() const
        {
            return AZ_ARRAY_SIZE(m_parameters) - s_startArgumentIndex;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        size_t BehaviorMethodImpl<R(C::*)(Args...)>::GetMinNumberOfArguments() const
        {
            // Iterate from end of MetadataParameters and count the number of consecutive valid BehaviorValue objects
            size_t numDefaultArguments = 0;
            for (size_t i = GetNumArguments() - 1; i >= 0 && GetDefaultValue(i); --i, ++numDefaultArguments)
            {
            }
            return GetNumArguments() - numDefaultArguments;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(C::*)(Args...)>::GetArgument(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return &m_parameters[index + s_startArgumentIndex];
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        void BehaviorMethodImpl<R(C::*)(Args...)>::OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits)
        {
            if (index < GetNumArguments())
            {
                m_parameters[index + s_startArgumentIndex].m_traits = (m_parameters[index + s_startArgumentIndex].m_traits & ~removeTraits) | addTraits;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        const AZStd::string* BehaviorMethodImpl<R(C::*)(Args...)>::GetArgumentName(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return &m_metadataParameters[index + s_startArgumentIndex].m_name;
            }
            return nullptr;
        }

        template<class R, class C, class... Args>
        void BehaviorMethodImpl<R(C::*)(Args...)>::SetArgumentName(size_t index, const AZStd::string& name)
        {
            if (index < GetNumArguments())
            {
                m_metadataParameters[index + s_startArgumentIndex].m_name = name;
            }
        }

        template<class R, class C, class... Args>
        const AZStd::string* BehaviorMethodImpl<R(C::*)(Args...)>::GetArgumentToolTip(size_t index) const
        {
            if (index <GetNumArguments())
            {
                return &m_metadataParameters[index + s_startArgumentIndex].m_toolTip;
            }
            return nullptr;
        }

        template<class R, class C, class... Args>
        void BehaviorMethodImpl<R(C::*)(Args...)>::SetArgumentToolTip(size_t index, const AZStd::string& toolTip)
        {
            if (index < GetNumArguments())
            {
                m_metadataParameters[index + s_startArgumentIndex].m_toolTip = toolTip;
            }
        }

        template<class R, class C, class... Args>
        void BehaviorMethodImpl<R(C::*)(Args...)>::SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue)
        {
            if (index < GetNumArguments())
            {
                if (defaultValue && defaultValue->GetValue().m_typeId != GetArgument(index)->m_typeId)
                {
                    AZ_Assert(false, "Argument %zu default value type, doesn't match! Default value should be the same type! Current type %s!", index, defaultValue->GetValue().m_name);
                    return;
                }
                m_metadataParameters[index + s_startArgumentIndex].m_defaultValue = defaultValue;
            }
        }

        template<class R, class C, class... Args>
        BehaviorDefaultValuePtr BehaviorMethodImpl<R(C::*)(Args...)>::GetDefaultValue(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return m_metadataParameters[index + s_startArgumentIndex].m_defaultValue;
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(C::*)(Args...)>::GetResult() const
        {
            return &m_parameters[0];
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::BehaviorEBusEvent(FunctionPointer functionPointer, BehaviorContext* context)
            : BehaviorMethod(context)
            , m_functionPtr(functionPointer)
        {
            SetParameters<R>(m_parameters, this);
            SetParameters<Args...>(&m_parameters[s_startNamedArgumentIndex], this);
            // optional ID parameter
            SetBusIdType<s_isBusIdParameter == 1>();
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::BehaviorEBusEvent(FunctionPointerConst functionPointer, BehaviorContext* context)
            : BehaviorEBusEvent(reinterpret_cast<FunctionPointer>(functionPointer), context)
        {
            m_isConst = true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        template<bool IsBusId>
        inline AZStd::enable_if_t<IsBusId> BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::SetBusIdType()
        {
            SetParameters<typename EBus::BusIdType>(&m_parameters[s_startArgumentIndex]);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        template<bool IsBusId>
        inline AZStd::enable_if_t<!IsBusId> BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::SetBusIdType()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        bool BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result) const
        {
            size_t totalArguments = GetNumArguments();
            if (numArguments < totalArguments)
            {
                // We are cloning all arguments on the stack, since Call is called only from Invoke we can reserve bigger "arguments" array
                // that can always handle all parameters. So far the don't use default values that ofter, so we will optimize for the common case first.
                BehaviorValueParameter* newArguments = reinterpret_cast<BehaviorValueParameter*>(alloca(sizeof(BehaviorValueParameter)*  totalArguments));
                // clone the input parameters (we don't need to clone temp buffers, etc. as they will be still on the stack)
                size_t argIndex = 0;
                for (; argIndex < numArguments; ++argIndex)
                {
                    new(&newArguments[argIndex]) BehaviorValueParameter(arguments[argIndex]);
                }

                // clone the default parameters if they exist
                for (; argIndex < totalArguments; ++argIndex)
                {
                    BehaviorDefaultValuePtr defaultValue = GetDefaultValue(argIndex);
                    if (!defaultValue)
                    {
                        AZ_Warning("Behavior", false, "Not enough arguments to make a call! %d needed %d", numArguments, totalArguments);
                        return false;
                    }
                    new(&newArguments[argIndex]) BehaviorValueParameter(defaultValue->GetValue());
                }

                arguments = newArguments;
            }

            if constexpr (s_isBusIdParameter)
            {
                if (!arguments[0].ConvertTo(m_parameters[1].m_typeId))
                {
                    AZ_Warning("Behavior", false, "Invalid BusIdType type can't convert! %s -> %s", arguments[0].m_name, m_parameters[1].m_name);
                    return false;
                }
            }

            for (size_t i = s_startNamedArgumentIndex; i < AZ_ARRAY_SIZE(m_parameters); ++i)
            {
                if (!arguments[i - 1].ConvertTo(m_parameters[i].m_typeId))
                {
                    AZ_Warning("Behavior", false, "Invalid parameter type for method '%s'! Can not convert method parameter %d from %s(%s) to %s(%s)", m_name.c_str(), i - 1, arguments[i - 1].m_name, arguments[i - 1].m_typeId.template ToString<AZStd::string>().c_str(), m_parameters[i].m_name, m_parameters[i].m_typeId.template ToString<AZStd::string>().c_str());
                    return false;
                }
            }

            AZ::Internal::EBusCaller<EventType, EBus, R, Args...>::Call(m_functionPtr, &arguments[0], result, AZStd::make_index_sequence<sizeof...(Args)>());

            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        bool BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::HasResult() const
        {
            return !AZStd::is_same<R, void>::value;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        bool BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::IsMember() const
        {
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        bool BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::HasBusId() const
        {
            return s_isBusIdParameter != 0;
        }

        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        const BehaviorParameter* BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetBusIdArgument() const
        {
            return HasBusId() ? GetArgument(0) : nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        size_t BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetNumArguments() const
        {
            return AZ_ARRAY_SIZE(m_parameters) - s_startArgumentIndex;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        size_t BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetMinNumberOfArguments() const
        {
            // Iterate from end of MetadataParameters and count the number of consecutive valid BehaviorValue objects
            size_t numDefaultArguments = 0;
            for (size_t i = GetNumArguments() - 1; i >= 0 && GetDefaultValue(i); --i, ++numDefaultArguments)
            {
            }
            return GetNumArguments() - numDefaultArguments;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        const BehaviorParameter* BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetArgument(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return &m_parameters[index + s_startArgumentIndex];
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        void BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits)
        {
            if (index < GetNumArguments())
            {
                m_parameters[index + s_startArgumentIndex].m_traits = (m_parameters[index + s_startArgumentIndex].m_traits & ~removeTraits) | addTraits;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        const AZStd::string* BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetArgumentName(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return &m_metadataParameters[index + s_startArgumentIndex].m_name;
            }
            return nullptr;
        }

        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        void BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::SetArgumentName(size_t index, const AZStd::string& name)
        {
            if (index < GetNumArguments())
            {
                m_metadataParameters[index + s_startArgumentIndex].m_name = name;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        const AZStd::string* BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetArgumentToolTip(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return &m_metadataParameters[index + s_startArgumentIndex].m_toolTip;
            }
            return nullptr;
        }

        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        void BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::SetArgumentToolTip(size_t index, const AZStd::string& toolTip)
        {
            if (index < GetNumArguments())
            {
                m_metadataParameters[index + s_startArgumentIndex].m_toolTip = toolTip;
            }
        }

        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        void BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue)
        {
            if (index < GetNumArguments())
            {
                if (defaultValue && defaultValue->GetValue().m_typeId != GetArgument(index)->m_typeId)
                {
                    AZ_Assert(false, "Argument %zu default value type, doesn't match! Default value should be the same type! Current type %s!", index, defaultValue->GetValue().m_name);
                    return;
                }
                m_metadataParameters[index + s_startArgumentIndex].m_defaultValue = defaultValue;
            }
        }

        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        BehaviorDefaultValuePtr BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetDefaultValue(size_t index) const
        {
            if (index < GetNumArguments())
            {
                return m_metadataParameters[index + s_startArgumentIndex].m_defaultValue;
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        const BehaviorParameter* BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetResult() const
        {
            return &m_parameters[0];
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        void SetFunctionParameters<R(Args...)>::Set(AZStd::vector<BehaviorParameter>& params)
        {
            // result, userdata, arguments
            params.resize(sizeof...(Args) + eBehaviorBusForwarderEventIndices::ParameterFirst);
            SetParameters<R>(&params[eBehaviorBusForwarderEventIndices::Result], nullptr);
            SetParameters<void*>(&params[eBehaviorBusForwarderEventIndices::UserData], nullptr);
            if constexpr (sizeof...(Args) > 0)
            {
                SetParameters<Args...>(&params[eBehaviorBusForwarderEventIndices::ParameterFirst], nullptr);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        bool SetFunctionParameters<R(Args...)>::Check(AZStd::vector<BehaviorParameter>& source)
        {
            Uuid argumentTypes[sizeof...(Args)] = { AzTypeInfo<Args>::Uuid()... };
            // use for error control
            const char* argumentNames[sizeof...(Args)] = { AzTypeInfo<Args>::Name()... };
            (void)argumentNames;
            if (source.size() != sizeof...(Args)+1) // +1 for result
            {
                return false;
            }
            // check result type
            if (source[0].m_typeId != AzTypeInfo<R>::Uuid())
            {
                return false;
            }
            for (size_t i = 0; i < sizeof...(Args); ++i)
            {
                if (source[i + 1].m_typeId != argumentTypes[i])
                {
                    return false;
                }
            }
            return true;
        }


        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        void SetFunctionParameters<R(C::*)(Args...)>::Set(AZStd::vector<BehaviorParameter>& params)
        {
            SetFunctionParameters<R(Args...)>::Set(params);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        bool SetFunctionParameters<R(C::*)(Args...)>::Check(AZStd::vector<BehaviorParameter>& source)
        {
            Uuid argumentTypes[sizeof...(Args)] = { AzTypeInfo<Args>::Uuid()... };
            // use for error control
            const char* argumentNames[sizeof...(Args)] = { AzTypeInfo<Args>::Name()... };
            (void)argumentNames;
            if (source.size() != sizeof...(Args)+1) // +1 for result
            {
                return false;
            }
            // check result type
            if (source[0].m_typeId != AzTypeInfo<R>::Uuid())
            {
                return false;
            }
            for (size_t i = 1; i < sizeof...(Args); ++i)
            {
                if (source[i + 1].m_typeId != argumentTypes[i])
                {
                    return false;
                }
            }
            return true;
        }


        //////////////////////////////////////////////////////////////////////////
        template<class T>
        void* BahaviorDefaultFactory<T>::Create(void* inplaceAddress, void* userData)
        {
            (void)userData;
            if (inplaceAddress)
            {
                return new(inplaceAddress)T();
            }
            else
            {
                return aznew T();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class T>
        void BahaviorDefaultFactory<T>::Destroy(void* objectPtr, bool isFreeMemory, void* userData)
        {
            (void)userData;
            T* object = reinterpret_cast<T*>(objectPtr);
            if (isFreeMemory)
            {
                delete object;
            }
            else
            {
                object->~T();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class T>
        void* BahaviorDefaultFactory<T>::Clone(void* targetAddress, void* sourceAddress, void* userData)
        {
            (void)userData;
            if (targetAddress)
            {
                return new(targetAddress)T(*reinterpret_cast<T*>(sourceAddress));
            }
            else
            {
                return aznew T(*reinterpret_cast<T*>(sourceAddress));
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class Handler>
        BehaviorEBusHandler* BehaviorEBusHandlerFactory<Handler>::Create()
        {
            return aznew Handler();
        }

        //////////////////////////////////////////////////////////////////////////
        template<class Handler>
        void BehaviorEBusHandlerFactory<Handler>::Destroy(BehaviorEBusHandler* handler)
        {
            delete handler;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class BusHandler, bool IsBusId = !AZStd::is_same<typename BusHandler::BusType::BusIdType, AZ::NullBusId>::value>
        struct EBusConnector
        {
            static bool Connect(BusHandler* handler, BehaviorValueParameter* id)
            {
                if (id && id->ConvertTo<typename BusHandler::BusType::BusIdType>())
                {
                    handler->BusConnect(*id->GetAsUnsafe<typename BusHandler::BusType::BusIdType>());
                    return true;
                }
                return false;
            }

            static bool IsConnected(BusHandler* handler)
            {
                return handler->BusIsConnected();
            }

            static bool IsConnectedId(BusHandler* handler, BehaviorValueParameter* id)
            {
                if (id && id->ConvertTo<typename BusHandler::BusType::BusIdType>())
                {
                    return handler->BusIsConnectedId(*id->GetAsUnsafe<typename BusHandler::BusType::BusIdType>());
                }
                else
                {
                    AZ_Warning("BehaviorContext", false, "Invalid Id argument. Please make sure the type of Id is correct.");
                    return false;
                }
            }
        };

        //////////////////////////////////////////////////////////////////////////
        template<class BusHandler>
        struct EBusConnector<BusHandler, false>
        {
            static bool Connect(BusHandler* handler, BehaviorValueParameter* id)
            {
                (void)id;
                handler->BusConnect();
                return true;
            }

            static bool IsConnected(BusHandler* handler)
            {
                return handler->BusIsConnected();
            }

            static bool IsConnectedId(BusHandler* handler, BehaviorValueParameter* id)
            {
                (void)id;
                AZ_Warning("BehaviorContext", false, "Function IsConnectedId is called on an EBus handler that was initially connected without Id. Please use IsConnected instead.");
                return handler->BusIsConnected();
            }
        };

        //////////////////////////////////////////////////////////////////////////
        // Passed to Attribute::SetContext data, to destroy the behavior method
        inline void DestroyAttributeMethod(void* contextData)
        {
            delete reinterpret_cast<BehaviorMethod*>(contextData);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class Owner>
        template<class T>
        void GenericAttributes<Owner>::SetAttributeContextData(T value, AZ::Attribute* attribute, const AZStd::true_type& /* is_function<remove_pointer<T>::type> && is_member_function_pointer<T>*/)
        {
            BehaviorMethod* method = aznew AZ::Internal::BehaviorMethodImpl<typename AZStd::remove_pointer<T>::type>(value, m_context, AZStd::string("Function-Attribute"));
            attribute->SetContextData(method, &DestroyAttributeMethod);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class Owner>
        template<class T>
        Owner* GenericAttributes<Owner>::Attribute(Crc32 idCrc, T value)
        {
            if (m_context->IsRemovingReflection())
            {
                return static_cast<Owner*>(this);
            }

            using ContainerType = AttributeContainerType<T>;

            //AZ_Assert(Internal::AttributeValueTypeClassChecker<T>::Check(m_classData->m_typeId), "Attribute (0x%08x) doesn't belong to '%s' class! You can't reference other classes!", idCrc, m_classData->m_name);
            AZ_Assert(m_currentAttributes, "You can attach attributes to Methods, Properties, Classes, EBuses and EBus Events!");
            if (m_currentAttributes)
            {
                AZ::Attribute* attribute = aznew ContainerType(value);
                SetAttributeContextData<T>(value, attribute, AZStd::integral_constant<bool, AZStd::is_member_function_pointer<T>::value | AZStd::is_function<typename AZStd::remove_pointer<T>::type>::value>());
                m_currentAttributes->push_back(AttributePair(idCrc, attribute));
            }
            return static_cast<Owner*>(this);
        }

        template<typename Bus>
        class EBusAttributes :
            public AZ::Internal::GenericAttributes<BehaviorContext::EBusBuilder<Bus>>
        {
        protected:

            using Base = AZ::Internal::GenericAttributes<BehaviorContext::EBusBuilder<Bus>>;

            EBusAttributes(BehaviorContext* context)
                : AZ::Internal::GenericAttributes<BehaviorContext::EBusBuilder<Bus>>(context)
            {
            }

            void SetEBusEventSender(BehaviorEBusEventSender* ebusSender);

        public:
            using AZ::Internal::GenericAttributes<BehaviorContext::EBusBuilder<Bus>>::Attribute;
            template<class T>
            BehaviorContext::EBusBuilder<Bus>* Attribute(Crc32 idCrc, T value);

            /**
            * Applies Attribute to the Event BehaviorMethod if an EBusEventSender is set
            */
            template<class T>
            BehaviorContext::EBusBuilder<Bus>* BroadcastAttribute(Crc32 idCrc, const T& value);

            /**
            * Applies Attribute to the Event BehaviorMethod if the EBusEventSender is set
            * and the EBus supports firing individual events
            */
            template<class T>
            BehaviorContext::EBusBuilder<Bus>* EventAttribute(Crc32 idCrc, const T& value);

            /**
            * Applies Attribute to the Event BehaviorMethod if the EBusEventSender is set
            * and the EBus supports queuing broadcast events
            */
            template<class T>
            BehaviorContext::EBusBuilder<Bus>* QueueBroadcastAttribute(Crc32 idCrc, const T& value);

            /**
            * Applies Attribute to the Event BehaviorMethod if the EBusEventSender is set
            * and the EBus supports queuing individual events
            */
            template<class T>
            BehaviorContext::EBusBuilder<Bus>* QueueEventAttribute(Crc32 idCrc, const T& value);

        private:
            BehaviorEBusEventSender * m_currentEBusSender = nullptr;
        };

        template<typename Bus>
        void EBusAttributes<Bus>::SetEBusEventSender(BehaviorEBusEventSender* ebusSender)
        {
            m_currentEBusSender = ebusSender;
        }

        template<class Bus>
        template<class T>
        BehaviorContext::EBusBuilder<Bus>* EBusAttributes<Bus>::BroadcastAttribute(Crc32 idCrc, const T& value)
        {
            if (m_currentEBusSender &&  m_currentEBusSender->m_broadcast)
            {
                AZ::Attribute* eventAttribute = aznew AttributeContainerType<T>(value);
                Base::template SetAttributeContextData<T>(value, eventAttribute, AZStd::integral_constant<bool, AZStd::is_member_function_pointer<T>::value || AZStd::is_function<typename AZStd::remove_pointer<T>::type>::value>());
                m_currentEBusSender->m_broadcast->m_attributes.emplace_back(AttributePair(idCrc, eventAttribute));
            }
            return static_cast<BehaviorContext::EBusBuilder<Bus>*>(this);
        }

        template<class Bus>
        template<class T>
        BehaviorContext::EBusBuilder<Bus>* EBusAttributes<Bus>::EventAttribute(Crc32 idCrc, const T& value)
        {
            if (!Base::m_context->IsRemovingReflection() && m_currentEBusSender && m_currentEBusSender->m_event)
            {
                AZ::Attribute* eventAttribute = aznew AttributeContainerType<T>(value);
                Base::template SetAttributeContextData<T>(value, eventAttribute, AZStd::integral_constant<bool, AZStd::is_member_function_pointer<T>::value || AZStd::is_function<typename AZStd::remove_pointer<T>::type>::value>());
                m_currentEBusSender->m_event->m_attributes.emplace_back(AttributePair(idCrc, eventAttribute));
            }
            return static_cast<BehaviorContext::EBusBuilder<Bus>*>(this);
        }

        template<class Bus>
        template<class T>
        BehaviorContext::EBusBuilder<Bus>* EBusAttributes<Bus>::QueueBroadcastAttribute(Crc32 idCrc, const T& value)
        {
            if (!Base::m_context->IsRemovingReflection() && m_currentEBusSender && m_currentEBusSender->m_queueBroadcast)
            {
                AZ::Attribute* eventAttribute = aznew AttributeContainerType<T>(value);
                Base::template SetAttributeContextData<T>(value, eventAttribute, AZStd::integral_constant<bool, AZStd::is_member_function_pointer<T>::value || AZStd::is_function<typename AZStd::remove_pointer<T>::type>::value>());
                m_currentEBusSender->m_queueBroadcast->m_attributes.emplace_back(AttributePair(idCrc, eventAttribute));
            }
            return static_cast<BehaviorContext::EBusBuilder<Bus>*>(this);
        }

        template<class Bus>
        template<class T>
        BehaviorContext::EBusBuilder<Bus>* EBusAttributes<Bus>::QueueEventAttribute(Crc32 idCrc, const T& value)
        {
            if (!Base::m_context->IsRemovingReflection() && m_currentEBusSender && m_currentEBusSender->m_queueEvent)
            {
                AZ::Attribute* eventAttribute = aznew AttributeContainerType<T>(value);
                Base::template SetAttributeContextData<T>(value, eventAttribute, AZStd::integral_constant<bool, AZStd::is_member_function_pointer<T>::value || AZStd::is_function<typename AZStd::remove_pointer<T>::type>::value>());
                m_currentEBusSender->m_queueEvent->m_attributes.emplace_back(AttributePair(idCrc, eventAttribute));
            }
            return static_cast<BehaviorContext::EBusBuilder<Bus>*>(this);
        }

        template<class Bus>
        template<class T>
        BehaviorContext::EBusBuilder<Bus>* EBusAttributes<Bus>::Attribute(Crc32 idCrc, T value)
        {
            if (Base::m_context->IsRemovingReflection())
            {
                return static_cast<BehaviorContext::EBusBuilder<Bus>*>(this);
            }

            // Apply attributes to each EBusEventSender BehaviorMethods if one is set on this instance
            BroadcastAttribute(idCrc, value);
            EventAttribute(idCrc, value);
            QueueBroadcastAttribute(idCrc, value);
            QueueEventAttribute(idCrc, value);

            // Apply attribute to the current bound attribute address which could on a EBus, EBusEventSender or Ebus CreateHandler instance
            if (Base::m_currentAttributes)
            {
                AZ::Attribute* ebusAttribute = aznew AttributeContainerType<T>(value);
                Base::template SetAttributeContextData<T>(value, ebusAttribute, AZStd::integral_constant<bool, AZStd::is_member_function_pointer<T>::value | AZStd::is_function<typename AZStd::remove_pointer<T>::type>::value>());
                Base::m_currentAttributes->emplace_back(AttributePair(idCrc, ebusAttribute));
            }

            return static_cast<BehaviorContext::EBusBuilder<Bus>*>(this);
        }

        bool IsInScope(const AttributeArray& attributes, const AZ::Script::Attributes::ScopeFlags scope);

    } // namespace Internal
} // namespace AZ

// pull AzStd on demand reflection
#include <AzCore/RTTI/AzStdOnDemandPrettyName.inl>
#include <AzCore/RTTI/AzStdOnDemandReflection.inl>

