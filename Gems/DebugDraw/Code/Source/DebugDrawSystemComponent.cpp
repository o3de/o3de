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

#include "DebugDraw_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/parallel/lock.h>

#include <IRenderAuxGeom.h>

#include <Cry_Camera.h>
#include <MathConversion.h>

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
        (void)required;
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
        AZ::TickBus::Handler::BusConnect();

        #ifdef DEBUGDRAW_GEM_EDITOR
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        #endif // DEBUGDRAW_GEM_EDITOR
    }

    void DebugDrawSystemComponent::Deactivate()
    {
        #ifdef DEBUGDRAW_GEM_EDITOR
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        #endif // DEBUGDRAW_GEM_EDITOR

        AZ::TickBus::Handler::BusDisconnect();
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

    void DebugDrawSystemComponent::OnTick([[maybe_unused]] float deltaTime, AZ::ScriptTimePoint time)
    {
        m_currentTime = time.GetSeconds();

        OnTickAabbs();
        OnTickLines();
        OnTickObbs();
        OnTickRays();
        OnTickSpheres();
        OnTickText();
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

    void DebugDrawSystemComponent::OnTickAabbs()
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

            ColorB lyColor(aabbElement.m_color.ToU32());
            Vec3 worldLocation(AZVec3ToLYVec3(aabbElement.m_worldLocation));
            AABB lyAABB(AZAabbToLyAABB(transformedAabb));
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(lyAABB, false, lyColor, EBoundingBoxDrawStyle::eBBD_Extremes_Color_Encoded);
        }

        removeExpiredDebugElementsFromVector(m_activeAabbs);
    }

    void DebugDrawSystemComponent::OnTickLines()
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

            Vec3 start(AZVec3ToLYVec3(lineElement.m_startWorldLocation));
            Vec3 end(AZVec3ToLYVec3(lineElement.m_endWorldLocation));
            ColorB lyColor(lineElement.m_color.ToU32());

            m_batchPoints.push_back(start);
            m_batchPoints.push_back(end);

            m_batchColors.push_back(lyColor);
            m_batchColors.push_back(lyColor);
        }

        if (!m_batchPoints.empty())
        {
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawLines(m_batchPoints.begin(), m_batchPoints.size(), m_batchColors.begin(), 1.0f);
        }

        removeExpiredDebugElementsFromVector(m_activeLines);
    }

    void DebugDrawSystemComponent::OnTickObbs()
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

            obbElement.m_worldLocation = transformedObb.GetPosition();

            ColorB lyColor(obbElement.m_color.ToU32());
            Vec3 worldLocation(AZVec3ToLYVec3(obbElement.m_worldLocation));
            OBB lyOBB(AZObbToLyOBB(transformedObb));
            lyOBB.c = Vec3(0.f);
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawOBB(lyOBB, worldLocation, false, lyColor, EBoundingBoxDrawStyle::eBBD_Extremes_Color_Encoded);
        }

        removeExpiredDebugElementsFromVector(m_activeObbs);
    }

    void DebugDrawSystemComponent::OnTickRays()
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

            ColorB lyColor(rayElement.m_color.ToU32());
            Vec3 start(AZVec3ToLYVec3(rayElement.m_worldLocation));
            Vec3 end(AZVec3ToLYVec3(endWorldLocation));
            Vec3 direction(AZVec3ToLYVec3(rayElement.m_worldDirection));
            float conePercentHeight = 0.5f;
            float coneHeight = direction.GetLength() * conePercentHeight;
            Vec3 coneBaseLocation = end - direction * conePercentHeight;
            float coneRadius = AZ::GetClamp(coneHeight * 0.07f, 0.05f, 0.2f);
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(start, lyColor, coneBaseLocation, lyColor, 5.0f);
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(coneBaseLocation, direction, coneRadius, coneHeight, lyColor, false);
        }

        removeExpiredDebugElementsFromVector(m_activeRays);
    }

    void DebugDrawSystemComponent::OnTickSpheres()
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

            ColorB lyColor(sphereElement.m_color.ToU32());
            Vec3 worldLocation(AZVec3ToLYVec3(sphereElement.m_worldLocation));
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(worldLocation, sphereElement.m_radius, lyColor, true);
        }

        removeExpiredDebugElementsFromVector(m_activeSpheres);
    }

    void DebugDrawSystemComponent::OnTickText()
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_activeTextsMutex);

        // Determine if we need gamma conversion
        bool needsGammaConversion = false;
        bool isInGameMode = true;
        #ifdef DEBUGDRAW_GEM_EDITOR
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(isInGameMode, &AzToolsFramework::EditorEntityContextRequestBus::Events::IsEditorRunningGame);
        if (isInGameMode)
        {
            needsGammaConversion = true;
        }
        #endif // DEBUGDRAW_GEM_EDITOR

        // Draw text elements and remove any that are expired
        AZStd::unordered_map<AZ::EntityId, AZ::u32> textPerEntityCount;
        int numScreenTexts = 0;
        AZ::EntityId lastTargetEntityId;

        for (auto& textElement : m_activeTexts)
        {
            if (textElement.m_drawMode == DebugDrawTextElement::DrawMode::OnScreen)
            {
                const AZ::Color textColor = needsGammaConversion ? textElement.m_color.GammaToLinear() : textElement.m_color;
                gEnv->pRenderer->GetIRenderAuxGeom()->Draw3dLabel(Vec3(20.f, 20.f + ((float)numScreenTexts * 15.0f), 0.5f), 1.4f, AZColorToLYColorF(textColor), textElement.m_text.c_str());
                ++numScreenTexts;
            }
            else if (textElement.m_drawMode == DebugDrawTextElement::DrawMode::InWorld)
            {
                SDrawTextInfo ti;
                ti.xscale = ti.yscale = 1.4f;
                ti.flags = eDrawText_2D | eDrawText_FixedSize | eDrawText_Monospace | eDrawText_Center;

                const AZ::Color textColor = needsGammaConversion ? textElement.m_color.GammaToLinear() : textElement.m_color;
                ti.color[0] = textColor.GetR();
                ti.color[1] = textColor.GetG();
                ti.color[2] = textColor.GetB();
                ti.color[3] = textColor.GetA();

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

                const CCamera& camera = gEnv->pSystem->GetViewCamera();
                const AZ::Vector3 cameraTranslation = LYVec3ToAZVec3(camera.GetPosition());
                Vec3 lyWorldLoc = AZVec3ToLYVec3(worldLocation);
                Vec3 screenPos(0.f);
                if (camera.Project(lyWorldLoc, screenPos, Vec2i(0, 0), Vec2i(0, 0)))
                {
                    // Handle spacing for world text so it doesn't draw on top of each other
                    // This works for text drawing on entities (considered one block), but not for world text.
                    // World text will get handled when we have screen-aware positioning of text elements
                    if (textElement.m_targetEntityId.IsValid())
                    {
                        auto iter = textPerEntityCount.find(textElement.m_targetEntityId);
                        if (iter != textPerEntityCount.end())
                        {
                            AZ::u32 count = iter->second;
                            screenPos.y += ((float)count * 15.0f);
                            iter->second = count + 1;
                        }
                        else
                        {
                            auto newEntry = textPerEntityCount.insert_key(textElement.m_targetEntityId);
                            newEntry.first->second = 1;
                        }
                    }
                    gEnv->pRenderer->GetIRenderAuxGeom()->Draw3dLabel(Vec3(screenPos.x, screenPos.y, 0.5f), 1.4f, AZColorToLYColorF(textColor), textElement.m_text.c_str());
                }
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
        else if (EditorDebugDrawLineComponent* lineComponent = azrtti_cast<EditorDebugDrawLineComponent*>(component))
        {
            CreateLineEntryForComponent(lineComponent->GetEntityId(), lineComponent->m_element);
        }
        #endif // DEBUGDRAW_GEM_EDITOR
        else if (DebugDrawRayComponent* rayComponent = azrtti_cast<DebugDrawRayComponent*>(component))
        {
            CreateRayEntryForComponent(rayComponent->GetEntityId(), rayComponent->m_element);
        }
        #ifdef DEBUGDRAW_GEM_EDITOR
        else if (EditorDebugDrawRayComponent* rayComponent = azrtti_cast<EditorDebugDrawRayComponent*>(component))
        {
            CreateRayEntryForComponent(rayComponent->GetEntityId(), rayComponent->m_element);
        }
        #endif // DEBUGDRAW_GEM_EDITOR
        else if (DebugDrawSphereComponent* sphereComponent = azrtti_cast<DebugDrawSphereComponent*>(component))
        {
            CreateSphereEntryForComponent(sphereComponent->GetEntityId(), sphereComponent->m_element);
        }
        #ifdef DEBUGDRAW_GEM_EDITOR
        else if (EditorDebugDrawSphereComponent* sphereComponent = azrtti_cast<EditorDebugDrawSphereComponent*>(component))
        {
            CreateSphereEntryForComponent(sphereComponent->GetEntityId(), sphereComponent->m_element);
        }
        #endif // DEBUGDRAW_GEM_EDITOR
        else if (DebugDrawObbComponent* obbComponent = azrtti_cast<DebugDrawObbComponent*>(component))
        {
            CreateObbEntryForComponent(obbComponent->GetEntityId(), obbComponent->m_element);
        }

        #ifdef DEBUGDRAW_GEM_EDITOR
        else if (EditorDebugDrawObbComponent* obbComponent = azrtti_cast<EditorDebugDrawObbComponent*>(component))
        {
            CreateObbEntryForComponent(obbComponent->GetEntityId(), obbComponent->m_element);
        }
        #endif // DEBUGDRAW_GEM_EDITOR

        else if (DebugDrawTextComponent* textComponent = azrtti_cast<DebugDrawTextComponent*>(component))
        {
            CreateTextEntryForComponent(textComponent->GetEntityId(), textComponent->m_element);
        }

        #ifdef DEBUGDRAW_GEM_EDITOR
        else if (EditorDebugDrawTextComponent* textComponent = azrtti_cast<EditorDebugDrawTextComponent*>(component))
        {
            CreateTextEntryForComponent(textComponent->GetEntityId(), textComponent->m_element);
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
