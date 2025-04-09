/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPatch.h>
#include <AzCore/DOM/DomUtils.h>

namespace AZ::Dom
{
    PatchOperation::PatchOperation(Path destinationPath, Type type, Value value)
        : m_domPath(AZStd::move(destinationPath))
        , m_type(type)
        , m_value(AZStd::move(value))
    {
    }

    PatchOperation::PatchOperation(Path destinationPath, Type type, Path sourcePath)
        : m_domPath(AZStd::move(destinationPath))
        , m_type(type)
        , m_value(AZStd::move(sourcePath))
    {
    }

    PatchOperation::PatchOperation(Path destinationPath, Type type)
        : m_domPath(AZStd::move(destinationPath))
        , m_type(type)
    {
    }

    bool PatchOperation::operator==(const PatchOperation& rhs) const
    {
        if (m_type != rhs.m_type)
        {
            return false;
        }

        switch (m_type)
        {
        case Type::Add:
            return m_domPath == rhs.m_domPath && Utils::DeepCompareIsEqual(GetValue(), rhs.GetValue());
        case Type::Remove:
            return m_domPath == rhs.m_domPath;
        case Type::Replace:
            return m_domPath == rhs.m_domPath && Utils::DeepCompareIsEqual(GetValue(), rhs.GetValue());
        case Type::Copy:
            return m_domPath == rhs.m_domPath && GetSourcePath() == rhs.GetSourcePath();
        case Type::Move:
            return m_domPath == rhs.m_domPath && GetSourcePath() == rhs.GetSourcePath();
        case Type::Test:
            return m_domPath == rhs.m_domPath && Utils::DeepCompareIsEqual(GetValue(), rhs.GetValue());
        default:
            AZ_Assert(false, "PatchOperation::GetDomRepresentation: invalid patch type specified");
            return false;
        }
    }

    bool PatchOperation::operator!=(const PatchOperation& rhs) const
    {
        return !operator==(rhs);
    }

    PatchOperation::Type PatchOperation::GetType() const
    {
        return m_type;
    }

    void PatchOperation::SetType(Type type)
    {
        m_type = type;
    }

    const Path& PatchOperation::GetDestinationPath() const
    {
        return m_domPath;
    }

    void PatchOperation::SetDestinationPath(Path path)
    {
        m_domPath = path;
    }

    const Value& PatchOperation::GetValue() const
    {
        return AZStd::get<Value>(m_value);
    }

    void PatchOperation::SetValue(Value value)
    {
        m_value = AZStd::move(value);
    }

    const Path& PatchOperation::GetSourcePath() const
    {
        return AZStd::get<Path>(m_value);
    }

    void PatchOperation::SetSourcePath(Path path)
    {
        m_value = AZStd::move(path);
    }

    AZ::Outcome<Value, AZStd::string> PatchOperation::Apply(Value rootElement) const
    {
        PatchOutcome outcome = ApplyInPlace(rootElement);
        if (!outcome.IsSuccess())
        {
            return AZ::Failure(outcome.TakeError());
        }
        return AZ::Success(AZStd::move(rootElement));
    }

    PatchOutcome PatchOperation::ApplyInPlace(Value& rootElement) const
    {
        switch (m_type)
        {
        case Type::Add:
            return ApplyAdd(rootElement);
        case Type::Remove:
            return ApplyRemove(rootElement);
        case Type::Replace:
            return ApplyReplace(rootElement);
        case Type::Copy:
            return ApplyCopy(rootElement);
        case Type::Move:
            return ApplyMove(rootElement);
        case Type::Test:
            return ApplyTest(rootElement);
        }
        return AZ::Failure<AZStd::string>("Unsupported DOM patch operation specified");
    }

    AZ::Outcome<Value, AZStd::string> PatchOperation::ApplyAndDenormalize(Value rootElement)
    {
        PatchOutcome outcome = ApplyInPlaceAndDenormalize(rootElement);
        if (!outcome.IsSuccess())
        {
            return AZ::Failure(outcome.TakeError());
        }
        return AZ::Success(AZStd::move(rootElement));
    }

