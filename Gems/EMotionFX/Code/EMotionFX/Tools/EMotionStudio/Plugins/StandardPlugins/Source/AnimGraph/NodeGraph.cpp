/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/numeric.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendTreeVisualNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNodeFactory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeConnection.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ZoomableLineEdit.h>

namespace EMStudio
{
    static const int sGroupRectContentPadding = 10;
    static const int sGroupRectTextHPadding = 8;
    static const int sGroupRectTextVPadding = 8;
    static const int sGroupRectTextFontSize = 14;
    float NodeGraph::sLowestScale = 0.15f;

    NodeGraph::NodeGraph(const QModelIndex& modelIndex, NodeGraphWidget* graphWidget)
        : QObject()
        , m_graphWidget(graphWidget)
        , m_currentModelIndex(modelIndex)
    {
        QModelIndex parent = m_currentModelIndex;
        while (parent.isValid())
        {
            if (parent.data(AnimGraphModel::ROLE_RTTI_TYPE_ID).value<AZ::TypeId>() == azrtti_typeid<EMotionFX::AnimGraphReferenceNode>())
            {
                m_parentReferenceNode = parent;
                break;
            }
            parent = parent.parent();
        }

        m_errorBlinkOffset = 0.0f;
        m_useAnimation = true;
        m_dashOffset = 0.0f;
        m_scale = 1.0f;
        m_scrollOffset = QPoint(0, 0);
        m_scalePivot = QPoint(0, 0);
        m_minStepSize = 1;
        m_maxStepSize = 75;
        m_entryNode      = nullptr;

        // init connection creation
        m_conStartOffset = QPoint(0, 0);
        m_conEndOffset = QPoint(0, 0);
        m_conPortNr = InvalidIndex16;
        m_conIsInputPort = true;
        m_conPort = nullptr;
        m_conIsValid = false;
        m_targetPort = nullptr;
        m_relinkConnection = nullptr;
        m_replaceTransitionHead = nullptr;
        m_replaceTransitionTail = nullptr;
        m_replaceTransitionSourceNode = nullptr;
        m_replaceTransitionTargetNode = nullptr;
        m_replaceTransitionStartOffset = QPoint(0, 0);
        m_replaceTransitionEndOffset = QPoint(0, 0);

        // setup scroll interpolator
        m_startScrollOffset = QPointF(0.0f, 0.0f);
        m_targetScrollOffset = QPointF(0.0f, 0.0f);
        m_scrollTimer.setSingleShot(false);
        connect(&m_scrollTimer, &QTimer::timeout, this, &NodeGraph::UpdateAnimatedScrollOffset);

        // setup scale interpolator
        m_startScale = 1.0f;
        m_targetScale = 1.0f;
        m_scaleTimer.setSingleShot(false);
        connect(&m_scaleTimer, &QTimer::timeout, this, &NodeGraph::UpdateAnimatedScale);

        m_replaceTransitionValid = false;

        // Overlay
        m_font.setPixelSize(12);
        m_textOptions.setAlignment(Qt::AlignCenter);
        m_fontMetrics = new QFontMetrics(m_font);

        // Group nodes
        m_groupFont.setPixelSize(sGroupRectTextFontSize);
        m_groupFontMetrics.reset(new QFontMetrics(m_font));
        m_nodeGroupNameLineEdit.reset(new ZoomableLineEdit(m_graphWidget));

        connect(m_nodeGroupNameLineEdit.get(), &QLineEdit::editingFinished, this, &NodeGraph::DisableNameEditForNodeGroup);
    }

    // destructor
    NodeGraph::~NodeGraph()
    {
        m_graphNodeByModelIndex.clear();

        delete m_fontMetrics;
    }

    AZStd::vector<GraphNode*> NodeGraph::GetSelectedGraphNodes() const
    {
        AZStd::vector<GraphNode*> nodes;
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = indexAndGraphNode.second.get();
            if (graphNode->GetIsSelected())
            {
                nodes.emplace_back(graphNode);
            }
        }
        return nodes;
    }

    AZStd::vector<EMotionFX::AnimGraphNode*> NodeGraph::GetSelectedAnimGraphNodes() const
    {
        AZStd::vector<EMotionFX::AnimGraphNode*> result;
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = indexAndGraphNode.second.get();
            if (graphNode->GetIsSelected())
            {
                result.push_back(graphNode->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>());
            }
        }
        return result;
    }

    AZStd::vector<NodeConnection*> NodeGraph::GetSelectedNodeConnections() const
    {
        AZStd::vector<NodeConnection*> connections;
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = indexAndGraphNode.second.get();

            // get the number of connections and iterate through them
            const size_t numConnections = graphNode->GetNumConnections();
            for (size_t c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection = graphNode->GetConnection(c);
                if (connection->GetIsSelected())
                {
                    connections.emplace_back(connection);
                }
            }
        }
        return connections;
    }

    bool NodeGraph::GetIsCreatingConnection() const
    {
        return (GetCreateConnectionNode() && !m_relinkConnection);
    }

    bool NodeGraph::GetIsRelinkingConnection() const
    {
        return (GetCreateConnectionNode() && m_relinkConnection);
    }

    void NodeGraph::DrawOverlay(QPainter& painter)
    {
        EMotionFX::AnimGraphInstance* animGraphInstance = m_currentModelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
        if (!animGraphInstance)
        {
            return;
        }

        AnimGraphPlugin* plugin = m_graphWidget->GetPlugin();
        if (plugin->GetDisplayFlags())
        {
            // Go through each node
            for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
            {
                GraphNode* graphNode = indexAndGraphNode.second.get();
                EMotionFX::AnimGraphNode* emfxNode = indexAndGraphNode.first.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                AZ_Assert(emfxNode, "Expecting a valid emfx node");

                if (!graphNode->GetIsVisible())
                {
                    continue;
                }

                // skip non-processed nodes and nodes that have no output pose
#if !defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
                if (!emfxNode->GetHasOutputPose() || !graphNode->GetIsProcessed() || graphNode->GetIsHighlighted())
                {
                    continue;
                }
#endif

                // get the unique data
                EMotionFX::AnimGraphNodeData* uniqueData = emfxNode->FindOrCreateUniqueNodeData(animGraphInstance);

                // draw the background darkened rect
                uint32 requiredHeight = 5;
                const uint32 rectWidth = 155;
                const uint32 heightSpacing = 11;
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYSPEED))
                {
                    requiredHeight += heightSpacing;
                }
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_GLOBALWEIGHT))
                {
                    requiredHeight += heightSpacing;
                }
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_SYNCSTATUS))
                {
                    requiredHeight += heightSpacing;
                }
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYPOSITION))
                {
                    requiredHeight += heightSpacing;
                }
#if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_UPDATE | AnimGraphPlugin::DISPLAYFLAG_PROFILING_TOPDOWN | AnimGraphPlugin::DISPLAYFLAG_PROFILING_POSTUPDATE | AnimGraphPlugin::DISPLAYFLAG_PROFILING_OUTPUT))
                {
                    requiredHeight += heightSpacing;
                }
#endif
                const QRect& nodeRect = graphNode->GetFinalRect();
                const QRect textRect(nodeRect.center().x() - rectWidth / 2, nodeRect.center().y() - requiredHeight / 2, rectWidth, requiredHeight);
                const uint32 alpha = (graphNode->GetIsHighlighted()) ? 225 : 175;
                const QColor backgroundColor(0, 0, 0, alpha);
                painter.setBrush(backgroundColor);
                painter.setPen(Qt::black);
                painter.drawRect(textRect);

                const QColor textColor = graphNode->GetIsHighlighted() ? QColor(0, 255, 0) : QColor(255, 255, 0);
                painter.setPen(textColor);
                painter.setFont(m_font);

                QPoint textPosition = textRect.topLeft();
                textPosition.setX(textPosition.x() + 3);
                textPosition.setY(textPosition.y() + 11);

                // add the playspeed
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYSPEED))
                {
                    m_qtTempString = AZStd::fixed_string<24>::format("Play Speed = %.2f", emfxNode->GetPlaySpeed(animGraphInstance)).c_str();
                    painter.drawText(textPosition, m_qtTempString);
                    textPosition.setY(textPosition.y() + heightSpacing);
                }

                // add the global weight
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_GLOBALWEIGHT))
                {
                    m_qtTempString = AZStd::fixed_string<24>::format("Global Weight = %.2f", uniqueData->GetGlobalWeight()).c_str();
                    painter.drawText(textPosition, m_qtTempString);
                    textPosition.setY(textPosition.y() + heightSpacing);
                }

                // add the sync
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_SYNCSTATUS))
                {
                    m_qtTempString = AZStd::fixed_string<24>::format("Synced = %s", animGraphInstance->GetIsSynced(emfxNode->GetObjectIndex()) ? "Yes" : "No").c_str();
                    painter.drawText(textPosition, m_qtTempString);
                    textPosition.setY(textPosition.y() + heightSpacing);
                }

                // add the play position
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYPOSITION))
                {
                    m_qtTempString = AZStd::fixed_string<32>::format("Play Time = %.3f / %.3f", uniqueData->GetCurrentPlayTime(), uniqueData->GetDuration()).c_str();
                    painter.drawText(textPosition, m_qtTempString);
                    textPosition.setY(textPosition.y() + heightSpacing);
                }

#if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_UPDATE | AnimGraphPlugin::DISPLAYFLAG_PROFILING_TOPDOWN | AnimGraphPlugin::DISPLAYFLAG_PROFILING_POSTUPDATE | AnimGraphPlugin::DISPLAYFLAG_PROFILING_OUTPUT))
                {
                    AZ::u8 profileFlags = 0;
                    profileFlags |= plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_UPDATE) ? static_cast<AZ::u8>(EMotionFX::AnimGraphNode::ProfileMode::Update) : 0;
                    profileFlags |= plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_TOPDOWN) ? static_cast<AZ::u8>(EMotionFX::AnimGraphNode::ProfileMode::TopDown) : 0;
                    profileFlags |= plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_POSTUPDATE) ? static_cast<AZ::u8>(EMotionFX::AnimGraphNode::ProfileMode::PostUpdate) : 0;
                    profileFlags |= plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_OUTPUT) ? static_cast<AZ::u8>(EMotionFX::AnimGraphNode::ProfileMode::Output) : 0;
                    emfxNode->SetProfileMode(profileFlags);
                    const AZ::u32 updateTime = aznumeric_caster(emfxNode->GetUpdateTime(animGraphInstance).count());
                    const AZ::u32 totalUpdateTime = aznumeric_caster(emfxNode->GetTotalUpdateTime(animGraphInstance).count());
                    m_qtTempString = AZStd::fixed_string<32>::format("Update = %u (%u) us", updateTime, totalUpdateTime).c_str();
                    painter.drawText(textPosition, m_qtTempString);
                    textPosition.setY(textPosition.y() + heightSpacing);

                    emfxNode->ClearProfileTimers(animGraphInstance);
                }
