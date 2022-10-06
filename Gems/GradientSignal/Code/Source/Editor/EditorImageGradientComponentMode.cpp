/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsWindow.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <Editor/EditorImageGradientComponentMode.h>
#include <Editor/EditorImageGradientRequestBus.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientModificationBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>

#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    //! Class that tracks data for undoing/redoing a paint stroke.
    //! This is currently a naive implementation that just tracks a list of every point that's been modified during the paint stroke.
    //! Duplicate positions will get recorded multiple times.
    class PaintBrushUndoBuffer : public AzToolsFramework::UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrushUndoBuffer, AZ::SystemAllocator, 0);
        AZ_RTTI(PaintBrushUndoBuffer, "{E37936AC-22E1-403A-A36B-55390832EDE4}");

        PaintBrushUndoBuffer(AZ::EntityId imageEntityId)
            : AzToolsFramework::UndoSystem::URSequencePoint("PaintStroke")
            , m_entityId(imageEntityId)
        {
        }

        virtual ~PaintBrushUndoBuffer() = default;

        void Undo() override
        {
            if (m_pixelIndices.empty())
            {
                return;
            }

            // Run through our buffer in backwards order to make sure that positions that appear more than once in our list
            // end with the correct final value.
            for (int32_t index = aznumeric_cast<int32_t>(m_pixelIndices.size()) - 1; index >= 0; index--)
            {
                ImageGradientModificationBus::Event(
                    m_entityId, &ImageGradientModificationBus::Events::SetPixelValueByPixelIndex, m_pixelIndices[index], m_oldValues[index]);
            }

            // Notify anything listening to the image gradient that the modified region has changed.
            // We expand the region by one pixel in each direction to account for any data affected by bilinear filtering as well.
            AZ::Aabb expandedDirtyArea = m_dirtyArea.GetExpanded(AZ::Vector3(m_xMetersPerPixel, m_yMetersPerPixel, 0.0f));
            LmbrCentral::DependencyNotificationBus::Event(
                m_entityId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, expandedDirtyArea);
        }

        void Redo() override
        {
            if (m_pixelIndices.empty())
            {
                return;
            }

            // Replay all the changes in forward order.
            ImageGradientModificationBus::Event(
                m_entityId, &ImageGradientModificationBus::Events::SetPixelValuesByPixelIndex, m_pixelIndices, m_newValues);

            // Notify anything listening to the image gradient that the modified region has changed.
            // We expand the region by one pixel in each direction to account for any data affected by bilinear filtering as well.
            AZ::Aabb expandedDirtyArea = m_dirtyArea.GetExpanded(AZ::Vector3(m_xMetersPerPixel, m_yMetersPerPixel, 0.0f));
            LmbrCentral::DependencyNotificationBus::Event(
                m_entityId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, expandedDirtyArea);
        }

        bool Changed() const override
        {
            return !m_pixelIndices.empty();
        }

        //! Add points to our undo buffer.
        void AddPoints(
            const AZStd::vector<AZ::Vector3>& positions, const AZStd::vector<PixelIndex>& pixelIndices,
            const AZStd::vector<float>& oldValues, const AZStd::vector<float>& newValues)
        {
            if (positions.empty())
            {
                return;
            }

            m_pixelIndices.reserve(m_pixelIndices.size() + positions.size());
            m_oldValues.reserve(m_oldValues.size() + positions.size());
            m_newValues.reserve(m_newValues.size() + positions.size());

            for (size_t index = 0; index < positions.size(); index++)
            {
                if (oldValues[index] != newValues[index])
                {
                    m_pixelIndices.emplace_back(pixelIndices[index]);
                    m_oldValues.emplace_back(oldValues[index]);
                    m_newValues.emplace_back(newValues[index]);
                    m_dirtyArea.AddPoint(positions[index]);
                }
            }
        }

        //! Set the fractional number of meters per pixel in the image.
        //! With bilinear filtering, world distances up to one pixel away in each direction from what we've changed
        //! can potentially be affected by our painted pixels, so we use this to expand our dirty area on change notifications.
        void SetMetersPerPixel(float xAmount, float yAmount)
        {
            m_xMetersPerPixel = xAmount;
            m_yMetersPerPixel = yAmount;
        }

    private:
        //! The entity containing the modified image gradient.
        AZ::EntityId m_entityId;

        //! The undo/redo data for the paint strokes.
        AZStd::vector<PixelIndex> m_pixelIndices;
        AZStd::vector<float> m_oldValues;
        AZStd::vector<float> m_newValues;

        //! Data for tracking the dirty area covered by these changes so that we can broadcast out the change region on undos/redos
        AZ::Aabb m_dirtyArea;
        float m_xMetersPerPixel = 0.0f;
        float m_yMetersPerPixel = 0.0f;
    };

    EditorImageGradientComponentMode::EditorImageGradientComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
        , m_ownerEntityComponentId(entityComponentIdPair)
    {
        EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::StartImageModification);
        ImageGradientModificationBus::Event(GetEntityId(), &ImageGradientModifications::StartImageModification);

        AzToolsFramework::PaintBrushNotificationBus::Handler::BusConnect(entityComponentIdPair);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_brushManipulator = AzToolsFramework::PaintBrushManipulator::MakeShared(worldFromLocal, entityComponentIdPair);
        Refresh();

        m_brushManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);

        CreateSubModeSelectionCluster();
    }

    EditorImageGradientComponentMode::~EditorImageGradientComponentMode()
    {
        RemoveSubModeSelectionCluster();

        AzToolsFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
        m_brushManipulator->Unregister();

        EndUndoBatch();

        // It's possible that we're leaving component mode as the result of an "undo" action.
        // If that's the case, don't prompt the user to save the changes.
        if (!AzToolsFramework::UndoRedoOperationInProgress())
        {
            EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::SaveImage);
        }

        ImageGradientModificationBus::Event(GetEntityId(), &ImageGradientModifications::EndImageModification);
        EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::EndImageModification);
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorImageGradientComponentMode::PopulateActionsImpl()
    {
        return m_brushManipulator->PopulateActionsImpl();
    }

    AZStd::string EditorImageGradientComponentMode::GetComponentModeName() const
    {
        return "Image Gradient Paint Mode";
    }

    bool EditorImageGradientComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_brushManipulator->HandleMouseInteraction(mouseInteraction);
    }

    void EditorImageGradientComponentMode::Refresh()
    {
    }

    void EditorImageGradientComponentMode::BeginUndoBatch()
    {
        AZ_Assert(m_undoBatch == nullptr, "Starting an undo batch while one is already active!");

        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            m_undoBatch, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "PaintStroke");

        m_paintBrushUndoBuffer = aznew PaintBrushUndoBuffer(GetEntityId());
        m_paintBrushUndoBuffer->SetParent(m_undoBatch);
    }

    void EditorImageGradientComponentMode::EndUndoBatch()
    {
        if (m_undoBatch != nullptr)
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::Bus::Events::EndUndoBatch);
            m_undoBatch = nullptr;
            m_paintBrushUndoBuffer = nullptr;
        }
    }

    void EditorImageGradientComponentMode::OnPaintBegin()
    {
        BeginUndoBatch();
    }

    void EditorImageGradientComponentMode::OnPaintEnd()
    {
        EndUndoBatch();
    }

    void EditorImageGradientComponentMode::OnPaint(const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn)
    {
        // The OnPaint notification means that we should paint new values into our image gradient.
        // To do this, we need to calculate the set of world space positions that map to individual pixels in the image,
        // then ask the paint brush for each position what value we should set that pixel to. Finally, we use those modified
        // values to change the image gradient.

        // Get the spacing to map individual pixels to world space positions.
        AZ::Vector2 imagePixelsPerMeter(0.0f);
        ImageGradientRequestBus::EventResult(imagePixelsPerMeter, GetEntityId(), &ImageGradientRequestBus::Events::GetImagePixelsPerMeter);
        if ((imagePixelsPerMeter.GetX() <= 0.0f) || (imagePixelsPerMeter.GetY() <= 0.0f))
        {
            return;
        }

        const float xMetersPerPixel = 1.0f / imagePixelsPerMeter.GetX();
        const float yMetersPerPixel = 1.0f / imagePixelsPerMeter.GetY();

        const AZ::Vector3 minDistances = dirtyArea.GetMin();
        const AZ::Vector3 maxDistances = dirtyArea.GetMax();
        const float zMinDistance = minDistances.GetZ();

        const int32_t xPoints = aznumeric_cast<int32_t>((maxDistances.GetX() - minDistances.GetX()) / xMetersPerPixel);
        const int32_t yPoints = aznumeric_cast<int32_t>((maxDistances.GetY() - minDistances.GetY()) / yMetersPerPixel);

        // Calculate the minimum set of world space points that map to those pixels.
        AZStd::vector<AZ::Vector3> points;
        points.reserve(xPoints * yPoints);
        for (float y = minDistances.GetY(); y <= maxDistances.GetY(); y += yMetersPerPixel)
        {
            for (float x = minDistances.GetX(); x <= maxDistances.GetX(); x += xMetersPerPixel)
            {
                points.emplace_back(x, y, zMinDistance);
            }
        }

        // Query the paintbrush with those points to get back the subset of points and brush values for each point that's
        // affected by the brush.
        AZStd::vector<AZ::Vector3> validPoints;
        AZStd::vector<float> intensities;
        AZStd::vector<float> opacities;
        valueLookupFn(points, validPoints, intensities, opacities);

        // Early out if none of the points were actually affected by the brush.
        if (validPoints.empty())
        {
            return;
        }

        // Get the pixel indices and current image gradient values for each position.
        AZStd::vector<PixelIndex> pixelIndices(validPoints.size());
        AZStd::vector<float> gradientValues(validPoints.size());
        ImageGradientModificationBus::Event(
            GetEntityId(), &ImageGradientModificationBus::Events::GetPixelIndicesForPositions, validPoints, pixelIndices);
        ImageGradientModificationBus::Event(
            GetEntityId(), &ImageGradientModificationBus::Events::GetPixelValuesByPixelIndex, pixelIndices, gradientValues);

        AZ_Assert(m_paintBrushUndoBuffer != nullptr, "Undo batch is expected to exist while painting");
        m_paintBrushUndoBuffer->SetMetersPerPixel(xMetersPerPixel, yMetersPerPixel);

        // For each position, blend the original gradient value with the paintbrush value
        AZStd::vector<float> paintedValues;
        paintedValues.reserve(validPoints.size());

        for (size_t index = 0; index < validPoints.size(); index++)
        {
            paintedValues.emplace_back(blendFn(gradientValues[index], intensities[index], opacities[index]));
        }

        // For each position, save the new value into the undo buffer
        m_paintBrushUndoBuffer->AddPoints(validPoints, pixelIndices, gradientValues, paintedValues);

        // Finally, modify the image with all of the changed values
        ImageGradientModificationBus::Event(
            GetEntityId(), &ImageGradientModificationBus::Events::SetPixelValuesByPixelIndex,
            pixelIndices,
            paintedValues);

        // Notify anything listening to the image gradient that the modified region has changed.
        // Because Image Gradients support bilinear filtering, we need to expand our dirty area by an extra pixel in each direction
        // so that the effects of the painted values on adjacent pixels are taken into account when refreshing.
        AZ::Aabb expandedDirtyArea(dirtyArea);
        expandedDirtyArea.Expand(AZ::Vector3(xMetersPerPixel, yMetersPerPixel, 0.0f));
        LmbrCentral::DependencyNotificationBus::Event(
            GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, expandedDirtyArea);
    }

    void EditorImageGradientComponentMode::CreateSubModeSelectionCluster()
    {
        auto RegisterClusterButton = [](AzToolsFramework::ViewportUi::ClusterId clusterId,
                                        const char* iconName,
                                        const char* tooltip) -> AzToolsFramework::ViewportUi::ButtonId
        {
            AzToolsFramework::ViewportUi::ButtonId buttonId;
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                buttonId,
                AzToolsFramework::ViewportUi::DefaultViewportId,
                &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton,
                clusterId,
                AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                AzToolsFramework::ViewportUi::DefaultViewportId,
                &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
                clusterId,
                buttonId,
                tooltip);

            return buttonId;
        };

        // create the cluster for showing the Paint Brush Settings window
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            m_paintBrushControlClusterId,
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            AzToolsFramework::ViewportUi::Alignment::TopLeft);

        // create and register the "Show Paint Brush Settings" button.
        // This button is needed because the window is only shown while in component mode, and the window can be closed by the user,
        // so we need to provide an alternate way for the user to re-open the window. 
        m_paintBrushSettingsButtonId = RegisterClusterButton(m_paintBrushControlClusterId, "Paint", "Show Paint Brush Settings");

        m_buttonSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(
            [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                if (buttonId == m_paintBrushSettingsButtonId)
                {
                    AzToolsFramework::OpenViewPane(PaintBrush::s_paintBrushSettingsName);
                }
            });
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_paintBrushControlClusterId,
            m_buttonSelectionHandler);
    }

    void EditorImageGradientComponentMode::RemoveSubModeSelectionCluster()
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_paintBrushControlClusterId);
    }

} // namespace GradientSignal
