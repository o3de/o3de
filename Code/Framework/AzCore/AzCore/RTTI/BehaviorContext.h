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
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_abstract.h>
#include <AzCore/std/typetraits/is_destructible.h>
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
    const static AZ::Crc32 RuntimeEBusAttribute = AZ_CRC_CE("RuntimeEBus"); ///< Signals that this reflected ebus should only be available at runtime, helps tools filter out data driven ebuses

    constexpr const char* k_PropertyNameGetterSuffix = "::Getter";
    constexpr const char* k_PropertyNameSetterSuffix = "::Setter";

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
        AZ_TYPE_INFO_WITH_NAME_DECL(BehaviorObject);

        BehaviorObject();
        BehaviorObject(void* address, const Uuid& typeId);
        BehaviorObject(void* address, IRttiHelper* rttiHelper);
        bool IsValid() const;

        void* m_address;
        Uuid m_typeId;
        IRttiHelper* m_rttiHelper{};
    };

    /**
     * Reflects information about a C++ function parameter, mostly for use in BehaviorContext Methods and Events. It only provides a
     * description or the C++ characteristics of the parameter, e.g. type and storage specification. It does not refer to any C++ value.
     *
     * \note During actual runtime calls, using the BehaviorContext generic calling mechanism, use \ref BehaviorArgument to wrap arguments
     * to the generic function call.
     */
    struct BehaviorParameter
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(BehaviorParameter);
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
            u32 addTraits = BehaviorParameter::Traits::TR_NONE, u32 removeTraits = BehaviorParameter::Traits::TR_NONE);

        AZStd::string m_name;
        AZStd::string m_toolTip;
        BehaviorDefaultValuePtr m_defaultValue;
        AZ::u32 m_addTraits;
        AZ::u32 m_removeTraits;
    };

    /**
     * BehaviorArgument is used to wrap an actual C++ argument during a generic call using the BehaviorContext calling mechanisms.
     * It is also used to wrap return values, as BehaviorContext return values are passed as the first argument into a BehaviorContextMethod.
     * \note This is generally used for calls on the C++ runtime stack. It should not be reused or stored as it likely stores temporary data
     * produced during conversion of the object on the stack.
     *
     * For reflecting type information of C+++ parameters use \ref BehaviorParameter.
     */
    struct BehaviorArgumentValueTypeTag_t {};
    constexpr BehaviorArgumentValueTypeTag_t BehaviorArgumentValueTypeTag;

    struct BehaviorArgument : public BehaviorParameter
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(BehaviorArgument);

        BehaviorArgument();
        BehaviorArgument(const BehaviorArgument&) = default;
        BehaviorArgument(BehaviorArgument&&);
        template<class T>
        BehaviorArgument(T* value);

        /// Special handling for the generic object holder.
        BehaviorArgument(BehaviorObject* value);
        BehaviorArgument(BehaviorArgumentValueTypeTag_t, BehaviorObject* value);

        template<class T>
        void Set(T* value);
        void Set(BehaviorObject* value);
        void Set(BehaviorArgumentValueTypeTag_t, BehaviorObject* value);
        void Set(const BehaviorParameter& param);
        void Set(const BehaviorArgument& param);

        void* GetValueAddress() const;

        /// Convert to BehaviorObject implicitly for passing generic parameters (usually not known at compile time)
        operator BehaviorObject() const;

        /// Converts internally the value to a specific type known at compile time. \returns true if conversion was successful.
        template<class T>
        bool ConvertTo();

        /// Converts a value to a specific one by typeID (usually when the type is not known at compile time)
        bool ConvertTo(const AZ::Uuid& typeId);

        /// This function is Unsafe, because it assumes that you have called ConvertTo<T> prior to called it and it returned true (basically mean the BehaviorArgument is converted to T)
        template<typename T>
        AZStd::decay_t<T>* GetAsUnsafe() const;

        BehaviorArgument& operator=(const BehaviorArgument&) = default;
        BehaviorArgument& operator=(BehaviorArgument&&);
        template<typename T>
        BehaviorArgument& operator=(T&& result);

        /// Stores a value (usually return value of a function).
        template<typename T>
        bool StoreResult(T&& result);

        /// Used internally to store values in the temp data
        template<typename T, typename = AZStd::enable_if_t<AZStd::is_trivially_destructible_v<AZStd::decay_t<T>>>>
        void StoreInTempData(T&& value);

        void* m_value;  ///< Pointer to value, keep it mind to check the traits as if the value is pointer, this will be pointer to pointer and use \ref GetValueAddress to get the actual value address
        AZStd::function<void()> m_onAssignedResult;
        BehaviorParameter::TempValueParameterAllocator m_tempData; ///< Temp data for conversion, etc. while preparing the parameter for a call (POD only)

    private:
        friend class BehaviorDefaultValue;

        template<typename T>
        void StoreInTempDataEvenIfNonTrivial(T&& value);
    };

    /**
    * Class that handles a single default value. The Value type is verified to match parameter signature
    */
    class BehaviorDefaultValue
        : public AZStd::intrusive_base
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorDefaultValue, AZ::SystemAllocator);

        BehaviorDefaultValue(const BehaviorDefaultValue&) = delete;
        BehaviorDefaultValue(BehaviorDefaultValue&&) = delete;
        BehaviorDefaultValue& operator=(const BehaviorDefaultValue&) = delete;
        BehaviorDefaultValue& operator=(BehaviorDefaultValue&&) = delete;

        /**
        * Create a default value for a specific method parameter. The Default values is stored by value
        * in a temp storage, so currently there is limit the \ref BehaviorArgument temp storage, we
        * can easily change that if it became a problem.
        */
        template<typename Value>
        BehaviorDefaultValue(Value&& value)
        {
            m_value.StoreInTempDataEvenIfNonTrivial(AZStd::forward<Value>(value));
            if constexpr (!AZStd::is_trivially_destructible_v<AZStd::decay_t<Value>>)
            {
                // BehaviorArgument does not destroy instances of types set
                // with StoreInTempData(). It does not take ownership of that
                // instance, because it is designed to reference existing
                // values on the stack. For a parameter's default value,
                // however, that instance must be owned by something, so that
                // ownership is the responsibility of the BehaviorDefaultValue
                // type. If the type of the default value has a non-trivial
                // destructor, this class will invoke that destructor when it
                // itself is destroyed.
                m_destructor = [](void* valueAddress)
                {
                    AZStd::destroy_at(reinterpret_cast<AZStd::decay_t<Value>*>(valueAddress));
                };
            }
        }

        ~BehaviorDefaultValue() override;

        const BehaviorArgument& GetValue() const;

        BehaviorArgument m_value;
    private:
        using DestructorFunc = void(*)(void*);
        DestructorFunc m_destructor = nullptr;
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
        AZ_TYPE_INFO_WITH_NAME_DECL(BehaviorAzEventDescription);
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
        using ResultOutcome = AZ::Outcome<void, AZStd::string>;
        BehaviorMethod(BehaviorContext* context);
        ~BehaviorMethod() override;

        template<class... Args>
        bool Invoke(Args&&... args) const;
        bool Invoke() const;

        template<class R, class... Args>
        bool InvokeResult(R& r, Args&&... args) const;
        template<class R>
        bool InvokeResult(R& r) const;
        void SetDeprecatedName(AZStd::string name);
        const AZStd::string& GetDeprecatedName() const;

        virtual bool Call(AZStd::span<BehaviorArgument> arguments, BehaviorArgument* result = nullptr) const = 0;
        bool Call(BehaviorArgument* arguments, unsigned int numArguments, BehaviorArgument* result = nullptr) const;

        //! Returns success if the method is callable with the supplied arguments otherwise an error message
        //! is returned with the reason the method is not callable
        virtual ResultOutcome IsCallable(AZStd::span<BehaviorArgument> arguments, BehaviorArgument* result = nullptr) const = 0;

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
        virtual void SetArgumentName(size_t index, AZStd::string name) = 0;
        virtual const AZStd::string* GetArgumentToolTip(size_t index) const = 0;
        virtual void SetArgumentToolTip(size_t index, AZStd::string name) = 0;
        virtual void SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue) = 0;
        virtual BehaviorDefaultValuePtr GetDefaultValue(size_t index) const = 0;
        virtual const BehaviorParameter* GetResult() const = 0;
        void ProcessAuxiliaryMethods(BehaviorContext* context, BehaviorMethod& method);

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
        AZ_TYPE_INFO_WITH_NAME_DECL(InputRestriction);
        AZ_CLASS_ALLOCATOR(InputRestriction, AZ::SystemAllocator);

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
        AZ_TYPE_INFO_WITH_NAME_DECL(BranchOnResultInfo);
        AZ_CLASS_ALLOCATOR(BranchOnResultInfo, AZ::SystemAllocator);

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
        AZ_TYPE_INFO_WITH_NAME_DECL(CheckedOperationInfo);
        AZ_CLASS_ALLOCATOR(CheckedOperationInfo, AZ::SystemAllocator);

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
        AZ_TYPE_INFO_WITH_NAME_DECL(OverloadArgumentGroupInfo);
        AZ_CLASS_ALLOCATOR(OverloadArgumentGroupInfo, AZ::SystemAllocator);

        AZStd::vector<AZStd::string> m_parameterGroupNames;
        AZStd::vector<AZStd::string> m_resultGroupNames;

        OverloadArgumentGroupInfo() = default;

        OverloadArgumentGroupInfo(const AZStd::vector<AZStd::string>&& parameterGroupNames, const AZStd::vector<AZStd::string>&& resultGroupName);
    };

    struct ExplicitOverloadInfo
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(ExplicitOverloadInfo);
        AZ_CLASS_ALLOCATOR(ExplicitOverloadInfo, AZ::SystemAllocator);

        AZStd::string m_name;
        AZStd::string m_categoryPath;
        AZStd::vector<AZStd::pair<AZ::BehaviorMethod*, AZ::BehaviorClass*>> m_overloads;

        ExplicitOverloadInfo() = default;

        ExplicitOverloadInfo(AZStd::string_view name, AZStd::string_view categoryPath);

        constexpr operator size_t() const
        {
            size_t h = 0;
            hash_combine(h, m_name);
            return h;
        }

        // \todo replace in hash operations with custom equality check
        bool operator==(const ExplicitOverloadInfo& other) const;
        bool operator!=(const ExplicitOverloadInfo& other) const;
    };
}