#endif
            }
        }

        if (GetScale() < 0.5f)
        {
            return;
        }

        // get the active graph and the corresponding emfx node and return if they are invalid or in case the opened node is no blend tree
        EMotionFX::AnimGraphNode* currentNode = m_currentModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        if (azrtti_typeid(currentNode) == azrtti_typeid<EMotionFX::BlendTree>())
        {
            // iterate through the nodes
            for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
            {
                GraphNode* graphNode = indexAndGraphNode.second.get();

                // All the connections are stored in the downstream node, so the target node is constant
                // across all connections
                EMotionFX::AnimGraphNode* emfxTargetNode = indexAndGraphNode.first.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

                // iterate through all connections connected to this node
                const size_t numConnections = graphNode->GetNumConnections();
                for (size_t c = 0; c < numConnections; ++c)
                {
                    NodeConnection*             visualConnection = graphNode->GetConnection(c);

                    // get the source and target nodes
                    GraphNode*                  sourceNode = visualConnection->GetSourceNode();
                    EMotionFX::AnimGraphNode*   emfxSourceNode = sourceNode->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

                    // only show values for connections that are processed
                    if (visualConnection->GetIsProcessed() == false)
                    {
                        continue;
                    }

                    const AZ::u16 inputPortNr = visualConnection->GetInputPortNr();
                    const AZ::u16 outputPortNr = visualConnection->GetOutputPortNr();
                    MCore::Attribute* attribute = emfxSourceNode->GetOutputValue(animGraphInstance, outputPortNr);

                    // fill the string with data
                    m_tempStringA.clear();
                    switch (attribute->GetType())
                    {
                    // float attributes
                    case MCore::AttributeFloat::TYPE_ID:
                    {
                        MCore::AttributeFloat* floatAttribute = static_cast<MCore::AttributeFloat*>(attribute);
                        m_tempStringA = AZStd::string::format("%.2f", floatAttribute->GetValue());
                        break;
                    }

                    // vector 2 attributes
                    case MCore::AttributeVector2::TYPE_ID:
                    {
                        MCore::AttributeVector2* vecAttribute = static_cast<MCore::AttributeVector2*>(attribute);
                        const AZ::Vector2& vec = vecAttribute->GetValue();
                        m_tempStringA = AZStd::string::format("(%.2f, %.2f)", static_cast<float>(vec.GetX()), static_cast<float>(vec.GetY()));
                        break;
                    }

                    // vector 3 attributes
                    case MCore::AttributeVector3::TYPE_ID:
                    {
                        MCore::AttributeVector3* vecAttribute = static_cast<MCore::AttributeVector3*>(attribute);
                        const AZ::Vector3& vec = vecAttribute->GetValue();
                        m_tempStringA = AZStd::string::format("(%.2f, %.2f, %.2f)", static_cast<float>(vec.GetX()), static_cast<float>(vec.GetY()), static_cast<float>(vec.GetZ()));
                        break;
                    }

                    // vector 4 attributes
                    case MCore::AttributeVector4::TYPE_ID:
                    {
                        MCore::AttributeVector4* vecAttribute = static_cast<MCore::AttributeVector4*>(attribute);
                        const AZ::Vector4& vec = vecAttribute->GetValue();
                        m_tempStringA = AZStd::string::format("(%.2f, %.2f, %.2f, %.2f)", static_cast<float>(vec.GetX()), static_cast<float>(vec.GetY()), static_cast<float>(vec.GetZ()), static_cast<float>(vec.GetW()));
                        break;
                    }

                    // boolean attributes
                    case MCore::AttributeBool::TYPE_ID:
                    {
                        MCore::AttributeBool* boolAttribute = static_cast<MCore::AttributeBool*>(attribute);
                        m_tempStringA = AZStd::string::format("%s", AZStd::to_string(boolAttribute->GetValue()).c_str());
                        break;
                    }

                    // rotation attributes
                    case MCore::AttributeQuaternion::TYPE_ID:
                    {
                        MCore::AttributeQuaternion* quatAttribute = static_cast<MCore::AttributeQuaternion*>(attribute);
                        const AZ::Vector3 eulerAngles = MCore::AzQuaternionToEulerAngles(quatAttribute->GetValue());
                        m_tempStringA = AZStd::string::format("(%.2f, %.2f, %.2f)", static_cast<float>(eulerAngles.GetX()), static_cast<float>(eulerAngles.GetY()), static_cast<float>(eulerAngles.GetZ()));
                        break;
                    }


                    // pose attribute
                    case EMotionFX::AttributePose::TYPE_ID:
                    {
                        // handle blend 2 nodes
                        if (azrtti_typeid(emfxTargetNode) == azrtti_typeid<EMotionFX::BlendTreeBlend2Node>())
                        {
                            // type-cast the target node to our blend node
                            EMotionFX::BlendTreeBlend2Node* blendNode = static_cast<EMotionFX::BlendTreeBlend2Node*>(emfxTargetNode);

                            // get the weight from the input port
                            float weight = blendNode->GetInputNumberAsFloat(animGraphInstance, EMotionFX::BlendTreeBlend2Node::INPUTPORT_WEIGHT);
                            weight = MCore::Clamp<float>(weight, 0.0f, 1.0f);

                            // map the weight to the connection
                            if (inputPortNr == 0)
                            {
                                m_tempStringA = AZStd::string::format("%.2f", 1.0f - weight);
                            }
                            else
                            {
                                m_tempStringA = AZStd::string::format("%.2f", weight);
                            }
                        }
                        // handle blend N nodes
                        else if (azrtti_typeid(emfxTargetNode) == azrtti_typeid<EMotionFX::BlendTreeBlendNNode>())
                        {
                            // type-cast the target node to our blend node
                            EMotionFX::BlendTreeBlendNNode* blendNode = static_cast<EMotionFX::BlendTreeBlendNNode*>(emfxTargetNode);

                            // get two nodes that we receive input poses from, and get the blend weight
                            float weight;
                            EMotionFX::AnimGraphNode* nodeA;
                            EMotionFX::AnimGraphNode* nodeB;
                            uint32 poseIndexA;
                            uint32 poseIndexB;
                            blendNode->FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &weight);

                            // map the weight to the connection
                            if (inputPortNr == poseIndexA)
                            {
                                m_tempStringA = AZStd::string::format("%.2f", 1.0f - weight);
                            }
                            else
                            {
                                m_tempStringA = AZStd::string::format("%.2f", weight);
                            }
                        }
                        break;
                    }

                    default:
                    {
                        attribute->ConvertToString(m_mcoreTempString);
                        m_tempStringA = m_mcoreTempString.c_str();
                    }
                    }

                    // only display the value in case it is not empty
                    if (!m_tempStringA.empty())
                    {
                        QPoint connectionAttachPoint = visualConnection->CalcFinalRect().center();

                        const int halfTextHeight = 6;
                        const int textWidth = m_fontMetrics->horizontalAdvance(m_tempStringA.c_str());
                        const int halfTextWidth = textWidth / 2;

                        const QRect textRect(connectionAttachPoint.x() - halfTextWidth - 1, connectionAttachPoint.y() - halfTextHeight, textWidth + 4, halfTextHeight * 2);
                        QPoint textPosition = textRect.bottomLeft();
                        textPosition.setY(textPosition.y() - 1);
                        textPosition.setX(textPosition.x() + 2);

                        const QColor backgroundColor(30, 30, 30);

                        // draw the background rect for the text
                        painter.setBrush(backgroundColor);
                        painter.setPen(Qt::black);
                        painter.drawRect(textRect);

                        // draw the text
                        const QColor& color = visualConnection->GetTargetNode()->GetInputPort(visualConnection->GetInputPortNr())->GetColor();
                        painter.setPen(color);
                        painter.setFont(m_font);
                        GraphNode::RenderText(painter, m_tempStringA.c_str(), color, m_font, *m_fontMetrics, Qt::AlignCenter, textRect);
                    }
                }
            }
        }
    }

    void NodeGraph::RenderEntryPoint(QPainter& painter, GraphNode* node)
    {
        if (node == nullptr)
        {
            return;
        }

        QPen oldPen = painter.pen();
        QColor color(150, 150, 150);
        QPen newPen(color);
        newPen.setWidth(3);
        painter.setBrush(color);
        painter.setPen(color);

        int32   arrowLength     = 30;
        int32   circleSize      = 4;
        QRect   rect            = node->GetRect();
        QPoint  start           = rect.topLeft() + QPoint(-arrowLength, 0) + QPoint(0, rect.height() / 2);
        QPoint  end             = rect.topLeft() + QPoint(0, rect.height() / 2);


        // calculate the line direction
        AZ::Vector2 lineDir = AZ::Vector2(aznumeric_cast<float>(end.x()), aznumeric_cast<float>(end.y())) - AZ::Vector2(aznumeric_cast<float>(start.x()), aznumeric_cast<float>(start.y()));
        float length = lineDir.GetLength();
        lineDir.Normalize();

        // draw the arrow
        QPointF direction;
        direction.setX(lineDir.GetX() * 10.0f);
        direction.setY(lineDir.GetY() * 10.0f);

        QPointF normalOffset((end.y() - start.y()) / length, (start.x() - end.x()) / length);

        QPointF points[3];
        points[0] = end;
        points[1] = end - direction + (normalOffset * 6.7f);
        points[2] = end - direction - (normalOffset * 6.7f);

        painter.drawPolygon(points, 3);

        // draw the end circle
        painter.drawEllipse(start, circleSize, circleSize);

        // draw the arror line
        painter.setPen(newPen);
        painter.drawLine(start, end + QPoint(-5, 0));

        painter.setPen(oldPen);
    }

    void NodeGraph::DrawSmoothedLineFast(QPainter& painter, int32 x1, int32 y1, int32 x2, int32 y2, int32 stepSize)
    {
        // special case where there is just one line up
        if (x1 == x2 || y1 == y2)
        {
            painter.drawLine(x1, y1, x2, y2);
        }
        else if (x2 > x1)
        {
            // draw the lines
            int32 lastX = x2;
            int32 lastY = y2;
            int32 x = x2;

            while (x < x1)
            {
                const float t =
                    MCore::CalcCosineInterpolationWeight((x - x2) / (float)(x1 - x2)); // calculate the smooth interpolated value
                const int32 y = aznumeric_cast<int32>(y2 + (y1 - y2) * t); // calculate the y coordinate
                painter.drawLine(lastX, lastY, x, y); // draw the line
                lastX = x;
                lastY = y;
                x += stepSize;
            }

            const float t = MCore::CalcCosineInterpolationWeight(1.0f); // calculate the smooth interpolated value
            const int32 y = aznumeric_cast<int32>(y2 + (y1 - y2) * t); // calculate the y coordinate
            painter.drawLine(lastX, lastY, x1, y); // draw the line
        }
        else
        {
            // find the min and max points
            int32 minY, maxY, startX, endX;
            if (y1 <= y2)
            {
                minY = y1;
                maxY = y2;
                startX = x1;
                endX = x2;
            }
            else
            {
                minY = y2;
                maxY = y1;
                startX = x2;
                endX = x1;
            }

            // draw the lines
            int32 lastY = minY;
            int32 lastX = startX;

            int32 y = minY;
            while (y < maxY)
            {
                const float t =
                    MCore::CalcCosineInterpolationWeight((y - minY) / (float)(maxY - minY)); // calculate the smooth interpolated value
                const int32 x = aznumeric_cast<int32>(startX + (endX - startX) * t); // calculate the y coordinate
                painter.drawLine(lastX, lastY, x, y); // draw the line
                lastX = x;
                lastY = y;
                y += stepSize;
            }

            const float t = MCore::CalcCosineInterpolationWeight(1.0f); // calculate the smooth interpolated value
            const int32 x = aznumeric_cast<int32>(startX + (endX - startX) * t); // calculate the y coordinate
            painter.drawLine(lastX, lastY, x, maxY); // draw the line
        }
    }

    void NodeGraph::UpdateNodesAndConnections(int32 width, int32 height, const QPoint& mousePos)
    {
        // calculate the visible rect
        const QRect visibleRect(0, 0, width, height);

        // update the nodes
        for (const AZStd::pair<QPersistentModelIndex, AZStd::unique_ptr<GraphNode> >& modelIndexAndGraphNode : m_graphNodeByModelIndex)
        {
            modelIndexAndGraphNode.second->Update(visibleRect, mousePos);
        }
    }


    // find the connection at the given mouse position
    NodeConnection* NodeGraph::FindConnection(const QPoint& mousePos)
    {
        // for all nodes in the graph
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = indexAndGraphNode.second.get();

            // iterate over all connections
            const size_t numConnections = graphNode->GetNumConnections();
            for (size_t c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection  = graphNode->GetConnection(c);
                if (connection->CheckIfIsCloseTo(mousePos))
                {
                    return connection;
                }
            }
        }

        // failure, there is no connection at the given mouse position
        return nullptr;
    }


    void NodeGraph::UpdateHighlightConnectionFlags(const QPoint& mousePos)
    {
        bool highlightedConnectionFound = false;

        // for all nodes in the graph
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = indexAndGraphNode.second.get();

            // iterate over all connections
            const size_t numConnections = graphNode->GetNumConnections();
            for (size_t c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection  = graphNode->GetConnection(c);
                GraphNode*      sourceNode  = connection->GetSourceNode();
                GraphNode*      targetNode  = connection->GetTargetNode();

                // set the highlight flag
                // Note: connections get reset in the Connection::Update() method already
                if (highlightedConnectionFound == false && connection->CheckIfIsCloseTo(mousePos))
                {
                    highlightedConnectionFound = true;
                    connection->SetIsHighlighted(true);

                    connection->SetIsHeadHighlighted(connection->CheckIfIsCloseToHead(mousePos));
                    connection->SetIsTailHighlighted(connection->CheckIfIsCloseToTail(mousePos));
                }
                else
                {
                    connection->SetIsHeadHighlighted(false);
                    connection->SetIsTailHighlighted(false);
                }

                if (m_replaceTransitionHead == connection)
                {
                    connection->SetIsHeadHighlighted(true);
                }

                if (m_replaceTransitionTail == connection)
                {
                    connection->SetIsTailHighlighted(true);
                }

                // enable highlighting if either the source or the target node is selected
                if (sourceNode && sourceNode->GetIsSelected())
                {
                    connection->SetIsConnectedHighlighted(true);
                }

                if (targetNode->GetIsSelected())
                {
                    connection->SetIsConnectedHighlighted(true);
                }

                // or in case the source or target node are highlighted
                if (targetNode->GetIsHighlighted() || (sourceNode && sourceNode->GetIsHighlighted()))
                {
                    connection->SetIsHighlighted(true);
                }
            }
        }
    }


    //#define GRAPH_PERFORMANCE_INFO
    //#define GRAPH_PERFORMANCE_FRAMEDURATION

    void NodeGraph::Render(const QItemSelectionModel& selectionModel, QPainter& painter, int32 width, int32 height, const QPoint& mousePos, float timePassedInSeconds)
    {
        // control the scroll speed of the dashed blend tree connections etc
        m_dashOffset -= 7.5f * timePassedInSeconds;
        m_errorBlinkOffset += 5.0f * timePassedInSeconds;

#ifdef GRAPH_PERFORMANCE_FRAMEDURATION
        MCore::Timer timer;
#endif

        // calculate the visible rect
        QRect visibleRect;
        visibleRect = QRect(0, 0, width, height);
        //visibleRect.adjust(50, 50, -50, -50);

        // setup the transform
        m_transform.reset();
        m_transform.translate(m_scalePivot.x(), m_scalePivot.y());
        m_transform.scale(m_scale, m_scale);
        m_transform.translate(-m_scalePivot.x() + m_scrollOffset.x(), -m_scalePivot.y() + m_scrollOffset.y());
        painter.setTransform(m_transform);

        // render the background
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer backgroundTimer;
        backgroundTimer.GetTimeDelta();
#endif
        RenderBackground(painter, width, height);
#ifdef GRAPH_PERFORMANCE_INFO
        const float backgroundTime = backgroundTimer.GetTimeDelta();
        MCore::LogInfo("   Background: %.2f ms", backgroundTime * 1000);
#endif

        // update the nodes
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer updateTimer;
#endif
        UpdateNodesAndConnections(width, height, mousePos);
        UpdateHighlightConnectionFlags(mousePos); // has to come after nodes and connections are updated
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::LogInfo("   Update: %.2f ms", updateTimer.GetTime() * 1000);
#endif

        // render the node groups
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer groupsTimer;
#endif
        RenderNodeGroups(painter);
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::LogInfo("   Groups: %.2f ms", groupsTimer.GetTime() * 1000);
#endif

        // calculate the connection stepsize
        // the higher the value, the less lines it renders (so faster)
        int32 stepSize = aznumeric_cast<int32>(((1.0f / (m_scale * (m_scale * 1.75f))) * 10) - 7);
        stepSize = MCore::Clamp<int32>(stepSize, m_minStepSize, m_maxStepSize);

        QRect scaledVisibleRect = m_transform.inverted().mapRect(visibleRect);

        bool renderShadow = false;
        if (GetScale() >= 0.3f)
        {
            renderShadow = true;
        }

        // render connections
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer connectionsTimer;
#endif
        QPen connectionsPen;
        QBrush connectionsBrush;
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = indexAndGraphNode.second.get();
            graphNode->RenderConnections(selectionModel, painter, &connectionsPen, &connectionsBrush, scaledVisibleRect, stepSize);
        }
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::LogInfo("   Connections: %.2f ms", connectionsTimer.GetTime() * 1000);
#endif

        // render all nodes
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::Timer nodesTimer;
#endif
        QPen nodesPen;
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = indexAndGraphNode.second.get();
            graphNode->Render(painter, &nodesPen, renderShadow);
        }
