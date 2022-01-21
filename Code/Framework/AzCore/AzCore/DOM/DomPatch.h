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

namespace AZ::Dom
{
    //! A patch operation that represents an atomic operation for mutating or validating a Value.
    //! PatchOperations can be created with helper methods in Patch. /see Patch
    class PatchOperation final
    {
    public:
        using PatchOutcome = AZ::Outcome<void, AZStd::string>;

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

        explicit PatchOperation(Path destinationPath, Type type, Value value);
        explicit PatchOperation(Path destionationPath, Type type, Path sourcePath);
        explicit PatchOperation(Path path, Type type);

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

        Value GetDomRepresentation() const;
        static AZ::Outcome<PatchOperation, AZStd::string> CreateFromDomRepresentation(Value domValue);

        AZ::Outcome<PatchOperation, AZStd::string> GetInverse(Value stateBeforeApplication) const;

    private:
        struct PathContext
        {
            Value& m_value;
            PathEntry m_key;
        };

        static constexpr AZ::u8 DefaultExistenceCheck = 0x0;
        static constexpr AZ::u8 VerifyFullPath = 0x1;
        static constexpr AZ::u8 AllowEndOfArray = 0x2;

        static AZ::Outcome<PathContext, AZStd::string> LookupPath(
            Value& rootElement, const Path& path, AZ::u8 existenceCheckFlags = DefaultExistenceCheck);

        PatchOutcome ApplyAdd(Value& rootElement) const;
        PatchOutcome ApplyRemove(Value& rootElement) const;
        PatchOutcome ApplyReplace(Value& rootElement) const;
        PatchOutcome ApplyCopy(Value& rootElement) const;
        PatchOutcome ApplyMove(Value& rootElement) const;
        PatchOutcome ApplyTest(Value& rootElement) const;

        Path m_domPath;
        Type m_type;
        AZStd::variant<AZStd::monostate, Value, Path> m_value;
    };

    class Patch;

    //! The current state of a Patch application operation.
    struct PatchApplicationState
    {
        //! The patch being applied.
        const Patch* m_patch = nullptr;
        //! The last operation attempted.
        const PatchOperation* m_lastOperation = nullptr;
        //! The outcome of the last operation, may be overridden to produce a different failure outcome.
        PatchOperation::PatchOutcome m_outcome;
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
        using OperationsContainer = AZStd::vector<PatchOperation>;

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

        AZ::Outcome<Value, AZStd::string> Apply(Value rootElement, StrategyFunctor strategy = PatchApplicationStrategy::HaltOnFailure) const;
        AZ::Outcome<void, AZStd::string> ApplyInPlace(Value& rootElement, StrategyFunctor strategy = PatchApplicationStrategy::HaltOnFailure) const;

        Value GetDomRepresentation() const;
        static AZ::Outcome<Patch, AZStd::string> CreateFromDomRepresentation(Value domValue);

    private:
        OperationsContainer m_operations;
    };

    //! A set of patches for applying a change and doing the inverse operation (i.e. undoing it).
    struct PatchInfo
    {
        Patch m_forwardPatches;
        Patch m_inversePatches;
    };

    //! Generates a set of patches such that m_forwardPatches.Apply(beforeState) shall produce a document equivalent to afterState, and
    //! a subsequent m_inversePatches.Apply(beforeState) shall produce the original document. This patch generation strategy does a
    //! hierarchical comparison and is not guaranteed to create the minimal set of patches required to transform between the two states.
    PatchInfo GenerateHierarchicalDeltaPatch(const Value& beforeState, const Value& afterState);
} // namespace AZ::Dom
