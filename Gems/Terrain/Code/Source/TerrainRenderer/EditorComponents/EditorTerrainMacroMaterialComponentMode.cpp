/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>

#include <AzFramework/PaintBrush/PaintBrushNotificationBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsWindow.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

#include <TerrainRenderer/EditorComponents/EditorTerrainMacroMaterialComponentMode.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>
#include <TerrainRenderer/Components/MacroMaterialImageModification.h>

namespace Terrain
{
    //! Class that tracks the data for undoing/redoing a paint stroke.
    class PaintBrushUndoBuffer : public AzToolsFramework::UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrushUndoBuffer, AZ::SystemAllocator);
        AZ_RTTI(PaintBrushUndoBuffer, "{6EF041B7-D59F-4CC1-B75E-0C04D6D091FD}");

        PaintBrushUndoBuffer(AZ::EntityId imageEntityId)
            : AzToolsFramework::UndoSystem::URSequencePoint("PaintStroke")
            , m_entityId(imageEntityId)
        {
        }

        virtual ~PaintBrushUndoBuffer() = default;

        void Undo() override
        {
            if (m_strokeImageBuffer->Empty())
            {
                return;
            }

            // Apply the "undo" buffer
            const bool undo = true;
            m_strokeImageBuffer->ApplyChangeBuffer(undo);

            // Notify anything listening to the terrain macro material that the modified region has changed.
            LmbrCentral::DependencyNotificationBus::Event(
                m_entityId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, m_dirtyArea);
        }

        void Redo() override
        {
            if (m_strokeImageBuffer->Empty())
            {
                return;
            }

            // Apply the "redo" buffer
            const bool undo = false;
            m_strokeImageBuffer->ApplyChangeBuffer(undo);

            // Notify anything listening to the terrain macro material that the modified region has changed.
            LmbrCentral::DependencyNotificationBus::Event(
                m_entityId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, m_dirtyArea);
        }

        bool Changed() const override
        {
            return !m_strokeImageBuffer->Empty();
        }

        void SetUndoBufferAndDirtyArea(AZStd::shared_ptr<ImageTileBuffer> buffer, const AZ::Aabb& dirtyArea)
        {
            m_strokeImageBuffer = buffer;
            m_dirtyArea = dirtyArea;
        }

    private:
        //! The entity containing the modified image gradient.
        const AZ::EntityId m_entityId;

        //! The undo/redo data for the paint strokes.
        AZStd::shared_ptr<ImageTileBuffer> m_strokeImageBuffer;

        //! Cached dirty area
        AZ::Aabb m_dirtyArea;
    };

    EditorTerrainMacroMaterialComponentMode::EditorTerrainMacroMaterialComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        TerrainMacroColorModificationNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());

        // Set our paint brush min/max world size range. The minimum size should be large enough to paint at least one pixel, and
        // the max size is clamped so that we can't paint more than 256 x 256 pixels per brush stamp.
        // 256 is an arbitrary number, but if we start getting much larger, performance can drop precipitously.
        // Note: To truly control performance, additional clamping is still needed, because large mouse movements in world space with
        // a tiny brush can still cause extremely large numbers of brush points to get calculated and checked.

        constexpr float MaxBrushPixelSize = 256.0f;
        AZ::Vector2 imagePixelsPerMeter(0.0f);
        TerrainMacroMaterialRequestBus::EventResult(
            imagePixelsPerMeter, GetEntityId(), &TerrainMacroMaterialRequestBus::Events::GetMacroColorImagePixelsPerMeter);

        float minBrushSize = AZStd::min(imagePixelsPerMeter.GetX(), imagePixelsPerMeter.GetY());
        float maxBrushSize = AZStd::max(imagePixelsPerMeter.GetX(), imagePixelsPerMeter.GetY());

        minBrushSize = (minBrushSize <= 0.0f) ? 0.0f : (1.0f / minBrushSize);
        maxBrushSize = (maxBrushSize <= 0.0f) ? 0.0f : (MaxBrushPixelSize / maxBrushSize);

        AzToolsFramework::GlobalPaintBrushSettingsRequestBus::Broadcast(
            &AzToolsFramework::GlobalPaintBrushSettingsRequestBus::Events::SetSizeRange, minBrushSize, maxBrushSize);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        // Create the paintbrush manipulator with the appropriate color space.
        m_brushManipulator = AzToolsFramework::PaintBrushManipulator::MakeShared(
            worldFromLocal, entityComponentIdPair, AzToolsFramework::PaintBrushColorMode::LinearColor);
        m_brushManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }

    EditorTerrainMacroMaterialComponentMode::~EditorTerrainMacroMaterialComponentMode()
    {
        EndUndoBatch();

        m_brushManipulator->Unregister();
        m_brushManipulator.reset();

        TerrainMacroColorModificationNotificationBus::Handler::BusDisconnect();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorTerrainMacroMaterialComponentMode::PopulateActionsImpl()
    {
        return m_brushManipulator->PopulateActionsImpl();
    }

    AZStd::string EditorTerrainMacroMaterialComponentMode::GetComponentModeName() const
    {
        return "Terrain Macro Material Paint Mode";
    }

    AZ::Uuid EditorTerrainMacroMaterialComponentMode::GetComponentModeType() const
    {
        return AZ::AzTypeInfo<EditorTerrainMacroMaterialComponentMode>::Uuid();
    }

    bool EditorTerrainMacroMaterialComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_brushManipulator->HandleMouseInteraction(mouseInteraction);
    }

    void EditorTerrainMacroMaterialComponentMode::Refresh()
    {
    }

    void EditorTerrainMacroMaterialComponentMode::BeginUndoBatch()
    {
        AZ_Assert(m_undoBatch == nullptr, "Starting an undo batch while one is already active!");

        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            m_undoBatch, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "PaintStroke");

        m_paintBrushUndoBuffer = aznew PaintBrushUndoBuffer(GetEntityId());
        m_paintBrushUndoBuffer->SetParent(m_undoBatch);
    }

    void EditorTerrainMacroMaterialComponentMode::EndUndoBatch()
    {
        if (m_undoBatch != nullptr)
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::Bus::Events::EndUndoBatch);
            m_undoBatch = nullptr;
            m_paintBrushUndoBuffer = nullptr;
        }
    }

    void EditorTerrainMacroMaterialComponentMode::OnTerrainMacroColorBrushStrokeBegin()
    {
        BeginUndoBatch();
    }

    void EditorTerrainMacroMaterialComponentMode::OnTerrainMacroColorBrushStrokeEnd(
        AZStd::shared_ptr<ImageTileBuffer> changedDataBuffer, const AZ::Aabb& dirtyRegion)
    {
        AZ_Assert(m_paintBrushUndoBuffer != nullptr, "Undo batch is expected to exist while painting");

        // Hand over ownership of the paint stroke buffer to the undo/redo buffer.
        m_paintBrushUndoBuffer->SetUndoBufferAndDirtyArea(changedDataBuffer, dirtyRegion);

        EndUndoBatch();
    }

} // namespace Terrain
