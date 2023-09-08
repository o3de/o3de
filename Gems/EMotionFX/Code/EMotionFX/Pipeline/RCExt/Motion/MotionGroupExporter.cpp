/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>

#include <SceneAPIExt/Rules/MetaDataRule.h>
#include <SceneAPIExt/Rules/MotionMetaDataRule.h>
#include <SceneAPIExt/Groups/IMotionGroup.h>
#include <RCExt/Motion/MotionGroupExporter.h>
#include <RCExt/ExportContexts.h>

#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainer = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        const AZStd::string MotionGroupExporter::s_fileExtension = "motion";

        MotionGroupExporter::MotionGroupExporter()
        {
            BindToCall(&MotionGroupExporter::ProcessContext);
        }

        void MotionGroupExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MotionGroupExporter, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult MotionGroupExporter::ProcessContext(MotionGroupExportContext& context) const
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            // If we wanted to preserve the input file's extension as a part of the product asset name, we would pass
            // context.m_scene.GetSourceExtension() here instead of emptySourceExtension. We aren't currently doing that for
            // EmotionFX files (.actor, .motion) because source assets (.motionset, .emfxworkspace) can have references to
            // existing product asset file names. Those names would need to get fixed up to contain the extension to preserve backwards
            // compatibility.
            // For example, 'walk.fbx' will produce 'walk.motion' here. If we pass in GetSourceExtension(), it would
            // produce 'walk.fbx.motion'. The reason to want the latter is so that multiple input files that vary by extension only
            // would produce different outputs (ex: 'walk.fbx' -> 'walk.fbx.motion', 'walk.obj' -> 'walk.obj.motion').
            // If this is ever desired, the source asset input serialization would need to be modified to correctly change the
            // assetId field that's stored in the source assets, and the version number on those files should be incremented.
            const AZStd::string emptySourceExtension;

            const AZStd::string& groupName = context.m_group.GetName();
            AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(
                groupName, context.m_outputDirectory, s_fileExtension, emptySourceExtension);

            if (filename.empty() || !SceneUtil::FileUtilities::EnsureTargetFolderExists(filename))
            {
                return SceneEvents::ProcessingResult::Failure;
            }

            EMotionFX::Motion* motion = aznew EMotionFX::Motion(groupName.c_str());
            if (!motion)
            {
                return SceneEvents::ProcessingResult::Failure;
            }
            motion->SetUnitType(MCore::Distance::UNITTYPE_METERS);

            SceneEvents::ProcessingResultCombiner result;

            const Group::IMotionGroup& motionGroup = context.m_group;
            MotionDataBuilderContext dataBuilderContext(context.m_scene, motionGroup, *motion, AZ::RC::Phase::Construction);
            result += SceneEvents::Process(dataBuilderContext);
            result += SceneEvents::Process<MotionDataBuilderContext>(dataBuilderContext, AZ::RC::Phase::Filling);
            result += SceneEvents::Process<MotionDataBuilderContext>(dataBuilderContext, AZ::RC::Phase::Finalizing);

            // Legacy meta data: Check if there is legacy (XML) event data rule and apply it.
            AZStd::vector<MCore::Command*> metaDataCommands;
            if (Rule::MetaDataRule::LoadMetaData(motionGroup, metaDataCommands))
            {
                if (!CommandSystem::MetaData::ApplyMetaDataOnMotion(motion, metaDataCommands))
                {
                    AZ_Error("EMotionFX", false, "Applying meta data to '%s' failed.", filename.c_str());
                }
            }

            // Apply motion meta data.
            AZStd::shared_ptr<EMotionFX::Pipeline::Rule::MotionMetaData> motionMetaData;
            if (EMotionFX::Pipeline::Rule::LoadFromGroup<EMotionFX::Pipeline::Rule::MotionMetaDataRule>(motionGroup, motionMetaData))
            {
                motion->SetEventTable(motionMetaData->GetClonedEventTable(motion));
                motion->SetMotionExtractionFlags(motionMetaData->GetMotionExtractionFlags());
            }

            ExporterLib::SaveMotion(filename, motion, MCore::Endian::ENDIAN_LITTLE);
            static AZ::Data::AssetType emotionFXMotionAssetType("{00494B8E-7578-4BA2-8B28-272E90680787}"); // from MotionAsset.h in EMotionFX Gem
            context.m_products.AddProduct(AZStd::move(filename), context.m_group.GetId(), emotionFXMotionAssetType,
                AZStd::nullopt, AZStd::nullopt);

            // The motion object served the purpose of exporting motion and is no longer needed
            MCore::Destroy(motion);
            motion = nullptr;

            return result.GetResult();
        }
    } // namespace Pipeline
} // namespace EMotionFX
