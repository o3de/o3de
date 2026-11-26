/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendTreeVisualNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeConnection.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>


namespace EMStudio
{
    // the constructor
    BlendTreeVisualNode::BlendTreeVisualNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node)
        : AnimGraphVisualNode(modelIndex, plugin, node)
    {
        SetSubTitle(node->GetPaletteName(), false);
    }


    // the destructor
    BlendTreeVisualNode::~BlendTreeVisualNode()
    {
    }


    // update port names and number of ports etc
    void BlendTreeVisualNode::Sync()
    {
        // remove all ports and connections
        RemoveAllInputPorts();
        RemoveAllOutputPorts();
        RemoveAllConnections();

        // add all input ports
        const AZStd::vector<EMotionFX::AnimGraphNode::Port>& inPorts = m_emfxNode->GetInputPorts();
        const AZ::u16 numInputs = aznumeric_caster(inPorts.size());
        m_inputPorts.reserve(numInputs);
        for (AZ::u16 i = 0; i < numInputs; ++i)
        {
            NodePort* port = AddInputPort(false);
            port->SetNameID(inPorts[i].m_nameId);
            port->SetColor(GetPortColor(inPorts[i]));
        }

        if (GetHasVisualOutputPorts())
        {
            // add all output ports
            const AZStd::vector<EMotionFX::AnimGraphNode::Port>& outPorts = m_emfxNode->GetOutputPorts();
            const AZ::u16 numOutputs = aznumeric_caster(outPorts.size());
            m_outputPorts.reserve(numOutputs);
            for (AZ::u16 i = 0; i < numOutputs; ++i)
            {
                NodePort* port = AddOutputPort(false);
                port->SetNameID(outPorts[i].m_nameId);
                port->SetColor(GetPortColor(outPorts[i]));
            }
        }

        // Recreate connections
        const int rows = m_modelIndex.model()->rowCount(m_modelIndex);
        for (int row = 0; row < rows; ++row)
        {
            const QModelIndex childIndex = m_modelIndex.model()->index(row, 0, m_modelIndex);
            if (childIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::CONNECTION)
            {
                EMotionFX::BlendTreeConnection* connection = childIndex.data(AnimGraphModel::ROLE_CONNECTION_POINTER).value<EMotionFX::BlendTreeConnection*>();

                GraphNode* source = m_parentGraph->FindGraphNode(connection->GetSourceNode());
                GraphNode* target = this;
                const AZ::u16 sourcePort = connection->GetSourcePort();
                const AZ::u16 targetPort = connection->GetTargetPort();

                NodeConnection* visualConnection = new NodeConnection(m_parentGraph, childIndex, target, targetPort, source, sourcePort);
                target->AddConnection(visualConnection);
            }
        }

        UpdateNameAndPorts();

        AnimGraphVisualNode::Sync();
    }


    // get the port color for a given EMotion FX port
    QColor BlendTreeVisualNode::GetPortColor(const EMotionFX::AnimGraphNode::Port& port) const
    {
        switch (port.m_compatibleTypes[0])
        {
        case EMotionFX::AttributePose::TYPE_ID:
            return QColor(150, 150, 255);
        case MCore::AttributeFloat::TYPE_ID:
            return QColor(0, 200, 0);
        case MCore::AttributeInt32::TYPE_ID:
            return QColor(0, 200, 100);
        case MCore::AttributeBool::TYPE_ID:
            return QColor(0, 200, 200);
        case MCore::AttributeVector2::TYPE_ID:
            return QColor(100, 0, 100);
        case MCore::AttributeVector3::TYPE_ID:
            return QColor(175, 0, 175);
        case MCore::AttributeVector4::TYPE_ID:
            return QColor(255, 0, 255);
        case MCore::AttributeString::TYPE_ID:
            return QColor(50, 75, 255);

        default:
            return QColor(50, 250, 250);
        }
    }


    // render the node
    void BlendTreeVisualNode::Render(QPainter& painter, QPen* pen, bool renderShadow)
    {
        // only render if the given node is visible
        if (m_isVisible == false)
        {
            return;
        }

        // render node shadow
        if (renderShadow)
        {
            RenderShadow(painter);
        }

        float opacityFactor = m_opacity;
        if (m_isEnabled == false)
        {
            opacityFactor *= 0.35f;
        }

        if (opacityFactor < 0.065f)
        {
            opacityFactor = 0.065f;
        }

        painter.setOpacity(opacityFactor);

        // check if we need to color all nodes or not and if the node has an error
        const bool colorAllNodes = GetAlwaysColor();
        const bool hasError = GetHasError();
        const bool isSelected = GetIsSelected();

        // border color
        QColor borderColor;
        pen->setWidth(1);
        if (isSelected)
        {
            borderColor.setRgb(255, 128, 0);

            if (m_parentGraph->GetScale() > 0.75f)
            {
                pen->setWidth(2);
            }
        }
        else
        {
            if (hasError)
            {
                borderColor.setRgb(255, 0, 0);
            }
            else
            {
                borderColor = m_borderColor;
            }
        }

        // background and header colors
        QColor bgColor;
        if (isSelected)
        {
            bgColor.setRgbF(0.93f, 0.547f, 0.0f, 1.0f); //  rgb(72, 63, 238)
        }
        else // not selected
        {
            if (m_isEnabled)
            {
                if (m_isProcessed || colorAllNodes)
                {
                    bgColor = m_baseColor;
                }
                else
                {
                    bgColor.setRgb(100, 100, 100);
                    //              painter.setOpacity( 0.25f );
                }
            }
            else
            {
                bgColor.setRgbF(0.3f, 0.3f, 0.3f, 1.0f);
            }
        }

        // blinking error
        if (hasError && !isSelected)
        {
            if (m_parentGraph->GetUseAnimation())
            {
                borderColor = m_parentGraph->GetErrorBlinkColor();
            }
            else
            {
                borderColor = Qt::red;
            }

            bgColor = borderColor;
        }

        QColor bgColor2;
        QColor headerBgColor;
        bgColor2   = bgColor.lighter(30);// make darker actually, 30% of the old color, same as bgColor * 0.3f;

        headerBgColor = bgColor.lighter(20);

        // text color
        QColor textColor;
        if (!isSelected)
        {
            if (m_isEnabled)
            {
                textColor = Qt::white;
            }
            else
            {
                textColor = QColor(100, 100, 100);
            }
        }
        else
        {
            textColor = QColor(bgColor);
        }

        if (m_isCollapsed == false)
        {
            // is highlighted/hovered (on-mouse-over effect)
            if (m_isHighlighted)
            {
                bgColor = bgColor.lighter(120);
                bgColor2 = bgColor2.lighter(120);
            }

            // draw the main rect
            painter.setPen(borderColor);

            if (!m_isProcessed && m_isEnabled && !isSelected && !colorAllNodes)
            {
                if (m_isHighlighted == false)
                {
                    painter.setBrush(QColor(40, 40, 40));
                }
                else
                {
                    painter.setBrush(QColor(50, 50, 50));
                }
            }
            else
            {
                QLinearGradient bgGradient(0, m_rect.top(), 0, m_rect.bottom());
                bgGradient.setColorAt(0.0f, bgColor);
                bgGradient.setColorAt(1.0f, bgColor2);
                painter.setBrush(bgGradient);
            }

            painter.drawRoundedRect(m_rect, BORDER_RADIUS, BORDER_RADIUS);

            // if the scale is so small that we can't see those small things anymore
            QRect fullHeaderRect(m_rect.left(), m_rect.top(), m_rect.width(), 30);
            QRect headerRect(m_rect.left(), m_rect.top(), m_rect.width(), 15);
            QRect subHeaderRect(m_rect.left(), m_rect.top() + 13, m_rect.width(), 15);

            // if the scale is so small that we can't see those small things anymore
            if (m_parentGraph->GetScale() < 0.3f)
            {
                painter.setOpacity(1.0f);
                painter.setClipping(false);
                return;
            }

            // draw the header
            painter.setClipping(true);
            painter.setPen(borderColor);
            painter.setClipRect(fullHeaderRect, Qt::ReplaceClip);
            painter.setBrush(headerBgColor);
            painter.drawRoundedRect(m_rect, BORDER_RADIUS, BORDER_RADIUS);
            painter.setClipping(false);

            // if the scale is so small that we can't see those small things anymore
            if (m_parentGraph->GetScale() > 0.5f)
            {
                // draw the input ports
                QColor portBrushColor, portPenColor;
                const AZ::u16 numInputs = aznumeric_caster(m_inputPorts.size());
                for (AZ::u16 i = 0; i < numInputs; ++i)
                {
                    // get the input port and the corresponding rect
                    NodePort* inputPort = &m_inputPorts[i];
                    const QRect& portRect = inputPort->GetRect();

                    // get and set the pen and brush colors
                    GetNodePortColors(inputPort, borderColor, headerBgColor, &portBrushColor, &portPenColor);
                    painter.setBrush(portBrushColor);
                    painter.setPen(portPenColor);

                    // draw the port rect
                    painter.drawRect(portRect);
                }

                if (GetHasVisualOutputPorts())
                {
                    // draw the output ports
                    const AZ::u16 numOutputs = aznumeric_caster(m_outputPorts.size());
                    for (AZ::u16 i = 0; i < numOutputs; ++i)
                    {
                        // get the output port and the corresponding rect
                        NodePort* outputPort = &m_outputPorts[i];
                        const QRect& portRect = outputPort->GetRect();

                        // get and set the pen and brush colors
                        GetNodePortColors(outputPort, borderColor, headerBgColor, &portBrushColor, &portPenColor);
                        painter.setBrush(portBrushColor);
                        painter.setPen(portPenColor);

                        // draw the port rect
                        painter.drawRect(portRect);
                    }
                }
            }
        }
        else // it is collapsed
        {
            // is highlighted/hovered (on-mouse-over effect)
            if (m_isHighlighted)
            {
                bgColor = bgColor.lighter(160);
                headerBgColor = headerBgColor.lighter(160);
            }

            // if the scale is so small that we can't see those small things anymore
            QRect fullHeaderRect(m_rect.left(), m_rect.top(), m_rect.width(), 30);
            QRect headerRect(m_rect.left(), m_rect.top(), m_rect.width(), 15);
            QRect subHeaderRect(m_rect.left(), m_rect.top() + 13, m_rect.width(), 15);

            // draw the header
            painter.setPen(borderColor);
            painter.setBrush(headerBgColor);
            painter.drawRoundedRect(fullHeaderRect, 7.0, 7.0);

            // if the scale is so small that we can't see those small things anymore
            if (m_parentGraph->GetScale() < 0.3f)
            {
                painter.setOpacity(1.0f);
                return;
            }

            painter.setClipping(true);
            painter.setClipRect(fullHeaderRect, Qt::ReplaceClip);
            painter.setClipping(false);
        }

        if (m_parentGraph->GetScale() > 0.3f)
        {
            // draw the collapse triangle
            if (isSelected)
            {
                painter.setBrush(textColor);
                painter.setPen(headerBgColor);
            }
            else
            {
                painter.setPen(Qt::black);
                painter.setBrush(QColor(175, 175, 175));
            }

            if (m_isCollapsed == false)
            {
                QPoint triangle[3];
                triangle[0].setX(m_arrowRect.left());
                triangle[0].setY(m_arrowRect.top());
                triangle[1].setX(m_arrowRect.right());
                triangle[1].setY(m_arrowRect.top());
                triangle[2].setX(m_arrowRect.center().x());
                triangle[2].setY(m_arrowRect.bottom());
                painter.drawPolygon(triangle, 3, Qt::WindingFill);
            }
            else
            {
                QPoint triangle[3];
                triangle[0].setX(m_arrowRect.left());
                triangle[0].setY(m_arrowRect.top());
                triangle[1].setX(m_arrowRect.right());
                triangle[1].setY(m_arrowRect.center().y());
                triangle[2].setX(m_arrowRect.left());
                triangle[2].setY(m_arrowRect.bottom());
                painter.drawPolygon(triangle, 3, Qt::WindingFill);
            }

            // draw the visualize area
            if (m_canVisualize)
            {
                RenderVisualizeRect(painter, bgColor, bgColor2);
            }

            // render the tracks etc
            if (m_isCollapsed == false && m_emfxNode->GetHasOutputPose() && m_isProcessed)
            {
                RenderTracks(painter, bgColor, bgColor2);
            }

            // render the marker which indicates that you can go inside this node
            RenderHasChildsIndicator(painter, pen, borderColor, bgColor2);
        }

        // render the text overlay with the pre-baked node name and port names etc.
        float textOpacity = MCore::Clamp<float>(m_parentGraph->GetScale() * m_parentGraph->GetScale() * 1.5f, 0.0f, 1.0f);

        painter.setOpacity(textOpacity);

        // draw the title
        painter.setBrush(Qt::NoBrush);
        painter.setPen(textColor);
        painter.setFont(m_headerFont);
        painter.drawStaticText(m_rect.left(), m_rect.top(), m_titleText);

        // draw the subtitle
        painter.setFont(m_subTitleFont);
        painter.drawStaticText(m_rect.left(), aznumeric_cast<int>(m_rect.top() + m_titleText.size().height() - 3), m_subTitleText);

        // draw the info text
        if (m_isCollapsed == false)
        {
            // draw info text
            QRect textRect;
            CalcInfoTextRect(textRect, false);
            painter.setFont(m_infoTextFont);
            painter.setPen(QColor(255, 128, 0));
            painter.drawStaticText(m_rect.left(), textRect.top() + 4, m_infoText);

            painter.setPen(textColor);
            painter.setFont(m_portNameFont);

            // draw input port text
            const AZ::u16 numInputs = aznumeric_caster(m_inputPorts.size());
            for (AZ::u16 i = 0; i < numInputs; ++i)
            {
                NodePort* inputPort = &m_inputPorts[i];
                const QRect& portRect = inputPort->GetRect();
                if (inputPort->GetNameID() == MCORE_INVALIDINDEX32)
                {
                    continue;
                }
                painter.drawStaticText(m_rect.left() + 8, portRect.top() - 3, m_inputPortText[i]);
            }

            // draw output port text
            const AZ::u16 numOutputs = aznumeric_caster(m_outputPorts.size());
            for (AZ::u16 i = 0; i < numOutputs; ++i)
            {
                NodePort* outputPort = &m_outputPorts[i];
                const QRect& portRect = outputPort->GetRect();
                if (outputPort->GetNameID() == MCORE_INVALIDINDEX32)
                {
                    continue;
                }
                painter.drawStaticText(aznumeric_cast<int>(m_rect.right() - 10 - m_outputPortText[i].size().width()), portRect.top() - 3, m_outputPortText[i]);
            }
        }

        painter.setOpacity(1.0f);

        RenderDebugInfo(painter);
    }


    // return the required height
    int32 BlendTreeVisualNode::CalcRequiredHeight() const
    {
        return GraphNode::CalcRequiredHeight() + 12;
    }
}   // namespace EMStudio
