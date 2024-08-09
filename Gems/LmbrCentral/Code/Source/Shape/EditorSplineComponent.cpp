/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSplineComponent.h"
#include "EditorSplineComponentMode.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Viewport/VertexContainerDisplay.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include "MathConversion.h"

namespace LmbrCentral
{
    static const float s_lineWidth = 0.1f; ///< The 'virtual' width of the line (for selection)

    void EditorSplineComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorSplineComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SplineService"));
        provided.push_back(AZ_CRC_CE("VariableVertexContainerService"));
        provided.push_back(AZ_CRC_CE("FixedVertexContainerService"));
    }

    void EditorSplineComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("VariableVertexContainerService"));
        incompatible.push_back(AZ_CRC_CE("FixedVertexContainerService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorSplineComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSplineComponent, EditorComponentBase>()
                ->Version(2)
                ->Field("Visible", &EditorSplineComponent::m_visibleInEditor)
                ->Field("Configuration", &EditorSplineComponent::m_splineCommon)
                ->Field("ComponentMode", &EditorSplineComponent::m_componentModeDelegate)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSplineComponent>(
                    "Spline", "Defines a sequence of points that can be interpolated.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Spline.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Spline.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/spline/")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSplineComponent::m_visibleInEditor, "Visible", "Always display this shape in the editor viewport")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSplineComponent::m_splineCommon, "Configuration", "Spline Configuration")
                        //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly) // disabled - prevents ChangeNotify attribute firing correctly
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSplineComponent::SplineChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSplineComponent::m_componentModeDelegate, "Component Mode", "Spline Component Mode")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }

        EditorSplineComponentMode::Reflect(context);
    }

    void EditorSplineComponent::Activate()
    {
        EditorComponentBase::Activate();

        const AZ::EntityId entityId = GetEntityId();

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(entityId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        SplineComponentRequestBus::Handler::BusConnect(entityId);
        AZ::VariableVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);

        m_cachedUniformScaleTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_cachedUniformScaleTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);

        // placeholder - create initial spline if empty
        AZ::VertexContainer<AZ::Vector3>& vertexContainer = m_splineCommon.m_spline->m_vertexContainer;
        if (vertexContainer.Empty())
        {
            vertexContainer.AddVertex(AZ::Vector3(-3.0f, 0.0f, 0.0f));
            vertexContainer.AddVertex(AZ::Vector3(-1.0f, 0.0f, 0.0f));
            vertexContainer.AddVertex(AZ::Vector3(1.0f, 0.0f, 0.0f));
            vertexContainer.AddVertex(AZ::Vector3(3.0f, 0.0f, 0.0f));
        }

        const auto vertexAdded = [this](size_t vertIndex)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexAdded, vertIndex);

            SplineChanged();
        };

        const auto vertexRemoved = [this](size_t vertIndex)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexRemoved, vertIndex);

            SplineChanged();
        };

        const auto vertexUpdated = [this](size_t vertIndex)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexUpdated, vertIndex);

            SplineChanged();
        };

        const auto verticesSet = [this]()
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVerticesSet,
                m_splineCommon.m_spline->GetVertices());

            SplineChanged();
        };

        const auto verticesCleared = [this]()
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVerticesCleared);

            SplineChanged();
        };

        const auto openCloseChanged = [this](const bool closed)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnOpenCloseChanged, closed);

            SplineChanged();
        };

        m_splineCommon.SetCallbacks(
            vertexAdded,
            vertexRemoved,
            vertexUpdated,
            verticesSet,
            verticesCleared,
            [this]() { OnChangeSplineType(); },
            openCloseChanged);

        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorSplineComponent, EditorSplineComponentMode>(
                AZ::EntityComponentIdPair(GetEntityId(), GetId()), this);
    }

    void EditorSplineComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();

        m_splineCommon.SetCallbacks(
            nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr);

        m_componentModeDelegate.Disconnect();

        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
        AZ::VariableVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
        SplineComponentRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
    }

    void EditorSplineComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<SplineComponent>())
        {
            component->m_splineCommon = m_splineCommon;
        }
    }

    static void DrawSpline(
        const AZ::Spline& spline, const size_t begin, const size_t end,
        const AZ::Transform& worldFromLocal, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const size_t granularity = spline.GetSegmentGranularity();

        for (size_t vertIndex = begin; vertIndex < end; ++vertIndex)
        {
            AZ::Vector3 p1 = worldFromLocal.TransformPoint(spline.GetVertex(vertIndex - 1));
            for (size_t granularityStep = 1; granularityStep <= granularity; ++granularityStep)
            {
                const AZ::Vector3 p2 = worldFromLocal.TransformPoint(spline.GetPosition(
                    AZ::SplineAddress(vertIndex - 1, granularityStep / static_cast<float>(granularity))));
                debugDisplay.DrawLine(p1, p2);
                p1 = p2;
            }
        }
    }

    static void DrawVertices(
        const AZ::Spline& spline, const AZ::Transform& worldFromLocal,
        const AzFramework::CameraState& cameraState,
        const size_t begin, const size_t end, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        for (size_t vertIndex = begin; vertIndex < end; ++vertIndex)
        {
            const AZ::Vector3& worldPosition = worldFromLocal.TransformPoint(spline.GetVertex(vertIndex));
            debugDisplay.DrawBall(
                worldPosition, 0.075f * AzToolsFramework::CalculateScreenToWorldMultiplier(worldPosition, cameraState));
        }
    }

    void EditorSplineComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const bool mouseHovered = m_accentType == AzToolsFramework::EntityAccentType::Hover;
        if (!IsSelected() && !m_visibleInEditor && !mouseHovered)
        {
            return;
        }

        AzFramework::CameraState cameraState;
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            cameraState, viewportInfo.m_viewportId,
            &AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);

        const AZ::Spline* spline = m_splineCommon.m_spline.get();
        const size_t vertexCount = spline->GetVertexCount();
        if (vertexCount == 0)
        {
            return;
        }

        // Set color
        if (IsSelected())
        {
            debugDisplay.SetColor(AzFramework::ViewportColors::SelectedColor);
        }
        else if(mouseHovered)
        {
            debugDisplay.SetColor(AzFramework::ViewportColors::HoverColor);
        }
        else
        {
            debugDisplay.SetColor(AzFramework::ViewportColors::DeselectedColor);
        }

        const bool inComponentMode = m_componentModeDelegate.AddedToComponentMode();

        // render spline
        if (spline->RTTI_IsTypeOf(AZ::LinearSpline::RTTI_Type()) || spline->RTTI_IsTypeOf(AZ::BezierSpline::RTTI_Type()))
        {
            DrawSpline(
                *spline, 1, spline->IsClosed() ? vertexCount + 1 : vertexCount,
                m_cachedUniformScaleTransform, debugDisplay);

            if (!inComponentMode)
            {
                DrawVertices(*spline, m_cachedUniformScaleTransform, cameraState, 0, vertexCount, debugDisplay);
            }
        }
        else if (spline->RTTI_IsTypeOf(AZ::CatmullRomSpline::RTTI_Type()))
        {
            // the minimum number of control points for a Catmull-Rom spline is 4; do not attempt to render the spline
            // unless this condition is met
            if (spline->GetVertexCount() >= 4)
            {
                // a Catmull-Rom spline uses the first and last points as control points only, omit them from display
                DrawSpline(
                    *spline, spline->IsClosed() ? 1 : 2, spline->IsClosed() ? vertexCount + 1 : vertexCount - 1,
                    m_cachedUniformScaleTransform, debugDisplay);
            }

            if (!inComponentMode)
            {
                DrawVertices(*spline, m_cachedUniformScaleTransform, cameraState, 1, vertexCount - 1, debugDisplay);
            }
        }

        if (inComponentMode)
        {
            AzToolsFramework::VertexContainerDisplay::DisplayVertexContainerIndices(
                debugDisplay, AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector3>(
                    m_splineCommon.m_spline->m_vertexContainer), m_cachedUniformScaleTransform, AZ::Vector3::CreateOne(), IsSelected());
        }
    }

    AZ::Aabb EditorSplineComponent::GetEditorSelectionBoundsViewport(
        const AzFramework::ViewportInfo& viewportInfo)
    {
        AzFramework::CameraState cameraState;
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            cameraState, viewportInfo.m_viewportId,
            &AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);

        const float screenToWorldScale = AzToolsFramework::CalculateScreenToWorldMultiplier(
            m_cachedUniformScaleTransform.GetTranslation(), cameraState);

        const float lineWidth = s_lineWidth * screenToWorldScale;
        AZ::Aabb aabb = GetWorldBounds();
        const AZ::Vector3 extents = aabb.GetExtents();

        AZ::Vector3 expandExtents = AZ::Vector3::CreateZero();
        if (extents.GetX() < lineWidth)
        {
            expandExtents.SetX(lineWidth);
        }

        if (extents.GetY() < lineWidth)
        {
            expandExtents.SetY(lineWidth);
        }

        if (extents.GetZ() < lineWidth)
        {
            expandExtents.SetZ(lineWidth);
        }

        aabb.Expand(expandExtents);

        return aabb;
    }

    bool EditorSplineComponent::EditorSelectionIntersectRayViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        const auto rayIntersectData = IntersectSpline(m_cachedUniformScaleTransform, src, dir, *m_splineCommon.m_spline);
        distance = rayIntersectData.m_rayDistance * m_cachedUniformScaleTransform.GetUniformScale();

        AzFramework::CameraState cameraState;
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            cameraState, viewportInfo.m_viewportId,
            &AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);

        const float screenToWorldScale = AzToolsFramework::CalculateScreenToWorldMultiplier(
            m_cachedUniformScaleTransform.GetTranslation(), cameraState);

        return (static_cast<float>(rayIntersectData.m_distanceSq) < powf(s_lineWidth * screenToWorldScale, 2.0f));
    }

    bool EditorSplineComponent::SupportsEditorRayIntersect()
    {
        return AzToolsFramework::HelpersVisible();
    }

    bool EditorSplineComponent::SupportsEditorRayIntersectViewport(const AzFramework::ViewportInfo& viewportInfo)
    {
        bool helpersVisible = false;
        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            helpersVisible, viewportInfo.m_viewportId,
            &AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Events::HelpersVisible);
        return helpersVisible;
    }

    void EditorSplineComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_cachedUniformScaleTransform = world;
    }

    void EditorSplineComponent::OnChangeSplineType()
    {
        EditorSplineComponentNotificationBus::Event(
            GetEntityId(), &EditorSplineComponentNotifications::OnSplineTypeChanged);

        SplineChanged();
    }

    void EditorSplineComponent::SplineChanged() const
    {
        SplineComponentNotificationBus::Event(
            GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
        AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(GetEntityId());
    }

    AZ::SplinePtr EditorSplineComponent::GetSpline()
    {
        return m_splineCommon.m_spline;
    }

    void EditorSplineComponent::ChangeSplineType(const SplineType splineType)
    {
        m_splineCommon.ChangeSplineType(splineType);
    }

    void EditorSplineComponent::SetClosed(const bool closed)
    {
        m_splineCommon.m_spline->SetClosed(closed);
        SplineChanged();
    }

    bool EditorSplineComponent::GetVertex(const size_t vertIndex, AZ::Vector3& vertex) const
    {
        return m_splineCommon.m_spline->m_vertexContainer.GetVertex(vertIndex, vertex);
    }

    bool EditorSplineComponent::UpdateVertex(const size_t vertIndex, const AZ::Vector3& vertex)
    {
        if (m_splineCommon.m_spline->m_vertexContainer.UpdateVertex(vertIndex, vertex))
        {
            SplineChanged();
            return true;
        }

        return false;
    }

    void EditorSplineComponent::AddVertex(const AZ::Vector3& vertex)
    {
        m_splineCommon.m_spline->m_vertexContainer.AddVertex(vertex);
        SplineChanged();
    }

    bool EditorSplineComponent::InsertVertex(const size_t vertIndex, const AZ::Vector3& vertex)
    {
        if (m_splineCommon.m_spline->m_vertexContainer.InsertVertex(vertIndex, vertex))
        {
            SplineChanged();
            return true;
        }

        return false;
    }

    bool EditorSplineComponent::RemoveVertex(const size_t vertIndex)
    {
        if (m_splineCommon.m_spline->m_vertexContainer.RemoveVertex(vertIndex))
        {
            SplineChanged();
            return true;
        }

        return false;
    }

    void EditorSplineComponent::SetVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_splineCommon.m_spline->m_vertexContainer.SetVertices(vertices);
        SplineChanged();
    }

    void EditorSplineComponent::ClearVertices()
    {
        m_splineCommon.m_spline->m_vertexContainer.Clear();
        SplineChanged();
    }

    size_t EditorSplineComponent::Size() const
    {
        return m_splineCommon.m_spline->m_vertexContainer.Size();
    }

    bool EditorSplineComponent::Empty() const
    {
        return m_splineCommon.m_spline->m_vertexContainer.Empty();
    }

    AZ::Aabb EditorSplineComponent::GetWorldBounds() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        m_splineCommon.m_spline->GetAabb(aabb, m_cachedUniformScaleTransform);
        return aabb;
    }

    AZ::Aabb EditorSplineComponent::GetLocalBounds() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        m_splineCommon.m_spline->GetAabb(aabb, AZ::Transform::CreateIdentity());
        return aabb;
    }
} // namespace LmbrCentral
