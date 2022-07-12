/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomComparison.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AZ::Dom
{
    PatchUndoRedoInfo GenerateHierarchicalDeltaPatch(
        const Value& beforeState, const Value& afterState, const DeltaPatchGenerationParameters& params)
    {
        PatchUndoRedoInfo patches;

        auto AddPatch = [&patches](PatchOperation op, PatchOperation inverse)
        {
            patches.m_forwardPatches.PushBack(AZStd::move(op));
            patches.m_inversePatches.PushFront(AZStd::move(inverse));
        };

        AZStd::function<void(const Path&, const Value&, const Value&)> compareValues;

        struct PendingComparison
        {
            Path m_path;
            const Value& m_before;
            const Value& m_after;

            PendingComparison(Path path, const Value& before, const Value& after)
                : m_path(AZStd::move(path))
                , m_before(before)
                , m_after(after)
            {
            }
        };
        AZStd::queue<PendingComparison> entriesToCompare;

        AZStd::unordered_set<AZ::Name::Hash> desiredKeys;
        auto compareObjects = [&](const Path& path, const Value& before, const Value& after)
        {
            desiredKeys.clear();
            Path subPath = path;
            for (auto it = after.MemberBegin(); it != after.MemberEnd(); ++it)
            {
                desiredKeys.insert(it->first.GetHash());
                subPath.Push(it->first);
                auto beforeIt = before.FindMember(it->first);
                if (beforeIt == before.MemberEnd())
                {
                    AddPatch(PatchOperation::AddOperation(subPath, it->second), PatchOperation::RemoveOperation(subPath));
                }
                else
                {
                    entriesToCompare.emplace(subPath, beforeIt->second, it->second);
                }
                subPath.Pop();
            }

            for (auto it = before.MemberBegin(); it != before.MemberEnd(); ++it)
            {
                if (!desiredKeys.contains(it->first.GetHash()))
                {
                    subPath.Push(it->first);
                    AddPatch(PatchOperation::RemoveOperation(subPath), PatchOperation::AddOperation(subPath, it->second));
                    subPath.Pop();
                }
            }
        };

        auto compareArrays = [&](const Path& path, const Value& before, const Value& after)
        {
            const size_t beforeSize = before.ArraySize();
            const size_t afterSize = after.ArraySize();

            // If more than replaceThreshold values differ, do a replace operation instead
            if (params.m_replaceThreshold != DeltaPatchGenerationParameters::NoReplace)
            {
                size_t changedValueCount = 0;
                const size_t entriesToEnumerate = AZStd::min(beforeSize, afterSize);
                for (size_t i = 0; i < entriesToEnumerate; ++i)
                {
                    if (before[i] != after[i])
                    {
                        ++changedValueCount;
                        if (changedValueCount >= params.m_replaceThreshold)
                        {
                            AddPatch(PatchOperation::ReplaceOperation(path, after), PatchOperation::ReplaceOperation(path, before));
                            return;
                        }
                    }
                }
            }

            Path subPath = path;
            for (size_t i = 0; i < afterSize; ++i)
            {
                if (i >= beforeSize)
                {
                    subPath.Push(PathEntry(PathEntry::EndOfArrayIndex));
                    AddPatch(PatchOperation::AddOperation(subPath, after[i]), PatchOperation::RemoveOperation(subPath));
                    subPath.Pop();
                }
                else
                {
                    subPath.Push(PathEntry(i));
                    entriesToCompare.emplace(subPath, before[i], after[i]);
                    subPath.Pop();
                }
            }

            if (beforeSize > afterSize)
            {
                subPath.Push(PathEntry(PathEntry::EndOfArrayIndex));
                for (size_t i = beforeSize; i > afterSize; --i)
                {
                    AddPatch(PatchOperation::RemoveOperation(subPath), PatchOperation::AddOperation(subPath, before[i - 1]));
                }
            }
        };

        auto compareNodes = [&](const Path& path, const Value& before, const Value& after)
        {
            if (before.GetNodeName() != after.GetNodeName())
            {
                AddPatch(PatchOperation::ReplaceOperation(path, after), PatchOperation::ReplaceOperation(path, before));
            }
            else
            {
                compareObjects(path, before, after);
                compareArrays(path, before, after);
            }
        };

        compareValues = [&](const Path& path, const Value& before, const Value& after)
        {
            if (before.GetType() != after.GetType())
            {
                AddPatch(PatchOperation::ReplaceOperation(path, after), PatchOperation::ReplaceOperation(path, before));
            }
            else if (before == after)
            {
                // If a shallow comparison succeeds we're pointing to an identical value or container
                // and don't need to drill down.
                return;
            }
            else if (before.IsObject())
            {
                compareObjects(path, before, after);
            }
            else if (before.IsArray())
            {
                compareArrays(path, before, after);
            }
            else if (before.IsNode())
            {
                compareNodes(path, before, after);
            }
            else
            {
                AddPatch(PatchOperation::ReplaceOperation(path, after), PatchOperation::ReplaceOperation(path, before));
            }
        };

        entriesToCompare.emplace(Path(), beforeState, afterState);
        while (!entriesToCompare.empty())
        {
            PendingComparison& comparison = entriesToCompare.front();
            compareValues(comparison.m_path, comparison.m_before, comparison.m_after);
            entriesToCompare.pop();
        }
        return patches;
    }
}
