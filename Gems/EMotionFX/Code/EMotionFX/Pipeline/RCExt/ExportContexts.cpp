/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPIExt/Groups/MotionGroup.h>
#include <SceneAPIExt/Groups/IActorGroup.h>
#include <RCExt/ExportContexts.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        //==========================================================================
        //  Motion
        //==========================================================================

        MotionGroupExportContext::MotionGroupExportContext(AZ::SceneAPI::Events::ExportEventContext& parent,
            const Group::IMotionGroup& group, AZ::RC::Phase phase)
            : m_products(parent.GetProductList())
            , m_scene(parent.GetScene())
            , m_outputDirectory(parent.GetOutputDirectory())
            , m_group(group)
            , m_phase(phase)
        {
        }

        MotionGroupExportContext::MotionGroupExportContext(AZ::SceneAPI::Events::ExportProductList& products, const AZ::SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
            const Group::IMotionGroup& group, AZ::RC::Phase phase)
            : m_products(products)
            , m_scene(scene)
            , m_outputDirectory(outputDirectory)
            , m_group(group)
            , m_phase(phase)
        {
        }

        MotionGroupExportContext::MotionGroupExportContext(const MotionGroupExportContext& copyContext, AZ::RC::Phase phase)
            : m_products(copyContext.m_products)
            , m_scene(copyContext.m_scene)
            , m_outputDirectory(copyContext.m_outputDirectory)
            , m_group(copyContext.m_group)
            , m_phase(phase)
        {
        }

        //==========================================================================

        MotionDataBuilderContext::MotionDataBuilderContext(const AZ::SceneAPI::Containers::Scene& scene, const Group::IMotionGroup& motionGroup,
            EMotionFX::Motion& motion, AZ::RC::Phase phase)
            : m_scene(scene)
            , m_group(motionGroup)
            , m_motion(motion)
            , m_phase(phase)
        {
        }

        MotionDataBuilderContext::MotionDataBuilderContext(const MotionDataBuilderContext& copyContext, AZ::RC::Phase phase)
            : m_scene(copyContext.m_scene)
            , m_group(copyContext.m_group)
            , m_motion(copyContext.m_motion)
            , m_phase(phase)
        {
        }

        //==========================================================================
        //  Actor
        //==========================================================================

        ActorGroupExportContext::ActorGroupExportContext(AZ::SceneAPI::Events::ExportEventContext& parent,
            const Group::IActorGroup& group, AZ::RC::Phase phase)
            : m_products(parent.GetProductList())
            , m_scene(parent.GetScene())
            , m_outputDirectory(parent.GetOutputDirectory())
            , m_group(group)
            , m_phase(phase)
        {
        }

        ActorGroupExportContext::ActorGroupExportContext(AZ::SceneAPI::Events::ExportProductList& products, const AZ::SceneAPI::Containers::Scene & scene, const AZStd::string& outputDirectory,
            const Group::IActorGroup & group, AZ::RC::Phase phase)
            : m_products(products)
            , m_scene(scene)
            , m_outputDirectory(outputDirectory)
            , m_group(group)
            , m_phase(phase)
        {
        }

        ActorGroupExportContext::ActorGroupExportContext(const ActorGroupExportContext & copyContext, AZ::RC::Phase phase)
            : m_products(copyContext.m_products)
            , m_scene(copyContext.m_scene)
            , m_outputDirectory(copyContext.m_outputDirectory)
            , m_group(copyContext.m_group)
            , m_phase(phase)
        {
        }

        //==========================================================================

        ActorBuilderContext::ActorBuilderContext(const AZ::SceneAPI::Containers::Scene& scene,
            const AZStd::string& outputDirectory, const Group::IActorGroup& actorGroup,
            EMotionFX::Actor* actor, AZStd::vector<AZStd::string>& materialReferences, AZ::RC::Phase phase)
            : m_scene(scene)
            , m_outputDirectory(outputDirectory)
            , m_group(actorGroup)
            , m_actor(actor)
            , m_phase(phase)
            , m_materialReferences(materialReferences)
        {
        }

        ActorBuilderContext::ActorBuilderContext(const ActorBuilderContext & copyContext, AZ::RC::Phase phase)
            : m_scene(copyContext.m_scene)
            , m_outputDirectory(copyContext.m_outputDirectory)
            , m_group(copyContext.m_group)
            , m_actor(copyContext.m_actor)
            , m_phase(phase)
            , m_materialReferences(copyContext.m_materialReferences)
        {
        }

        //==========================================================================

        ActorMorphBuilderContext::ActorMorphBuilderContext(const AZ::SceneAPI::Containers::Scene& scene,
            AZStd::vector<AZ::u32>* meshNodeIndices, const Group::IActorGroup& actorGroup,
            EMotionFX::Actor* actor,
            AZ::SceneAPI::CoordinateSystemConverter& coordinateSystemConverter,
            AZ::RC::Phase phase)
            : m_scene(scene)
            , m_meshNodeIndices(meshNodeIndices)
            , m_group(actorGroup)
            , m_actor(actor)
            , m_coordinateSystemConverter(coordinateSystemConverter)
            , m_phase(phase)
        {
        }

        ActorMorphBuilderContext::ActorMorphBuilderContext(const ActorMorphBuilderContext & copyContext, AZ::RC::Phase phase)
            : m_scene(copyContext.m_scene)
            , m_meshNodeIndices(copyContext.m_meshNodeIndices)
            , m_group(copyContext.m_group)
            , m_actor(copyContext.m_actor)
            , m_coordinateSystemConverter(copyContext.m_coordinateSystemConverter)
            , m_phase(phase)
        {
        }
    }
}
