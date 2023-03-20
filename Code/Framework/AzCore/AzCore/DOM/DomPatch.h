/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPath.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/std/containers/deque.h>

namespace AZ::Dom
{
    using PatchOutcome = AZ::Outcome<void, AZStd::string>;
    void CombinePatchOutcomes(PatchOutcome& lhs, PatchOutcome&& rhs);

    //! A patch operation that represents an atomic operation for mutating or validating a Value.
    //! PatchOperations can be created with helper methods in Patch. /see Patch
    class PatchOperation final
    {
    public:
        //! The operation to perform.
        enum class Type
        {
            Add, //!< Inserts or replaces the value at DestinationPath with Value
            Remove, //!< Removes the entry at DestinationPath
            Replace, //!< Replaces the value at DestinationPath with Value
            Copy, //!< Copies the contents of SourcePath to DestinationPath
            Move, //!< Moves the contents of SourcePath to DestinationPath
            Test //!< Ensures the contents of DestinationPath match Value or fails, performs no mutations
        };

        PatchOperation() = default;
        PatchOperation(const PatchOperation&) = default;
        PatchOperation(PatchOperation&&) = default;

        PatchOperation(Path destionationPath, Type type, Value value);
        PatchOperation(Path destionationPath, Type type, Path sourcePath);
        PatchOperation(Path path, Type type);

        static PatchOperation AddOperation(Path destinationPath, Value value);
        static PatchOperation RemoveOperation(Path pathToRemove);
        static PatchOperation ReplaceOperation(Path destinationPath, Value value);
        static PatchOperation CopyOperation(Path destinationPath, Path sourcePath);
        static PatchOperation MoveOperation(Path destinationPath, Path sourcePath);
        static PatchOperation TestOperation(Path testPath, Value value);

        PatchOperation& operator=(const PatchOperation&) = default;
        PatchOperation& operator=(PatchOperation&&) = default;

        bool operator==(const PatchOperation& rhs) const;
        bool operator!=(const PatchOperation& rhs) const;

        Type GetType() const;
        void SetType(Type type);

        const Path& GetDestinationPath() const;
        void SetDestinationPath(Path path);

        const Value& GetValue() const;
        void SetValue(Value value);

        const Path& GetSourcePath() const;
        void SetSourcePath(Path path);

        AZ::Outcome<Value, AZStd::string> Apply(Value rootElement) const;
        PatchOutcome ApplyInPlace(Value& rootElement) const;
        AZ::Outcome<Value, AZStd::string> ApplyAndDenormalize(Value rootElement);
        PatchOutcome ApplyInPlaceAndDenormalize(Value& rootElement);

        //! Returns true if this contains an "EndOfArray" entry in any relevant paths, meaning resolving
        //! the path requires a lookup inside a target DOM.
        bool ContainsNormalizedEntries() const;

        Value GetDomRepresentation() const;
        static AZ::Outcome<PatchOperation, AZStd::string> CreateFromDomRepresentation(Value domValue);

        using InversePatches = AZStd::fixed_vector<PatchOperation, 2>;
        AZ::Outcome<AZStd::fixed_vector<PatchOperation, 2>, AZStd::string> GetInverse(Value stateBeforeApplication) const;

        enum class ExistenceCheckFlags : AZ::u8
        {
            DefaultExistenceCheck = 0x0,
            VerifyFullPath = 0x1,
            AllowEndOfArray = 0x2,
        };

    private:
        struct PathContext
        {
            Value& m_value;
            PathEntry m_key;
        };

        // For a given path and target value, removes any EndOfArray entries 
        // and replaces them with the resolved path
        static bool DenormalizePath(Dom::Path& path, const Dom::Value& sourceValue);
        static AZ::Outcome<PathContext, AZStd::string> LookupPath(
            Value& rootElement, const Path& path, ExistenceCheckFlags existenceCheckFlags = ExistenceCheckFlags::DefaultExistenceCheck);

        PatchOutcome ApplyAdd(Value& rootElement) const;
        PatchOutcome ApplyRemove(Value& rootElement) const;
        PatchOutcome ApplyReplace(Value& rootElement) const;
        PatchOutcome ApplyCopy(Value& rootElement) const;
        PatchOutcome ApplyMove(Value& rootElement) const;
        PatchOutcome ApplyTest(Value& rootElement) const;

        AZStd::variant<AZStd::monostate, Value, Path> m_value;
        Path m_domPath;
        Type m_type;
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(PatchOperation::ExistenceCheckFlags);

    class Patch;

