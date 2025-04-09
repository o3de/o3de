/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <SceneAPI/SceneCore/Import/SceneImportSettings.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ::SceneAPI
{
    void SceneImportSettings::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context); serializeContext)
        {
            serializeContext->Class<SceneImportSettings>()
                ->Version(0)
                ->Field("OptimizeScene", &SceneImportSettings::m_optimizeScene)
                ->Field("OptimizeMeshes", &SceneImportSettings::m_optimizeMeshes);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext)
            {
                editContext->Class<SceneImportSettings>("Import Settings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute("AutoExpand", true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SceneImportSettings::m_optimizeScene,
                        "Collapse/Join Scene Nodes",
                        "Nodes without animations, bones, lights, or cameras assigned are collapsed and joined. "
                        "This is useful for non-optimized files that have hundreds or thousands of nodes within them that aren't "
                        "needed to remain separate in O3DE. This should not be used on files where the nodes need to remain separate "
                        "for individual submesh control and transformations.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SceneImportSettings::m_optimizeMeshes,
                        "Merge Duplicate Meshes",
                        "Non-instanced unskinned meshes with the same vertices and faces are merged into instanced meshes. "
                        "This will reduce the number of draw calls in the scene.");
            }
        }
    }
}// namespace AZ::SceneAPI