    PatchOutcome PatchOperation::ApplyInPlaceAndDenormalize(Value& rootElement)
    {
        switch (m_type)
        {
        case Type::Add:
        case Type::Remove:
        case Type::Replace:
            if (!DenormalizePath(m_domPath, rootElement))
            {
                return AZ::Failure<AZStd::string>("Failed to denormalize patch destination path, an invalid value or path has been specified");
            }
            break;
        case Type::Copy:
        case Type::Move:
            if (!DenormalizePath(m_domPath, rootElement))
            {
                return AZ::Failure<AZStd::string>("Failed to denormalize patch destination path, an invalid value or path has been specified");
            }
            if (!DenormalizePath(AZStd::get<Dom::Path>(m_value), rootElement))
            {
                return AZ::Failure<AZStd::string>("Failed to denormalize patch source path, an invalid value or path has been specified");
            }
            break;
        }
        return ApplyInPlace(rootElement);
    }

    bool PatchOperation::DenormalizePath(Dom::Path& path, const Dom::Value& sourceValue)
    {
        // If we end with an EndOfArray marker, replace it with a marker for the size of sourceValue's array
        if (path.Size() > 0 && path[path.Size() - 1].IsEndOfArray())
        {
            path.Pop();
            const Dom::Value* lookupValue = sourceValue.FindChild(path);
            if (lookupValue == nullptr || (!lookupValue->IsArray() && !lookupValue->IsNode()))
            {
                return false;
            }
            path.Push(lookupValue->ArraySize());
        }
        return true;
    }

    bool PatchOperation::ContainsNormalizedEntries() const
    {
        switch (m_type)
        {
        case Type::Add:
        case Type::Remove:
        case Type::Replace:
            return GetDestinationPath().ContainsNormalizedEntries();
        case Type::Copy:
        case Type::Move:
            return GetSourcePath().ContainsNormalizedEntries() || GetDestinationPath().ContainsNormalizedEntries();
        default:
            return false;
        }
    }

    Value PatchOperation::GetDomRepresentation() const
    {
        Value serializedPatch(Dom::Type::Object);
        switch (m_type)
        {
        case Type::Add:
            serializedPatch["op"].SetString("add");
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            serializedPatch["value"] = GetValue();
            break;
        case Type::Remove:
            serializedPatch["op"].SetString("remove");
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            break;
        case Type::Replace:
            serializedPatch["op"].SetString("replace");
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            serializedPatch["value"] = GetValue();
            break;
        case Type::Copy:
            serializedPatch["op"].SetString("copy");
            serializedPatch["from"].CopyFromString(GetSourcePath().ToString());
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            break;
        case Type::Move:
            serializedPatch["op"].SetString("move");
            serializedPatch["from"].CopyFromString(GetSourcePath().ToString());
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            break;
        case Type::Test:
            serializedPatch["op"].SetString("test");
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            serializedPatch["value"] = GetValue();
            break;
        default:
            AZ_Assert(false, "PatchOperation::GetDomRepresentation: invalid patch type specified");
        }
        return serializedPatch;
    }