namespace AZ
{
    // AZ::Event support
    using BehaviorFunction = AZStd::function<void(BehaviorArgument* result, BehaviorArgument* arguments, int numArguments)>;
    using EventHandlerCreationFunction = AZStd::function<BehaviorObject(void*, BehaviorFunction&&)>;

    struct EventHandlerCreationFunctionHolder
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(EventHandlerCreationFunctionHolder);
        AZ_CLASS_ALLOCATOR(EventHandlerCreationFunctionHolder, AZ::SystemAllocator);

        EventHandlerCreationFunction m_function;
    };
}
namespace AZ::Internal
{
    AZ::TypeId GetUnderlyingTypeId(const IRttiHelper& enumRttiHelper);

    // Converts sourceAddress to targetType
    bool ConvertValueTo(void* sourceAddress, const IRttiHelper* sourceRtti, const AZ::Uuid& targetType, void*& targetAddress, BehaviorParameter::TempValueParameterAllocator& tempAllocator);

    // Assumes parameters array is big enough to store all parameters
    template<class... Args>
    void SetParameters(BehaviorParameter* parameters, OnDemandReflectionOwner* onDemandReflection = nullptr);


    //! Uses a the BehaviorArgument struct to wrap types that implement the GetO3deType* functions
    //! This is used by the BehaviorMethod and BehaviorEBus to implement a type-erased structure
    //! in order to reduce template instantiations from this header
    //! The BehaviorContext.h header is one of the most expensive headers when it comes to compile
    //! time within O3DE
    using BehaviorMethodFunctor = AZStd::function<void(BehaviorArgument*, AZStd::span<BehaviorArgument>)>;
    //! BehaviorMethod reflection will only support functions up to 32 parameters
    constexpr size_t MaxBehaviorParameters = 32;

    class BehaviorMethodImpl : public BehaviorMethod
    {
    public:
        using ClassType = void;

        AZ_CLASS_ALLOCATOR(BehaviorMethodImpl, AZ::SystemAllocator);

        static const int s_startArgumentIndex = 1; // +1 for result type

        template <class R, class... Args>
        BehaviorMethodImpl(R(*functionPointer)(Args...), BehaviorContext* context, AZStd::string name = {});

        template <class R, class C, class... Args>
        BehaviorMethodImpl(R(C::* functionPointer)(Args...), BehaviorContext* context, AZStd::string name = {});

