/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

#include <Atom/RHI/RHIUtils.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>

namespace
{
    AZ::Uuid UuidFromEntityId(const AZ::EntityId& entityId)
    {
        AZ::u64 entityIdNumber = static_cast<AZ::u64>(entityId);
        return AZ::Uuid::CreateData(reinterpret_cast<const AZStd::byte*>(&entityIdNumber), sizeof(AZ::u64));
    }
}

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
        provided.push_back(AZ_CRC_CE("DebugDrawService"));
    }

    void DebugDrawSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DebugDrawService"));
    }

    void DebugDrawSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
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
            for (const auto& obb : m_activeObbs)
            {
                RemoveRaytracingData(obb);
            }
            m_activeObbs.clear();
        }
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
            m_activeRays.clear();
        }
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
            for (const auto& sphere : m_activeSpheres)
            {
                RemoveRaytracingData(sphere);
            }
            m_activeSpheres.clear();
        }
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
            m_activeTexts.clear();
        }

        m_sphereRayTracingTypeHandle.Free();
        m_obbRayTracingTypeHandle.Free();
        m_spheresRayTracingIndicesBuffer.reset();
        m_spheresRayTracingIndices.Reset();
    }

    void DebugDrawSystemComponent::OnBootstrapSceneReady(AZ::RPI::Scene* scene)
    {
        AZ_Assert(scene, "Invalid scene received in OnBootstrapSceneReady");
        AZ::RPI::SceneNotificationBus::Handler::BusDisconnect();
        AZ::RPI::SceneNotificationBus::Handler::BusConnect(scene->GetId());
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
            AZStd::vector<DebugDrawObbElementWrapper> elementsToSave;
            for (const DebugDrawObbElementWrapper& element : m_activeObbs)
            {
                if (element.m_owningEditorComponent != AZ::InvalidComponentId)
                {
                    elementsToSave.push_back(element);
                }
            }
            m_activeObbs.clear();
            m_activeObbs.assign_rv(AZStd::forward<AZStd::vector<DebugDrawObbElementWrapper>>(elementsToSave));
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
            AZStd::vector<DebugDrawSphereElementWrapper> elementsToSave;
            for (const DebugDrawSphereElementWrapper& element : m_activeSpheres)
            {
                if (element.m_owningEditorComponent != AZ::InvalidComponentId)
                {
                    elementsToSave.push_back(element);
                }
            }
            m_activeSpheres.clear();
            m_activeSpheres.assign_rv(AZStd::forward<AZStd::vector<DebugDrawSphereElementWrapper>>(elementsToSave));
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

            if (m_rayTracingFeatureProcessor && obbElement.m_isRayTracingEnabled &&
                (obbElement.m_worldLocation != obbElement.m_previousWorldLocation ||
                 obbElement.m_scale != obbElement.m_previousScale ||
                 transformedObb.GetRotation() != obbElement.m_previousRotation))
            {
                AZ::Transform obbTransform(obbElement.m_worldLocation, transformedObb.GetRotation(), 1.f);
                m_rayTracingFeatureProcessor->SetProceduralGeometryTransform(UuidFromEntityId(obbElement.m_targetEntityId), obbTransform, obbElement.m_scale);
                obbElement.m_previousWorldLocation = obbElement.m_worldLocation;
                obbElement.m_previousScale = obbElement.m_scale;
                obbElement.m_previousRotation = transformedObb.GetRotation();
            }
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

            if (m_rayTracingFeatureProcessor && sphereElement.m_isRayTracingEnabled &&
                (sphereElement.m_worldLocation != sphereElement.m_previousWorldLocation ||
                 sphereElement.m_radius != sphereElement.m_previousRadius))
            {
                AZ::Transform sphereTransform(sphereElement.m_worldLocation, AZ::Quaternion::CreateIdentity(), sphereElement.m_radius);
                m_rayTracingFeatureProcessor->SetProceduralGeometryTransform(
                    UuidFromEntityId(sphereElement.m_targetEntityId), sphereTransform);
                sphereElement.m_previousWorldLocation = sphereElement.m_worldLocation;
                sphereElement.m_previousRadius = sphereElement.m_radius;
            }
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
        float currentOnScreenY = 20.f; // Initial shift down for the 1st line, then recalculate shifts down for next lines accounting for textElement.m_fontScale
        AZ::EntityId lastTargetEntityId;

        for (auto& textElement : m_activeTexts)
        {
            const AZ::Color textColor = needsGammaConversion ? textElement.m_color.GammaToLinear() : textElement.m_color;
            debugDisplay.SetColor(textColor);
            if (textElement.m_drawMode == DebugDrawTextElement::DrawMode::OnScreen)
            {
                if (textElement.m_useOnScreenCoordinates)
                {
                    // Reuse textElement.m_worldLocation for 2D OnScreen positioning.
                    debugDisplay.Draw2dTextLabel(textElement.m_worldLocation.GetX(),textElement.m_worldLocation.GetY(),textElement.m_fontScale
                        , textElement.m_text.c_str(), textElement.m_bCenter);
                }
                else
                {
                    // Hardcoded 2D OnScreen positioning as in original code. Note original code below with constant shifts.
                    debugDisplay.Draw2dTextLabel(100.0f, currentOnScreenY, textElement.m_fontScale, textElement.m_text.c_str());
                    // Prepare the shift down for a next line assuming default m_textSizeFactor = 12.0f + line gap. 
                    // Could be more precise if Draw2dTextLabel() returned drawn text size with current viewport settings.
                    currentOnScreenY += textElement.m_fontScale * 14.0f + 2.0f; 
                }
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

                debugDisplay.DrawTextLabel(worldLocation, textElement.m_size, textElement.m_text.c_str(), textElement.m_centered);
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
                DebugDrawObbElementWrapper& element = *iter;
                if (element.m_targetEntityId == entityId)
                {
                    RemoveRaytracingData(element);
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
                DebugDrawSphereElementWrapper& element = *iter;
                if (element.m_targetEntityId == entityId)
                {
                    RemoveRaytracingData(element);
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
                DebugDrawObbElementWrapper& element = *iter;
                if (element.m_targetEntityId == componentEntityId && element.m_owningEditorComponent == componentId)
                {
                    RemoveRaytracingData(element);
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
                DebugDrawSphereElementWrapper& element = *iter;
                if (element.m_targetEntityId == componentEntityId && element.m_owningEditorComponent == componentId)
                {
                    RemoveRaytracingData(element);
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
        DebugDrawAabbElement& newElement = m_activeAabbs.emplace_back();
        newElement.m_aabb = aabb;
        newElement.m_color = color;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawAabbOnEntity(const AZ::EntityId& targetEntity, const AZ::Aabb& aabb, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeAabbsMutex);
        DebugDrawAabbElement& newElement = m_activeAabbs.emplace_back();
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
        DebugDrawLineElement& newElement = m_activeLines.emplace_back();

        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_startWorldLocation = startLocation;
        newElement.m_endWorldLocation = endLocation;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawLineEntityToLocation(const AZ::EntityId& startEntity, const AZ::Vector3& endLocation, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
        DebugDrawLineElement& newElement = m_activeLines.emplace_back();
        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_startEntityId = startEntity; // Start of line is at this entity's location
        newElement.m_endWorldLocation = endLocation;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawLineEntityToEntity(const AZ::EntityId& startEntity, const AZ::EntityId& endEntity, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeLinesMutex);
        DebugDrawLineElement& newElement = m_activeLines.emplace_back();
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
        DebugDrawObbElementWrapper& newElement = m_activeObbs.emplace_back();
        newElement.m_obb = obb;
        newElement.m_color = color;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawObbOnEntity(const AZ::EntityId& targetEntity, const AZ::Obb& obb, const AZ::Color& color, bool enableRayTracing, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);
        DebugDrawObbElementWrapper& newElement = m_activeObbs.emplace_back();
        newElement.m_targetEntityId = targetEntity;
        newElement.m_obb = obb;
        newElement.m_scale = obb.GetHalfLengths();
        newElement.m_color = color;
        newElement.m_isRayTracingEnabled = enableRayTracing;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);

        AddRaytracingData(newElement);
    }

    void DebugDrawSystemComponent::CreateObbEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawObbElement& element)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeObbsMutex);
        DebugDrawObbElementWrapper& newElement = m_activeObbs.emplace_back();
        newElement.m_targetEntityId = componentEntityId;
        newElement.m_obb = element.m_obb;
        newElement.m_duration = -1.0f; // Component-spawned text has infinite duration currently (can change in the future)
        newElement.m_color = element.m_color;
        newElement.m_worldLocation = element.m_worldLocation;
        newElement.m_owningEditorComponent = element.m_owningEditorComponent;
        newElement.m_scale = element.m_scale;
        newElement.m_isRayTracingEnabled = element.m_isRayTracingEnabled;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);

        AddRaytracingData(newElement);
    }


    ///////////////////////////////////////////////////////////////////////
    // Rays
    ///////////////////////////////////////////////////////////////////////

    void DebugDrawSystemComponent::DrawRayLocationToDirection(const AZ::Vector3& worldLocation, const AZ::Vector3& worldDirection, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
        DebugDrawRayElement& newElement = m_activeRays.emplace_back();
        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_worldLocation = worldLocation;
        newElement.m_worldDirection = worldDirection;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawRayEntityToDirection(const AZ::EntityId& startEntity, const AZ::Vector3& worldDirection, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
        DebugDrawRayElement& newElement = m_activeRays.emplace_back();
        newElement.m_color = color;
        newElement.m_duration = duration;
        newElement.m_startEntityId = startEntity;
        newElement.m_worldDirection = worldDirection;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawRayEntityToEntity(const AZ::EntityId& startEntity, const AZ::EntityId& endEntity, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeRaysMutex);
        DebugDrawRayElement& newElement = m_activeRays.emplace_back();
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
        DebugDrawSphereElementWrapper& newElement = m_activeSpheres.emplace_back();
        newElement.m_worldLocation = worldLocation;
        newElement.m_radius = radius;
        newElement.m_color = color;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawSphereOnEntity(const AZ::EntityId& targetEntity, float radius, const AZ::Color& color, bool enableRayTracing, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
        DebugDrawSphereElementWrapper& newElement = m_activeSpheres.emplace_back();
        newElement.m_targetEntityId = targetEntity;
        newElement.m_radius = radius;
        newElement.m_color = color;
        newElement.m_isRayTracingEnabled = enableRayTracing;
        newElement.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);

        AddRaytracingData(newElement);
    }
    void DebugDrawSystemComponent::CreateSphereEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawSphereElement& element)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeSpheresMutex);
        DebugDrawSphereElementWrapper& newElement = m_activeSpheres.emplace_back();
        newElement.m_duration = -1.0f; // Component-spawned text has infinite duration currently (can change in the future)
        newElement.m_color = element.m_color;
        newElement.m_targetEntityId = componentEntityId;
        newElement.m_worldLocation = element.m_worldLocation;
        newElement.m_radius = element.m_radius;
        newElement.m_isRayTracingEnabled = element.m_isRayTracingEnabled;
        newElement.m_owningEditorComponent = element.m_owningEditorComponent;
        AZ::TickRequestBus::BroadcastResult(newElement.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);

        AddRaytracingData(newElement);
    }

    ///////////////////////////////////////////////////////////////////////
    // Text
    ///////////////////////////////////////////////////////////////////////

    void DebugDrawSystemComponent::DrawTextAtLocation(const AZ::Vector3& worldLocation, const AZStd::string& text, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
        DebugDrawTextElement& newText = m_activeTexts.emplace_back();
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
        DebugDrawTextElement& newText = m_activeTexts.emplace_back();
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
        DebugDrawTextElement& newText = m_activeTexts.emplace_back();
        newText.m_drawMode = DebugDrawTextElement::DrawMode::OnScreen;
        //newText.m_category = 0;
        newText.m_text = text;
        newText.m_color = color;
        newText.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newText.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawScaledTextOnScreen(const AZStd::string& text, float fontScale, const AZ::Color& color, float duration)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
        DebugDrawTextElement& newText = m_activeTexts.emplace_back();
        newText.m_drawMode = DebugDrawTextElement::DrawMode::OnScreen;
        newText.m_text = text;
        newText.m_fontScale = fontScale;
        newText.m_color = color;
        newText.m_duration = duration;
        AZ::TickRequestBus::BroadcastResult(newText.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DrawScaledTextOnScreenPos(float x, float y, const AZStd::string& text, float fontScale, const AZ::Color& color, float duration, bool bCenter)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
        DebugDrawTextElement& newText = m_activeTexts.emplace_back();
        newText.m_drawMode = DebugDrawTextElement::DrawMode::OnScreen;
        newText.m_text = text;
        newText.m_fontScale = fontScale;
        newText.m_color = color;
        newText.m_duration = duration;
        newText.m_bCenter = bCenter;
        newText.m_useOnScreenCoordinates = true;
        newText.m_worldLocation.Set(x, y, 1.f);
        AZ::TickRequestBus::BroadcastResult(newText.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::DebugDrawSystemComponent::CreateTextEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawTextElement& element)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);
        m_activeTexts.push_back(element);
        DebugDrawTextElement& newText = m_activeTexts.back();
        newText.m_duration = -1.0f; // Component-spawned text has infinite duration currently (can change in the future)
        newText.m_targetEntityId = componentEntityId;
        AZ::TickRequestBus::BroadcastResult(newText.m_activateTime, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
    }

    void DebugDrawSystemComponent::AddRaytracingData(DebugDrawSphereElementWrapper& element)
    {
        if (!element.m_isRayTracingEnabled)
        {
            return;
        }

        if (!m_sphereRayTracingTypeHandle.IsValid())
        {
            m_rayTracingFeatureProcessor =
                AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::RayTracingFeatureProcessorInterface>(element.m_targetEntityId);

            auto shaderAsset = AZ::RPI::FindShaderAsset("shaders/sphereintersection.azshader");
            auto rayTracingShader = AZ::RPI::Shader::FindOrCreate(shaderAsset, AZ::RHI::GetDefaultSupervariantNameWithNoFloat16Fallback());

            AZ::RPI::CommonBufferDescriptor desc;
            desc.m_bufferName = "SpheresBuffer";
            desc.m_poolType = AZ::RPI::CommonBufferPoolType::ReadOnly;
            desc.m_byteCount = sizeof(float); // Start with just 1 element
            desc.m_elementSize = sizeof(float);
            desc.m_elementFormat = AZ::RHI::Format::R32_FLOAT;
            desc.m_bufferData = nullptr;
            m_spheresRayTracingIndicesBuffer = AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

            m_sphereRayTracingTypeHandle = m_rayTracingFeatureProcessor->RegisterProceduralGeometryType(
                "DebugDraw::Sphere",
                rayTracingShader,
                "SphereIntersection",
                m_spheresRayTracingIndicesBuffer->GetBufferView()->GetBindlessReadIndex());
        }

        element.m_localInstanceIndex = m_spheresRayTracingIndices.AddEntry(0);

        size_t requiredSizeInBytes = m_spheresRayTracingIndices.GetIndexList().size() * sizeof(float);
        if (requiredSizeInBytes > m_spheresRayTracingIndicesBuffer->GetBufferSize())
        {
            m_spheresRayTracingIndicesBuffer->Resize(requiredSizeInBytes);
            m_rayTracingFeatureProcessor->SetProceduralGeometryTypeBindlessBufferIndex(
                m_sphereRayTracingTypeHandle.GetWeakHandle(), m_spheresRayTracingIndicesBuffer->GetBufferView()->GetBindlessReadIndex());

            // Need to copy all existing data to resized buffer
            AZStd::vector<float> radii(m_spheresRayTracingIndices.GetIndexList().size());
            for (const DebugDrawSphereElementWrapper& sphere : m_activeSpheres)
            {
                radii[sphere.m_localInstanceIndex] = sphere.m_radius;
            }
            m_spheresRayTracingIndicesBuffer->UpdateData(radii.data(), radii.size() * sizeof(radii[0]));
        }

        m_spheresRayTracingIndicesBuffer->UpdateData(&element.m_radius, sizeof(float), element.m_localInstanceIndex * sizeof(float));

        AZ::Render::RayTracingFeatureProcessorInterface::SubMeshMaterial material;
        material.m_baseColor = element.m_color;
        material.m_roughnessFactor = 0.9f;

        m_rayTracingFeatureProcessor->AddProceduralGeometry(
            m_sphereRayTracingTypeHandle.GetWeakHandle(),
            UuidFromEntityId(element.m_targetEntityId),
            AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 1.f),
            material,
            AZ::RHI::RayTracingAccelerationStructureInstanceInclusionMask::STATIC_MESH,
            element.m_localInstanceIndex);
    }

    void DebugDrawSystemComponent::AddRaytracingData(DebugDrawObbElementWrapper& element)
    {
        if (!element.m_isRayTracingEnabled)
        {
            return;
        }

        if (!m_obbRayTracingTypeHandle.IsValid())
        {
            m_rayTracingFeatureProcessor =
                AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::RayTracingFeatureProcessorInterface>(element.m_targetEntityId);

            auto shaderAsset = AZ::RPI::FindShaderAsset("shaders/obbintersection.azshader");
            auto rayTracingShader = AZ::RPI::Shader::FindOrCreate(shaderAsset, AZ::RHI::GetDefaultSupervariantNameWithNoFloat16Fallback());

            m_obbRayTracingTypeHandle =
                m_rayTracingFeatureProcessor->RegisterProceduralGeometryType("DebugDraw::Obb", rayTracingShader, "ObbIntersection");
        }

        AZ::Render::RayTracingFeatureProcessorInterface::SubMeshMaterial material;
        material.m_baseColor = element.m_color;
        material.m_roughnessFactor = 0.9f;

        m_rayTracingFeatureProcessor->AddProceduralGeometry(
            m_obbRayTracingTypeHandle.GetWeakHandle(),
            UuidFromEntityId(element.m_targetEntityId),
            AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 1.f),
            material,
            AZ::RHI::RayTracingAccelerationStructureInstanceInclusionMask::STATIC_MESH,
            0);
    }

    void DebugDrawSystemComponent::RemoveRaytracingData(const DebugDrawSphereElementWrapper& element)
    {
        if (m_rayTracingFeatureProcessor && element.m_isRayTracingEnabled)
        {
            m_spheresRayTracingIndices.RemoveEntry(element.m_localInstanceIndex);
            m_rayTracingFeatureProcessor->RemoveProceduralGeometry(UuidFromEntityId(element.m_targetEntityId));
            if (m_rayTracingFeatureProcessor->GetProceduralGeometryCount(m_sphereRayTracingTypeHandle.GetWeakHandle()) == 0)
            {
                m_sphereRayTracingTypeHandle.Free();
                m_spheresRayTracingIndicesBuffer.reset();
                m_spheresRayTracingIndices.Reset();
            }
        }
    }

    void DebugDrawSystemComponent::RemoveRaytracingData(const DebugDrawObbElementWrapper& element)
    {
        if (m_rayTracingFeatureProcessor && element.m_isRayTracingEnabled)
        {
            m_rayTracingFeatureProcessor->RemoveProceduralGeometry(UuidFromEntityId(element.m_targetEntityId));
            if (m_rayTracingFeatureProcessor->GetProceduralGeometryCount(m_obbRayTracingTypeHandle.GetWeakHandle()) == 0)
            {
                m_obbRayTracingTypeHandle.Free();
            }
        }
    }
} // namespace DebugDraw