    AZ::Outcome<PatchOperation, AZStd::string> PatchOperation::CreateFromDomRepresentation(Value domValue)
    {
        if (!domValue.IsObject())
        {
            return AZ::Failure<AZStd::string>("PatchOperation failed to load: PatchOperation must be specified as an Object");
        }

        auto loadField = [&](const char* field, AZStd::optional<Dom::Type> type = {}) -> AZ::Outcome<Value, AZStd::string>
        {
            auto it = domValue.FindMember(field);
            if (it == domValue.MemberEnd())
            {
                return AZ::Failure(AZStd::string::format("PatchOperation failed to load: no \"%s\" specified", field));
            }

            if (type.has_value() && it->second.GetType() != type)
            {
                return AZ::Failure(AZStd::string::format("PatchOperation failed to load: \"%s\" is invalid", field));
            }

            return AZ::Success(it->second);
        };

        auto opLoad = loadField("op", Dom::Type::String);
        if (!opLoad.IsSuccess())
        {
            return AZ::Failure(opLoad.TakeError());
        }
        AZStd::string_view op = opLoad.GetValue().GetString();
        if (op == "add")
        {
            auto pathLoad = loadField("path", Dom::Type::String);
            if (!pathLoad.IsSuccess())
            {
                return AZ::Failure(pathLoad.TakeError());
            }
            auto valueLoad = loadField("value");
            if (!valueLoad.IsSuccess())
            {
                return AZ::Failure(valueLoad.TakeError());
            }

            return AZ::Success(PatchOperation::AddOperation(Path(pathLoad.GetValue().GetString()), valueLoad.TakeValue()));
        }
        else if (op == "remove")
        {
            auto pathLoad = loadField("path", Dom::Type::String);
            if (!pathLoad.IsSuccess())
            {
                return AZ::Failure(pathLoad.TakeError());
            }

            return AZ::Success(PatchOperation::RemoveOperation(Path(pathLoad.GetValue().GetString())));
        }
        else if (op == "replace")
        {
            auto pathLoad = loadField("path", Dom::Type::String);
            if (!pathLoad.IsSuccess())
            {
                return AZ::Failure(pathLoad.TakeError());
            }
            auto valueLoad = loadField("value");
            if (!valueLoad.IsSuccess())
            {
                return AZ::Failure(valueLoad.TakeError());
            }

            return AZ::Success(PatchOperation::ReplaceOperation(Path(pathLoad.GetValue().GetString()), valueLoad.TakeValue()));
        }
        else if (op == "copy")
        {
            auto destLoad = loadField("path", Dom::Type::String);
            if (!destLoad.IsSuccess())
            {
                return AZ::Failure(destLoad.TakeError());
            }
            auto sourceLoad = loadField("from", Dom::Type::String);
            if (!sourceLoad.IsSuccess())
            {
                return AZ::Failure(sourceLoad.TakeError());
            }

            return AZ::Success(
                PatchOperation::CopyOperation(Path(destLoad.GetValue().GetString()), Path(sourceLoad.GetValue().GetString())));
        }
        else if (op == "move")
        {
            auto destLoad = loadField("path", Dom::Type::String);
            if (!destLoad.IsSuccess())
            {
                return AZ::Failure(destLoad.TakeError());
            }
            auto sourceLoad = loadField("from", Dom::Type::String);
            if (!sourceLoad.IsSuccess())
            {
                return AZ::Failure(sourceLoad.TakeError());
            }

            return AZ::Success(
                PatchOperation::MoveOperation(Path(destLoad.GetValue().GetString()), Path(sourceLoad.GetValue().GetString())));
        }
        else if (op == "test")
        {
            auto pathLoad = loadField("path", Dom::Type::String);
            if (!pathLoad.IsSuccess())
            {
                return AZ::Failure(pathLoad.TakeError());
            }
            auto valueLoad = loadField("value");
            if (!valueLoad.IsSuccess())
            {
                return AZ::Failure(valueLoad.TakeError());
            }

            return AZ::Success(PatchOperation::TestOperation(Path(pathLoad.GetValue().GetString()), valueLoad.TakeValue()));
        }
        else
        {
            return AZ::Failure<AZStd::string>("PatchOperation failed to create DOM representation: invalid \"op\" specified");
        }
    }

