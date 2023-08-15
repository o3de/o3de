/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/Groups/ImportGroup.h>
#include <SceneAPI/SceneData/Behaviors/ImportGroup.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            const int ImportGroup::s_importGroupPreferredTabOrder = 0;

            void ImportGroup::Activate()
            {
                Events::ManifestMetaInfoBus::Handler::BusConnect();
                Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void ImportGroup::Deactivate()
            {
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void ImportGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ImportGroup, BehaviorComponent>()->Version(1);
                }
            }

            void ImportGroup::GetCategoryAssignments(CategoryRegistrationList& categories, [[maybe_unused]] const Containers::Scene& scene)
            {
                categories.emplace_back("Import", SceneData::ImportGroup::TYPEINFO_Uuid(), s_importGroupPreferredTabOrder);
            }

            void ImportGroup::InitializeObject([[maybe_unused]] const Containers::Scene& scene, [[maybe_unused]] DataTypes::IManifestObject& target)
            {
                if (!target.RTTI_IsTypeOf(SceneData::ImportGroup::TYPEINFO_Uuid()))
                {
                    return;
                }
            }

            Events::ProcessingResult ImportGroup::UpdateManifest(
                Containers::Scene& scene, [[maybe_unused]] ManifestAction action,
                [[maybe_unused]] RequestingApplication requester)
            {
                // ignore empty scenes (i.e. only has the root node)
                if (scene.GetGraph().GetNodeCount() == 1)
                {
                    return Events::ProcessingResult::Ignored;
                }

                // If there's already an ImportGroup in the manifest, leave it there and return.
                size_t count = scene.GetManifest().GetEntryCount();
                for (size_t index = 0; index < count; index++)
                {
                    if (auto* importGroup = azrtti_cast<DataTypes::IImportGroup*>(scene.GetManifest().GetValue(index).get()); importGroup)
                    {
                        return Events::ProcessingResult::Success;
                    }
                }

                // There's no ImportGroup yet, so add one.
                auto importGroup = AZStd::make_shared<SceneData::ImportGroup>();
                scene.GetManifest().AddEntry(importGroup);
                return Events::ProcessingResult::Success;
            }
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ
