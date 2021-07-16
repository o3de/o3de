/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef TRANSFORM_COMMAND_H
#define TRANSFORM_COMMAND_H

#if 0

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/ComponentBus.h>
#include <HexEdFramework/FrameworkCore/UndoSystem.h>
#include <HexEd/WorldEditor/ToolsComponents/TransformComponentBus.h>

#pragma once

namespace AzToolsFramework
{
    namespace UndoSystem
    {
        class UndoCacheInterface;
    }

    typedef AZStd::vector<AZ::EntityId> EntityList;

    // transform command specializes undo to just care about the transform of an entity instead of the entire thing, for performance.
    class TransformCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(TransformCommand, AZ::SystemAllocator, 0);
        AZ_RTTI(TransformCommand);

        TransformCommand(const AZ::u64& contextId, const AZStd::string& friendlyName, const EditorFramework::EntityList& captureEntities);
        virtual ~TransformCommand() {}

        // the default will work for selections with out an undo stack(maybe move the undo stack here too
        void CaptureNewTransform(const AZ::EntityId entityId);
        void RevertToPriorTransform(const AZ::EntityId entityID);

        virtual void Undo();
        virtual void Redo();

        virtual void Post();

    protected:
        AZ::u64 m_contextId;

        typedef AZStd::unordered_map<AZ::EntityId, Components::SRT> CapturedTransforms;

        CapturedTransforms m_priorTransforms;
        CapturedTransforms m_nextTransforms;

    private:
        UndoCacheInterface* m_undoCacheInterface;
    };
}

#endif // disabled

#endif
