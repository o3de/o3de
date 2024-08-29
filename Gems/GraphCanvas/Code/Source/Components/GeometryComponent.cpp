/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QGraphicsSceneMouseEvent>

#include <AzCore/Serialization/EditContext.h>

#include <Components/GeometryComponent.h>

#include <Components/Nodes/NodeComponent.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

namespace GraphCanvas
{
    //////////////////////
    // GeometryComponent
    //////////////////////
    const float GeometryComponent::IS_CLOSE_TOLERANCE = 0.001f;

    bool GeometryComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 3)
        {
            AZ::Crc32 positionId = AZ_CRC_CE("Position");

            GeometrySaveData saveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(positionId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_position);
            }
            
            classElement.RemoveElementByName(positionId);
            classElement.AddElementWithData(context, "SaveData", saveData);
        }

        return true;
    }

    void GeometryComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<GeometrySaveData>()
            ->Version(1)
            ->Field("Position", &GeometrySaveData::m_position)
        ;

        serializeContext->Class<GeometryComponent, AZ::Component>()
            ->Version(4, &GeometryComponentVersionConverter)
            ->Field("SaveData", &GeometryComponent::m_saveData)
        ;
    }

    GeometryComponent::GeometryComponent()
        : m_animating(false)
    {
    }

    GeometryComponent::~GeometryComponent()
    {
        GeometryRequestBus::Handler::BusDisconnect();
    }

    void GeometryComponent::Init()
    {
        GeometryRequestBus::Handler::BusConnect(GetEntityId());
        EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
    }

    void GeometryComponent::Activate()
    {
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());        
    }

    void GeometryComponent::Deactivate()
    {
        VisualNotificationBus::Handler::BusDisconnect();
        SceneMemberNotificationBus::Handler::BusDisconnect();
    }

    void GeometryComponent::OnSceneSet(const AZ::EntityId& scene)
    {
        VisualNotificationBus::Handler::BusConnect(GetEntityId());

        m_saveData.RegisterIds(GetEntityId(), scene);
    }

    AZ::Vector2 GeometryComponent::GetPosition() const
    {
        return m_saveData.m_position;
    }

    void GeometryComponent::SetPosition(const AZ::Vector2& position)
    {
        if (!position.IsClose(m_saveData.m_position)
            && (!IsAnimating() || !m_animatingPosition.IsClose(position)))
        {
            if (!IsAnimating())
            {
                m_saveData.m_position = position;
            }
            else
            {
                m_animatingPosition = position;
            }

            GeometryNotificationBus::Event(GetEntityId(), &GeometryNotifications::OnPositionChanged, GetEntityId(), position);

            if (!IsAnimating())
            {
                m_saveData.SignalDirty();
            }
        }
    }

    void GeometryComponent::SignalBoundsChanged()
    {
        GeometryNotificationBus::Event(GetEntityId(), &GeometryNotifications::OnBoundsChanged);
    }

    void GeometryComponent::SetIsPositionAnimating(bool animating)
    {
        if (m_animating != animating)
        {
            m_animating = animating;

            if (m_animating)
            {
                // Store the animating position separate from the savedata position
                // so any attempts to save will cause appropriate data to be saved
                // while visually I can animate cleanly between the values.
                m_animatingPosition = m_saveData.m_position;
            }
            else
            {
                AZ::Vector2 forcedPosition = m_saveData.m_position;

                // Force the alignment to wherever we were aiming at.
                ForceSetPosition(forcedPosition);
            }
        }
    }

    void GeometryComponent::SetAnimationTarget(const AZ::Vector2& targetPoint)
    {
        m_saveData.m_position = targetPoint;
        m_saveData.SignalDirty();
    }

    void GeometryComponent::OnItemChange([[maybe_unused]] const AZ::EntityId& entityId, QGraphicsItem::GraphicsItemChange change, const QVariant& value)
    {
        AZ_Assert(entityId == GetEntityId(), "EIDs should match");

        switch (change)
        {
        case QGraphicsItem::ItemPositionChange:
        {
            QPointF qt = value.toPointF();
            SetPosition(ConversionUtils::QPointToVector(qt));

            break;
        }
        default:
            break;
        }
    }

    void GeometryComponent::WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
    {
        GeometrySaveData* saveData = saveDataContainer.FindCreateSaveData<GeometrySaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void GeometryComponent::ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
    {
        GeometrySaveData* saveData = saveDataContainer.FindSaveDataAs<GeometrySaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
        }
    }

    void GeometryComponent::ForceSetPosition(const AZ::Vector2& forcedPosition)
    {
        if (forcedPosition.IsZero())
        {
            m_saveData.m_position = AZ::Vector2(1, 1);
        }
        else
        {
            m_saveData.m_position = AZ::Vector2::CreateZero();
        }

        SetPosition(forcedPosition);
    }

    bool GeometryComponent::IsAnimating() const
    {
        return m_animating;
    }
}