    AZ::Outcome<AZStd::fixed_vector<PatchOperation, 2>, AZStd::string> PatchOperation::GetInverse(Value stateBeforeApplication) const
    {
        switch (m_type)
        {
        case Type::Add:
            {
                // Add -> Replace (if value already existed in an object) otherwise
                // Add -> Remove
                if (m_domPath.Size() > 0 && m_domPath[m_domPath.Size() - 1].IsKey())
                {
                    const Value* existingValue = stateBeforeApplication.FindChild(m_domPath);
                    if (existingValue != nullptr)
                    {
                        return AZ::Success<InversePatches>({PatchOperation::ReplaceOperation(m_domPath, *existingValue)});
                    }
                }
                return AZ::Success<InversePatches>({PatchOperation::RemoveOperation(m_domPath)});
            }
        case Type::Remove:
            {
                // Remove -> Add
                const Value* existingValue = stateBeforeApplication.FindChild(m_domPath);
                if (existingValue == nullptr)
                {
                    AZStd::string errorMessage = "Unable to invert DOM remove patch, source path not found: ";
                    m_domPath.AppendToString(errorMessage);
                    return AZ::Failure(AZStd::move(errorMessage));
                }
                return AZ::Success<InversePatches>({PatchOperation::AddOperation(m_domPath, *existingValue)});
            }
        case Type::Replace:
            {
                // Replace -> Replace (with old value)
                const Value* existingValue = stateBeforeApplication.FindChild(m_domPath);
                if (existingValue == nullptr)
                {
                    AZStd::string errorMessage = "Unable to invert DOM replace patch, source path not found: ";
                    m_domPath.AppendToString(errorMessage);
                    return AZ::Failure(AZStd::move(errorMessage));
                }
                return AZ::Success<InversePatches>({PatchOperation::ReplaceOperation(m_domPath, *existingValue)});
            }
        case Type::Copy:
            {
                // Copy -> Replace (with old value)
                const Value* existingValue = stateBeforeApplication.FindChild(m_domPath);
                if (existingValue == nullptr)
                {
                    AZStd::string errorMessage = "Unable to invert DOM copy patch, source path not found: ";
                    m_domPath.AppendToString(errorMessage);
                    return AZ::Failure(AZStd::move(errorMessage));
                }
                return AZ::Success<InversePatches>({PatchOperation::ReplaceOperation(m_domPath, *existingValue)});
            }
        case Type::Move:
            {
                const Value* sourceValue = stateBeforeApplication.FindChild(GetSourcePath());
                if (sourceValue == nullptr)
                {
                    AZStd::string errorMessage = "Unable to invert DOM copy patch, source path not found: ";
                    m_domPath.AppendToString(errorMessage);
                    return AZ::Failure(AZStd::move(errorMessage));
                }

                // If there was a value at the destination path, invert with an add / replace
                const Value* destinationValue = stateBeforeApplication.FindChild(GetDestinationPath());
                if (destinationValue != nullptr)
                {
                    InversePatches result({PatchOperation::AddOperation(GetSourcePath(), *sourceValue)});
                    result.push_back(PatchOperation::ReplaceOperation(GetDestinationPath(), *destinationValue));
                    return AZ::Success<InversePatches>({
                        PatchOperation::AddOperation(GetSourcePath(), *sourceValue),
                        PatchOperation::ReplaceOperation(GetDestinationPath(), *destinationValue),
                    });
                }
                // Otherwise, just do a move
                return AZ::Success<InversePatches>({PatchOperation::MoveOperation(GetDestinationPath(), GetSourcePath())});
            }
        case Type::Test:
            {
                // Test -> Test (no change)
                // When inverting a sequence of patches, applying them in reverse order should allow the test to continue to succeed
                return AZ::Success<InversePatches>({*this});
            }
        }
        return AZ::Failure<AZStd::string>("Unable to invert DOM patch, unknown type specified");
    }