        template <class R, class C, class... Args>
        BehaviorMethodImpl(R(C::* functionPointer)(Args...) const, BehaviorContext* context, AZStd::string name = {});

#if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must be distinguished from non-noexcept overloads
        template <class R, class... Args>
        BehaviorMethodImpl(R(*functionPointer)(Args...) noexcept, BehaviorContext* context, AZStd::string name = {});

        template <class R, class C, class... Args>
        BehaviorMethodImpl(R(C::* functionPointer)(Args...) noexcept, BehaviorContext* context, AZStd::string name = {});

        template <class R, class C, class... Args>
        BehaviorMethodImpl(R(C::* functionPointer)(Args...) const noexcept, BehaviorContext* context, AZStd::string name = {});
#endif

        bool Call(AZStd::span<BehaviorArgument> arguments, BehaviorArgument* result) const override;
        // The implementation should return true if the method can be called with the specified BehaviorArgument list
        ResultOutcome IsCallable(AZStd::span<BehaviorArgument> arguments, BehaviorArgument* result) const override;

        bool HasResult() const override;
        bool IsMember() const override;
        bool HasBusId() const override;

        const BehaviorParameter* GetBusIdArgument() const override;

        size_t GetNumArguments() const override;
        size_t GetMinNumberOfArguments() const override;

        const BehaviorParameter* GetArgument(size_t index) const override;
        const AZStd::string* GetArgumentName(size_t index) const override;
        void SetArgumentName(size_t index, AZStd::string name) override;
        const AZStd::string* GetArgumentToolTip(size_t index) const override;
        void SetArgumentToolTip(size_t index, AZStd::string name) override;
        void SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue) override;
        BehaviorDefaultValuePtr GetDefaultValue(size_t index) const override;
        const BehaviorParameter* GetResult() const override;

        void OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits) override;

    protected:
        BehaviorMethodFunctor m_functor;

        AZStd::fixed_vector<BehaviorParameter, MaxBehaviorParameters> m_parameters;
        //! Stores the per parameter metadata which is used to add names, tooltips, trait, default values, etc... to the parameters
        AZStd::fixed_vector<BehaviorParameterMetadata, MaxBehaviorParameters> m_metadataParameters;

    private:
        int m_startNamedArgumentIndex = s_startArgumentIndex; // +1 for result type
        bool m_hasNonVoidReturn{};
        bool m_isMemberFunc{};
    };
}

namespace AZ::Internal
{
    enum class BehaviorEventType
    {
        BE_BROADCAST,
        BE_EVENT_ID,
        BE_QUEUE_BROADCAST,
        BE_QUEUE_EVENT_ID,
    };

    class BehaviorEBusEvent : public BehaviorMethod
    {
    public:
        static const int s_startArgumentIndex = 1; // +1 for result type

        AZ_CLASS_ALLOCATOR(BehaviorEBusEvent, AZ::SystemAllocator);

        template<class EBus, class R, class BusType, class... Args>
        BehaviorEBusEvent(R(*functionPointer)(BusType*, Args...), BehaviorContext* context, BehaviorEventType eventType, AZStd::type_identity<EBus>);

        template<class EBus, class R, class C, class... Args>
        BehaviorEBusEvent(R(C::* functionPointer)(Args...), BehaviorContext* context, BehaviorEventType eventType, AZStd::type_identity<EBus>);

        template<class EBus, class R, class C, class... Args>
        BehaviorEBusEvent(R(C::* functionPointer)(Args...) const, BehaviorContext* context, BehaviorEventType eventType, AZStd::type_identity<EBus>);

#if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must be distinguished from non-noexcept overloads
        template<class EBus, class R, class BusType, class... Args>
        BehaviorEBusEvent(R(*functionPointer)(BusType*, Args...) noexcept, BehaviorContext* context, BehaviorEventType eventType, AZStd::type_identity<EBus>);

        template<class EBus, class R, class C, class... Args>
        BehaviorEBusEvent(R(C::* functionPointer)(Args...) noexcept, BehaviorContext* context, BehaviorEventType eventType, AZStd::type_identity<EBus>);

        template<class EBus, class R, class C, class... Args>
        BehaviorEBusEvent(R(C::* functionPointer)(Args...) const noexcept, BehaviorContext* context, BehaviorEventType eventType, AZStd::type_identity<EBus>);
#endif

        bool Call(AZStd::span<BehaviorArgument> arguments, BehaviorArgument* result) const override;
        ResultOutcome IsCallable(AZStd::span<BehaviorArgument> arguments, BehaviorArgument* result = nullptr) const;
        bool HasResult() const override;
        bool IsMember() const override;
        bool HasBusId() const override;

        const BehaviorParameter* GetBusIdArgument() const override;

        size_t GetNumArguments() const override;
        size_t GetMinNumberOfArguments() const override;

        const BehaviorParameter* GetArgument(size_t index) const override;
        const AZStd::string* GetArgumentName(size_t index) const override;
        void SetArgumentName(size_t index, AZStd::string name) override;
        const AZStd::string* GetArgumentToolTip(size_t index) const override;
        void SetArgumentToolTip(size_t index, AZStd::string name) override;
        void SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue) override;
        BehaviorDefaultValuePtr GetDefaultValue(size_t index) const override;
        const BehaviorParameter* GetResult() const override;

        void OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits) override;

    protected:
        template<class EBus, class R, class BusType, class... Args>
        void InitParameters();

        BehaviorMethodFunctor m_functor;

        AZStd::fixed_vector<BehaviorParameter, MaxBehaviorParameters> m_parameters;
        //! Stores the per parameter metadata which is used to add names, tooltips, trait, default values, etc... to the parameters
        AZStd::fixed_vector<BehaviorParameterMetadata, MaxBehaviorParameters> m_metadataParameters;

    private:
        int m_startNamedArgumentIndex = s_startArgumentIndex; // +1 for result type
        BehaviorEventType m_eventType{};
        bool m_hasNonVoidReturn{};
    };
}

