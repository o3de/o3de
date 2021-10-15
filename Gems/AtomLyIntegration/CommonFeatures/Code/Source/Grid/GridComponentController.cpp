/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Grid/GridComponentController.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConstants.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/Utils/Utils.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>

namespace AZ
{
    namespace Render
    {
        void GridComponentController::Reflect(ReflectContext* context)
        {
            GridComponentConfig::Reflect(context);

            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<GridComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &GridComponentController::m_configuration)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<GridComponentRequestBus>("GridComponentRequestBus")
                    ->Event("GetSize", &GridComponentRequestBus::Events::GetSize)
                    ->Event("SetSize", &GridComponentRequestBus::Events::SetSize)
                    ->Event("GetAxisColor", &GridComponentRequestBus::Events::GetAxisColor)
                    ->Event("SetAxisColor", &GridComponentRequestBus::Events::SetAxisColor)
                    ->Event("GetPrimaryColor", &GridComponentRequestBus::Events::GetPrimaryColor)
                    ->Event("SetPrimaryColor", &GridComponentRequestBus::Events::SetPrimaryColor)
                    ->Event("GetPrimarySpacing", &GridComponentRequestBus::Events::GetPrimarySpacing)
                    ->Event("SetPrimarySpacing", &GridComponentRequestBus::Events::SetPrimarySpacing)
                    ->Event("GetSecondaryColor", &GridComponentRequestBus::Events::GetSecondaryColor)
                    ->Event("SetSecondaryColor", &GridComponentRequestBus::Events::SetSecondaryColor)
                    ->Event("GetSecondarySpacing", &GridComponentRequestBus::Events::GetSecondarySpacing)
                    ->Event("SetSecondarySpacing", &GridComponentRequestBus::Events::SetSecondarySpacing)
                    ->VirtualProperty("Size", "GetSize", "SetSize")
                    ->VirtualProperty("AxisColor", "GetAxisColor", "SetAxisColor")
                    ->VirtualProperty("PrimaryColor", "GetPrimaryColor", "SetPrimaryColor")
                    ->VirtualProperty("PrimarySpacing", "GetPrimarySpacing", "SetPrimarySpacing")
                    ->VirtualProperty("SecondaryColor", "GetSecondaryColor", "SetSecondaryColor")
                    ->VirtualProperty("SecondarySpacing", "GetSecondarySpacing", "SetSecondarySpacing")
                    ;
            }
        }

        void GridComponentController::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GridService", 0x3844bbe0));
        }

        void GridComponentController::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("GridService", 0x3844bbe0));
        }

        GridComponentController::GridComponentController(const GridComponentConfig& config)
            : m_configuration(config)
        {
        }

        void GridComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
            m_dirty = true;

            RPI::ScenePtr scene = RPI::RPISystemInterface::Get()->GetDefaultScene();
            if (scene)
            {
                AZ::RPI::SceneNotificationBus::Handler::BusConnect(scene->GetId());
            }

            GridComponentRequestBus::Handler::BusConnect(m_entityId);
            AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        }

        void GridComponentController::Deactivate()
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            GridComponentRequestBus::Handler::BusDisconnect();
            AZ::RPI::SceneNotificationBus::Handler::BusDisconnect();

            m_entityId = EntityId(EntityId::InvalidEntityId);
        }

        void GridComponentController::SetConfiguration(const GridComponentConfig& config)
        {
            m_configuration = config;
            m_dirty = true;
        }

        const GridComponentConfig& GridComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void GridComponentController::SetSize(float gridSize)
        {
            m_configuration.m_gridSize = AZStd::clamp(gridSize, MinGridSize, MaxGridSize);
            m_dirty = true;
        }

        float GridComponentController::GetSize() const
        {
            return m_configuration.m_gridSize;
        }

        void GridComponentController::SetPrimarySpacing(float gridPrimarySpacing)
        {
            m_configuration.m_primarySpacing = AZStd::max(gridPrimarySpacing, MinSpacing);
            m_dirty = true;
        }

        float GridComponentController::GetPrimarySpacing() const
        {
            return m_configuration.m_primarySpacing;
        }

        void GridComponentController::SetSecondarySpacing(float gridSecondarySpacing)
        {
            m_configuration.m_secondarySpacing = AZStd::max(gridSecondarySpacing, MinSpacing);
            m_dirty = true;
        }

        float GridComponentController::GetSecondarySpacing() const
        {
            return m_configuration.m_secondarySpacing;
        }

        void GridComponentController::SetAxisColor(const AZ::Color& gridAxisColor)
        {
            m_configuration.m_axisColor = gridAxisColor;
        }

        AZ::Color GridComponentController::GetAxisColor() const
        {
            return m_configuration.m_axisColor;
        }

        void GridComponentController::SetPrimaryColor(const AZ::Color& gridPrimaryColor)
        {
            m_configuration.m_primaryColor = gridPrimaryColor;
        }

        AZ::Color GridComponentController::GetPrimaryColor() const
        {
            return m_configuration.m_primaryColor;
        }

        void GridComponentController::SetSecondaryColor(const AZ::Color& gridSecondaryColor)
        {
            m_configuration.m_secondaryColor = gridSecondaryColor;
        }

        AZ::Color GridComponentController::GetSecondaryColor() const
        {
            return m_configuration.m_secondaryColor;
        }

        void GridComponentController::OnBeginPrepareRender()
        {
            auto* auxGeomFP = AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::RPI::AuxGeomFeatureProcessorInterface>(m_entityId);
            if (!auxGeomFP)
            {
                return;
            }
            if (auto auxGeom = auxGeomFP->GetDrawQueue())
            {
                BuildGrid();

                AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
                drawArgs.m_verts = m_secondaryGridPoints.data();
                drawArgs.m_vertCount = aznumeric_cast<uint32_t>(m_secondaryGridPoints.size());        
                drawArgs.m_colors = &m_configuration.m_secondaryColor;
                drawArgs.m_colorCount = 1;        
                auxGeom->DrawLines(drawArgs);

                drawArgs.m_verts = m_primaryGridPoints.data();
                drawArgs.m_vertCount = aznumeric_cast<uint32_t>(m_primaryGridPoints.size());        
                drawArgs.m_colors = &m_configuration.m_primaryColor;
                auxGeom->DrawLines(drawArgs);

                drawArgs.m_verts = m_axisGridPoints.data();
                drawArgs.m_vertCount = aznumeric_cast<uint32_t>(m_axisGridPoints.size());        
                drawArgs.m_colors = &m_configuration.m_axisColor;
                auxGeom->DrawLines(drawArgs);
            }
        }

        void GridComponentController::OnTransformChanged(const Transform& local, const Transform& world)
        {
            AZ_UNUSED(local);
            AZ_UNUSED(world);
            m_dirty = true;
        }

        void GridComponentController::BuildGrid()
        {
            if (m_dirty)
            {
                m_dirty = false;

                AZ::Transform transform;
                AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

                const float halfLength = m_configuration.m_gridSize / 2.0f;

                m_axisGridPoints.clear();
                m_axisGridPoints.reserve(4);
                m_axisGridPoints.push_back(transform.TransformPoint(AZ::Vector3(-halfLength, 0, 0.0f)));
                m_axisGridPoints.push_back(transform.TransformPoint(AZ::Vector3(halfLength, 0, 0.0f)));
                m_axisGridPoints.push_back(transform.TransformPoint(AZ::Vector3(0, -halfLength, 0.0f)));
                m_axisGridPoints.push_back(transform.TransformPoint(AZ::Vector3(0, halfLength, 0.0f)));

                m_primaryGridPoints.clear();
                m_primaryGridPoints.reserve(aznumeric_cast<size_t>(4.0f * m_configuration.m_gridSize / m_configuration.m_primarySpacing));
                for (float position = m_configuration.m_primarySpacing; position <= halfLength; position += m_configuration.m_primarySpacing)
                {
                    m_primaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(-halfLength, -position, 0.0f)));
                    m_primaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(halfLength, -position, 0.0f)));
                    m_primaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(-halfLength, position, 0.0f)));
                    m_primaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(halfLength, position, 0.0f)));
                    m_primaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(-position, -halfLength, 0.0f)));
                    m_primaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(-position, halfLength, 0.0f)));
                    m_primaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(position, -halfLength, 0.0f)));
                    m_primaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(position, halfLength, 0.0f)));
                }

                m_secondaryGridPoints.clear();
                m_secondaryGridPoints.reserve(aznumeric_cast<size_t>(4.0f * m_configuration.m_gridSize / m_configuration.m_secondarySpacing));
                for (float position = m_configuration.m_secondarySpacing; position <= halfLength; position += m_configuration.m_secondarySpacing)
                {
                    m_secondaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(-halfLength, -position, 0.0f)));
                    m_secondaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(halfLength, -position, 0.0f)));
                    m_secondaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(-halfLength, position, 0.0f)));
                    m_secondaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(halfLength, position, 0.0f)));
                    m_secondaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(-position, -halfLength, 0.0f)));
                    m_secondaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(-position, halfLength, 0.0f)));
                    m_secondaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(position, -halfLength, 0.0f)));
                    m_secondaryGridPoints.push_back(transform.TransformPoint(AZ::Vector3(position, halfLength, 0.0f)));
                }

                GridComponentNotificationBus::Event(m_entityId, &GridComponentNotificationBus::Events::OnGridChanged);
            }
        }
    } // namespace Render
} // namespace AZ