    AZ::Outcome<PatchOperation::PathContext, AZStd::string> PatchOperation::LookupPath(
        Value& rootElement, const Path& path, ExistenceCheckFlags flags)
    {
        const bool verifyFullPath = (flags & ExistenceCheckFlags::VerifyFullPath) != ExistenceCheckFlags::DefaultExistenceCheck;
        const bool allowEndOfArray = (flags & ExistenceCheckFlags::AllowEndOfArray) != ExistenceCheckFlags::DefaultExistenceCheck;

        Path target = path;
        if (target.IsEmpty())
        {
            Value wrapper(Dom::Type::Array);
            wrapper.ArrayPushBack(rootElement);
            return AZ::Success<PathContext>({ wrapper, PathEntry(0) });
        }

        if (verifyFullPath || !allowEndOfArray)
        {
            for (size_t i = 0; i < path.Size(); ++i)
            {
                const PathEntry& entry = path[i];
                if (entry.IsEndOfArray() && (!allowEndOfArray || i != path.Size() - 1))
                {
                    return AZ::Failure<AZStd::string>("Append to array index (\"-\") specified for path that must already exist");
                }
            }
        }

        PathEntry destinationIndex = target[target.Size() - 1];
        target.Pop();

        Value* targetValue = rootElement.FindMutableChild(target);
        if (targetValue == nullptr)
        {
            AZStd::string errorMessage = "Path not found: ";
            target.AppendToString(errorMessage);
            return AZ::Failure(AZStd::move(errorMessage));
        }

        if (destinationIndex.IsIndex() || destinationIndex.IsEndOfArray())
        {
            if (!targetValue->IsArray() && !targetValue->IsNode())
            {
                return AZ::Failure<AZStd::string>("Array index specified for a value that is not an array or node");
            }

            if (destinationIndex.IsIndex())
            {
                // If allowEndOfArray is true, we might get an index exactly equal to our length if we
                // received a denormalized path.
                if (allowEndOfArray && destinationIndex.GetIndex() > targetValue->ArraySize())
                {
                    return AZ::Failure<AZStd::string>("Array index out of bounds");
                }
                else if (!allowEndOfArray && destinationIndex.GetIndex() >= targetValue->ArraySize())
                {
                    return AZ::Failure<AZStd::string>("Array index out of bounds");
                }
            }
        }
        else
        {
            if (!targetValue->IsObject() && !targetValue->IsNode())
            {
                return AZ::Failure<AZStd::string>("Key specified for a value that is not an object or node");
            }

            if (verifyFullPath)
            {
                if (auto it = targetValue->FindMember(destinationIndex.GetKey()); it == targetValue->MemberEnd())
                {
                    return AZ::Failure<AZStd::string>("Key not found in container");
                }
            }
        }

        return AZ::Success<PathContext>({ *targetValue, AZStd::move(destinationIndex) });
    }

    PatchOutcome PatchOperation::ApplyAdd(Value& rootElement) const
    {
        auto pathLookup = LookupPath(rootElement, m_domPath, ExistenceCheckFlags::AllowEndOfArray);
        if (!pathLookup.IsSuccess())
        {
            return AZ::Failure(pathLookup.TakeError());
        }
        const PathContext& context = pathLookup.GetValue();
        const PathEntry& destinationIndex = context.m_key;
        Value& targetValue = context.m_value;

        if (destinationIndex.IsIndex() || destinationIndex.IsEndOfArray())
        {
            if (destinationIndex.IsEndOfArray())
            {
                targetValue.ArrayPushBack(GetValue());
            }
            else
            {
                const size_t index = destinationIndex.GetIndex();
                auto& arrayToChange = targetValue.GetMutableArray();
                arrayToChange.insert(arrayToChange.begin() + index, GetValue());
            }
        }
        else
        {
            targetValue[destinationIndex] = GetValue();
        }
        return AZ::Success();
    }

