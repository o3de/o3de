/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/GraphView/GraphViewConstructPresets.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>

namespace AtomToolsFramework
{
    void GraphViewConstructPresets::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GraphViewConstructPresets, GraphCanvas::EditorConstructPresets>()
                ->Version(0)
                ;

            if (auto editContext = serialize->GetEditContext())
            {
                editContext->Class<GraphViewConstructPresets>("GraphViewConstructPresets", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void GraphViewConstructPresets::InitializeConstructType(GraphCanvas::ConstructType constructType)
    {
        if (constructType == GraphCanvas::ConstructType::NodeGroup)
        {
            auto presetBucket = static_cast<GraphCanvas::NodeGroupPresetBucket*>(ModPresetBucket(GraphCanvas::ConstructType::NodeGroup));
            if (presetBucket)
            {
                presetBucket->ClearPresets();

                const AZStd::map<AZStd::string, AZ::Color> defaultPresetGroups = { { "Logic", AZ::Color(0.188f, 0.972f, 0.243f, 1.0f) },
                                                                                   { "Function", AZ::Color(0.396f, 0.788f, 0.788f, 1.0f) },
                                                                                   { "Output", AZ::Color(0.866f, 0.498f, 0.427f, 1.0f) },
                                                                                   { "Input", AZ::Color(0.396f, 0.788f, 0.549f, 1.0f) } };

                for (const auto& defaultPresetPair : defaultPresetGroups)
                {
                    if (auto preset = presetBucket->CreateNewPreset(defaultPresetPair.first))
                    {
                        const auto& presetSaveData = preset->GetPresetData();
                        if (auto saveData = presetSaveData.FindSaveDataAs<GraphCanvas::CommentNodeTextSaveData>())
                        {
                            saveData->m_backgroundColor = defaultPresetPair.second;
                        }
                    }
                }
            }
        }
        else if (constructType == GraphCanvas::ConstructType::CommentNode)
        {
            auto presetBucket = static_cast<GraphCanvas::CommentPresetBucket*>(ModPresetBucket(GraphCanvas::ConstructType::CommentNode));
            if (presetBucket)
            {
                presetBucket->ClearPresets();
            }
        }
    }
} // namespace AtomToolsFramework
