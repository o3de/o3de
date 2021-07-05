/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "BlendGraphWidgetCallback.h"
//#include "GraphNode.h"
#include "AnimGraphPlugin.h"
#include "NodeGraph.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AttributeVector3.h>
#include <MCore/Source/AttributeVector4.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/MotionInstance.h>


namespace EMStudio
{
    // constructor
    BlendGraphWidgetCallback::BlendGraphWidgetCallback(BlendGraphWidget* widget)
        : GraphWidgetCallback(widget)
    {
        mBlendGraphWidget = widget;

        mFont.setPixelSize(12);
        mTextOptions.setAlignment(Qt::AlignCenter);
        mFontMetrics = new QFontMetrics(mFont);
    }


    // destructor
    BlendGraphWidgetCallback::~BlendGraphWidgetCallback()
    {
        delete mFontMetrics;
    }


    void BlendGraphWidgetCallback::DrawOverlay(QPainter& painter)
    {
        // get the plugin and return directly in case we're not showing the processed nodes
        AnimGraphPlugin* plugin = mBlendGraphWidget->GetPlugin();
        //if (plugin->GetShowProcessed() == false)
        //  return;

        // if we're going to display some visualization information
        //  if (plugin->GetDisplayPlaySpeeds() || plugin->GetDisplayGlobalWeights() || plugin->GetDisplaySyncStatus())
        if (plugin->GetDisplayFlags() != 0)
        {
            // get the active graph and the corresponding emfx node and return if they are invalid or in case the opened node is no blend tree
            NodeGraph*                  activeGraph = mBlendGraphWidget->GetActiveGraph();
            EMotionFX::AnimGraphNode* currentNode  = mBlendGraphWidget->GetCurrentNode();
            if (activeGraph == nullptr || currentNode == nullptr)
            {
                return;
            }

            // get the currently selected actor instance and its anim graph instance and return if they are not valid
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance == nullptr || actorInstance->GetAnimGraphInstance() == nullptr)
            {
                return;
            }

            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

            // get the number of nodes and iterate through them
            const uint32 numNodes = activeGraph->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                GraphNode*                  graphNode   = activeGraph->GetNode(i);
                EMotionFX::AnimGraphNode*  emfxNode    = currentNode->RecursiveFindNodeById(graphNode->GetId());

                // skip invisible graph nodes
                if (graphNode->GetIsVisible() == false)
                {
                    continue;
                }

                // make sure the corresponding anim graph node is valid
                if (emfxNode == nullptr)
                {
                    continue;
                }

                // skip non-processed nodes and nodes that have no output pose
                if (emfxNode->GetHasOutputPose() == false || graphNode->GetIsProcessed() == false)
                {
                    continue;
                }

                if (graphNode->GetIsHighlighted())
                {
                    continue;
                }

                // get the unique data
                EMotionFX::AnimGraphNodeData* uniqueData = emfxNode->FindUniqueNodeData(animGraphInstance);

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
                const QRect& nodeRect = graphNode->GetFinalRect();
                QRect textRect(nodeRect.center().x() - rectWidth / 2, nodeRect.center().y() - requiredHeight / 2, rectWidth, requiredHeight);
                const uint32 alpha = (graphNode->GetIsHighlighted()) ? 225 : 175;
                const QColor backgroundColor(0, 0, 0, alpha);
                painter.setBrush(backgroundColor);
                painter.setPen(Qt::black);
                painter.drawRect(textRect);

                QColor textColor(255, 255, 0);
                //textColor = graphNode->GetBaseColor();
                if (graphNode->GetIsHighlighted())
                {
                    textColor = QColor(0, 255, 0);
                }

                painter.setPen(textColor);
                painter.setFont(mFont);

                QPoint textPosition = textRect.topLeft();
                textPosition.setX(textPosition.x() + 3);
                textPosition.setY(textPosition.y() + 11);

                // add the playspeed
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYSPEED))
                {
                    mQtTempString.sprintf("Play Speed = %.2f", emfxNode->GetPlaySpeed(animGraphInstance));
                    painter.drawText(textPosition, mQtTempString);
                    textPosition.setY(textPosition.y() + heightSpacing);
                }

                // add the global weight
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_GLOBALWEIGHT))
                {
                    mQtTempString.sprintf("Global Weight = %.2f", uniqueData->GetGlobalWeight());
                    painter.drawText(textPosition, mQtTempString);
                    textPosition.setY(textPosition.y() + heightSpacing);
                }

                // add the sync
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_SYNCSTATUS))
                {
                    mQtTempString.sprintf("Synced = %s", animGraphInstance->GetIsSynced(emfxNode->GetObjectIndex()) ? "Yes" : "No");
                    painter.drawText(textPosition, mQtTempString);
                    textPosition.setY(textPosition.y() + heightSpacing);
                }

                // add the play position
                if (plugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYPOSITION))
                {
                    mQtTempString.sprintf("Play Time = %.3f / %.3f", uniqueData->GetCurrentPlayTime(), uniqueData->GetDuration());
                    painter.drawText(textPosition, mQtTempString);
                    textPosition.setY(textPosition.y() + heightSpacing);
                }
            }
        }


        const EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (!actorInstance)
        {
            return;
        }

        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (!animGraphInstance)
        {
            return;
        }

        // get the active graph and the corresponding emfx node and return if they are invalid or in case the opened node is no blend tree
        NodeGraph*                activeGraph  = mBlendGraphWidget->GetActiveGraph();
        EMotionFX::AnimGraphNode* currentNode  = mBlendGraphWidget->GetCurrentNode();

        if (!activeGraph || !currentNode || azrtti_typeid(currentNode) != azrtti_typeid<EMotionFX::BlendTree>())
        {
            return;
        }

        const EMotionFX::AnimGraph* simulatedAnimGraph  = animGraphInstance->GetAnimGraph();
        const EMotionFX::AnimGraph* renderedAnimGraph   = currentNode->GetAnimGraph();
        if (simulatedAnimGraph != renderedAnimGraph)
        {
            AzFramework::StringFunc::Path::GetFileName(simulatedAnimGraph->GetFileName(), m_tempStringA);
            AzFramework::StringFunc::Path::GetFileName(renderedAnimGraph->GetFileName(), m_tempStringB);

            m_tempStringC = AZStd::string::format("Simulated anim graph on character (%s) differs from the currently shown one (%s).", m_tempStringA.c_str(), m_tempStringB.c_str());
            GraphNode::RenderText(painter, m_tempStringC.c_str(), QColor(255, 0, 0), mFont, *mFontMetrics, Qt::AlignLeft, QRect(8, 0, 50, 20));
        }

        if (activeGraph->GetScale() < 0.5f)
        {
            return;
        }

        // get the number of nodes and iterate through them
        const uint32 numNodes = activeGraph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode*                  graphNode   = activeGraph->GetNode(i);
            EMotionFX::AnimGraphNode*   emfxNode    = currentNode->RecursiveFindNodeById(graphNode->GetId());

            // make sure the corresponding anim graph node is valid
            if (emfxNode == nullptr)
            {
                continue;
            }

            // iterate through all connections connected to this node
            const uint32 numConnections = graphNode->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                NodeConnection*             visualConnection = graphNode->GetConnection(c);

                // get the source and target nodes
                GraphNode*                  sourceNode      = visualConnection->GetSourceNode();
                EMotionFX::AnimGraphNode*   emfxSourceNode  = currentNode->RecursiveFindNodeById(sourceNode->GetId());
                GraphNode*                  targetNode      = visualConnection->GetTargetNode();
                EMotionFX::AnimGraphNode*   emfxTargetNode  = currentNode->RecursiveFindNodeById(targetNode->GetId());

                //QColor color(255,0,255);
                QColor color = visualConnection->GetTargetNode()->GetInputPort(visualConnection->GetInputPortNr())->GetColor();

                // only show values for connections that are processed
                if (visualConnection->GetIsProcessed() == false)
                {
                    continue;
                }

                const uint32 inputPortNr    = visualConnection->GetInputPortNr();
                const uint32 outputPortNr   = visualConnection->GetOutputPortNr();
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
                    AZ::Vector2 vec = vecAttribute->GetValue();
                    m_tempStringA = AZStd::string::format("(%.2f, %.2f)", static_cast<float>(vec.GetX()), static_cast<float>(vec.GetY()));
                    break;
                }

                // vector 3 attributes
                case MCore::AttributeVector3::TYPE_ID:
                {
                    MCore::AttributeVector3* vecAttribute = static_cast<MCore::AttributeVector3*>(attribute);
                    AZ::PackedVector3f vec = vecAttribute->GetValue();
                    m_tempStringA = AZStd::string::format("(%.2f, %.2f, %.2f)", static_cast<float>(vec.GetX()), static_cast<float>(vec.GetY()), static_cast<float>(vec.GetZ()));
                    break;
                }

                // vector 4 attributes
                case MCore::AttributeVector4::TYPE_ID:
                {
                    MCore::AttributeVector4* vecAttribute = static_cast<MCore::AttributeVector4*>(attribute);
                    AZ::Vector4 vec = vecAttribute->GetValue();
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
                    const AZ::Vector3 eulerAngles = quatAttribute->GetValue().ToEuler();
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
                    if (azrtti_typeid(emfxTargetNode) == azrtti_typeid<EMotionFX::BlendTreeBlendNNode>())
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

                    int halfTextHeight = 6;
                    int textWidth = mFontMetrics->width(m_tempStringA.c_str());
                    int halfTextWidth = textWidth / 2;

                    QRect textRect(connectionAttachPoint.x() - halfTextWidth - 1, connectionAttachPoint.y() - halfTextHeight, textWidth + 4, halfTextHeight * 2);
                    QPoint textPosition = textRect.bottomLeft();
                    textPosition.setY(textPosition.y() - 1);
                    textPosition.setX(textPosition.x() + 2);

                    const QColor backgroundColor(30, 30, 30);

                    // draw the background rect for the text
                    painter.setBrush(backgroundColor);
                    painter.setPen(Qt::black);
                    painter.drawRect(textRect);

                    // draw the text
                    painter.setPen(color);
                    painter.setFont(mFont);
                    // OLD:
                    //painter.drawText( textPosition, mTempString.c_str() );
                    // NEW:
                    GraphNode::RenderText(painter, m_tempStringA.c_str(), color, mFont, *mFontMetrics, Qt::AlignCenter, textRect);
                }
            }
        }
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_BlendGraphWidgetCallback.cpp>