    PatchOutcome PatchOperation::ApplyRemove(Value& rootElement) const
    {
        auto pathLookup = LookupPath(rootElement, m_domPath, ExistenceCheckFlags::VerifyFullPath);
        if (!pathLookup.IsSuccess())
        {
            return AZ::Failure(pathLookup.TakeError());
        }
        const PathContext& context = pathLookup.GetValue();
        const PathEntry& destinationIndex = context.m_key;
        Value& targetValue = context.m_value;

        if (destinationIndex.IsIndex() || destinationIndex.IsEndOfArray())
        {
            size_t index = destinationIndex.IsEndOfArray() ? targetValue.ArraySize() - 1 : destinationIndex.GetIndex();
            targetValue.ArrayErase(targetValue.MutableArrayBegin() + index);
        }
        else
        {
            auto it = targetValue.FindMutableMember(destinationIndex.GetKey());
            targetValue.EraseMember(it);
        }
        return AZ::Success();
    }

    PatchOutcome PatchOperation::ApplyReplace(Value& rootElement) const
    {
        auto pathLookup = LookupPath(rootElement, m_domPath, ExistenceCheckFlags::VerifyFullPath);
        if (!pathLookup.IsSuccess())
        {
            return AZ::Failure(pathLookup.TakeError());
        }

        rootElement[m_domPath] = GetValue();
        return AZ::Success();
    }

    PatchOutcome PatchOperation::ApplyCopy(Value& rootElement) const
    {
        auto sourceLookup = LookupPath(rootElement, GetSourcePath(), ExistenceCheckFlags::VerifyFullPath);
        if (!sourceLookup.IsSuccess())
        {
            return AZ::Failure(sourceLookup.TakeError());
        }

        auto destLookup = LookupPath(rootElement, m_domPath, ExistenceCheckFlags::DefaultExistenceCheck);
        if (!destLookup.IsSuccess())
        {
            return AZ::Failure(destLookup.TakeError());
        }

        rootElement[m_domPath] = rootElement[GetSourcePath()];
        return AZ::Success();
    }

    PatchOutcome PatchOperation::ApplyMove(Value& rootElement) const
    {
        auto sourceLookup = LookupPath(rootElement, GetSourcePath(), ExistenceCheckFlags::VerifyFullPath);
        if (!sourceLookup.IsSuccess())
        {
            return AZ::Failure(sourceLookup.TakeError());
        }

        auto destLookup = LookupPath(rootElement, m_domPath, ExistenceCheckFlags::DefaultExistenceCheck);
        if (!destLookup.IsSuccess())
        {
            return AZ::Failure(destLookup.TakeError());
        }

        const PathContext& sourceContext = sourceLookup.GetValue();
        Value valueToMove = sourceContext.m_value[sourceContext.m_key];
        if (sourceContext.m_key.IsEndOfArray())
        {
            sourceContext.m_value.ArrayPopBack();
        }
        else if (sourceContext.m_key.IsIndex())
        {
            sourceContext.m_value.ArrayErase(sourceContext.m_value.MutableArrayBegin() + sourceContext.m_key.GetIndex());
        }
        else
        {
            sourceContext.m_value.EraseMember(sourceContext.m_key.GetKey());
        }

        auto newDestLookup = LookupPath(rootElement, m_domPath, ExistenceCheckFlags::AllowEndOfArray);
        const PathContext& destContext = newDestLookup.GetValue();
        const PathEntry& destinationIndex = destContext.m_key;
        Value& targetValue = destContext.m_value;

        if (destinationIndex.IsIndex() || destinationIndex.IsEndOfArray())
        {
            const size_t index = destinationIndex.GetIndex();
            if (destinationIndex.IsEndOfArray() || targetValue.ArraySize() == index)
            {
                targetValue.ArrayPushBack(AZStd::move(valueToMove));
            }
            else
            {
                auto& arrayToChange = targetValue.GetMutableArray();
                arrayToChange.insert(arrayToChange.begin() + index, AZStd::move(valueToMove));
            }
        }
        else
        {
            targetValue[destinationIndex] = AZStd::move(valueToMove);
        }

        return AZ::Success();
    }

    PatchOutcome PatchOperation::ApplyTest(Value& rootElement) const
    {
        auto pathLookup = LookupPath(rootElement, m_domPath, ExistenceCheckFlags::VerifyFullPath);
        if (!pathLookup.IsSuccess())
        {
            return AZ::Failure(pathLookup.TakeError());
        }

        if (!Utils::DeepCompareIsEqual(rootElement[m_domPath], GetValue()))
        {
            return AZ::Failure<AZStd::string>("Test failed, values don't match");
        }

        return AZ::Success();
    }

    void CombinePatchOutcomes(PatchOutcome& lhs, PatchOutcome&& rhs)
    {
        if (!rhs.IsSuccess())
        {
            if (lhs.IsSuccess())
            {
                lhs = rhs;
            }
            else
            {
                lhs.GetError().reserve(lhs.GetError().length() + rhs.GetError().length() + 1);
                lhs.GetError().append(1, '\n');
                lhs.GetError().append(rhs.GetError());
            }
        }
    }

    namespace PatchApplicationStrategy
    {
        void HaltOnFailure(PatchApplicationState& state)
        {
            if (!state.m_outcome.IsSuccess())
            {
                state.m_shouldContinue = false;
            }
        }

        void IgnoreFailureAndContinue([[maybe_unused]] PatchApplicationState& state)
        {
        }
    } // namespace PatchApplicationStrategy

    Patch::Patch(AZStd::initializer_list<PatchOperation> init)
        : m_operations(init)
    {
    }

    bool Patch::operator==(const Patch& rhs) const
    {
        if (m_operations.size() != rhs.m_operations.size())
        {
            return false;
        }

        for (size_t i = 0; i < m_operations.size(); ++i)
        {
            if (m_operations[i] != rhs.m_operations[i])
            {
                return false;
            }
        }

        return true;
    }

    bool Patch::operator!=(const Patch& rhs) const
    {
        return !operator==(rhs);
    }

    const Patch::OperationsContainer& Patch::GetOperations() const
    {
        return m_operations;
    }

    void Patch::PushBack(PatchOperation op)
    {
        m_operations.push_back(AZStd::move(op));
    }

    void Patch::PushFront(PatchOperation op)
    {
        m_operations.insert(m_operations.begin(), AZStd::move(op));
    }

    void Patch::Pop()
    {
        m_operations.pop_back();
    }

    void Patch::Clear()
    {
        m_operations.clear();
    }

    const PatchOperation& Patch::At(size_t index) const
    {
        return m_operations[index];
    }

    size_t Patch::Size() const
    {
        return m_operations.size();
    }

    PatchOperation& Patch::operator[](size_t index)
    {
        return m_operations[index];
    }

    const PatchOperation& Patch::operator[](size_t index) const
    {
        return m_operations[index];
    }

    auto Patch::begin() -> OperationsContainer::iterator
    {
        return m_operations.begin();
    }

    auto Patch::end() -> OperationsContainer::iterator
    {
        return m_operations.end();
    }

    auto Patch::begin() const -> OperationsContainer::const_iterator
    {
        return m_operations.begin();
    }

    auto Patch::end() const -> OperationsContainer::const_iterator
    {
        return m_operations.end();
    }

    auto Patch::cbegin() const -> OperationsContainer::const_iterator
    {
        return m_operations.begin();
    }

    auto Patch::cend() const -> OperationsContainer::const_iterator
    {
        return m_operations.end();
    }

    size_t Patch::size() const
    {
        return m_operations.size();
    }

    AZ::Outcome<Value, AZStd::string> Patch::Apply(Value rootElement, StrategyFunctor strategy) const
    {
        auto result = ApplyInPlace(rootElement, strategy);
        if (!result.IsSuccess())
        {
            return AZ::Failure(result.TakeError());
        }
        return AZ::Success(AZStd::move(rootElement));
    }

