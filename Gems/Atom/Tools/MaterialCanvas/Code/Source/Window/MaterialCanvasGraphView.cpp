/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Document/MaterialCanvasDocumentRequestBus.h>
#include <Window/MaterialCanvasGraphView.h>

namespace MaterialCanvas
{
    void MaterialCanvasGraphConstructPresets::InitializeConstructType(GraphCanvas::ConstructType constructType)
    {
        if (constructType == GraphCanvas::ConstructType::NodeGroup)
        {
            auto presetBucket = static_cast<GraphCanvas::NodeGroupPresetBucket*>(ModPresetBucket(GraphCanvas::ConstructType::NodeGroup));
            if (presetBucket)
            {
                presetBucket->ClearPresets();

                const AZStd::map<AZStd::string, AZ::Color> defaultPresetGroups = {
                    { "Logic", AZ::Color(0.188f, 0.972f, 0.243f, 1.0f) },
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

    MaterialCanvasGraphView::MaterialCanvasGraphView(
        const AZ::Crc32& toolId, const AZ::Uuid& documentId, const AtomToolsFramework::GraphViewConfig& graphViewConfig, QWidget* parent)
        : AtomToolsFramework::GraphView(toolId, GraphCanvas::GraphId(), graphViewConfig, parent)
        , m_documentId(documentId)
    {
        m_materialCanvasGraphConstructPresets.SetEditorId(m_toolId);

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
        OnDocumentOpened(m_documentId);
        m_openedBefore = false;
    }

    MaterialCanvasGraphView::~MaterialCanvasGraphView()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    void MaterialCanvasGraphView::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            GraphCanvas::GraphId activeGraphId = GraphCanvas::GraphId();
            MaterialCanvasDocumentRequestBus::EventResult(
                activeGraphId, m_documentId, &MaterialCanvasDocumentRequestBus::Events::GetGraphId);
            SetActiveGraphId(activeGraphId, true);

            // Show the entire graph and center the view the first time a graph is opened
            if (!m_openedBefore && activeGraphId.IsValid())
            {
                GraphCanvas::ViewId viewId;
                GraphCanvas::SceneRequestBus::EventResult(viewId, activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
                GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
                m_openedBefore = true;
            }
            return;
        }

        SetActiveGraphId(GraphCanvas::GraphId(), false);
    }

    void MaterialCanvasGraphView::OnDocumentClosed([[maybe_unused]] const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            SetActiveGraphId(GraphCanvas::GraphId(), true);
        }
    }

    void MaterialCanvasGraphView::OnDocumentDestroyed([[maybe_unused]] const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            SetActiveGraphId(GraphCanvas::GraphId(), true);
        }
    }

    GraphCanvas::EditorConstructPresets* MaterialCanvasGraphView::GetConstructPresets() const
    {
        return &m_materialCanvasGraphConstructPresets;
    }

    const GraphCanvas::ConstructTypePresetBucket* MaterialCanvasGraphView::GetConstructTypePresetBucket(
        GraphCanvas::ConstructType constructType) const
    {
        return m_materialCanvasGraphConstructPresets.FindPresetBucket(constructType);
    }
} // namespace MaterialCanvas

#include <Window/moc_MaterialCanvasGraphView.cpp>
