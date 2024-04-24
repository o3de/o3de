/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SplineComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace LmbrCentral
{
    using SplineComboBoxVec = AZStd::vector<AZ::Edit::EnumConstant<SplineType>>;
    static SplineComboBoxVec PopulateSplineTypeList()
    {
        return SplineComboBoxVec
        {
            { SplineType::LINEAR, "Linear" },
            { SplineType::BEZIER, "Bezier" },
            { SplineType::CATMULL_ROM, "Catmull-Rom" }
        };
    }
    static bool IsMatchingType(AZ::SplinePtr spline, SplineType splineType)
    {
        auto splineTypeHash = spline->RTTI_GetType().GetHash();
        switch (splineType)
        {
        case SplineType::LINEAR:
            {
                return (splineTypeHash == AZ::LinearSpline::RTTI_Type().GetHash());
            }
            break;
        case SplineType::BEZIER:
            {
                return (splineTypeHash == AZ::BezierSpline::RTTI_Type().GetHash());
            }
            break;
        case SplineType::CATMULL_ROM:
            {
                return (splineTypeHash == AZ::CatmullRomSpline::RTTI_Type().GetHash());
            }
            break;
        default:
            break;
        }

        return false;
    }

    static AZ::SplinePtr MakeSplinePtr(SplineType splineType)
    {
        switch (splineType)
        {
        case SplineType::LINEAR:
            {
                return AZStd::make_shared<AZ::LinearSpline>();
            }
            break;
        case SplineType::BEZIER:
            {
                return AZStd::make_shared<AZ::BezierSpline>();
            }
            break;
        case SplineType::CATMULL_ROM:
            {
                return AZStd::make_shared<AZ::CatmullRomSpline>();
            }
            break;
        default:
            {
                AZ_Assert(false, "Unhandled spline type %d in %s", splineType, __FUNCTION__);
            }
            break;
        }

        return nullptr;
    }

    static AZ::SplinePtr CopySplinePtr(SplineType splineType, const AZ::SplinePtr& spline)
    {
        switch (splineType)
        {
        case SplineType::LINEAR:
            {
                return AZStd::make_shared<AZ::LinearSpline>(*spline);
            }
            break;
        case SplineType::BEZIER:
            {
                return AZStd::make_shared<AZ::BezierSpline>(*spline);
            }
            break;
        case SplineType::CATMULL_ROM:
            {
                return AZStd::make_shared<AZ::CatmullRomSpline>(*spline);
            }
            break;
        default:
            {
                AZ_Assert(false, "Unhandled spline type %d in %s", splineType, __FUNCTION__);
            }
            break;
        }

        return nullptr;
    }

    SplineCommon::SplineCommon()
    {
        m_spline = MakeSplinePtr(m_splineType);
    }

    void SplineCommon::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineCommon>()
                ->Version(1)
                ->Field("Spline Type", &SplineCommon::m_splineType)
                ->Field("Spline", &SplineCommon::m_spline);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SplineCommon>("Configuration", "Spline configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly) // disabled - prevents ChangeNotify attribute firing correctly
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SplineCommon::m_splineType, "Spline Type", "Interpolation type to use between vertices.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &PopulateSplineTypeList)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SplineCommon::OnChangeSplineType)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SplineCommon::m_spline, "Spline", "Data representing the spline.")
                        //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly) // disabled - prevents ChangeNotify attribute firing correctly
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void SplineCommon::ChangeSplineType(SplineType splineType)
    {
        m_splineType = splineType;
        OnChangeSplineType();
    }

    void SplineCommon::SetCallbacks(
        const AZ::IndexFunction& OnAddVertex, const AZ::IndexFunction& OnRemoveVertex,
        const AZ::IndexFunction& OnUpdateVertex, const AZ::VoidFunction& OnSetVertices,
        const AZ::VoidFunction& OnClearVertices, const AZ::VoidFunction& OnChangeType,
        const AZ::BoolFunction& OnOpenClose)
    {
        m_onAddVertex = OnAddVertex;
        m_onRemoveVertex = OnRemoveVertex;
        m_onUpdateVertex = OnUpdateVertex;
        m_onSetVertices = OnSetVertices;
        m_onClearVertices = OnClearVertices;

        m_onChangeType = OnChangeType;
        m_onOpenCloseChange = OnOpenClose;

        m_spline->SetCallbacks(
            OnAddVertex, OnRemoveVertex,
            OnUpdateVertex, OnSetVertices,
            OnClearVertices, OnOpenClose);
    }

    AZ::u32 SplineCommon::OnChangeSplineType()
    {
        AZ::u32 ret = AZ::Edit::PropertyRefreshLevels::None;

        if (!IsMatchingType(m_spline, m_splineType))
        {
            m_spline = CopySplinePtr(m_splineType, m_spline);
            m_spline->SetCallbacks(
                m_onAddVertex, m_onRemoveVertex, m_onUpdateVertex,
                m_onSetVertices, m_onClearVertices, m_onOpenCloseChange);

            ret = AZ::Edit::PropertyRefreshLevels::EntireTree;

            if (m_onChangeType)
            {
                m_onChangeType();
            }
        }

        return ret;
    }

    /// BehaviorContext forwarder for SplineComponentNotificationBus
    class BehaviorSplineComponentNotificationBusHandler
        : public SplineComponentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorSplineComponentNotificationBusHandler, "{05816EA4-A4F0-4FB4-A82B-D6537B215D25}", AZ::SystemAllocator, OnSplineChanged);

        void OnSplineChanged() override
        {
            Call(FN_OnSplineChanged);
        }
    };

    void SplineComponent::Reflect(AZ::ReflectContext* context)
    {
        SplineCommon::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &SplineComponent::m_splineCommon);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SplineComponentNotificationBus>("SplineComponentNotificationBus")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)->
                Handler<BehaviorSplineComponentNotificationBusHandler>();

            behaviorContext->EBus<SplineComponentRequestBus>("SplineComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                ->Attribute(AZ::Script::Attributes::Module, "shape")
                ->Event("GetSpline",
                    [](SplineComponentRequests* handler) -> const AZ::Spline&
                    {
                        return *(handler->GetSpline());
                    })
                ->Event("SetClosed", &SplineComponentRequestBus::Events::SetClosed)
                ->Event("AddVertex", &SplineComponentRequestBus::Events::AddVertex)
                ->Event("UpdateVertex", &SplineComponentRequestBus::Events::UpdateVertex)
                ->Event("InsertVertex", &SplineComponentRequestBus::Events::InsertVertex)
                ->Event("RemoveVertex", &SplineComponentRequestBus::Events::RemoveVertex)
                ->Event("ClearVertices", &SplineComponentRequestBus::Events::ClearVertices)
                ->Event("GetVertex",
                    [](SplineComponentRequests* handler, size_t index) -> AZStd::tuple<AZ::Vector3, bool>
                    {
                        AZ::Vector3 vertex(0.0f);
                        bool vertexFound = handler->GetVertex(index, vertex);
                        return AZStd::make_tuple(vertex, vertexFound);
                    })
                ->Event("GetVertexCount", &SplineComponentRequestBus::Events::Size);
        }
    }

    void SplineComponent::Activate()
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        SplineComponentRequestBus::Handler::BusConnect(GetEntityId());

        const auto splineChanged = [this]()
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
        };

        const auto vertexAdded = [this, splineChanged](size_t index)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexAdded, index);

            splineChanged();
        };

        const auto vertexRemoved = [this, splineChanged](size_t index)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexRemoved, index);

            splineChanged();
        };

        const auto vertexUpdated = [this, splineChanged](size_t index)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexUpdated, index);

            splineChanged();
        };

        const auto verticesSet = [this, splineChanged]()
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(),
                &SplineComponentNotificationBus::Events::OnVerticesSet,
                m_splineCommon.m_spline->GetVertices()
            );

            splineChanged();
        };

        const auto verticesCleared = [this, splineChanged]()
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnVerticesCleared);

            splineChanged();
        };

        const auto openCloseChanged = [this, splineChanged](const bool closed)
        {
            SplineComponentNotificationBus::Event(
                GetEntityId(), &SplineComponentNotificationBus::Events::OnOpenCloseChanged, closed);

            splineChanged();
        };

        m_splineCommon.SetCallbacks(
            vertexAdded,
            vertexRemoved,
            vertexUpdated,
            verticesSet,
            verticesCleared,
            splineChanged,
            openCloseChanged);
    }

    void SplineComponent::Deactivate()
    {
        SplineComponentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void SplineComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
    }

    AZ::SplinePtr SplineComponent::GetSpline()
    {
        return m_splineCommon.m_spline;
    }

    void SplineComponent::ChangeSplineType(SplineType splineType)
    {
        m_splineCommon.ChangeSplineType(splineType);
    }

    bool SplineComponent::UpdateVertex(size_t index, const AZ::Vector3& vertex)
    {
        return m_splineCommon.m_spline->m_vertexContainer.UpdateVertex(index, vertex);
    }

    bool SplineComponent::GetVertex(size_t index, AZ::Vector3& vertex) const
    {
        return m_splineCommon.m_spline->m_vertexContainer.GetVertex(index, vertex);
    }

    void SplineComponent::AddVertex(const AZ::Vector3& vertex)
    {
        m_splineCommon.m_spline->m_vertexContainer.AddVertex(vertex);
    }

    bool SplineComponent::InsertVertex(size_t index, const AZ::Vector3& vertex)
    {
        return m_splineCommon.m_spline->m_vertexContainer.InsertVertex(index, vertex);
    }

    bool SplineComponent::RemoveVertex(size_t index)
    {
        return m_splineCommon.m_spline->m_vertexContainer.RemoveVertex(index);
    }

    void SplineComponent::SetVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_splineCommon.m_spline->m_vertexContainer.SetVertices(vertices);
    }

    void SplineComponent::ClearVertices()
    {
        m_splineCommon.m_spline->m_vertexContainer.Clear();
    }

    bool SplineComponent::Empty() const
    {
        return m_splineCommon.m_spline->m_vertexContainer.Empty();
    }

    size_t SplineComponent::Size() const
    {
        return m_splineCommon.m_spline->m_vertexContainer.Size();
    }

    void SplineComponent::SetClosed(const bool closed)
    {
        // set closed callback calls OnSplineChanged
        m_splineCommon.m_spline->SetClosed(closed);
    }
} // namespace LmbrCentral
