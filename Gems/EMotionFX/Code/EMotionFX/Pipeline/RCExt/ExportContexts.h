#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Utilities/CoordinateSystemConverter.h>

#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace RC
    {
        enum class Phase
        {
            Construction,   // The target is created.
            Filling,        // Data is added to the target.
            Finalizing      // Work on the target has completed.
        };
    }

    namespace SceneAPI
    {
        namespace Events
        {
            class ExportProductList;
        }
    }
}

namespace EMotionFX
{
    class Motion;
    class Actor;

    namespace Pipeline
    {
        namespace Group
        {
            class IMotionGroup;
            class IActorGroup;
        }

        // Context structure to export a specific Animation (Motion) Group
        struct MotionGroupExportContext
            : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(MotionGroupExportContext, "{03B84A87-D1C2-4392-B78B-AC1174CA296E}", AZ::SceneAPI::Events::ICallContext);

            MotionGroupExportContext(AZ::SceneAPI::Events::ExportEventContext& parent,
                const Group::IMotionGroup& group, AZ::RC::Phase phase);
            MotionGroupExportContext(AZ::SceneAPI::Events::ExportProductList& products, const AZ::SceneAPI::Containers::Scene& scene,
                const AZStd::string& outputDirectory, const Group::IMotionGroup& group, AZ::RC::Phase phase);
            MotionGroupExportContext(const MotionGroupExportContext& copyContent, AZ::RC::Phase phase);
            MotionGroupExportContext(const MotionGroupExportContext& copyContent) = delete;
            ~MotionGroupExportContext() override = default;

            MotionGroupExportContext& operator=(const MotionGroupExportContext& other) = delete;

            AZ::SceneAPI::Events::ExportProductList&        m_products;
            const AZ::SceneAPI::Containers::Scene&          m_scene;
            const AZStd::string&                            m_outputDirectory;
            const Group::IMotionGroup&                      m_group;
            const AZ::RC::Phase                             m_phase;
        };

        // Context structure for building the motion data structure for the purpose of exporting.
        struct MotionDataBuilderContext
            : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(MotionDataBuilderContext, "{1C5795BB-2130-499E-96AD-50926EFC8CE9}", AZ::SceneAPI::Events::ICallContext);

            MotionDataBuilderContext(const AZ::SceneAPI::Containers::Scene& scene, const Group::IMotionGroup& motionGroup,
                EMotionFX::Motion& motion, AZ::RC::Phase phase);
            MotionDataBuilderContext(const MotionDataBuilderContext& copyContext, AZ::RC::Phase phase);
            MotionDataBuilderContext(const MotionDataBuilderContext& copyContext) = delete;
            ~MotionDataBuilderContext() override = default;

            MotionDataBuilderContext& operator=(const MotionDataBuilderContext& other) = delete;

            const AZ::SceneAPI::Containers::Scene& m_scene;
            const Group::IMotionGroup&             m_group;
            EMotionFX::Motion&                     m_motion;
            const AZ::RC::Phase                    m_phase;
        };

        // Context structure to export a specific Actor Group
        struct ActorGroupExportContext
            : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(ActorGroupExportContext, "{9FBECA5A-8EDB-4178-8A66-793A5F55B194}", AZ::SceneAPI::Events::ICallContext);

            ActorGroupExportContext(AZ::SceneAPI::Events::ExportEventContext& parent,
                const Group::IActorGroup& group, AZ::RC::Phase phase);
            ActorGroupExportContext(AZ::SceneAPI::Events::ExportProductList& products, const AZ::SceneAPI::Containers::Scene& scene,
                const AZStd::string& outputDirectory, const Group::IActorGroup& group, AZ::RC::Phase phase);
            ActorGroupExportContext(const ActorGroupExportContext& copyContext, AZ::RC::Phase phase);
            ActorGroupExportContext(const ActorGroupExportContext& copyContext) = delete;
            ~ActorGroupExportContext() override = default;

            ActorGroupExportContext& operator=(const ActorGroupExportContext& other) = delete;

            AZ::SceneAPI::Events::ExportProductList&        m_products;
            const AZ::SceneAPI::Containers::Scene&          m_scene;
            const AZStd::string&                            m_outputDirectory;
            const Group::IActorGroup&                       m_group;
            const AZ::RC::Phase                             m_phase;
        };

        // Context structure for building the actor data structure for the purpose of exporting.
        struct ActorBuilderContext
            : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(ActorBuilderContext, "{92048988-F567-4E6C-B6BD-3EFD2A5B6AA1}", AZ::SceneAPI::Events::ICallContext);

            ActorBuilderContext(const AZ::SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
                const Group::IActorGroup& actorGroup, EMotionFX::Actor* actor, AZStd::vector<AZStd::string>& materialReferences, AZ::RC::Phase phase);
            ActorBuilderContext(const ActorBuilderContext& copyContext, AZ::RC::Phase phase);
            ActorBuilderContext(const ActorBuilderContext& copyContext) = delete;
            ~ActorBuilderContext() override = default;

            ActorBuilderContext& operator=(const ActorBuilderContext& other) = delete;

            const AZ::SceneAPI::Containers::Scene&          m_scene;
            const AZStd::string&                            m_outputDirectory;
            EMotionFX::Actor*                               m_actor;
            const Group::IActorGroup&                       m_group;
            AZStd::vector<AZStd::string>&                   m_materialReferences;
            const AZ::RC::Phase                             m_phase;
        };
        
        // Context structure for building the actors morph data structure for the purpose of exporting.
        struct ActorMorphBuilderContext
            : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(ActorMorphBuilderContext, "{A9D4B0B1-016B-4714-BD95-85A9DEFC254B}", AZ::SceneAPI::Events::ICallContext);
        
            ActorMorphBuilderContext(const AZ::SceneAPI::Containers::Scene& scene, AZStd::vector<AZ::u32>* meshNodeIndices,
                const Group::IActorGroup& actorGroup, EMotionFX::Actor* actor, AZ::SceneAPI::CoordinateSystemConverter& coordinateSystemConverter, AZ::RC::Phase phase);
            ActorMorphBuilderContext(const ActorMorphBuilderContext& copyContext, AZ::RC::Phase phase);
            ActorMorphBuilderContext(const ActorMorphBuilderContext& copyContext) = delete;
            ~ActorMorphBuilderContext() override = default;

            ActorMorphBuilderContext& operator=(const ActorMorphBuilderContext& other) = delete;

            const AZ::SceneAPI::Containers::Scene&  m_scene;
            AZStd::vector<AZ::u32>*                 m_meshNodeIndices;
            EMotionFX::Actor*                       m_actor;
            const Group::IActorGroup&               m_group;
            AZ::SceneAPI::CoordinateSystemConverter m_coordinateSystemConverter;
            const AZ::RC::Phase                     m_phase;
        };
    }
}