    //! The current state of a Patch application operation.
    struct PatchApplicationState
    {
        //! The outcome of the last operation, may be overridden to produce a different failure outcome.
        PatchOutcome m_outcome = AZ::Success();
        //! The patch being applied.
        const Patch* m_patch = nullptr;
        //! The last operation attempted.
        const PatchOperation* m_lastOperation = nullptr;
        //! The current state of the value being patched, will be returned if the patch operation succeeds.
        Value* m_currentState = nullptr;
        //! If set to false, the patch operation should halt.
        bool m_shouldContinue = true;
    };

    namespace PatchApplicationStrategy
    {
        //! The default patching strategy. Applies all operations in a patch, but halts if any one operation fails.
        void HaltOnFailure(PatchApplicationState& state);
        //! Patching strategy that attemps to apply all operations in a patch, but ignores operation failures and continues.
        void IgnoreFailureAndContinue(PatchApplicationState& state);
    } // namespace PatchApplicationStrategy

    //! A set of operations that can be applied to a Value to produce a new Value.
    //! \see PatchOperation
    class Patch final
    {
    public:
        using StrategyFunctor = AZStd::function<void(PatchApplicationState&)>;
        using OperationsContainer = AZStd::deque<PatchOperation>;

        Patch() = default;
        Patch(const Patch&) = default;
        Patch(Patch&&) = default;
        Patch(AZStd::initializer_list<PatchOperation> init);

        template<class InputIterator>
        Patch(InputIterator first, InputIterator last)
            : m_operations(first, last)
        {
        }

        Patch& operator=(const Patch&) = default;
        Patch& operator=(Patch&&) = default;

        bool operator==(const Patch& rhs) const;
        bool operator!=(const Patch& rhs) const;

        const OperationsContainer& GetOperations() const;
        void PushBack(PatchOperation op);
        void PushFront(PatchOperation op);
        void Pop();
        void Clear();
        const PatchOperation& At(size_t index) const;
        size_t Size() const;

        PatchOperation& operator[](size_t index);
        const PatchOperation& operator[](size_t index) const;

        OperationsContainer::iterator begin();
        OperationsContainer::iterator end();
        OperationsContainer::const_iterator begin() const;
        OperationsContainer::const_iterator end() const;
        OperationsContainer::const_iterator cbegin() const;
        OperationsContainer::const_iterator cend() const;
        size_t size() const;

        //! Applies this patch to the given DOM element.
        //! \param rootElement The DOM element to patch.
        //! \param strategy A callback to be run after every patch application, see PatchApplicationState.
        //! \return an outcome with either the patched element or an error string
        AZ::Outcome<Value, AZStd::string> Apply(Value rootElement, StrategyFunctor strategy = PatchApplicationStrategy::HaltOnFailure) const;
        //! Applies this patch to the given DOM element in place, changing it to the new value.
        //! \param rootElement The DOM element to patch.
        //! \param strategy A callback to be run after every patch application, see PatchApplicationState.
        //! \return an outcome with either the patched element or an error string
        PatchOutcome ApplyInPlace(Value& rootElement, StrategyFunctor strategy = PatchApplicationStrategy::HaltOnFailure) const;

        //! Applies this patch to the given DOM element.
        //! After applying the patch, any "EndOfArray" patch entries are denormalized into their resolved paths.
        //! This operation mutates the underyling patch operations; make a copy if you wish to keep the original patch.
        //! \param rootElement The DOM element to patch.
        //! \param strategy A callback to be run after every patch application, see PatchApplicationState.
        //! \return an outcome with either the patched element or an error string
        AZ::Outcome<Value, AZStd::string> ApplyAndDenormalize(
            Value rootElement, StrategyFunctor strategy = PatchApplicationStrategy::HaltOnFailure);
        //! Applies this patch to the given DOM element in place, changing it to the new value.
        //! After applying the patch, any "EndOfArray" patch entries are denormalized into their resolved paths.
        //! This operation mutates the underyling patch operations; make a copy if you wish to keep the original patch.
        //! \param rootElement The DOM element to patch.
        //! \param strategy A callback to be run after every patch application, see PatchApplicationState.
        //! \return an outcome with either the patched element or an error string
        PatchOutcome ApplyInPlaceAndDenormalize(Value& rootElement, StrategyFunctor strategy = PatchApplicationStrategy::HaltOnFailure);

        Value GetDomRepresentation() const;
        static AZ::Outcome<Patch, AZStd::string> CreateFromDomRepresentation(Value domValue);

        //! Returns true if any of this patch's operations contain  an "EndOfArray" entry
        //! inside their paths, meaning the path requires a lookup inside a target DOM.
        bool ContainsNormalizedEntries() const;

    private:
        OperationsContainer m_operations;
    };
} // namespace AZ::Dom
