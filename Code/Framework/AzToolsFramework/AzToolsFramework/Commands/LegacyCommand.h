/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AzToolsFramework
{
    /**
     * AzToolsFramework URSequencePoint wrapper around legacy IUndoObject
     * Allows using IUndoObject with AzToolsFramework undo system
     */
    template<typename UndoObjectType>
    class LegacyCommand
        : public AzToolsFramework::UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(LegacyCommand<UndoObjectType>, "{9ED33CB6-04D0-4924-A121-D8C27DC09066}", AzToolsFramework::UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(LegacyCommand<UndoObjectType>, AZ::SystemAllocator, 0);

        explicit LegacyCommand(const AZStd::string& friendlyName, AZStd::unique_ptr<UndoObjectType>&& legacyUndo)
            : AzToolsFramework::UndoSystem::URSequencePoint(friendlyName)
        {
            m_legacyUndo = AZStd::move(legacyUndo);
        }
        virtual ~LegacyCommand() = default;

        void Undo() override { m_legacyUndo->Undo(); }
        void Redo() override { m_legacyUndo->Redo(); }

        bool Changed() const override { return true; }

    protected:
        AZStd::unique_ptr<UndoObjectType> m_legacyUndo;
    };
} // namespace AzToolsFramework