namespace AZ
{
    namespace Internal
    {
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
        template<class R, class C, class... Args>
        struct BehaviorOnDemandReflectHelper<R(C, Args...)>
        {
            static void QueueReflect(OnDemandReflectionOwner* onDemandReflection)
            {
                if (onDemandReflection)
                {
                    static constexpr size_t c_functionParameterAndReturnSize = sizeof...(Args) + 2;
                    // deal with OnDemand reflection
                    AZ::Uuid argumentTypes[c_functionParameterAndReturnSize] = { AzTypeInfo<R>::Uuid(), AZ::Uuid::CreateNull(), (AzTypeInfo<Args>::Uuid())... };
                    StaticReflectionFunctionPtr reflectHooks[c_functionParameterAndReturnSize] = {
                        OnDemandReflectHook<AZStd::remove_pointer_t<AZStd::decay_t<R>>>::Get(),
                        OnDemandReflectHook<AZStd::remove_pointer_t<AZStd::decay_t<C>>>::Get(),
                        (OnDemandReflectHook<AZStd::remove_pointer_t<AZStd::decay_t<Args>>>::Get())...
                    };
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
        template<class R, class C, class... Args>
        struct BehaviorOnDemandReflectHelper<R(C, Args...) noexcept>
            : BehaviorOnDemandReflectHelper<R(C, Args...)>
        {};
#endif

        template<class... Functions>
        void OnDemandReflectFunctions(OnDemandReflectionOwner* onDemandReflection, AZStd::Internal::pack_traits_arg_sequence<Functions...>);


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
            AZ_CLASS_ALLOCATOR(BehaviorValuesSpecialization, AZ::SystemAllocator);

            template<class LastValue>
            inline void SetValues(LastValue&& value)
            {
                m_values[sizeof...(Values) - 1] = aznew BehaviorDefaultValue(AZStd::forward<LastValue>(value));
            }

            template<class CurrentValue, class... RestValues>
            inline void SetValues(CurrentValue&& value, RestValues&&... values)
            {
                m_values[sizeof...(Values) - sizeof...(RestValues) - 1] = aznew BehaviorDefaultValue(AZStd::forward<CurrentValue>(value));
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

    struct UnwrapperFuncDeleter
    {
        using Deleter = void(*)(void*);

        void operator()(void* ptr) const;

        Deleter m_deleter{};
    };
    // Wraps storage for the unwrapper functor that cannot be
    // stored within the <= sizeof(void*) amount of storage
    using UnwrapperPtr = AZStd::unique_ptr<void, UnwrapperFuncDeleter>;
    struct UnwrapperUserData
    {
        UnwrapperUserData();
        UnwrapperUserData(UnwrapperUserData&&);
        UnwrapperUserData& operator=(UnwrapperUserData&&);
        ~UnwrapperUserData();

        UnwrapperPtr m_unwrapperPtr;
    };
    using BehaviorClassUnwrapperFunction = void(*)(void* /*classPtr*/, BehaviorObject& /*unwrappedBehaviorObject*/,
        const UnwrapperUserData& /*userData*/);

    /**
     * Behavior representation of reflected class.
     */
    class BehaviorClass
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorClass, SystemAllocator);

        BehaviorClass();
        ~BehaviorClass();

        /// Hooks to override default memory allocation for the class (AZ_CLASS_ALLOCATOR is used by default)
        using AllocateType = void* (*)(void* userData);
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

        struct ScopedBehaviorObject
        {
            using CleanupFunction = AZStd::function<void(const AZ::BehaviorObject&)>;
            ScopedBehaviorObject();
            // Accepting an rvalue BehaviorObject to imply "ownership" over it
            // Does not actually use any move semantics
            ScopedBehaviorObject(AZ::BehaviorObject&& behaviorObject, CleanupFunction cleanupFunction);

            // Prevents copy constructor and copy assignment from being instantiated
            // which would cause the cleanupFunction to run twice.
            ScopedBehaviorObject(ScopedBehaviorObject&&) = default;
            ScopedBehaviorObject& operator=(ScopedBehaviorObject&&);
            ~ScopedBehaviorObject();

            bool IsValid() const;

            BehaviorObject m_behaviorObject;
        private:
            CleanupFunction m_cleanupFunction;
        };

        // Create the object with default constructor if possible otherwise returns an invalid object
        BehaviorObject Create() const;

        // Create the object with default constructor in the provided memory if possible otherwise returns an invalid object
        BehaviorObject Create(void* address) const;

        // Create a scoped behavior object using the  default constructor
        // The scoped object will automatically destruct AND deallocate the created object
        ScopedBehaviorObject CreateWithScope() const;
        // Create a scoped behavior object using default constructor in the provided memory if possible.
        // The scoped object will automatically destruct the created object
        // This function does not deallocate since the memory was provided by the user
        ScopedBehaviorObject CreateWithScope(void* address) const;

        // Create the object using the first registered constructor which can accept all the arguments in the provided memory address
        // The scoped object will automatically destruct AND deallocate the created object
        ScopedBehaviorObject CreateWithScope(AZStd::span<BehaviorArgument> arguments) const;
        // Create the object using the first registered constructor which can accept all the arguments in the provided memory address
        ScopedBehaviorObject CreateWithScope(void* address, AZStd::span<BehaviorArgument> arguments) const;

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
        AZStd::vector<BehaviorMethod*> GetOverloads(const AZStd::string& string) const;
        AZStd::vector<BehaviorMethod*> GetOverloadsIncludeMethod(BehaviorMethod* method) const;
        AZStd::vector<BehaviorMethod*> GetOverloadsExcludeMethod(BehaviorMethod* method) const;
        void PostProcessMethod(BehaviorContext* context, BehaviorMethod& method);

        const BehaviorProperty* FindPropertyByReflectedName(AZStd::string_view reflectedName) const;
        const BehaviorMethod* FindGetterByReflectedName(AZStd::string_view reflectedName) const;
        const BehaviorMethod* FindSetterByReflectedName(AZStd::string_view reflectedName) const;

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
        UnwrapperUserData m_unwrapperUserData;
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
        AZ_CLASS_ALLOCATOR(BehaviorProperty, AZ::SystemAllocator);

        BehaviorProperty(BehaviorContext* context);
        ~BehaviorProperty() override;

        template<class Getter, class Setter>
        bool Set(Getter getter, Setter setter, BehaviorClass* currentClass, BehaviorContext* context);

        AZ::TypeId GetTypeId() const;

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
        AZ_CLASS_ALLOCATOR(BehaviorEBus, SystemAllocator);

        typedef void(*QueueFunctionType)(void* /*userData1*/, void* /*userData2*/);

        struct VirtualProperty
        {
            VirtualProperty(BehaviorEBusEventSender* getter, BehaviorEBusEventSender* setter);

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

    template<class Function>
    using BehaviorParameterOverridesArray = AZStd::array<BehaviorParameterOverrides, AZStd::function_traits<Function>::num_args>;

    class BehaviorEBusHandler
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(BehaviorEBusHandler);
        AZ_RTTI_NO_TYPE_INFO_DECL();

        // Since we can share hooks we should probably pass the event name
        using GenericHookType = void(*)(void* /*userData*/, const char* /*eventName*/, int /*eventIndex*/, BehaviorArgument* /*result*/, int /*numParameters*/, BehaviorArgument* /*parameters*/);

        struct BusForwarderEvent
        {
            BusForwarderEvent() = default;

            const char* m_name{};
            AZ::Crc32   m_eventId;
            void* m_function{}; ///< Pointer user handler R Function(userData, Args...)
            bool m_isFunctionGeneric{};
            void* m_userData{};

            /// \note, even if this function returns no result, the first parameter is STILL space for the result
            bool HasResult() const;

            AZStd::vector<BehaviorParameter> m_parameters; // result, userdata, arguments...
            AZStd::vector<BehaviorParameterMetadata> m_metadataParameters; //< Custom Metadata for the parameter. Contains Names and Tooltips for each parameter.
            // Not consolidated with the above vector, because existing internal functions expect BehaviorParameters to be laid out contiguously
        };

        using EventArray = AZStd::vector<BusForwarderEvent>;

        BehaviorEBusHandler() = default;

        virtual ~BehaviorEBusHandler() = default;

        virtual int GetFunctionIndex(const char* name) const = 0;

        template<class BusId>
        bool Connect(BusId id);

        virtual bool Connect(BehaviorArgument* id = nullptr) = 0;

        virtual void Disconnect(BehaviorArgument* id = nullptr) = 0;

        template<class BusId>
        void Disconnect(BusId id);

        virtual bool IsConnected() = 0;
        virtual bool IsConnectedId(BehaviorArgument* id) = 0;

        template<class Hook>
        bool InstallHook(int index, Hook h, void* userData = nullptr);

        template<class Hook>
        bool InstallHook(const char* name, Hook h, void* userData = nullptr);

        bool InstallGenericHook(int index, GenericHookType hook, void* userData = nullptr);

        bool InstallGenericHook(const char* name, GenericHookType hook, void* userData = nullptr);

        const EventArray& GetEvents() const;

#if !defined(AZ_RELEASE_BUILD) // m_scriptPath is only available in non-Release mode
        AZStd::string m_scriptPath;
#endif

        AZStd::string GetScriptPath() const;

        void SetScriptPath(const char* scriptPath);

    protected:
        template<class Event>
        void SetEvent(Event e, const char* name);

        template<class Event>
        void SetEvent(Event e, AZStd::string_view name, const BehaviorParameterOverridesArray<Event>& args);

        template<class... Args>
        void Call(int index, Args&&... args) const;

        template<class R, class... Args>
        void CallResult(R& result, int index, Args&&... args) const;

        EventArray m_events;
    };
} // namespace AZ


namespace AZ
{
    // Behavior context events you can listen for
    class BehaviorContextEvents : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusInterface settings
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef BehaviorContext* BusIdType;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        /// Called when a new global method is reflected in behavior context or removed from it
        virtual void OnAddGlobalMethod(const char* methodName, BehaviorMethod* method);
        virtual void OnRemoveGlobalMethod(const char* methodName, BehaviorMethod* method);

        /// Called when a new global property is reflected in behavior context or remove from it
        virtual void OnAddGlobalProperty(const char* propertyName, BehaviorProperty* prop);
        virtual void OnRemoveGlobalProperty(const char* propertyName, BehaviorProperty* prop);

        /// Called when a class is added or removed
        virtual void OnAddClass(const char* className, BehaviorClass* behaviorClass);
        virtual void OnRemoveClass(const char* className, BehaviorClass* behaviorClass);

        /// Called when a ebus is added or removed
        virtual void OnAddEBus(const char* ebusName, BehaviorEBus* ebus);
        virtual void OnRemoveEBus(const char* ebusName, BehaviorEBus* ebus);
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
            ebus->m_getCurrentId = aznew AZ::Internal::BehaviorMethodImpl(static_cast<const typename Bus::BusIdType*(*)()>(&Bus::GetCurrentBusId), this, AZStd::string(Bus::GetName()) + "::GetCurrentBusId");
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
            return aznew AZ::Internal::BehaviorMethodImpl(static_cast<void(*)(BehaviorEBus::QueueFunctionType, void*, void*)>(&QueueFunction<Bus>), this, AZStd::string(Bus::GetName()) + "::QueueFunction");
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
                return azmalloc(sizeof(T), AZStd::alignment_of<T>::value, AZ::SystemAllocator);
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
        static void SetClassEqualityComparer(BehaviorClass* behaviorClass, const T*)
        {
            behaviorClass->m_equalityComparer = &DefaultEqualityComparer<T>;
        }

        template<class T>
        static void SetClassDefaultMoveConstructor(BehaviorClass* behaviorClass, const AZStd::true_type& /*AZStd::is_move_constructible<T>*/)
        {
            behaviorClass->m_mover = &DefaultMoveConstruct<T>;
        }

    public:
        AZ_CLASS_ALLOCATOR(BehaviorContext, SystemAllocator);
        AZ_TYPE_INFO_WITH_NAME_DECL(BehaviorContext);
        AZ_RTTI_NO_TYPE_INFO_DECL();

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

        template <class T>
        struct ClassBuilder;

        /// Internal structure to maintain EBus information while describing it.
        template<typename Bus>
        struct EBusBuilder;

         BehaviorContext();
        ~BehaviorContext();

        ///< \deprecated Use "Method(const char*, Function, const BehaviorParameterOverridesArray<Function>&, const char*)" instead
        ///< This method does not support passing in argument names and tooltips nor does it support overriding specific parameter Behavior traits
        template<class Function>
        GlobalMethodBuilder Method(const char* name, Function f, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        ///< \deprecated Use "Method(const char*, Function, const char*, const BehaviorParameterOverridesArray<Function>&, const char*)" instead
        ///< This method does not support passing in argument names and tooltips nor does it support overriding specific parameter Behavior traits
        template<class Function>
        GlobalMethodBuilder Method(const char* name, Function f, const char* deprecatedName, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        template<class Function>
        GlobalMethodBuilder Method(const char* name, Function f, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc = nullptr);

        template<class Function>
        GlobalMethodBuilder Method(const char* name, Function f, const char* deprecatedName, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc = nullptr);

        template<class Getter, class Setter>
        GlobalPropertyBuilder Property(const char* name, Getter getter, Setter setter);

        /// All enums are treated as the enum type
        template<auto Value>
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
        * in a temp storage, so currently there is limit the \ref BehaviorArgument temp storage, we
        * can easily change that if it became a problem.
        */
        template<class Value>
        BehaviorDefaultValuePtr MakeDefaultValue(Value&& defaultValue);

        /**
        * Create a container of default values to be used with methods. Default values are stored by value
        * in a temp storage, so currently there is limit the \ref BehaviorArgument temp storage, we
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

        const BehaviorMethod* FindMethodByReflectedName(AZStd::string_view reflectedName) const;
        const BehaviorProperty* FindPropertyByReflectedName(AZStd::string_view reflectedName) const;
        const BehaviorMethod* FindGetterByReflectedName(AZStd::string_view reflectedName) const;
        const BehaviorMethod* FindSetterByReflectedName(AZStd::string_view reflectedName) const;
        const BehaviorClass* FindClassByReflectedName(AZStd::string_view reflectedName) const;
        const BehaviorClass* FindClassByTypeId(const AZ::TypeId& typeId) const;
        const BehaviorEBus* FindEBusByReflectedName(AZStd::string_view reflectedName) const;

        //! Prefer to access BehaviorContext methods, properties, classes and EBuses through the Find* functions
        //! instead of direct member access
        AZStd::unordered_map<AZStd::string, BehaviorMethod*> m_methods;
        AZStd::unordered_map<AZStd::string, BehaviorProperty*> m_properties;
        AZStd::unordered_map<AZStd::string, BehaviorClass*> m_classes;
        AZStd::unordered_map<AZ::Uuid, BehaviorClass*> m_typeToClassMap;
        AZStd::unordered_map<AZStd::string, BehaviorEBus*> m_ebuses;

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
    void Disconnect(AZ::BehaviorArgument* id = nullptr) override {\
        AZ::Internal::EBusConnector<_Handler>::Disconnect(this, id);\
    }\
    _Handler(){\
        m_events.resize(FN_MAX);\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_REG_EVENT, AZ_EBUS_SEQ(__VA_ARGS__))\
    }\
    bool Connect(AZ::BehaviorArgument* id = nullptr) override {\
        return AZ::Internal::EBusConnector<_Handler>::Connect(this, id);\
    }\
    bool IsConnected() override {\
        return AZ::Internal::EBusConnector<_Handler>::IsConnected(this);\
    }\
    bool IsConnectedId(AZ::BehaviorArgument* id) override {\
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
    void Disconnect(AZ::BehaviorArgument* id = nullptr) override {\
        AZ::Internal::EBusConnector<_Handler>::Disconnect(this, id);\
    }\
    _Handler(){\
        m_events.resize(FN_MAX);\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_REG_EVENT, AZ_EBUS_SEQ(__VA_ARGS__))\
    }\
    bool Connect(AZ::BehaviorArgument* id = nullptr) override {\
        return AZ::Internal::EBusConnector<_Handler>::Connect(this, id);\
    }\
    bool IsConnected() override {\
        return AZ::Internal::EBusConnector<_Handler>::IsConnected(this);\
    }\
    bool IsConnectedId(AZ::BehaviorArgument* id) override {\
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
    void Disconnect(AZ::BehaviorArgument* id = nullptr) override {\
        AZ::Internal::EBusConnector<_Handler>::Disconnect(this, id);\
    }\
    _Handler(){\
        m_events.resize(FN_MAX);\
        AZ_BEHAVIOR_EBUS_MACRO_CALLER(AZ_BEHAVIOR_EBUS_REG_EVENT, __VA_ARGS__)\
    }\
    bool Connect(AZ::BehaviorArgument* id = nullptr) override {\
        return AZ::Internal::EBusConnector<_Handler>::Connect(this, id);\
    }\
    bool IsConnected() override {\
        return AZ::Internal::EBusConnector<_Handler>::IsConnected(this);\
    }\
    bool IsConnectedId(AZ::BehaviorArgument* id) override {\
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
    template<class T>
    inline BehaviorArgument::BehaviorArgument(T* value)
    {
        Set<T>(value);
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T, typename>
    inline void BehaviorArgument::StoreInTempData(T&& value)
    {
        StoreInTempDataEvenIfNonTrivial(AZStd::forward<T>(value));
    }

    template<typename T>
    inline void BehaviorArgument::StoreInTempDataEvenIfNonTrivial(T&& value)
    {
        AZ::Internal::SetParameters<T>(this);
        m_value = m_tempData.allocate(sizeof(T), AZStd::alignment_of<T>::value, 0);
        AZStd::construct_at(reinterpret_cast<AZStd::decay_t<T>*>(m_value), AZStd::forward<T>(value));
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    AZ_FORCE_INLINE void BehaviorArgument::Set(T* value)
    {
        AZ::Internal::SetParameters<AZStd::decay_t<T>>(this);
        m_value = (void*)value;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    AZ_FORCE_INLINE bool BehaviorArgument::ConvertTo()
    {
        return ConvertTo(AzTypeInfo<AZStd::decay_t<T>>::Uuid());
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    AZ_FORCE_INLINE AZStd::decay_t<T>* BehaviorArgument::GetAsUnsafe() const
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

        template<class T>
        static bool Set(BehaviorArgument& param, T&& result, bool IsValueCopy)
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

    template<typename T, typename U>
    constexpr bool SetResult::IsCopyAssignable<T, U, AZStd::void_t<decltype(AZStd::declval<T>() = AZStd::declval<U>())>> = true;

    template<typename T>
    AZ_FORCE_INLINE BehaviorArgument& BehaviorArgument::operator=(T&& result)
    {
        StoreResult(AZStd::forward<T>(result));
        return *this;
    }

    template<typename T>
    AZ_FORCE_INLINE bool BehaviorArgument::StoreResult(T&& result)
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
        BehaviorArgument arguments[] = { &args... };
        return Call(arguments, sizeof...(Args), nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class R, class... Args>
    bool BehaviorMethod::InvokeResult(R& r, Args&&... args) const
    {
        if (!HasResult())
        {
            return false;
        }
        BehaviorArgument arguments[sizeof...(args)] = { &args... };
        BehaviorArgument result(&r);
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

        BehaviorArgument result(&r);
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
        AZStd::string getterPropertyName = currentClass ? currentClass->m_name : AZStd::string{};
        if (!getterPropertyName.empty())
        {
            getterPropertyName += "::";
        }
        getterPropertyName += m_name;
        getterPropertyName += k_PropertyNameGetterSuffix;

        using GetterFunctionTraits = AZStd::function_traits<Getter>;
        constexpr bool isClassType = !AZStd::is_same_v<typename GetterFunctionTraits::class_type, AZStd::Internal::error_type>;
        using GetterCastType = AZStd::conditional_t<
            isClassType,
            typename GetterFunctionTraits::type,
            typename GetterFunctionTraits::function_object_signature*>;
        m_getter = aznew AZ::Internal::BehaviorMethodImpl(static_cast<GetterCastType>(getter), context, getterPropertyName);

        if constexpr (isClassType)
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
        AZStd::string setterPropertyName = currentClass ? currentClass->m_name : AZStd::string{};
        if (!setterPropertyName.empty())
        {
            setterPropertyName += "::";
        }
        setterPropertyName += m_name;
        setterPropertyName += k_PropertyNameSetterSuffix;

        using SetterFunctionTraits = AZStd::function_traits<Setter>;
        constexpr bool isClassType = !AZStd::is_same_v<typename SetterFunctionTraits::class_type, AZStd::Internal::error_type>;
        using SetterCastType = AZStd::conditional_t<
            isClassType,
            typename SetterFunctionTraits::type,
            typename SetterFunctionTraits::function_object_signature*>;
        m_setter = aznew AZ::Internal::BehaviorMethodImpl(static_cast<SetterCastType>(setter), context, setterPropertyName);

        if constexpr (isClassType)
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
        using EventTraits = AZStd::function_traits<Event>;
        using EventCastType = AZStd::conditional_t<
            !AZStd::is_same_v<typename EventTraits::class_type, AZStd::Internal::error_type>,
            typename EventTraits::type,
            typename EventTraits::function_object_signature*>;
        m_broadcast = aznew AZ::Internal::BehaviorEBusEvent(static_cast<EventCastType>(e), context,
            AZ::Internal::BehaviorEventType::BE_BROADCAST, AZStd::type_identity<EBus>{});
        m_broadcast->m_name = eventName;

        SetEvent<EBus>(e, eventName, context, typename AZStd::is_same<typename EBus::BusIdType, NullBusId>::type());
        SetQueueBroadcast<EBus>(e, eventName, context, typename AZStd::is_same<typename EBus::QueuePolicy::BusMessageCall, typename AZ::Internal::NullBusMessageCall>::type());
        SetQueueEvent<EBus>(e, eventName, context, typename AZStd::conditional<
            AZStd::is_same_v<typename EBus::BusIdType, NullBusId> || AZStd::is_same_v<typename EBus::QueuePolicy::BusMessageCall, typename AZ::Internal::NullBusMessageCall>,
            AZStd::true_type, AZStd::false_type>::type());
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetEvent(Event, const char*, BehaviorContext*, const AZStd::true_type& /*is NullBusId*/)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/)
    {
        using EventTraits = AZStd::function_traits<Event>;
        using EventCastType = AZStd::conditional_t<
            !AZStd::is_same_v<typename EventTraits::class_type, AZStd::Internal::error_type>,
            typename EventTraits::type,
            typename EventTraits::function_object_signature*>;
        m_event = aznew AZ::Internal::BehaviorEBusEvent(static_cast<EventCastType>(e), context,
            AZ::Internal::BehaviorEventType::BE_EVENT_ID, AZStd::type_identity<EBus>{});
        m_event->m_name = eventName;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueBroadcast(Event, const char*, BehaviorContext*, const AZStd::true_type& /*is NullBusId*/)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueBroadcast(Event e, const char* eventName, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/)
    {
        using EventTraits = AZStd::function_traits<Event>;
        using EventCastType = AZStd::conditional_t<
            !AZStd::is_same_v<typename EventTraits::class_type, AZStd::Internal::error_type>,
            typename EventTraits::type,
            typename EventTraits::function_object_signature*>;
        m_queueBroadcast = aznew AZ::Internal::BehaviorEBusEvent(static_cast<EventCastType>(e), context,
            AZ::Internal::BehaviorEventType::BE_QUEUE_BROADCAST, AZStd::type_identity<EBus>{});

        m_queueBroadcast->m_name = eventName;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueEvent(Event, const char*, BehaviorContext*, const AZStd::true_type& /* is Queue and is BusId valid*/)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueEvent(Event e, const char* eventName, BehaviorContext* context, const AZStd::false_type& /* is Queue and is BusId valid*/)
    {
        using EventTraits = AZStd::function_traits<Event>;
        using EventCastType = AZStd::conditional_t<
            !AZStd::is_same_v<typename EventTraits::class_type, AZStd::Internal::error_type>,
            typename EventTraits::type,
            typename EventTraits::function_object_signature*>;
        m_queueEvent = aznew AZ::Internal::BehaviorEBusEvent(static_cast<EventCastType>(e), context,
            AZ::Internal::BehaviorEventType::BE_QUEUE_EVENT_ID, AZStd::type_identity<EBus>{});
        m_queueEvent->m_name = eventName;
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

    // BehaviorContext overloads which returns GlobalMethodBuilder instances
    template<class Function>
    BehaviorContext::GlobalMethodBuilder BehaviorContext::Method(const char* name, Function f, BehaviorValues* defaultValues, const char* dbgDesc)
    {
        return Method(name, f, nullptr, defaultValues, dbgDesc);
    }


    //////////////////////////////////////////////////////////////////////////
    ///< \deprecated Use "Method(const char*, Function, const BehaviorParameterOverridesArray<Function>&, const char*)" instead
    template<class Function>
    BehaviorContext::GlobalMethodBuilder BehaviorContext::Method(const char* name, Function f, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc)
    {
        return Method(name, f, nullptr, args, dbgDesc);
    }

    //////////////////////////////////////////////////////////////////////////
    ///< \deprecated Use "Method(const char*, Function, const char*, const BehaviorParameterOverridesArray<Function>&, const char*)" instead
    template<class Function>
    BehaviorContext::GlobalMethodBuilder BehaviorContext::Method(const char* name, Function f, const char* deprecatedName, BehaviorValues* defaultValues, const char* dbgDesc)
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
    BehaviorContext::GlobalMethodBuilder BehaviorContext::Method(const char* name, Function f, const char* deprecatedName, const BehaviorParameterOverridesArray<Function>& args, const char* dbgDesc)
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

        static_assert(!AZStd::is_member_function_pointer_v<Function>, "This is a member method declared as global! use script.Class<Type>(Name)->Method()->Value()!\n");
        using FunctionTraits = AZStd::function_traits<Function>;
        using FunctionCastType = AZStd::conditional_t<
            !AZStd::is_same_v<typename FunctionTraits::class_type, AZStd::Internal::error_type>,
            typename FunctionTraits::type,
            typename FunctionTraits::function_object_signature*>;
        BehaviorMethod* method = aznew AZ::Internal::BehaviorMethodImpl(static_cast<FunctionCastType>(f), this, name);
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
                for (const auto& i : m_methods)
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
    template<auto Value>
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
            (BehaviorOnDemandReflectHelper<typename AZStd::function_traits<Functions>::function_type>::QueueReflect(onDemandReflection), ...);
        }

        // First removes the const and reference qualifiers from the argument type, next remove any pointer decoration from that type
        // and finally if that type is an enum type, convert it to the underlying integral type
        template<class T>
        using UnqualifiedRemovePointerUnderlyingType = AZStd::RemoveEnumT<AZStd::remove_pointer_t<AZStd::decay_t<T>>>;

        // Assumes parameters array is big enough to store all parameters
        template<class... Args>
        inline void SetParametersStripped(BehaviorParameter* parameters, OnDemandReflectionOwner* onDemandReflection)
        {
            // +1 to avoid zero array size
            Uuid argumentTypes[sizeof...(Args) + 1] = { AzTypeInfo<UnqualifiedRemovePointerUnderlyingType<Args>>::Uuid()... };
            const char* argumentNames[sizeof...(Args) + 1] = { AzTypeInfo<Args>::Name()... };
            bool argumentIsPointer[sizeof...(Args) + 1] = { AZStd::is_pointer_v<AZStd::remove_reference_t<Args>>... };
            bool argumentIsConst[sizeof...(Args) + 1] = { AZStd::is_const_v<AZStd::remove_pointer_t<AZStd::remove_reference_t<Args>>>... };
            bool argumentIsReference[sizeof...(Args) + 1] = { AZStd::is_reference_v<Args>... };
            IRttiHelper* rttiHelper[sizeof...(Args) + 1] = { GetRttiHelper<UnqualifiedRemovePointerUnderlyingType<Args>>()... };
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
                StaticReflectionFunctionPtr reflectHooks[sizeof...(Args) + 1] = { OnDemandReflectHook<UnqualifiedRemovePointerUnderlyingType<Args>>::Get()... };
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
            SetParametersStripped<Args...>(parameters, onDemandReflection);
        }
    }
}


namespace AZ
{
    namespace Internal
    {
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
            static bool Connect(BusHandler* handler, BehaviorArgument* id)
            {
                if (id && id->ConvertTo<typename BusHandler::BusType::BusIdType>())
                {
                    handler->BusConnect(*id->GetAsUnsafe<typename BusHandler::BusType::BusIdType>());
                    return true;
                }
                return false;
            }

            static bool Disconnect(BusHandler* handler, BehaviorArgument* id)
            {
                if (id)
                {
                    if (id->ConvertTo<typename BusHandler::BusType::BusIdType>())
                    {
                        handler->BusDisconnect(*id->GetAsUnsafe<typename BusHandler::BusType::BusIdType>());
                        return true;
                    }
                }
                else // null id passed, attempt id-less disconnect
                {
                    handler->BusDisconnect();
                    return true;
                }
                return false;
            }

            static bool IsConnected(BusHandler* handler)
            {
                return handler->BusIsConnected();
            }

            static bool IsConnectedId(BusHandler* handler, BehaviorArgument* id)
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
            static bool Connect(BusHandler* handler, BehaviorArgument* id)
            {
                (void)id;
                handler->BusConnect();
                return true;
            }

            static bool Disconnect(BusHandler* handler, BehaviorArgument* id)
            {
                (void)id;
                handler->BusDisconnect();
                return true;
            }

            static bool IsConnected(BusHandler* handler)
            {
                return handler->BusIsConnected();
            }

            static bool IsConnectedId(BusHandler* handler, BehaviorArgument* id)
            {
                (void)id;
                AZ_Warning("BehaviorContext", false, "Function IsConnectedId is called on an EBus handler that was initially connected without Id. Please use IsConnected instead.");
                return handler->BusIsConnected();
            }
        };

        //////////////////////////////////////////////////////////////////////////
        template<class Owner>
        template<class T>
        void GenericAttributes<Owner>::SetAttributeContextData(T value, AZ::Attribute* attribute, const AZStd::true_type& /* is_function<remove_pointer<T>::type> && is_member_function_pointer<T>*/)
        {
            BehaviorMethod* method = aznew AZ::Internal::BehaviorMethodImpl(value, m_context, AZStd::string("Function-Attribute"));
            auto DestroyAttributeMethod = [](void* contextData)
            {
                // Passed to Attribute::SetContext data, to destroy the behavior method
                delete reinterpret_cast<BehaviorMethod*>(contextData);
            };
            attribute->SetContextData(method, DestroyAttributeMethod);
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

        bool IsInScope(const AttributeArray& attributes, const AZ::Script::Attributes::ScopeFlags scope);

    } // namespace Internal
} // namespace AZ

// Include inline implementation of BehaviorMethodImpl constructors
#include "BehaviorMethodImpl.inl"

// Implement inline functions for the BehaviorEvent constructors
#include "BehaviorEBusEvent.inl"
#include "BehaviorEBusHandler.inl"

// Include inline implementation of the ClassBuilder struct for building BehaviorClass instances
#include "BehaviorClassBuilder.inl"

// Include inline implementation of the ClassBuilder struct for building BehaviorEBus instances
#include "BehaviorEBusBuilder.inl"

// pull AzStd on demand reflection
#include <AzCore/RTTI/AzStdOnDemandPrettyName.inl>
#include <AzCore/RTTI/AzStdOnDemandReflection.inl>

DECLARE_EBUS_EXTERN_DLL_MULTI_ADDRESS(BehaviorContextEvents);
