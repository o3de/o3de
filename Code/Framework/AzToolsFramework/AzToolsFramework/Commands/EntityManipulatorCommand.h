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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AzToolsFramework
{
    /// Provide interface for Entity manipulator Undo/Redo requests.
    class EditorManipulatorCommandUndoRedoRequests
        : public AZ::EBusTraits
    {
    public:
        using BusIdType = AzFramework::EntityContextId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        /// Set the transform of the current entity manipulator.
        virtual void UndoRedoEntityManipulatorCommand(
            AZ::u8 pivotOverride, const AZ::Transform& transform, AZ::EntityId entityId) = 0;

    protected:
        ~EditorManipulatorCommandUndoRedoRequests() = default;
    };

    /// Type to inherit to implement EditorManipulatorCommandUndoRedoRequests.
    using EditorManipulatorCommandUndoRedoRequestBus = AZ::EBus<EditorManipulatorCommandUndoRedoRequests>;

    /// Stores a manipulator transform change.
    class EntityManipulatorCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityManipulatorCommand, AZ::SystemAllocator, 0);
        AZ_RTTI(EntityManipulatorCommand, "{A50E3220-B690-4DB3-B067-549626F477D8}");

        struct PivotOverride
        {
            enum Enum : AZ::u8
            {
                None = 0,
                Translation = 1 << 0,
                Orientation = 1 << 1,
            };
        };

        /// State relevant to EntityManipulatorCommand before and after
        /// any kind of Entity manipulation.
        struct State
        {
            State() = default;
            State(AZ::u8 pivotOverrideBefore, const AZ::Transform& transformBefore, AZ::EntityId entityIdBefore);

            State(const State&) = default;
            State& operator=(const State&) = default;
            State(State&&) = default;
            State& operator=(State&&) = default;
            ~State() = default;

            AZ::Transform m_transform = AZ::Transform::CreateIdentity();
            AZ::EntityId m_entityId;
            AZ::u8 m_pivotOverride = 0;
        };

        EntityManipulatorCommand(
            const State& state, const AZStd::string& friendlyName);

        void SetManipulatorAfter(const State& state);

        // URSequencePoint
        void Undo() override;
        void Redo() override;
        bool Changed() const override;

    private:
        State m_stateBefore;
        State m_stateAfter;
    };

    bool operator==(
        const EntityManipulatorCommand::State& lhs, const EntityManipulatorCommand::State& rhs);
    bool operator!=(
        const EntityManipulatorCommand::State& lhs, const EntityManipulatorCommand::State& rhs);

    AZ::u8 BuildPivotOverride(
        bool translationOverride, bool orientationOverride);

    bool PivotHasTranslationOverride(AZ::u8 pivotOverride);
    bool PivotHasOrientationOverride(AZ::u8 pivotOverride);
    bool PivotHasTransformOverride(AZ::u8 pivotOverride);

} // namespace AzToolsFramework