    AZ::Outcome<void, AZStd::string> Patch::ApplyInPlace(Value& rootElement, StrategyFunctor strategy) const
    {
        PatchApplicationState state;
        state.m_currentState = &rootElement;
        state.m_patch = this;

        for (const PatchOperation& operation : m_operations)
        {
            state.m_lastOperation = &operation;
            CombinePatchOutcomes(state.m_outcome, operation.ApplyInPlace(rootElement));
            strategy(state);
            if (!state.m_shouldContinue)
            {
                break;
            }
        }
        return state.m_outcome;
    }

    AZ::Outcome<Value, AZStd::string> Patch::ApplyAndDenormalize(Value rootElement, StrategyFunctor strategy)
    {
        auto result = ApplyInPlaceAndDenormalize(rootElement, strategy);
        if (!result.IsSuccess())
        {
            return AZ::Failure(result.TakeError());
        }
        return AZ::Success(AZStd::move(rootElement));
    }

    PatchOutcome Patch::ApplyInPlaceAndDenormalize(Value& rootElement, StrategyFunctor strategy)
    {
        PatchApplicationState state;
        state.m_currentState = &rootElement;
        state.m_patch = this;

        for (PatchOperation& operation : m_operations)
        {
            state.m_lastOperation = &operation;
            CombinePatchOutcomes(state.m_outcome, operation.ApplyInPlaceAndDenormalize(rootElement));
            strategy(state);
            if (!state.m_shouldContinue)
            {
                break;
            }
        }
        return state.m_outcome;
    }

    bool Patch::ContainsNormalizedEntries() const
    {
        for (const PatchOperation& operation : m_operations)
        {
            if (operation.ContainsNormalizedEntries())
            {
                return true;
            }
        }
        return false;
    }

    Value Patch::GetDomRepresentation() const
    {
        Value domValue(Dom::Type::Array);
        for (const PatchOperation& operation : m_operations)
        {
            domValue.ArrayPushBack(operation.GetDomRepresentation());
        }
        return domValue;
    }

    AZ::Outcome<Patch, AZStd::string> Patch::CreateFromDomRepresentation(Value domValue)
    {
        if (!domValue.IsArray())
        {
            return AZ::Failure<AZStd::string>("Patch must be an array");
        }

        Patch patch;
        for (auto it = domValue.ArrayBegin(); it != domValue.ArrayEnd(); ++it)
        {
            auto operationLoadResult = PatchOperation::CreateFromDomRepresentation(*it);
            if (!operationLoadResult.IsSuccess())
            {
                return AZ::Failure(operationLoadResult.TakeError());
            }
            patch.PushBack(operationLoadResult.TakeValue());
        }
        return AZ::Success(AZStd::move(patch));
    }

    PatchOperation PatchOperation::AddOperation(Path destinationPath, Value value)
    {
        return PatchOperation(AZStd::move(destinationPath), PatchOperation::Type::Add, AZStd::move(value));
    }

    PatchOperation PatchOperation::RemoveOperation(Path pathToRemove)
    {
        return PatchOperation(AZStd::move(pathToRemove), PatchOperation::Type::Remove);
    }

    PatchOperation PatchOperation::ReplaceOperation(Path destinationPath, Value value)
    {
        return PatchOperation(AZStd::move(destinationPath), PatchOperation::Type::Replace, AZStd::move(value));
    }

    PatchOperation PatchOperation::CopyOperation(Path destinationPath, Path sourcePath)
    {
        return PatchOperation(AZStd::move(destinationPath), PatchOperation::Type::Copy, AZStd::move(sourcePath));
    }

    PatchOperation PatchOperation::MoveOperation(Path destinationPath, Path sourcePath)
    {
        return PatchOperation(AZStd::move(destinationPath), PatchOperation::Type::Move, AZStd::move(sourcePath));
    }

    PatchOperation PatchOperation::TestOperation(Path testPath, Value value)
    {
        return PatchOperation(AZStd::move(testPath), PatchOperation::Type::Test, AZStd::move(value));
    }
} // namespace AZ::Dom
