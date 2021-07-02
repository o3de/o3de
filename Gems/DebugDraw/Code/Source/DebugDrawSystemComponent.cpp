/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DebugDraw_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/parallel/lock.h>

#include "DebugDrawSystemComponent.h"

// Editor specific
#ifdef DEBUGDRAW_GEM_EDITOR
#include "EditorDebugDrawComponentCommon.h" // for Reflection
#include "EditorDebugDrawLineComponent.h"
#include "EditorDebugDrawRayComponent.h"
#include "EditorDebugDrawSphereComponent.h"
#include "EditorDebugDrawObbComponent.h"
#include "EditorDebugDrawTextComponent.h"
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#endif // DEBUGDRAW_GEM_EDITOR

#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>

namespace DebugDraw
{
    void DebugDrawSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        #ifdef DEBUGDRAW_GEM_EDITOR
        EditorDebugDrawComponentSettings::Reflect(context);
        #endif // DEBUGDRAW_GEM_EDITOR

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DebugDrawSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<DebugDrawSystemComponent>("DebugDraw", "Provides game runtime debug visualization.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<DebugDrawRequestBus>("DebugDrawRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Debug")
                ->Event("DrawAabb", &DebugDrawRequestBus::Events::DrawAabb)
                ->Event("DrawAabbOnEntity", &DebugDrawRequestBus::Events::DrawAabbOnEntity)
                ->Event("DrawLineLocationToLocation", &DebugDrawRequestBus::Events::DrawLineLocationToLocation)
                ->Event("DrawLineEntityToLocation", &DebugDrawRequestBus::Events::DrawLineEntityToLocation)
                ->Event("DrawLineEntityToEntity", &DebugDrawRequestBus::Events::DrawLineEntityToEntity)
                ->Event("DrawObb", &DebugDrawRequestBus::Events::DrawObb)
                ->Event("DrawObbOnEntity", &DebugDrawRequestBus::Events::DrawObbOnEntity)
                ->Event("DrawRayLocationToDirection", &DebugDrawRequestBus::Events::DrawRayLocationToDirection)
                ->Event("DrawRayEntityToDirection", &DebugDrawRequestBus::Events::DrawRayEntityToDirection)
                ->Event("DrawRayEntityToEntity", &DebugDrawRequestBus::Events::DrawRayEntityToEntity)
                ->Event("DrawSphereAtLocation", &DebugDrawRequestBus::Events::DrawSphereAtLocation)
                ->Event("DrawSphereOnEntity", &DebugDrawRequestBus::Events::DrawSphereOnEntity)
                ->Event("DrawTextAtLocation", &DebugDrawRequestBus::Events::DrawTextAtLocation)
                ->Event("DrawTextOnEntity", &DebugDrawRequestBus::Events::DrawTextOnEntity)
                ->Event("DrawTextOnScreen", &DebugDrawRequestBus::Events::DrawTextOnScreen)
            ;
        }
    }

    void DebugDrawSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("DebugDrawService", 0x651d8874));
    }

    void DebugDrawSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("DebugDrawService", 0x651d8874));
    }

    void DebugDrawSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("RPISystem", 0xf2add773));
    }

    void DebugDrawSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void DebugDrawSystemComponent::Init()
    {
    }

    void DebugDrawSystemComponent::Activate()
    {
        DebugDrawInternalRequestBus::Handler::BusConnect();
        DebugDrawRequestBus::Handler::BusConnect();
        AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();

        #ifdef DEBUGDRAW_GEM_EDITOR
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        #endif // DEBUGDRAW_GEM_EDITOR
    }

    void DebugDrawSystemComponent::Deactivate()
    {
        #ifdef DEBUGDRAW_GEM_EDITOR
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        #endif // DEBUGDRAW_GEM_EDITOR

        AZ::RPI::SceneNotificationBus::Handler::BusDisconnect();
        DebugDrawRequestBus::Handler::BusDisconnect();
        DebugDrawInternalRequestBus::Handler::BusDisconnect();

        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeAabbsMutex);
            m_activeAabbs.clear();
        }
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
            m_activeLines.clear();
        }
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);
            m_activeObbs.clear();
        }
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
            m_activeRays.clear();
        }
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
            m_activeSpheres.clear();
        }
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
            m_activeTexts.clear();
        }
    }

    void DebugDrawSystemComponent::OnBootstrapSceneReady(AZ::RPI::Scene* scene)
    {
        AZ_Assert(scene, "Invalid scene received in OnBootstrapSceneReady");
        AZ::RPI::SceneNotificationBus::Handler::BusConnect(scene->GetId());
        AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();
    }

    #ifdef DEBUGDRAW_GEM_EDITOR
    void DebugDrawSystemComponent::OnStopPlayInEditor()
    {
        // Remove all debug elements that weren't triggered by editor components
        // We need this check because OnStopPlayInEditor() is called AFTER editor entities
        // have been re-activated, so at this time we have both our game AND editor
        // debug drawings active

        // Aabbs
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeAabbsMutex);
            AZStd::vector<DebugDrawAabbElement> elementsToSave;
            for (const DebugDrawAabbElement& element : m_activeAabbs)
            {
                if (element.m_owningEditorComponent != AZ::InvalidComponentId)
                {
                    elementsToSave.push_back(element);
                }
            }
            m_activeAabbs.clear();
            m_activeAabbs.assign_rv(AZStd::forward<AZStd::vector<DebugDrawAabbElement>>(elementsToSave));
        }

        // Lines
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
            AZStd::vector<DebugDrawLineElement> elementsToSave;
            for (const DebugDrawLineElement& element : m_activeLines)
            {
                if (element.m_owningEditorComponent != AZ::InvalidComponentId)
                {
                    elementsToSave.push_back(element);
                }
            }
            m_activeLines.clear();
            m_activeLines.assign_rv(AZStd::forward<AZStd::vector<DebugDrawLineElement>>(elementsToSave));
        }

        // Obbs
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);
            AZStd::vector<DebugDrawObbElement> elementsToSave;
            for (const DebugDrawObbElement& element : m_activeObbs)
            {
                if (element.m_owningEditorComponent != AZ::InvalidComponentId)
                {
                    elementsToSave.push_back(element);
                }
            }
            m_activeObbs.clear();
            m_activeObbs.assign_rv(AZStd::forward<AZStd::vector<DebugDrawObbElement>>(elementsToSave));
        }

        // Rays
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
            AZStd::vector<DebugDrawRayElement> elementsToSave;
            for (const DebugDrawRayElement& element : m_activeRays)
            {
                if (element.m_owningEditorComponent != AZ::InvalidComponentId)
                {
                    elementsToSave.push_back(element);
                }
            }
            m_activeRays.clear();
            m_activeRays.assign_rv(AZStd::forward<AZStd::vector<DebugDrawRayElement>>(elementsToSave));
        }

        // Spheres
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
            AZStd::vector<DebugDrawSphereElement> elementsToSave;
            for (const DebugDrawSphereElement& element : m_activeSpheres)
            {
                if (element.m_owningEditorComponent != AZ::InvalidComponentId)
                {
                    elementsToSave.push_back(element);
                }
            }
            m_activeSpheres.clear();
            m_activeSpheres.assign_rv(AZStd::forward<AZStd::vector<DebugDrawSphereElement>>(elementsToSave));
        }

        // Text
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
            AZStd::vector<DebugDrawTextElement> elementsToSave;
            for (const DebugDrawTextElement& element : m_activeTexts)
            {
                if (element.m_owningEditorComponent != AZ::InvalidComponentId)
                {
                    elementsToSave.push_back(element);
                }
            }
            m_activeTexts.clear();
            m_activeTexts.assign_rv(AZStd::forward<AZStd::vector<DebugDrawTextElement>>(elementsToSave));
        }
    }
    #endif // DEBUGDRAW_GEM_EDITOR

    void DebugDrawSystemComponent::OnBeginPrepareRender()
    {
        AZ::ScriptTimePoint time;
        AZ::TickRequestBus::BroadcastResult(time, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
        m_currentTime = time.GetSeconds();

        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(
            debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
        AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");

        AzFramework::DebugDisplayRequests* debugDisplay =
            AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

        if (debugDisplay)
        {
            OnTickAabbs(*debugDisplay);
            OnTickLines(*debugDisplay);
            OnTickObbs(*debugDisplay);
            OnTickRays(*debugDisplay);
            OnTickSpheres(*debugDisplay);
            OnTickText(*debugDisplay);
        }
    }

    template <typename F>
    void DebugDrawSystemComponent::removeExpiredDebugElementsFromVector(AZStd::vector<F>& vectorToExpire)
    {
        auto removalCondition = std::remove_if(std::begin(vectorToExpire), std::end(vectorToExpire), [this](F& element)
        {
            return element.m_duration == 0.0f || (element.m_duration > 0.0f && (element.m_activateTime.GetSeconds() + element.m_duration <= m_currentTime));
        });
        vectorToExpire.erase(removalCondition, std::end(vectorToExpire));
    }

    void DebugDrawSystemComponent::OnTickAabbs(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeAabbsMutex);

        // Draw Aabb elements and remove any that are expired
        for (auto& aabbElement : m_activeAabbs)
        {
            AZ::Aabb transformedAabb(aabbElement.m_aabb);

            // Query for entity location if this Aabb is attached to an entity
            if (aabbElement.m_targetEntityId.IsValid())
            {
                AZ::TransformBus::EventResult(aabbElement.m_worldLocation, aabbElement.m_targetEntityId, &AZ::TransformBus::Events::GetWorldTranslation);

                // Re-center
                AZ::Vector3 currentCenter = transformedAabb.GetCenter();
                transformedAabb.Set(transformedAabb.GetMin() - currentCenter + aabbElement.m_worldLocation, transformedAabb.GetMax() - currentCenter + aabbElement.m_worldLocation);
            }
            debugDisplay.SetColor(aabbElement.m_color);
            debugDisplay.DrawSolidBox(transformedAabb.GetMin(), transformedAabb.GetMax());
        }

        removeExpiredDebugElementsFromVector(m_activeAabbs);
    }

    void DebugDrawSystemComponent::OnTickLines(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
        size_t numActiveLines = m_activeLines.size();

        m_batchPoints.clear();
        m_batchColors.clear();

        m_batchPoints.reserve(numActiveLines * 2);
        m_batchColors.reserve(numActiveLines * 2);

        // Draw line elements and remove any that are expired
        for (auto& lineElement : m_activeLines)
        {
            // Query for entity locations if this line starts or ends at valid entities.
            // Nice thing with this setup where we're using the lineElement's locations to query is that
            // when one of the entities gets destroyed, we'll keep drawing to its last known location (if
            // that entity deactivation didn't result in the line no longer being drawn)
            if (lineElement.m_startEntityId.IsValid())
            {
                AZ::TransformBus::EventResult(
                    lineElement.m_startWorldLocation,
                    lineElement.m_startEntityId,
                    &AZ::TransformBus::Events::GetWorldTranslation);
            }

            if (lineElement.m_endEntityId.IsValid())
            {
                AZ::TransformBus::EventResult(
                    lineElement.m_endWorldLocation,
                    lineElement.m_endEntityId,
                    &AZ::TransformBus::Events::GetWorldTranslation);
            }

            debugDisplay.SetColor(lineElement.m_color);
            debugDisplay.DrawLine(lineElement.m_startWorldLocation, lineElement.m_endWorldLocation);
        }

        removeExpiredDebugElementsFromVector(m_activeLines);
    }

    void DebugDrawSystemComponent::OnTickObbs(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);

        // Draw Obb elements and remove any that are expired
        for (auto& obbElement : m_activeObbs)
        {
            AZ::Obb transformedObb = obbElement.m_obb;

            // Entity-attached Obbs get positioned and rotated according to entity transform
            if (obbElement.m_targetEntityId.IsValid())
            {
                AZ::Transform entityTM;
                AZ::TransformBus::EventResult(entityTM, obbElement.m_targetEntityId, &AZ::TransformBus::Events::GetWorldTM);
                obbElement.m_worldLocation = entityTM.GetTranslation();
                transformedObb.SetPosition(AZ::Vector3::CreateZero());
                transformedObb = entityTM * transformedObb;

                //set half lengths based on editor values
                for (unsigned i = 0; i <= 2; ++i)
                {
                    transformedObb.SetHalfLength(i, obbElement.m_scale.GetElement(i));
                }
            }
            else
            {
                obbElement.m_worldLocation = transformedObb.GetPosition();
            }
            debugDisplay.SetColor(obbElement.m_color);
            debugDisplay.DrawSolidOBB(obbElement.m_worldLocation, transformedObb.GetAxisX(), transformedObb.GetAxisY(), transformedObb.GetAxisZ(), transformedObb.GetHalfLengths());
        }

        removeExpiredDebugElementsFromVector(m_activeObbs);
    }

    void DebugDrawSystemComponent::OnTickRays(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);

        // Draw ray elements and remove any that are expired
        for (auto& rayElement : m_activeRays)
        {
            // Query for entity locations if this ray starts or ends at valid entities.
            if (rayElement.m_startEntityId.IsValid())
            {
                AZ::TransformBus::EventResult(rayElement.m_worldLocation, rayElement.m_startEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
            }

            AZ::Vector3 endWorldLocation(rayElement.m_worldLocation + rayElement.m_worldDirection);
            if (rayElement.m_endEntityId.IsValid())
            {
                AZ::TransformBus::EventResult(endWorldLocation, rayElement.m_endEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
                rayElement.m_worldDirection = (endWorldLocation - rayElement.m_worldLocation);
            }

            float conePercentHeight = 0.5f;
            float coneHeight = rayElement.m_worldDirection.GetLength() * conePercentHeight;
            AZ::Vector3 coneBaseLocation = endWorldLocation - rayElement.m_worldDirection * conePercentHeight;
            float coneRadius = AZ::GetClamp(coneHeight * 0.07f, 0.05f, 0.2f);
            debugDisplay.SetColor(rayElement.m_color);
            debugDisplay.SetLineWidth(5.0f);
            debugDisplay.DrawLine(rayElement.m_worldLocation, coneBaseLocation);
            debugDisplay.DrawSolidCone(coneBaseLocation, rayElement.m_worldDirection, coneRadius, coneHeight, false);
        }

        removeExpiredDebugElementsFromVector(m_activeRays);
    }

    void DebugDrawSystemComponent::OnTickSpheres(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);

        // Draw sphere elements and remove any that are expired
        for (auto& sphereElement : m_activeSpheres)
        {
            // Query for entity location if this sphere is attached to an entity
            if (sphereElement.m_targetEntityId.IsValid())
            {
                AZ::TransformBus::EventResult(sphereElement.m_worldLocation, sphereElement.m_targetEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
            }
            debugDisplay.SetColor(sphereElement.m_color);
            debugDisplay.DrawBall(sphereElement.m_worldLocation, sphereElement.m_radius, true);
        }

        removeExpiredDebugElementsFromVector(m_activeSpheres);
    }

    void DebugDrawSystemComponent::OnTickText(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);

        // Determine if we need gamma conversion
        bool needsGammaConversion = false;

        #ifdef DEBUGDRAW_GEM_EDITOR
        bool isInGameMode = true;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(isInGameMode, &AzToolsFramework::EditorEntityContextRequestBus::Events::IsEditorRunningGame);
        if (isInGameMode)
        {
            needsGammaConversion = true;
        }
        #endif // DEBUGDRAW_GEM_EDITOR

        // Draw text elements and remove any that are expired
        int numScreenTexts = 0;
        AZ::EntityId lastTargetEntityId;

        for (auto& textElement : m_activeTexts)
        {
            const AZ::Color textColor = needsGammaConversion ? textElement.m_color.GammaToLinear() : textElement.m_color;
            debugDisplay.SetColor(textColor);
            if (textElement.m_drawMode == DebugDrawTextElement::DrawMode::OnScreen)
            {
                debugDisplay.Draw2dTextLabel(100.0f, 20.f + ((float)numScreenTexts * 15.0f), 1.4f, textElement.m_text.c_str() );
                ++numScreenTexts;
            }
            else if (textElement.m_drawMode == DebugDrawTextElement::DrawMode::InWorld)
            {
                AZ::Vector3 worldLocation;
                if (textElement.m_targetEntityId.IsValid())
                {
                    // Entity text
                    AZ::TransformBus::EventResult(worldLocation, textElement.m_targetEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
                }
                else
                {
                    // World text
                    worldLocation = textElement.m_worldLocation;
                }

                debugDisplay.DrawTextLabel(worldLocation, 1.4f, textElement.m_text.c_str() );
            }
        }

        removeExpiredDebugElementsFromVector(m_activeTexts);
    }

    void DebugDrawSystemComponent::RegisterDebugDrawComponent(AZ::Component* component)
    {
        AZ_Assert(component != nullptr, "Null component being registered!");

        AZ::EntityBus::MultiHandler::BusConnect(component->GetEntityId());

        if (DebugDrawLineComponent* lineComponent = azrtti_cast<DebugDrawLineComponent*>(component))
        {
            CreateLineEntryForComponent(lineComponent->GetEntityId(), lineComponent->m_element);
        }
        #ifdef DEBUGDRAW_GEM_EDITOR
        else if (EditorDebugDrawLineComponent* editorLineComponent = azrtti_cast<EditorDebugDrawLineComponent*>(component))
        {
            CreateLineEntryForComponent(editorLineComponent->GetEntityId(), editorLineComponent->m_element);
        }
        #endif // DEBUGDRAW_GEM_EDITOR
        else if (DebugDrawRayComponent* rayComponent = azrtti_cast<DebugDrawRayComponent*>(component))
        {
            CreateRayEntryForComponent(rayComponent->GetEntityId(), rayComponent->m_element);
        }
        #ifdef DEBUGDRAW_GEM_EDITOR
        else if (EditorDebugDrawRayComponent* editorRayComponent = azrtti_cast<EditorDebugDrawRayComponent*>(component))
        {
            CreateRayEntryForComponent(editorRayComponent->GetEntityId(), editorRayComponent->m_element);
        }
        #endif // DEBUGDRAW_GEM_EDITOR
        else if (DebugDrawSphereComponent* sphereComponent = azrtti_cast<DebugDrawSphereComponent*>(component))
        {
            CreateSphereEntryForComponent(sphereComponent->GetEntityId(), sphereComponent->m_element);
        }
        #ifdef DEBUGDRAW_GEM_EDITOR
        else if (EditorDebugDrawSphereComponent* editorSphereComponent = azrtti_cast<EditorDebugDrawSphereComponent*>(component))
        {
            CreateSphereEntryForComponent(editorSphereComponent->GetEntityId(), editorSphereComponent->m_element);
        }
        #endif // DEBUGDRAW_GEM_EDITOR
        else if (DebugDrawObbComponent* obbComponent = azrtti_cast<DebugDrawObbComponent*>(component))
        {
            CreateObbEntryForComponent(obbComponent->GetEntityId(), obbComponent->m_element);
        }

        #ifdef DEBUGDRAW_GEM_EDITOR
        else if (EditorDebugDrawObbComponent* editorObbComponent = azrtti_cast<EditorDebugDrawObbComponent*>(component))
        {
            CreateObbEntryForComponent(editorObbComponent->GetEntityId(), editorObbComponent->m_element);
        }
        #endif // DEBUGDRAW_GEM_EDITOR

        else if (DebugDrawTextComponent* textComponent = azrtti_cast<DebugDrawTextComponent*>(component))
        {
            CreateTextEntryForComponent(textComponent->GetEntityId(), textComponent->m_element);
        }

        #ifdef DEBUGDRAW_GEM_EDITOR
        else if (EditorDebugDrawTextComponent* editorTextComponent = azrtti_cast<EditorDebugDrawTextComponent*>(component))
        {
            CreateTextEntryForComponent(editorTextComponent->GetEntityId(), editorTextComponent->m_element);
        }
        #endif // DEBUGDRAW_GEM_EDITOR
    }

    void DebugDrawSystemComponent::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);

        // Remove all associated entity-based debug elements for this entity

        // Lines
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
            for (auto iter = m_activeLines.begin(); iter != m_activeLines.end();)
            {
                DebugDrawLineElement& element = *iter;
                if (element.m_startEntityId == entityId)
                {
                    m_activeLines.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

        // Rays
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
            for (auto iter = m_activeRays.begin(); iter != m_activeRays.end();)
            {
                DebugDrawRayElement& element = *iter;
                if (element.m_startEntityId == entityId)
                {
                    m_activeRays.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

        // Obbs
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);

            for (auto iter = m_activeObbs.begin(); iter != m_activeObbs.end();)
            {
                DebugDrawObbElement& element = *iter;
                if (element.m_targetEntityId == entityId)
                {
                    m_activeObbs.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

        // Spheres
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
            for (auto iter = m_activeSpheres.begin(); iter != m_activeSpheres.end();)
            {
                DebugDrawSphereElement& element = *iter;
                if (element.m_targetEntityId == entityId)
                {
                    m_activeSpheres.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

        // Text
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
            for (auto iter = m_activeTexts.begin(); iter != m_activeTexts.end();)
            {
                DebugDrawTextElement& element = *iter;
                if (element.m_targetEntityId == entityId)
                {
                    m_activeTexts.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }
    }

    void DebugDrawSystemComponent::UnregisterDebugDrawComponent(AZ::Component* component)
    {
        const AZ::EntityId componentEntityId = component->GetEntityId();
        const AZ::ComponentId componentId = component->GetId();

        // Remove specific associated entity-based debug element for this entity/component combo

        // Lines
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
            for (auto iter = m_activeLines.begin(); iter != m_activeLines.end();)
            {
                DebugDrawLineElement& element = *iter;
                if (element.m_startEntityId == componentEntityId && element.m_owningEditorComponent == componentId)
                {
                    m_activeLines.erase(iter);
                    break; // Only one element per component
                }
                else
                {
                    ++iter;
                }
            }
        }

        // Rays
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
            for (auto iter = m_activeRays.begin(); iter != m_activeRays.end();)
            {
                DebugDrawRayElement& element = *iter;
                if (element.m_startEntityId == componentEntityId && element.m_owningEditorComponent == componentId)
                {
                    m_activeRays.erase(iter);
                    break; // Only one element per component
                }
                else
                {
                    ++iter;
                }
            }
        }

        // Obbs
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);
            for (auto iter = m_activeObbs.begin(); iter != m_activeObbs.end();)
            {
                DebugDrawObbElement& element = *iter;
                if (element.m_targetEntityId == componentEntityId && element.m_owningEditorComponent == componentId)
                {
                    m_activeObbs.erase(iter);
                    break; // Only one element per component
                }
                else
                {
                    ++iter;
                }
            }
        }

        // Spheres
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
            for (auto iter = m_activeSpheres.begin(); iter != m_activeSpheres.end();)
            {
                DebugDrawSphereElement& element = *iter;
                if (element.m_targetEntityId == componentEntityId && element.m_owningEditorComponent == componentId)
                {
                    m_activeSpheres.erase(iter);
                    break; // Only one element per component
                }
                else
                {
                    ++iter;
                }
            }
        }

        // Text
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
            for (auto iter = m_activeTexts.begin(); iter != m_activeTexts.end();)
            {
                DebugDrawTextElement& element = *iter;
                if (element.m_targetEntityId == componentEntityId && element.m_owningEditorComponent == componentId)
                {
                    m_activeTexts.erase(iter);
                    break; // Only one element per component
                }
                else
                {
                    ++iter;
                }
            }
        }
    }


    ///////////////////////////////////////////////////////////////////////
    // Aabbs
    ///////////////////////////////////////////////////////////////////////

    void DebugDrawSystemComponent::DrawAabb(const AZ::Aabb& aabb, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeAabbsMutex);
        m_activeAabbs.push_back();
        DebugDrawAabbElement& newElement = m_activeAabbs.back();
        newElement.m_aabb = aabb;
        newElement.m_color = color;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawAabbOnEntity(const AZ::EntityId& targetEntity, const AZ::Aabb& aabb, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeAabbsMutex);
        m_activeAabbs.push_back();
        DebugDrawAabbElement& newElement = m_activeAabbs.back();
        newElement.m_targetEntityId = targetEntity;
        newElement.m_aabb = aabb;
        newElement.m_color = color;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::CreateAabbEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawAabbElement& element)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeAabbsMutex);
        m_activeAabbs.push_back(element);
        DebugDrawAabbElement& newElement = m_activeAabbs.back();
        newElement.m_duration = -1.0f; // Component-spawned text has infinite duration currently (can change in the future)
        newElement.m_targetEntityId = componentEntityId;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }


    ///////////////////////////////////////////////////////////////////////
    // Lines
    ///////////////////////////////////////////////////////////////////////

    void DebugDrawSystemComponent::DrawLineBatchLocationToLocation(const AZStd::vector<DebugDraw::DebugDrawLineElement>& lineBatch)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
        m_activeLines.insert(m_activeLines.end(), lineBatch.begin(), lineBatch.end());
    }

    void DebugDrawSystemComponent::DrawLineLocationToLocation(const AZ::Vector3& startLocation, const AZ::Vector3& endLocation, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
        m_activeLines.push_back();
        DebugDrawLineElement& newElement = m_activeLines.back();

        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_startWorldLocation = startLocation;
        newElement.m_endWorldLocation = endLocation;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawLineEntityToLocation(const AZ::EntityId& startEntity, const AZ::Vector3& endLocation, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
        m_activeLines.push_back();
        DebugDrawLineElement& newElement = m_activeLines.back();
        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_startEntityId = startEntity; // Start of line is at this entity's location
        newElement.m_endWorldLocation = endLocation;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawLineEntityToEntity(const AZ::EntityId& startEntity, const AZ::EntityId& endEntity, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
        m_activeLines.push_back();
        DebugDrawLineElement& newElement = m_activeLines.back();
        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_startEntityId = startEntity; // Start of line is at start entity's location
        newElement.m_endEntityId = endEntity; // End of line is at end entity's location
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::CreateLineEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawLineElement& element)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
        m_activeLines.push_back(element);
        DebugDrawLineElement& newElement = m_activeLines.back();
        newElement.m_duration = -1.0f; // Component-spawned text has infinite duration currently (can change in the future)
        newElement.m_startEntityId = componentEntityId;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }


    ///////////////////////////////////////////////////////////////////////
    // Obbs
    ///////////////////////////////////////////////////////////////////////

    void DebugDrawSystemComponent::DrawObb(const AZ::Obb& obb, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);
        m_activeObbs.push_back();
        DebugDrawObbElement& newElement = m_activeObbs.back();
        newElement.m_obb = obb;
        newElement.m_color = color;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawObbOnEntity(const AZ::EntityId& targetEntity, const AZ::Obb& obb, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);
        m_activeObbs.push_back();
        DebugDrawObbElement& newElement = m_activeObbs.back();
        newElement.m_targetEntityId = targetEntity;
        newElement.m_obb = obb;
        newElement.m_color = color;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::CreateObbEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawObbElement& element)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);
        m_activeObbs.push_back(element);
        DebugDrawObbElement& newElement = m_activeObbs.back();
        newElement.m_duration = -1.0f; // Component-spawned text has infinite duration currently (can change in the future)
        newElement.m_targetEntityId = componentEntityId;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }


    ///////////////////////////////////////////////////////////////////////
    // Rays
    ///////////////////////////////////////////////////////////////////////

    void DebugDrawSystemComponent::DrawRayLocationToDirection(const AZ::Vector3& worldLocation, const AZ::Vector3& worldDirection, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
        m_activeRays.push_back();
        DebugDrawRayElement& newElement = m_activeRays.back();
        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_worldLocation = worldLocation;
        newElement.m_worldDirection = worldDirection;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawRayEntityToDirection(const AZ::EntityId& startEntity, const AZ::Vector3& worldDirection, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
        m_activeRays.push_back();
        DebugDrawRayElement& newElement = m_activeRays.back();
        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_startEntityId = startEntity;
        newElement.m_worldDirection = worldDirection;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawRayEntityToEntity(const AZ::EntityId& startEntity, const AZ::EntityId& endEntity, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
        m_activeRays.push_back();
        DebugDrawRayElement& newElement = m_activeRays.back();
        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_startEntityId = startEntity;
        newElement.m_endEntityId = endEntity;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::CreateRayEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawRayElement& element)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
        m_activeRays.push_back(element);
        DebugDrawRayElement& newElement = m_activeRays.back();
        newElement.m_duration = -1.0f; // Component-spawned text has infinite duration currently (can change in the future)
        newElement.m_startEntityId = componentEntityId;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }


    ///////////////////////////////////////////////////////////////////////
    // Spheres
    ///////////////////////////////////////////////////////////////////////

    void DebugDrawSystemComponent::DrawSphereAtLocation(const AZ::Vector3& worldLocation, float radius, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
        m_activeSpheres.push_back();
        DebugDrawSphereElement& newElement = m_activeSpheres.back();
        newElement.m_worldLocation = worldLocation;
        newElement.m_radius = radius;
        newElement.m_color = color;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawSphereOnEntity(const AZ::EntityId& targetEntity, float radius, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
        m_activeSpheres.push_back();
        DebugDrawSphereElement& newElement = m_activeSpheres.back();
        newElement.m_targetEntityId = targetEntity;
        newElement.m_radius = radius;
        newElement.m_color = color;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }
    void DebugDrawSystemComponent::CreateSphereEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawSphereElement& element)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
        m_activeSpheres.push_back(element);
        DebugDrawSphereElement& newElement = m_activeSpheres.back();
        newElement.m_duration = -1.0f; // Component-spawned text has infinite duration currently (can change in the future)
        newElement.m_targetEntityId = componentEntityId;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    ///////////////////////////////////////////////////////////////////////
    // Text
    ///////////////////////////////////////////////////////////////////////

    void DebugDrawSystemComponent::DrawTextAtLocation(const AZ::Vector3& worldLocation, const AZStd::string& text, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
        m_activeTexts.push_back();
        DebugDrawTextElement& newText = m_activeTexts.back();
        newText.m_drawMode = DebugDrawTextElement::DrawMode::InWorld;
        newText.m_text = text;
        newText.m_color = color;
        newText.m_duration = duration;
        newText.m_worldLocation = worldLocation;
        AZ::TickRequestBus::BroadcastResult(newText.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawTextOnEntity(const AZ::EntityId& targetEntity, const AZStd::string& text, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
        m_activeTexts.push_back();
        DebugDrawTextElement& newText = m_activeTexts.back();
        newText.m_drawMode = DebugDrawTextElement::DrawMode::InWorld;
        newText.m_text = text;
        newText.m_color = color;
        newText.m_duration = duration;
        newText.m_targetEntityId = targetEntity;
        AZ::TickRequestBus::BroadcastResult(newText.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawTextOnScreen(const AZStd::string& text, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
        m_activeTexts.push_back();
        DebugDrawTextElement& newText = m_activeTexts.back();
        newText.m_drawMode = DebugDrawTextElement::DrawMode::OnScreen;
        //newText.m_category = 0;
        newText.m_text = text;
        newText.m_color = color;
        newText.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newText.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::CreateTextEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawTextElement& element)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
        m_activeTexts.push_back(element);
        DebugDrawTextElement& newText = m_activeTexts.back();
        newText.m_duration = -1.0f; // Component-spawned text has infinite duration currently (can change in the future)
        newText.m_targetEntityId = componentEntityId;
        AZ::TickRequestBus::BroadcastResult(newText.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }
} // namespace DebugDraw