#ifdef GRAPH_PERFORMANCE_INFO
        MCore::LogInfo("   Nodes: %.2f ms", nodesTimer.GetTime() * 1000);
#endif

        // render the connection we are creating, if any
        RenderCreateConnection(painter);

        RenderReplaceTransition(painter);
        StateConnection::RenderInterruptedTransitions(painter, GetAnimGraphModel(), *this);

        // render the entry state arrow
        RenderEntryPoint(painter, m_entryNode);

#ifdef GRAPH_PERFORMANCE_FRAMEDURATION
        MCore::LogInfo("GraphRenderingTime: %.2f ms.", timer.GetTime() * 1000);
#endif

        RenderTitlebar(painter, width);
    }

    void NodeGraph::RenderTitlebar(QPainter& painter, const QString& text, int32 width)
    {
        painter.save();
        painter.resetTransform();

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0));
        painter.setOpacity(0.25f);
        const QPoint upperLeft(0, 0);
        const QPoint bottomRight(width, 24);
        const QRect titleRect = QRect(upperLeft, bottomRight);
        painter.drawRect(titleRect);

        painter.setOpacity(1.0f);
        painter.setPen(QColor(233, 233, 233));
        painter.setFont(m_font);
        painter.drawText(titleRect, text, QTextOption(Qt::AlignCenter));

        painter.restore();
    }

    void NodeGraph::RenderTitlebar(QPainter& painter, int32 width)
    {
        const QString& titleBarText = m_graphWidget->GetTitleBarText();
        if (m_parentReferenceNode.isValid())
        {
            EMotionFX::AnimGraphNode* node = m_parentReferenceNode.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            EMotionFX::AnimGraphReferenceNode* referenceNode = static_cast<EMotionFX::AnimGraphReferenceNode*>(node);
            EMotionFX::AnimGraph* referencedAnimGraph = referenceNode->GetReferencedAnimGraph();

            QString titleLabel;
            if (!referencedAnimGraph)
            {
                // If referencedAnimGraph does not exist, exit reference node.
                AnimGraphPlugin* plugin = m_graphWidget->GetPlugin();
                if (plugin != nullptr)
                {
                    const QModelIndex& modelIndex = m_parentReferenceNode.parent();
                    plugin->GetAnimGraphModel().Focus(modelIndex);
                }
                return;
            }
            // If the reference anim graph is in an error state ( probably due to circular dependency ), we should show some error message.
            else if (referenceNode->GetHasCycles())
            {
                titleLabel = QString("Can't show the reference anim graph because cicular dependency.");
            }
            else
            {
                AZStd::string filename;
                AzFramework::StringFunc::Path::GetFullFileName(referencedAnimGraph->GetFileName(), filename);
                titleLabel = QString("Referenced graph: '%1' (read-only)").arg(filename.c_str());
            }

            RenderTitlebar(painter, titleLabel, width);
        }
        else if (!titleBarText.isEmpty())
        {
            RenderTitlebar(painter, titleBarText, width);
        }
    }

    void NodeGraph::SelectNodesInRect(const QRect& rect, bool overwriteCurSelection, bool toggleMode)
    {
        QItemSelectionModel& selectionModel = GetAnimGraphModel().GetSelectionModel();
        const QModelIndexList oldSelectionModelIndices = selectionModel.selectedRows();

        QItemSelection newSelection;
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            const QModelIndex& modelIndex = indexAndGraphNode.first;
            GraphNode* node = indexAndGraphNode.second.get();
            const bool nodePreviouslySelected = std::find(oldSelectionModelIndices.begin(), oldSelectionModelIndices.end(), modelIndex) != oldSelectionModelIndices.end();
            const bool nodeNewlySelected = node->GetRect().intersects(rect);

            AnimGraphModel::AddToItemSelection(newSelection, modelIndex, nodePreviouslySelected, nodeNewlySelected, toggleMode, overwriteCurSelection);

            const size_t numConnections = node->GetNumConnections();
            for (size_t c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection = node->GetConnection(c);
                const bool connectionPreviouslySelected = std::find(oldSelectionModelIndices.begin(), oldSelectionModelIndices.end(), connection->GetModelIndex()) != oldSelectionModelIndices.end();
                const bool connectionNewlySelected = connection->Intersects(rect);

                AnimGraphModel::AddToItemSelection(newSelection, connection->GetModelIndex(), connectionPreviouslySelected, connectionNewlySelected, toggleMode, overwriteCurSelection);
            }
        }

        selectionModel.select(newSelection, QItemSelectionModel::Current | QItemSelectionModel::Rows | QItemSelectionModel::Clear | QItemSelectionModel::Select);
    }

    void NodeGraph::SelectAllNodes()
    {
        QItemSelection selection;
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            selection.select(indexAndGraphNode.first, indexAndGraphNode.first);
        }

        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            const QModelIndex& modelIndex = indexAndGraphNode.first;
            const int rows = modelIndex.model()->rowCount(modelIndex);
            for (int row = 0; row < rows; ++row)
            {
                const QModelIndex childConnection = modelIndex.model()->index(row, 0, modelIndex);
                selection.select(childConnection, childConnection);
            }
        }

        GetAnimGraphModel().GetSelectionModel().select(selection, QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }


    // find the node
    GraphNode* NodeGraph::FindNode(const QPoint& globalPoint)
    {
        // for all nodes
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* node = indexAndGraphNode.second.get();
            // check if the point is inside the node rect
            if (node->GetIsInside(globalPoint))
            {
                return node;
            }
        }

        // not found
        return nullptr;
    }


    // unselect all nodes
    void NodeGraph::UnselectAllNodes()
    {
        m_graphWidget->GetPlugin()->GetAnimGraphModel().GetSelectionModel().clearSelection();
    }

    void NodeGraph::SelectConnectionCloseTo(const QPoint& point, bool overwriteCurSelection, bool toggle)
    {
        QItemSelectionModel& selectionModel = GetAnimGraphModel().GetSelectionModel();
        const QModelIndexList oldSelectionModelIndices = selectionModel.selectedRows();
        QItemSelection newSelection;

        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* node = indexAndGraphNode.second.get();

            const size_t numConnections = node->GetNumConnections();
            for (size_t c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection = node->GetConnection(c);
                const bool isNewlySelected = connection->CheckIfIsCloseTo(point);
                const bool isPreviouslySelected = std::find(oldSelectionModelIndices.begin(), oldSelectionModelIndices.end(), connection->GetModelIndex()) != oldSelectionModelIndices.end();

                AnimGraphModel::AddToItemSelection(newSelection, connection->GetModelIndex(), isPreviouslySelected, isNewlySelected, toggle, overwriteCurSelection);
            }
        }

        selectionModel.select(newSelection, QItemSelectionModel::Current | QItemSelectionModel::Rows | QItemSelectionModel::Clear | QItemSelectionModel::Select);
    }

    void NodeGraph::RenderBackground(QPainter& painter, int32 width, int32 height)
    {
        // grid line color
        painter.setPen(QColor(40, 40, 40));

        // calculate the coordinates in 'zoomed out and scrolled' coordinates, of the window rect
        QPoint upperLeft    = m_transform.inverted().map(QPoint(0, 0));
        QPoint lowerRight   = m_transform.inverted().map(QPoint(width, height));

        // calculate the start and end ranges in 'scrolled and zoomed out' coordinates
        // we need to render sub-grids covering that area
        const int32 startX  = upperLeft.x() - (upperLeft.x() % 100) - 100;
        const int32 startY  = upperLeft.y() - (upperLeft.y() % 100) - 100;
        const int32 endX    = lowerRight.x();
        const int32 endY    = lowerRight.y();
        /*
            // render the subgrid patches
            for (int32 x=startX; x<endX; x+=300)
                for (int32 y=startY; y<endY; y+=300)
                    RenderSubGrid(painter, x, y);
        */

        // calculate the alpha
        float scale = m_scale * m_scale * 1.5f;
        scale = MCore::Clamp<float>(scale, 0.0f, 1.0f);
        const int32 alpha = aznumeric_cast<int32>(MCore::CalcCosineInterpolationWeight(scale) * 255);

        if (alpha < 10)
        {
            return;
        }

        m_gridPen.setColor(QColor(61, 61, 61, alpha));
        m_subgridPen.setColor(QColor(55, 55, 55, alpha));

        // setup spacing and size of the grid
        const int32 spacing = 10;       // grid cell size of 20

        // draw subgridlines first
        painter.setPen(m_subgridPen);

        // draw vertical lines
        for (int32 x = startX; x < endX; x += spacing)
        {
            if ((x - startX) % 100 != 0)
            {
                painter.drawLine(x, startY, x, endY);
            }
        }

        // draw horizontal lines
        for (int32 y = startY; y < endY; y += spacing)
        {
            if ((y - startY) % 100 != 0)
            {
                painter.drawLine(startX, y, endX, y);
            }
        }

        // draw render grid lines
        painter.setPen(m_gridPen);

        // draw vertical lines
        for (int32 x = startX; x < endX; x += spacing)
        {
            if ((x - startX) % 100 == 0)
            {
                painter.drawLine(x, startY, x, endY);
            }
        }

        // draw horizontal lines
        for (int32 y = startY; y < endY; y += spacing)
        {
            if ((y - startY) % 100 == 0)
            {
                painter.drawLine(startX, y, endX, y);
            }
        }
    }

    // NOTE: Based on code from: http://alienryderflex.com/intersect/
    //
    //  Determines the intersection point of the line segment defined by points A and B
    //  with the line segment defined by points C and D.
    //
    //  Returns true if the intersection point was found, and stores that point in X,Y.
    //  Returns false if there is no determinable intersection point, in which case X,Y will
    //  be unmodified.
    bool NodeGraph::LinesIntersect(double Ax, double Ay, double Bx, double By,
        double Cx, double Cy, double Dx, double Dy,
        double* X, double* Y)
    {
        double distAB, theCos, theSin, newX, ABpos;

        //  Fail if either line segment is zero-length.
        if ((Ax == Bx && Ay == By) || (Cx == Dx && Cy == Dy))
        {
            return false;
        }

        //  Fail if the segments share an end-point.
        if ((Ax == Cx && Ay == Cy) || (Bx == Cx && By == Cy) || (Ax == Dx && Ay == Dy) || (Bx == Dx && By == Dy))
        {
            return false;
        }

        //  (1) Translate the system so that point A is on the origin.
        Bx -= Ax;
        By -= Ay;
        Cx -= Ax;
        Cy -= Ay;
        Dx -= Ax;
        Dy -= Ay;

        //  Discover the length of segment A-B.
        distAB = sqrt(Bx * Bx + By * By);

        //  (2) Rotate the system so that point B is on the positive X axis.
        theCos  = Bx / distAB;
        theSin  = By / distAB;
        newX    = Cx * theCos + Cy * theSin;
        Cy      = Cy * theCos - Cx * theSin;
        Cx      = newX;
        newX    = Dx * theCos + Dy * theSin;
        Dy      = Dy * theCos - Dx * theSin;
        Dx      = newX;

        //  Fail if segment C-D doesn't cross line A-B.
        if ((Cy < 0.0 && Dy < 0.0) || (Cy >= 0.0 && Dy >= 0.0))
        {
            return false;
        }

        //  (3) Discover the position of the intersection point along line A-B.
        ABpos = Dx + (Cx - Dx) * Dy / (Dy - Cy);

        //  Fail if segment C-D crosses line A-B outside of segment A-B.
        if (ABpos < 0.0 || ABpos > distAB)
        {
            return false;
        }

        //  (4) Apply the discovered position to line A-B in the original coordinate system.
        if (X)
        {
            *X = Ax + ABpos * theCos;
        }
        if (Y)
        {
            *Y = Ay + ABpos * theSin;
        }

        // intersection found
        return true;
    }


    // check intersection between line and rect
    bool NodeGraph::LineIntersectsRect(const QRect& b, float x1, float y1, float x2, float y2, double* outX, double* outY)
    {
        // check first if any of the points are inside the rect
        if (outX == nullptr && outY == nullptr)
        {
            if (b.contains(QPoint(aznumeric_cast<int>(x1), aznumeric_cast<int>(y1))) || b.contains(QPoint(aznumeric_cast<int>(x2), aznumeric_cast<int>(y2))))
            {
                return true;
            }
        }

        // if not test for intersection with the line segments
        // check the top
        if (LinesIntersect(x1, y1, x2, y2, b.topLeft().x(), b.topLeft().y(), b.topRight().x(), b.topRight().y(), outX, outY))
        {
            return true;
        }

        // check the bottom
        if (LinesIntersect(x1, y1, x2, y2, b.bottomLeft().x(), b.bottomLeft().y(), b.bottomRight().x(), b.bottomRight().y(), outX, outY))
        {
            return true;
        }

        // check the left
        if (LinesIntersect(x1, y1, x2, y2, b.topLeft().x(), b.topLeft().y(), b.bottomLeft().x(), b.bottomLeft().y(), outX, outY))
        {
            return true;
        }

        // check the right
        if (LinesIntersect(x1, y1, x2, y2, b.topRight().x(), b.topRight().y(), b.bottomRight().x(), b.bottomRight().y(), outX, outY))
        {
            return true;
        }

        return false;
    }


    // distance to a line
    float NodeGraph::DistanceToLine(float x1, float y1, float x2, float y2, float px, float py)
    {
        const AZ::Vector2 pos(px, py);
        const AZ::Vector2 lineStart(x1, y1);
        const AZ::Vector2 lineEnd(x2, y2);

        // a vector from start to end of the line
        const AZ::Vector2 startToEnd = lineEnd - lineStart;

        // the distance of pos projected on the line
        float t = (pos - lineStart).Dot(startToEnd) / startToEnd.GetLengthSq();

        // make sure that we clip this distance to be sure its on the line segment
        if (t < 0.0f)
        {
            t = 0.0f;
        }
        if (t > 1.0f)
        {
            t = 1.0f;
        }

        // calculate the position projected on the line
        const AZ::Vector2 projected = lineStart + t * startToEnd;

        // the vector from the projected position to the point we are testing with
        return (pos - projected).GetLength();
    }


    // calc the number of selected nodes
    size_t NodeGraph::CalcNumSelectedNodes() const
    {
        return AZStd::accumulate(begin(m_graphNodeByModelIndex), end(m_graphNodeByModelIndex), size_t{0}, [](size_t total, const auto& indexAndGraphNode)
        {
            return total + indexAndGraphNode.second->GetIsSelected();
        });
    }


    // calc the selection rect
    QRect NodeGraph::CalcRectFromSelection(bool includeConnections) const
    {
        QRect result;

        // for all nodes
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* node = indexAndGraphNode.second.get();

            // add the rect
            if (node->GetIsSelected())
            {
                result = result.united(node->GetRect());
            }

            // if we want to include connections in the rect
            if (includeConnections)
            {
                // for all connections
                const size_t numConnections = node->GetNumConnections();
                for (size_t c = 0; c < numConnections; ++c)
                {
                    if (node->GetConnection(c)->GetIsSelected())
                    {
                        result = result.united(node->GetConnection(c)->CalcRect());
                    }
                }
            }
        }

        return result;
    }


    // calculate the rect from the entire graph
    QRect NodeGraph::CalcRectFromGraph() const
    {
        QRect result;

        // for all nodes
        for (const AZStd::pair<QPersistentModelIndex, AZStd::unique_ptr<GraphNode> >& modelIndexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = modelIndexAndGraphNode.second.get();

            // add the rect
            result |= graphNode->GetRect();

            // for all connections
            const size_t numConnections = graphNode->GetNumConnections();
            for (size_t c = 0; c < numConnections; ++c)
            {
                result |= graphNode->GetConnection(c)->CalcRect();
            }
        }

        return result;
    }


    // make the given rect visible
    void NodeGraph::ZoomOnRect(const QRect& rect, int32 width, int32 height, bool animate)
    {
        QRect localRect = rect;

        // calculate the space left after we move this the rect to the upperleft of the screen
        const int32 widthLeft   = width - localRect.width();
        const int32 heightLeft  = height - localRect.height();

        //MCore::LOG("Screen: width=%d, height=%d, Rect: width=%d, height=%d", width, height, localRect.width(), localRect.height());

        if (widthLeft > 0 && heightLeft > 0)
        {
            // center the rect in the middle of the screen
            QPoint offset;
            const int32 left = localRect.left();
            const int32 top = localRect.top();
            offset.setX(-left + widthLeft / 2);
            offset.setY(-top  + heightLeft / 2);

            if (animate)
            {
                ZoomTo(1.0f);
                ScrollTo(offset);
            }
            else
            {
                m_scrollOffset = offset;
                m_scale = 1.0f;
            }
        }
        else
        {
            // grow the rect a bit to keep some empty space around the borders
            localRect.adjust(-5, -5, 5, 5);

            // put the center of the selection in the middle of the screen
            QPoint offset = -localRect.center() + QPoint(width / 2, height / 2);
            if (animate)
            {
                ScrollTo(offset);
            }
            else
            {
                m_scrollOffset = offset;
            }

            // set the zoom factor so it exactly fits
            // find out how many extra pixels we need to fit on screen
            const int32 widthDif  = localRect.width()  - width;
            const int32 heightDif = localRect.height() - height;

            // calculate how much zoom out we need for width and height
            float widthZoom  = 1.0f;
            float heightZoom = 1.0f;

            if (widthDif > 0)
            {
                widthZoom  = 1.0f / ((widthDif / (float)width) + 1.0f);
            }

            if (heightDif > 0)
            {
                heightZoom = 1.0f / ((heightDif / (float)height) + 1.0f);
            }

            if (animate == false)
            {
                m_scale = MCore::Min<float>(widthZoom, heightZoom);
            }
            else
            {
                ZoomTo(MCore::Min<float>(widthZoom, heightZoom));
            }
        }
    }


    // start an animated scroll to the given scroll offset
    void NodeGraph::ScrollTo(const QPointF& point)
    {
        m_startScrollOffset  = m_scrollOffset;
        m_targetScrollOffset = point;
        m_scrollTimer.start(1000 / 60);
        m_scrollPreciseTimer.Stamp();
    }


    // update the animated scroll offset
    void NodeGraph::UpdateAnimatedScrollOffset()
    {
        const float duration = 0.75f; // duration in seconds

        float timePassed = m_scrollPreciseTimer.GetDeltaTimeInSeconds();
        if (timePassed > duration)
        {
            timePassed = duration;
            m_scrollTimer.stop();
        }

        const float t = timePassed / duration;
        m_scrollOffset = MCore::CosineInterpolate<QPointF>(m_startScrollOffset, m_targetScrollOffset, t).toPoint();
    }


    // update the animated scale
    void NodeGraph::UpdateAnimatedScale()
    {
        const float duration = 0.75f; // duration in seconds

        float timePassed = m_scalePreciseTimer.GetDeltaTimeInSeconds();
        if (timePassed > duration)
        {
            timePassed = duration;
            m_scaleTimer.stop();
        }

        const float t = timePassed / duration;
        m_scale = MCore::CosineInterpolate<float>(m_startScale, m_targetScale, t);
    }

    //static float scaleExp = 1.0f;

    // zoom in
    void NodeGraph::ZoomIn()
    {
        //scaleExp += 0.2f;
        //if (scaleExp > 1.0f)
        //scaleExp = 1.0f;

        //float t = -6 + (6 * scaleExp);
        //float newScale = 1/(1+exp(-t)) * 2;

        float newScale = m_scale + 0.35f;
        newScale = MCore::Clamp<float>(newScale, sLowestScale, 1.0f);
        ZoomTo(newScale);
    }



    // zoom out
    void NodeGraph::ZoomOut()
    {
        float newScale = m_scale - 0.35f;
        //scaleExp -= 0.2f;
        //if (scaleExp < 0.01f)
        //scaleExp = 0.01f;

        //  float t = -6 + (6 * scaleExp);
        //float newScale = 1/(1+exp(-t)) * 2;
        //MCore::LogDebug("%f", newScale);
        newScale = MCore::Clamp<float>(newScale, sLowestScale, 1.0f);
        ZoomTo(newScale);
    }


    // zoom to a given amount
    void NodeGraph::ZoomTo(float scale)
    {
        m_startScale     = m_scale;
        m_targetScale    = scale;
        m_scaleTimer.start(1000 / 60);
        m_scalePreciseTimer.Stamp();
        if (scale < sLowestScale)
        {
            sLowestScale = scale;
        }
    }


    // stop an animated zoom
    void NodeGraph::StopAnimatedZoom()
    {
        m_scaleTimer.stop();
    }


    // stop an animated scroll
    void NodeGraph::StopAnimatedScroll()
    {
        m_scrollTimer.stop();
    }


    // fit the graph on the screen
    void NodeGraph::FitGraphOnScreen(int32 width, int32 height, const QPoint& mousePos, bool animate)
    {
        // fit the entire graph in the view
        UpdateNodesAndConnections(width, height, mousePos);
        QRect sceneRect = CalcRectFromGraph();

        //  MCore::LOG("sceneRect: (%d, %d, %d, %d)", sceneRect.left(), sceneRect.top(), sceneRect.width(), sceneRect.height());

        if (sceneRect.isEmpty() == false)
        {
            const int border = aznumeric_cast<int>(10.0f * (1.0f / m_scale));
            sceneRect.adjust(-border, -border, border, border);
            ZoomOnRect(sceneRect, width, height, animate);
        }
    }


    // find the port at a given location
    NodePort* NodeGraph::FindPort(int32 x, int32 y, GraphNode** outNode, AZ::u16* outPortNr, bool* outIsInputPort, bool includeInputPorts)
    {
        // get the number of nodes in the graph and iterate through them
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = indexAndGraphNode.second.get();

            // skip the node in case it is collapsed
            if (graphNode->GetIsCollapsed())
            {
                continue;
            }

            // check if we're in a port of the given node
            NodePort* result = graphNode->FindPort(x, y, outPortNr, outIsInputPort, includeInputPorts);
            if (result)
            {
                *outNode = graphNode;
                return result;
            }
        }

        // failure, no port at the given coordinates
        return nullptr;
    }


    // start creating a connection
    void NodeGraph::StartCreateConnection(AZ::u16 portNr, bool isInputPort, GraphNode* portNode, NodePort* port, const QPoint& startOffset)
    {
        m_conPortNr          = portNr;
        m_conIsInputPort     = isInputPort;
        m_conNodeIndex       = portNode->GetModelIndex();
        m_conPort            = port;
        m_conStartOffset     = startOffset;
    }


    // start relinking a connection
    void NodeGraph::StartRelinkConnection(NodeConnection* connection, AZ::u16 portNr, GraphNode* node)
    {
        m_conPortNr              = portNr;
        m_conNodeIndex           = node->GetModelIndex();
        m_relinkConnection       = connection;

        //MCore::LogInfo( "StartRelinkConnection: Connection=(%s->%s) portNr=%i, graphNode=%s", connection->GetSourceNode()->GetName(), connection->GetTargetNode()->GetName(), portNr, node->GetName()  );
    }


    void NodeGraph::StartReplaceTransitionHead(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode)
    {
        m_replaceTransitionHead = connection;

        m_replaceTransitionStartOffset   = startOffset;
        m_replaceTransitionEndOffset     = endOffset;
        m_replaceTransitionSourceNode    = sourceNode;
        m_replaceTransitionTargetNode    = targetNode;
    }


    void NodeGraph::StartReplaceTransitionTail(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode)
    {
        m_replaceTransitionTail = connection;

        m_replaceTransitionStartOffset   = startOffset;
        m_replaceTransitionEndOffset     = endOffset;
        m_replaceTransitionSourceNode    = sourceNode;
        m_replaceTransitionTargetNode    = targetNode;
    }


    void NodeGraph::GetReplaceTransitionInfo(NodeConnection** outOldConnection, QPoint* outOldStartOffset, QPoint* outOldEndOffset, GraphNode** outOldSourceNode, GraphNode** outOldTargetNode)
    {
        if (m_replaceTransitionHead)
        {
            *outOldConnection = m_replaceTransitionHead;
        }
        if (m_replaceTransitionTail)
        {
            *outOldConnection = m_replaceTransitionTail;
        }

        *outOldStartOffset = m_replaceTransitionStartOffset;
        *outOldEndOffset   = m_replaceTransitionEndOffset;
        *outOldSourceNode  = m_replaceTransitionSourceNode;
        *outOldTargetNode  = m_replaceTransitionTargetNode;
    }


    void NodeGraph::StopReplaceTransitionHead()
    {
        m_replaceTransitionHead = nullptr;
    }


    void NodeGraph::StopReplaceTransitionTail()
    {
        m_replaceTransitionTail = nullptr;
    }

    // reset members
    void NodeGraph::StopRelinkConnection()
    {
        m_conPortNr              = InvalidIndex16;
        m_conNodeIndex           = {};
        m_relinkConnection       = nullptr;
        m_conIsValid             = false;
        m_targetPort             = nullptr;
    }

    // reset members
    void NodeGraph::StopCreateConnection()
    {
        m_conPortNr          = InvalidIndex16;
        m_conIsInputPort     = true;
        m_conNodeIndex       = {};
        m_conPort            = nullptr;
        m_targetPort         = nullptr;
        m_conIsValid         = false;
    }

    // render the connection we're creating, if any
    void NodeGraph::RenderReplaceTransition(QPainter& painter)
    {
        // prepare the Qt painter
        QColor headTailColor(0, 255, 0);
        painter.setPen(headTailColor);
        painter.setBrush(headTailColor);
        const uint32 circleRadius = 4;

        // get the number of nodes and iterate through them
        for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = indexAndGraphNode.second.get();

            // get the number of connections and iterate through them
            const size_t numConnections = graphNode->GetNumConnections();
            for (size_t j = 0; j < numConnections; ++j)
            {
                NodeConnection* connection = graphNode->GetConnection(j);

                // in case the mouse is over the transition
                if (connection->GetIsTailHighlighted() && connection->GetIsWildcardTransition() == false)
                {
                    // calculate its start and end points
                    QPoint start, end;
                    connection->CalcStartAndEndPoints(start, end);

                    // calculate the normalized direction vector of the transition from tail to head
                    AZ::Vector2 dir = AZ::Vector2(aznumeric_cast<float>(end.x() - start.x()), aznumeric_cast<float>(end.y() - start.y()));
                    dir.Normalize();

                    AZ::Vector2 newStart = AZ::Vector2(aznumeric_cast<float>(start.x()), aznumeric_cast<float>(start.y())) + dir * (float)circleRadius;
                    painter.drawEllipse(QPoint(aznumeric_cast<int>(newStart.GetX()), aznumeric_cast<int>(newStart.GetY())), circleRadius, circleRadius);
                    return;
                }
            }
        }
    }

    GraphNode* NodeGraph::GetCreateConnectionNode() const
    {
        NodeGraph* activeGraph = m_graphWidget->GetActiveGraph();
        if (!activeGraph)
        {
            return nullptr;
        }

        return activeGraph->FindGraphNode(m_conNodeIndex);
    }

    // render the connection we're creating, if any
    void NodeGraph::RenderCreateConnection(QPainter& painter)
    {
        if (GetIsRelinkingConnection())
        {
            // gather some information from the connection
            NodeConnection* connection      = GetRelinkConnection();
            QPoint          start           = connection->GetSourceRect().center();
            QPoint          end             = m_graphWidget->GetMousePos();

            QPen pen;
            pen.setColor(QColor(100, 100, 100));
            pen.setStyle(Qt::DotLine);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);

            QRect areaRect(end.x() - 150, end.y() - 150, 300, 300);
            for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
            {
                GraphNode* node = indexAndGraphNode.second.get();

                if (node->GetIsCollapsed())
                {
                    continue;
                }

                // if the node isn't intersecting the area rect it is not close enough
                if (areaRect.intersects(node->GetRect()) == false)
                {
                    continue;
                }

                // now check all ports to see if they would be valid
                const AZ::u16 numInputPorts = node->GetNumInputPorts();
                for (AZ::u16 i = 0; i < numInputPorts; ++i)
                {
                    if (CheckIfIsRelinkConnectionValid(m_relinkConnection, node, i, true))
                    {
                        QPoint tempStart = end;
                        QPoint tempEnd = node->GetInputPort(i)->GetRect().center();

                        if ((tempStart - tempEnd).manhattanLength() < 150)
                        {
                            painter.drawLine(tempStart, tempEnd);
                        }
                    }
                }
            }

            // figure out the color of the connection line
            if (m_targetPort)
            {
                if (m_conIsValid)
                {
                    painter.setPen(QColor(0, 255, 0));
                }
                else
                {
                    painter.setPen(QColor(255, 0, 0));
                }
            }
            else
            {
                painter.setPen(QColor(255, 255, 0));
            }

            // render the smooth line towards the mouse cursor
            painter.setBrush(Qt::NoBrush);

            DrawSmoothedLineFast(painter, start.x(), start.y(), end.x(), end.y(), 1);
        }


        // if we're not creating a connection there is nothing to render
        if (GetIsCreatingConnection() == false)
        {
            return;
        }

        //------------------------------------------
        // draw the suggested valid connections
        //------------------------------------------
        QPoint start = m_graphWidget->GetMousePos();
        QPoint end;

        QPen pen;
        pen.setColor(QColor(100, 100, 100));
        pen.setStyle(Qt::DotLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        if (m_graphWidget->CreateConnectionShowsHelpers())
        {
            QRect areaRect(start.x() - 150, start.y() - 150, 300, 300);
            for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
            {
                GraphNode* node = indexAndGraphNode.second.get();

                if (node->GetIsCollapsed())
                {
                    continue;
                }

                // if the node isn't intersecting the area rect it is not close enough
                if (areaRect.intersects(node->GetRect()) == false)
                {
                    continue;
                }

                // now check all ports to see if they would be valid
                const AZ::u16 numInputPorts = node->GetNumInputPorts();
                for (AZ::u16 i = 0; i < numInputPorts; ++i)
                {
                    if (m_graphWidget->CheckIfIsCreateConnectionValid(i, node, node->GetInputPort(i), true))
                    {
                        end = node->GetInputPort(i)->GetRect().center();

                        if ((start - end).manhattanLength() < 150)
                        {
                            painter.drawLine(start, end);
                        }
                    }
                }

                // now check all ports to see if they would be valid
                const AZ::u16 numOutputPorts = node->GetNumOutputPorts();
                for (AZ::u16 a = 0; a < numOutputPorts; ++a)
                {
                    if (m_graphWidget->CheckIfIsCreateConnectionValid(a, node, node->GetOutputPort(a), false))
                    {
                        end = node->GetOutputPort(a)->GetRect().center();

                        if ((start - end).manhattanLength() < 150)
                        {
                            painter.drawLine(start, end);
                        }
                    }
                }
            }
        }

        //------------------------------

        // update the end point
        start = GetCreateConnectionNode()->GetRect().topLeft() + GetCreateConnectionStartOffset();
        end   = m_graphWidget->GetMousePos();

        // figure out the color of the connection line
        if (m_targetPort)
        {
            if (m_conIsValid)
            {
                painter.setPen(QColor(0, 255, 0));
            }
            else
            {
                painter.setPen(QColor(255, 0, 0));
            }
        }
        else
        {
            painter.setPen(QColor(255, 255, 0));
        }

        // render the smooth line towards the mouse cursor
        painter.setBrush(Qt::NoBrush);

        if (m_graphWidget->CreateConnectionMustBeCurved())
        {
            DrawSmoothedLineFast(painter, start.x(), start.y(), end.x(), end.y(), 1);
        }
        else
        {
            QRect sourceRect = GetCreateConnectionNode()->GetRect();
            sourceRect.adjust(-2, -2, 2, 2);

            if (sourceRect.contains(end))
            {
                return;
            }

            // calc the real start point
            double realX, realY;
            if (NodeGraph::LineIntersectsRect(sourceRect, aznumeric_cast<float>(start.x()), aznumeric_cast<float>(start.y()), aznumeric_cast<float>(end.x()), aznumeric_cast<float>(end.y()), &realX, &realY))
            {
                start.setX(aznumeric_cast<int>(realX));
                start.setY(aznumeric_cast<int>(realY));
            }

            painter.drawLine(start, end);
        }
    }


    // check if this connection already exists
    bool NodeGraph::CheckIfHasConnection(GraphNode* sourceNode, AZ::u16 outputPortNr, GraphNode* targetNode, AZ::u16 inputPortNr) const
    {
        const size_t numConnections = targetNode->GetNumConnections();
        for (size_t i = 0; i < numConnections; ++i)
        {
            NodeConnection* connection = targetNode->GetConnection(i);

            // check if the connection properties are equal
            if (connection->GetInputPortNr() == inputPortNr)
            {
                if (connection->GetSourceNode() == sourceNode)
                {
                    if (connection->GetOutputPortNr() == outputPortNr)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }


    NodeConnection* NodeGraph::FindInputConnection(GraphNode* targetNode, AZ::u16 targetPortNr) const
    {
        if (targetNode == nullptr || targetPortNr == InvalidIndex16)
        {
            return nullptr;
        }

        const size_t numConnections = targetNode->GetNumConnections();
        for (size_t i = 0; i < numConnections; ++i)
        {
            NodeConnection* connection = targetNode->GetConnection(i);

            // check if the connection ports are equal
            if (connection->GetInputPortNr() == targetPortNr)
            {
                return connection;
            }
        }

        return nullptr;
    }


    void NodeGraph::OnRowsInserted(const QModelIndexList& modelIndexes)
    {
        GraphNodeFactory* graphNodeFactory = m_graphWidget->GetPlugin()->GetGraphNodeFactory();

        for (const QModelIndex& modelIndex : modelIndexes)
        {
            if (modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE)
            {
                EMotionFX::AnimGraphNode* childNode = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                GraphNode* graphNode = graphNodeFactory->CreateGraphNode(modelIndex, m_graphWidget->GetPlugin(), childNode);
                AZ_Assert(graphNode, "Expected valid graph node");

                // Set properties that dont change ever
                graphNode->SetParentGraph(this);

                AZStd::unique_ptr<GraphNode> graphNodePtr(graphNode);
                m_graphNodeByModelIndex.emplace(modelIndex, AZStd::move(graphNodePtr));
            }
        }

        // Add all the connections for the inserted nodes, we need to do it in a different iteration pass because
        // the upstream node could have just been inserted
        for (const QModelIndex& modelIndex : modelIndexes)
        {
            const AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
            switch (itemType)
            {
            case AnimGraphModel::ModelItemType::NODE:
            {
                GraphNode* graphNode = FindGraphNode(modelIndex);
                graphNode->Sync();
                break;
            }
            case AnimGraphModel::ModelItemType::TRANSITION:
            {
                EMotionFX::AnimGraphStateTransition* transition = modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
                // get the source and target nodes
                GraphNode* source = nullptr;
                if (transition->GetSourceNode())
                {
                    source = FindGraphNode(transition->GetSourceNode());
                }
                GraphNode* target = FindGraphNode(transition->GetTargetNode());
                StateConnection* connection = new StateConnection(this, modelIndex, source, target, transition->GetIsWildcardTransition());
                connection->SetIsDisabled(transition->GetIsDisabled());
                connection->SetIsSynced(transition->GetSyncMode() != EMotionFX::AnimGraphObject::SYNCMODE_DISABLED);
                target->AddConnection(connection);
                break;
            }
            case AnimGraphModel::ModelItemType::CONNECTION:
            {
                EMotionFX::BlendTreeConnection* connection = modelIndex.data(AnimGraphModel::ROLE_CONNECTION_POINTER).value<EMotionFX::BlendTreeConnection*>();
                GraphNode* source = FindGraphNode(connection->GetSourceNode());
                const QModelIndex parentModelIndex = modelIndex.model()->parent(modelIndex);
                EMotionFX::AnimGraphNode* parentNode = parentModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                GraphNode* target = FindGraphNode(parentNode);
                const AZ::u16 sourcePort = connection->GetSourcePort();
                const AZ::u16 targetPort = connection->GetTargetPort();
                NodeConnection* visualConnection = new NodeConnection(this, modelIndex, target, targetPort, source, sourcePort);
                target->AddConnection(visualConnection);
                break;
            }
            }
        }
    }

    void NodeGraph::SyncTransition(StateConnection* visualStateConnection, const EMotionFX::AnimGraphStateTransition* transition, GraphNode* targetGraphNode)
    {
        visualStateConnection->SetIsDisabled(transition->GetIsDisabled());

        GraphNode* newSourceNode = FindGraphNode(transition->GetSourceNode());
        visualStateConnection->SetSourceNode(newSourceNode);

        visualStateConnection->SetTargetNode(targetGraphNode);
    }

    void NodeGraph::OnRowsAboutToBeRemoved(const QModelIndexList& modelIndexes)
    {
        for (const QModelIndex& modelIndex : modelIndexes)
        {
            const AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
            switch (itemType)
            {
            case AnimGraphModel::ModelItemType::NODE:
            {
                GraphNodeByModelIndex::const_iterator it = m_graphNodeByModelIndex.find(modelIndex);
                if (it != m_graphNodeByModelIndex.end())
                {
                    if (it->second.get() == m_entryNode)
                    {
                        m_entryNode = nullptr;
                    }
                    m_graphNodeByModelIndex.erase(it);
                }
                break;
            }
            case AnimGraphModel::ModelItemType::TRANSITION:
            {
                // We need to locate the transition in the view (which is in the target node), but the transition is already removed.
                // So we have to rely on the UI data.
                for (const GraphNodeByModelIndex::value_type& target : m_graphNodeByModelIndex)
                {
                    AZStd::vector<NodeConnection*>& connections = target.second->GetConnections();
                    const size_t connectionsCount = connections.size();
                    for (size_t i = 0; i < connectionsCount; ++i)
                    {
                        if (connections[i]->GetType() == StateConnection::TYPE_ID)
                        {
                            StateConnection* visualStateConnection = static_cast<StateConnection*>(connections[i]);
                            if (visualStateConnection->GetModelIndex() == modelIndex)
                            {
                                delete connections[i];
                                connections.erase(AZStd::next(begin(connections), i));
                                break;
                            }
                        }
                    }
                }
                break;
            }
            case AnimGraphModel::ModelItemType::CONNECTION:
            {
                const QModelIndex parentModelIndex = modelIndex.model()->parent(modelIndex);
                GraphNode* target = FindGraphNode(parentModelIndex);
                target->RemoveConnection(modelIndex);
                break;
            }
            default:
                break;
            }
        }
    }

    void NodeGraph::OnDataChanged(const QModelIndex& modelIndex, const QVector<int>& roles)
    {
        const AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
        switch (itemType)
        {
        case AnimGraphModel::ModelItemType::NODE:
        {
            GraphNodeByModelIndex::const_iterator it = m_graphNodeByModelIndex.find(modelIndex);
            if (it != m_graphNodeByModelIndex.end())
            {
                if (roles.empty())
                {
                    it->second->Sync();
                }
                else
                {
                    for (const int role : roles)
                    {
                        switch (role)
                        {
                        case AnimGraphModel::ROLE_NODE_ENTRY_STATE:
                            SetEntryNode(it->second.get());
                            break;
                        default:
                            AZ_Warning("EMotionFX", false, "NodeGraph::OnDataChanged, unknown role received: %d", role);
                            it->second->Sync();
                        }
                    }
                }
            }
            break;
        }
        case AnimGraphModel::ModelItemType::TRANSITION:
        {
            const EMotionFX::AnimGraphStateTransition* transition = modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

            EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
            if (targetNode)
            {
                GraphNode* targetGraphNode = FindGraphNode(targetNode);

                bool foundConnection = false;
                AZStd::vector<NodeConnection*>& connections = targetGraphNode->GetConnections();
                for (NodeConnection* connection : connections)
                {
                    if (connection->GetType() == StateConnection::TYPE_ID)
                    {
                        StateConnection* visualStateConnection = static_cast<StateConnection*>(connection);
                        if (visualStateConnection->GetModelIndex() == modelIndex)
                        {
                            SyncTransition(visualStateConnection, transition, targetGraphNode);
                            foundConnection = true;
                            break;
                        }
                    }
                }

                // Fallback method in case the connection was not found as part of the target graph node's connections,
                // which means we adjusted the transition's head.
                if (!foundConnection)
                {
                    for (const GraphNodeByModelIndex::value_type& indexAndGraphNode : m_graphNodeByModelIndex)
                    {
                        GraphNode* visualNode = indexAndGraphNode.second.get();

                        for (NodeConnection* connection : visualNode->GetConnections())
                        {
                            if (connection->GetType() == StateConnection::TYPE_ID)
                            {
                                StateConnection* visualStateConnection = static_cast<StateConnection*>(connection);
                                if (visualStateConnection->GetModelIndex() == modelIndex)
                                {
                                    // Transfer ownership from the previous visual node to where we relinked the transition to.
                                    [[maybe_unused]] const bool connectionRemoveResult = visualNode->RemoveConnection(transition, false);
                                    AZ_Error("EMotionFX", connectionRemoveResult, "Removing connection failed.");
                                    targetGraphNode->AddConnection(visualStateConnection);

                                    SyncTransition(visualStateConnection, transition, targetGraphNode);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        case AnimGraphModel::ModelItemType::CONNECTION:
        {
            // there is no command to edit connections, we remove and add them again. The command that adjusts connections only
            // works for transitions
            break;
        }
        default:
            break;
        }
    }

    GraphNode* NodeGraph::FindGraphNode(const QModelIndex& modelIndex)
    {
        GraphNodeByModelIndex::iterator it = m_graphNodeByModelIndex.find(modelIndex);
        if (it != m_graphNodeByModelIndex.end())
        {
            return it->second.get();
        }

        return nullptr;
    }

    GraphNode* NodeGraph::FindGraphNode(const EMotionFX::AnimGraphNode* node)
    {
        return const_cast<GraphNode*>(std::as_const(*this).FindGraphNode(node));
    }

    const GraphNode* NodeGraph::FindGraphNode(const EMotionFX::AnimGraphNode* node) const
    {
        for (const AZStd::pair<QPersistentModelIndex, AZStd::unique_ptr<GraphNode> >& modelIndexAndGraphNode : m_graphNodeByModelIndex)
        {
            // Since the OS wont allocate different objects on the same address, we can use the pointer to
            // locate the object
            if (modelIndexAndGraphNode.first.data(AnimGraphModel::ROLE_POINTER).value<void*>() == node)
            {
                return modelIndexAndGraphNode.second.get();
            }
        }
        return nullptr;
    }

    StateConnection* NodeGraph::FindStateConnection(const QModelIndex& modelIndex)
    {
        // This function could get called when the model index is about to be removed. So we can't use the model index data directly to find the transition as it will be invalid.
        // We have to rely on the UI data.
        for (const GraphNodeByModelIndex::value_type& target : m_graphNodeByModelIndex)
        {
            AZStd::vector<NodeConnection*>& connections = target.second->GetConnections();
            for (NodeConnection* connection : connections)
            {
                if (connection->GetType() == StateConnection::TYPE_ID)
                {
                    StateConnection* visualStateConnection = static_cast<StateConnection*>(connection);
                    if (visualStateConnection->GetModelIndex() == modelIndex)
                    {
                        return visualStateConnection;
                    }
                }
            }
        }

        return nullptr;
    }

    NodeConnection* NodeGraph::FindNodeConnection(const QModelIndex& modelIndex)
    {
        const AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
        if (itemType == AnimGraphModel::ModelItemType::CONNECTION)
        {
            const QModelIndex parentModelIndex = modelIndex.model()->parent(modelIndex);
            if (parentModelIndex.isValid())
            {
                GraphNode* target = FindGraphNode(parentModelIndex);
                if (target)
                {
                    AZStd::vector<NodeConnection*>& connections = target->GetConnections();
                    for (NodeConnection* connection : connections)
                    {
                        if (connection->GetType() == NodeConnection::TYPE_ID)
                        {
                            NodeConnection* visualNodeConnection = static_cast<NodeConnection*>(connection);
                            if (visualNodeConnection->GetModelIndex() == modelIndex)
                            {
                                return visualNodeConnection;
                            }
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    void NodeGraph::UpdateVisualGraphFlags()
    {
        // for all nodes in the graph
        for (const GraphNodeByModelIndex::value_type& nodes : m_graphNodeByModelIndex)
        {
            GraphNode* graphNode = nodes.second.get();
            const EMotionFX::AnimGraphNode* emfxNode = nodes.first.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            const EMotionFX::AnimGraphInstance* graphNodeAnimGraphInstance = nodes.first.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();

            if (graphNodeAnimGraphInstance)
            {
                graphNode->SetIsProcessed(graphNodeAnimGraphInstance->GetIsOutputReady(emfxNode->GetObjectIndex()));
                graphNode->SetIsUpdated(graphNodeAnimGraphInstance->GetIsUpdateReady(emfxNode->GetObjectIndex()));

                const size_t numConnections = graphNode->GetNumConnections();
                for (size_t c = 0; c < numConnections; ++c)
                {
                    NodeConnection* connection = graphNode->GetConnection(c);
                    if (connection->GetType() == NodeConnection::TYPE_ID)
                    {
                        const EMotionFX::BlendTreeConnection* emfxConnection = connection->GetModelIndex().data(AnimGraphModel::ROLE_CONNECTION_POINTER).value<EMotionFX::BlendTreeConnection*>();
                        connection->SetIsProcessed(emfxConnection->GetIsVisited());
                    }
                }
            }
            else
            {
                graphNode->SetIsProcessed(false);
                graphNode->SetIsUpdated(false);

                const size_t numConnections = graphNode->GetNumConnections();
                for (size_t c = 0; c < numConnections; ++c)
                {
                    NodeConnection* connection = graphNode->GetConnection(c);
                    if (connection->GetType() == NodeConnection::TYPE_ID)
                    {
                        connection->SetIsProcessed(false);
                    }
                }
            }

            const size_t numConnections = graphNode->GetNumConnections();
            for (size_t c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection = graphNode->GetConnection(c);
                if (connection->GetType() == NodeConnection::TYPE_ID)
                {
                    const EMotionFX::BlendTreeConnection* emfxConnection = connection->GetModelIndex().data(AnimGraphModel::ROLE_CONNECTION_POINTER).value<EMotionFX::BlendTreeConnection*>();
                    if (graphNodeAnimGraphInstance)
                    {
                        connection->SetIsProcessed(emfxConnection->GetIsVisited());
                    }
                    else
                    {
                        connection->SetIsProcessed(false);
                    }
                }
            }
        }
    }

    // check if a connection is valid or not
    bool NodeGraph::CheckIfIsRelinkConnectionValid(NodeConnection* connection, GraphNode* newTargetNode, AZ::u16 newTargetPortNr, bool isTargetInput)
    {
        GraphNode* targetNode = connection->GetSourceNode();
        GraphNode* sourceNode = newTargetNode;
        AZ::u16 sourcePortNr = connection->GetOutputPortNr();
        AZ::u16 targetPortNr = newTargetPortNr;

        // don't allow connection to itself
        if (sourceNode == targetNode)
        {
            return false;
        }

        // if we're not dealing with state nodes
        if (sourceNode->GetType() != StateGraphNode::TYPE_ID || targetNode->GetType() != StateGraphNode::TYPE_ID)
        {
            if (isTargetInput == false)
            {
                return false;
            }
        }

        // if this were states, it's all fine
        if (sourceNode->GetType() == StateGraphNode::TYPE_ID || targetNode->GetType() == StateGraphNode::TYPE_ID)
        {
            return true;
        }

        // check if there is already a connection in the port
        AZ_Assert(sourceNode->GetType() == BlendTreeVisualNode::TYPE_ID, "Expected blend tree node");
        AZ_Assert(targetNode->GetType() == BlendTreeVisualNode::TYPE_ID, "Expected blend tree node");
        BlendTreeVisualNode* targetBlendNode = static_cast<BlendTreeVisualNode*>(sourceNode);
        BlendTreeVisualNode* sourceBlendNode = static_cast<BlendTreeVisualNode*>(targetNode);

        EMotionFX::AnimGraphNode* emfxSourceNode = sourceBlendNode->GetEMFXNode();
        EMotionFX::AnimGraphNode* emfxTargetNode = targetBlendNode->GetEMFXNode();
        EMotionFX::AnimGraphNode::Port& sourcePort = emfxSourceNode->GetOutputPort(sourcePortNr);
        EMotionFX::AnimGraphNode::Port& targetPort = emfxTargetNode->GetInputPort(targetPortNr);

        // if the port data types are not compatible, don't allow the connection
        if (targetPort.CheckIfIsCompatibleWith(sourcePort) == false)
        {
            return false;
        }

        return true;
    }

    void NodeGraph::RecursiveSetOpacity(EMotionFX::AnimGraphNode* startNode, float opacity)
    {
        GraphNode* graphNode = FindGraphNode(startNode);
        AZ_Assert(graphNode, "Expected graph node");
        graphNode->SetOpacity(opacity);
        graphNode->ResetBorderColor();

        // recurse through the inputs
        const size_t numConnections = startNode->GetNumConnections();
        for (size_t i = 0; i < numConnections; ++i)
        {
            EMotionFX::BlendTreeConnection* connection = startNode->GetConnection(i);
            RecursiveSetOpacity(connection->GetSourceNode(), opacity);
        }
    }

    AnimGraphModel& NodeGraph::GetAnimGraphModel() const
    {
        return m_graphWidget->GetPlugin()->GetAnimGraphModel();
    }

    void NodeGraph::Reinit()
    {
        AZ_Assert(m_currentModelIndex.isValid(), "Expected valid model index");
        AZ_Assert(m_graphNodeByModelIndex.empty(), "Expected empty node graph");

        GraphNodeFactory* graphNodeFactory = m_graphWidget->GetPlugin()->GetGraphNodeFactory();

        // Add all the nodes
        AZStd::vector<GraphNodeByModelIndex::iterator> nodeModelIterators;
        const int rows = m_currentModelIndex.model()->rowCount(m_currentModelIndex);
        for (int row = 0; row < rows; ++row)
        {
            const QModelIndex modelIndex = m_currentModelIndex.model()->index(row, 0, m_currentModelIndex);
            const AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
            if (itemType == AnimGraphModel::ModelItemType::NODE)
            {
                EMotionFX::AnimGraphNode* childNode = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                GraphNode* graphNode = graphNodeFactory->CreateGraphNode(modelIndex, m_graphWidget->GetPlugin(), childNode);
                AZ_Assert(graphNode, "Expected valid graph node");

                // Set properties that dont change ever
                graphNode->SetParentGraph(this);

                AZStd::unique_ptr<GraphNode> graphNodePtr(graphNode);
                AZStd::pair<GraphNodeByModelIndex::iterator, bool> it = m_graphNodeByModelIndex.emplace(modelIndex, AZStd::move(graphNodePtr));
                nodeModelIterators.emplace_back(it.first);
            }
        }

        // Now sync. Connections are added during sync, we need the step above first to create all the nodes.
        for (const GraphNodeByModelIndex::iterator& nodeModelIt : nodeModelIterators)
        {
            nodeModelIt->second->Sync();
        }

        // do another iteration over the element's rows to create the transitions
        for (int row = 0; row < rows; ++row)
        {
            const QModelIndex modelIndex = m_currentModelIndex.model()->index(row, 0, m_currentModelIndex);
            const AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
            if (itemType == AnimGraphModel::ModelItemType::TRANSITION)
            {
                EMotionFX::AnimGraphStateTransition* transition = modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
                // get the source and target nodes
                GraphNode* source = nullptr;
                if (transition->GetSourceNode())
                {
                    source = FindGraphNode(transition->GetSourceNode());
                }
                GraphNode* target = FindGraphNode(transition->GetTargetNode());
                StateConnection* connection = new StateConnection(this, modelIndex, source, target, transition->GetIsWildcardTransition());
                connection->SetIsDisabled(transition->GetIsDisabled());
                connection->SetIsSynced(transition->GetSyncMode() != EMotionFX::AnimGraphObject::SYNCMODE_DISABLED);
                target->AddConnection(connection);
            }
        }

        EMotionFX::AnimGraphObject* currentGraphObject = m_currentModelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_OBJECT_PTR).value<EMotionFX::AnimGraphObject*>();
        if (azrtti_typeid(currentGraphObject) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(currentGraphObject);

            // set the entry state
            EMotionFX::AnimGraphNode* entryNode = stateMachine->GetEntryState();
            if (!entryNode)
            {
                SetEntryNode(nullptr);
            }
            else
            {
                GraphNode* entryGraphNode = FindGraphNode(entryNode);
                SetEntryNode(entryGraphNode);
            }
        }
        else if (azrtti_typeid(currentGraphObject) == azrtti_typeid<EMotionFX::BlendTree>())
        {
            EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(currentGraphObject);
            EMotionFX::AnimGraphNode* virtualFinalNode = blendTree->GetVirtualFinalNode();
            if (virtualFinalNode)
            {
                RecursiveSetOpacity(blendTree->GetFinalNode(), 0.065f);
                RecursiveSetOpacity(virtualFinalNode, 1.0f);

                if (virtualFinalNode != blendTree->GetFinalNode())
                {
                    GraphNode* virtualFinalGraphNode = FindGraphNode(virtualFinalNode);
                    virtualFinalGraphNode->SetBorderColor(QColor(0, 255, 0));
                }
            }
        }

        const QRect& graphWidgetRect = m_graphWidget->geometry();
        SetScalePivot(QPoint(graphWidgetRect.width() / 2, graphWidgetRect.height() / 2));
        FitGraphOnScreen(graphWidgetRect.width(), graphWidgetRect.height(), QPoint(0, 0), false);
    }

    QRect NodeGraph::ComputeNodeGroupRect(const EMotionFX::AnimGraphNodeGroup* nodeGroup) const
    {
        EMotionFX::AnimGraphNode* currentNode =
            m_currentModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

        QRect groupRect;
        QRect nodeRect;

        int32_t top = std::numeric_limits<int32_t>::max();
        int32_t bottom = std::numeric_limits<int32_t>::lowest();
        int32_t left = std::numeric_limits<int32_t>::max();
        int32_t right = std::numeric_limits<int32_t>::lowest();

        // get the number of nodes inside the node group and skip the group in case there are no nodes in
        const size_t numNodes = nodeGroup->GetNumNodes();
        if (numNodes == 0)
        {
            return groupRect;
        }

        bool nodesInGroupDisplayed = false;
        for (size_t j = 0; j < numNodes; ++j)
        {
            // get the graph node by the id and skip it if the node is not inside the currently visible node graph
            const EMotionFX::AnimGraphNodeId nodeId = nodeGroup->GetNode(j);
            EMotionFX::AnimGraphNode* node = currentNode->RecursiveFindNodeById(nodeId);
            const GraphNode* graphNode = FindGraphNode(node);
            if (graphNode)
            {
                nodesInGroupDisplayed = true;
                nodeRect = graphNode->GetRect();
                top = MCore::Min3<int32_t>(top, nodeRect.top(), nodeRect.bottom());
                bottom = MCore::Max3<int32_t>(bottom, nodeRect.top(), nodeRect.bottom());
                left = MCore::Min3<int32_t>(left, nodeRect.left(), nodeRect.right());
                right = MCore::Max3<int32_t>(right, nodeRect.left(), nodeRect.right());
            }

            if (nodesInGroupDisplayed)
            {
                groupRect.setTop(top - (sGroupRectContentPadding + m_groupFont.pixelSize() + 2 * sGroupRectTextVPadding));
                groupRect.setBottom(bottom + sGroupRectContentPadding);
                groupRect.setLeft(left - sGroupRectContentPadding);
                groupRect.setRight(right + sGroupRectContentPadding);
            }
        }

        return groupRect;
    }

    bool NodeGraph::CheckInsideNodeGroupTitleRect(const EMotionFX::AnimGraphNodeGroup* nodeGroup, const QPoint& globalPoint) const
    {
        QRect groupRect = ComputeNodeGroupRect(nodeGroup);

        if (groupRect.isEmpty())
        {
            return false;
        }

        groupRect.setHeight(m_groupFontMetrics->height());
        groupRect.setLeft(groupRect.left() + sGroupRectContentPadding);

        return GetTransform().mapRect(groupRect).contains(globalPoint);
    }

    EMotionFX::AnimGraphNodeGroup* NodeGraph::FindNodeGroup(const QPoint& globalPoint) const
    {
        EMotionFX::AnimGraphNode* currentNode = m_currentModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        EMotionFX::AnimGraph* animGraph = currentNode->GetAnimGraph();

        // get the number of node groups and iterate through them
        const size_t numNodeGroups = animGraph->GetNumNodeGroups();
        for (size_t i = 0; i < numNodeGroups; ++i)
        {
            // get the current node group
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            // skip the node group if it isn't visible
            if (nodeGroup->GetIsVisible() == false)
            {
                continue;
            }

            QRect groupRect = GetTransform().mapRect(ComputeNodeGroupRect(nodeGroup));
            if (groupRect.contains(globalPoint))
            {
                return nodeGroup;
            }
        }

        return nullptr;
    }

    void NodeGraph::RenderNodeGroups(QPainter& painter)
    {
        EMotionFX::AnimGraphNode* currentNode =
            m_currentModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        EMotionFX::AnimGraph* animGraph = currentNode->GetAnimGraph();

        // get the number of node groups and iterate through them
        const size_t numNodeGroups = animGraph->GetNumNodeGroups();
        for (size_t i = 0; i < numNodeGroups; ++i)
        {
            // get the current node group
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            // skip the node group if it isn't visible
            if (nodeGroup->GetIsVisible() == false)
            {
                continue;
            }

            QRect groupRect = ComputeNodeGroupRect(nodeGroup);

            if (!groupRect.isNull())
            {
                // get the color from the node group and set it to the painter
                AZ::Color azColor;
                azColor.FromU32(nodeGroup->GetColor());

                // Draw outer rect
                QColor color = AzQtComponents::ToQColor(azColor);
                color.setAlpha(255);
                painter.setPen(color);
                color.setAlpha(40);
                painter.setBrush(color);
                painter.drawRect(groupRect);

                // Draw label rect
                groupRect.setHeight(sGroupRectTextFontSize + 2 * sGroupRectTextHPadding);
                color.setAlpha(255);
                painter.setBrush(color);
                painter.drawRect(groupRect);

                if (nodeGroup->IsNameEditOngoing())
                {
                    const int x_delta = (groupRect.left() + sGroupRectTextHPadding) * GetScale();
                    const int y_delta = (groupRect.top() +
                                         (m_groupFont.pixelSize() + 2 * sGroupRectTextVPadding - m_nodeGroupNameLineEdit->height()) / 2.0) *
                        GetScale();
                    m_nodeGroupNameLineEdit->move(m_transform.dx() + x_delta, m_transform.dy() + y_delta);
                    m_nodeGroupNameLineEdit->setBaseSize(
                        QSize(groupRect.width() - 2 * sGroupRectTextHPadding, m_groupFontMetrics->height()));
                    m_nodeGroupNameLineEdit->setBaseFontPixelSize(m_groupFont.pixelSize());
                    m_nodeGroupNameLineEdit->setScale(GetScale());
                    m_nodeGroupNameLineEdit->show();
                }
                else
                {
                    // Draw group name label
                    QRect textRect = groupRect;
                    textRect.setHeight(m_groupFontMetrics->height());
                    textRect.setLeft(textRect.left() + sGroupRectTextHPadding);
                    textRect.setTop(textRect.top() + sGroupRectTextVPadding);

                    // draw the name on top
                    GraphNode::RenderText(
                        painter,
                        nodeGroup->GetName(),
                        AzQtComponents::ToQColor(AZ::Colors::Black),
                        m_groupFont,
                        *m_groupFontMetrics,
                        Qt::AlignLeft,
                        textRect);
                }
            }
        } // for all node groups
    }

    void NodeGraph::EnableNameEditForNodeGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup)
    {
        if (!nodeGroup)
        {
            return;
        }

        nodeGroup->SetNameEditOngoing(true);
        m_currentNameEditNodeGroup = nodeGroup;
        m_nodeGroupNameLineEdit->setText(nodeGroup->GetName());
        m_nodeGroupNameLineEdit->setFocus();
    }

    void NodeGraph::DisableNameEditForNodeGroup()
    {
        if (m_currentNameEditNodeGroup)
        {
            m_currentNameEditNodeGroup->SetName(m_nodeGroupNameLineEdit->text().toStdString().c_str());
            m_nodeGroupNameLineEdit->setText({});
            m_currentNameEditNodeGroup->SetNameEditOngoing(false);
            m_currentNameEditNodeGroup = nullptr;
            // This needs to be done after setting m_currentNameEditNodeGroup = nullptr because
            // it triggers the QLineEdit::editingFinished signal which this function is connected to,
            // which is effectively a recursive function call.
            m_nodeGroupNameLineEdit->hide();
        }
    }

    void NodeGraph::RemoveNodeGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup)
    {
        EMotionFX::AnimGraphNode* currentNode =
            m_currentModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        EMotionFX::AnimGraph* animGraph = currentNode->GetAnimGraph();

        size_t idx = animGraph->FindNodeGroupIndexByName(nodeGroup->GetName());
        if (idx != InvalidIndex)
        {
            animGraph->RemoveNodeGroup(idx);
        }
    }

}   // namespace EMotionStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_NodeGraph.cpp>
