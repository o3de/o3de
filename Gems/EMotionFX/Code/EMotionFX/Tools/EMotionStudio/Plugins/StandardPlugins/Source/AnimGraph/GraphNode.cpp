/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "GraphNode.h"
#include "NodeConnection.h"
#include "NodeGraph.h"
#include <MCore/Source/LogManager.h>


namespace EMStudio
{
    // statics
    QColor GraphNode::s_portHighlightColo   = QColor(255, 128, 0);
    QColor GraphNode::s_portHighlightBGColor = QColor(128, 64, 0);


    // constructor
    GraphNode::GraphNode(const QModelIndex& modelIndex, const char* name, AZ::u16 numInputs, AZ::u16 numOutputs)
        : m_modelIndex(modelIndex)
    {
        m_rect                   = QRect(0, 0, 200, 128);
        m_baseColor              = QColor(74, 63, 238);
        m_visualizeColor         = QColor(0, 255, 0);
        m_opacity                = 1.0f;
        m_finalRect              = m_rect;
        m_isDeletable            = true;
        m_isHighlighted          = false;
        m_conFromOutputOnly      = false;
        m_isCollapsed            = false;
        m_isProcessed            = false;
        m_isUpdated              = false;
        m_isEnabled              = true;
        m_visualize              = false;
        m_canVisualize           = false;
        m_visualizeHighlighted   = false;
        m_nameAndPortsUpdated    = false;
        m_canHaveChildren        = false;
        m_hasVisualGraph         = false;
        m_hasVisualOutputPorts   = true;
        m_maxInputWidth          = 0;
        m_maxOutputWidth         = 0;

        m_headerFont.setPixelSize(12);
        m_headerFont.setBold(true);
        m_portNameFont.setPixelSize(9);
        m_infoTextFont.setPixelSize(10);
        m_infoTextFont.setBold(true);
        m_subTitleFont.setPixelSize(10);

        // has child node indicator
        m_substPoly.resize(4);

        m_textOptionsCenter.setAlignment(Qt::AlignCenter);
        m_textOptionsCenterHv.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_textOptionsAlignRight.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_textOptionsAlignLeft.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        m_inputPorts.resize(numInputs);
        m_outputPorts.resize(numOutputs);

        // initialize the port metrics
        m_portFontMetrics    = new QFontMetrics(m_portNameFont);
        m_headerFontMetrics  = new QFontMetrics(m_headerFont);
        m_infoFontMetrics    = new QFontMetrics(m_infoTextFont);
        m_subTitleFontMetrics = new QFontMetrics(m_subTitleFont);

        SetName(name, false);
        ResetBorderColor();
    }


    // destructor
    GraphNode::~GraphNode()
    {
        // delete the font metrics
        delete m_portFontMetrics;
        delete m_headerFontMetrics;
        delete m_infoFontMetrics;
        delete m_subTitleFontMetrics;

        RemoveAllConnections();
    }


    // update the transparent pixmap that contains all text for the node
    void GraphNode::UpdateTextPixmap()
    {
        // init the title text
        m_titleText.setTextOption(m_textOptionsCenter);
        m_titleText.setTextFormat(Qt::PlainText);
        m_titleText.setPerformanceHint(QStaticText::AggressiveCaching);
        m_titleText.setTextWidth(m_rect.width());
        m_titleText.setText(m_elidedName);
        m_titleText.prepare(QTransform(), m_headerFont);

        // init the title text
        m_subTitleText.setTextOption(m_textOptionsCenter);
        m_subTitleText.setTextFormat(Qt::PlainText);
        m_subTitleText.setPerformanceHint(QStaticText::AggressiveCaching);
        m_subTitleText.setTextWidth(m_rect.width());
        m_subTitleText.setText(m_elidedSubTitle);
        m_subTitleText.prepare(QTransform(), m_subTitleFont);

        // draw the info text
        QRect textRect;
        CalcInfoTextRect(textRect, true);
        m_infoText.setTextOption(m_textOptionsCenterHv);
        m_infoText.setTextFormat(Qt::PlainText);
        m_infoText.setPerformanceHint(QStaticText::AggressiveCaching);
        m_infoText.setTextWidth(m_rect.width());
        m_infoText.setText(m_elidedNodeInfo);
        m_infoText.prepare(QTransform(), m_subTitleFont);

        // input ports
        const size_t numInputs = m_inputPorts.size();
        m_inputPortText.resize(numInputs);
        for (size_t i = 0; i < numInputs; ++i)
        {
            QStaticText& staticText = m_inputPortText[i];
            staticText.setTextFormat(Qt::PlainText);
            staticText.setPerformanceHint(QStaticText::AggressiveCaching);
            staticText.setText(m_inputPorts[i].GetName());
            staticText.prepare(QTransform(), m_portNameFont);
        }

        // output ports
        const size_t numOutputs = m_outputPorts.size();
        m_outputPortText.resize(numOutputs);
        for (size_t i = 0; i < numOutputs; ++i)
        {
            QStaticText& staticText = m_outputPortText[i];
            staticText.setTextFormat(Qt::PlainText);
            staticText.setPerformanceHint(QStaticText::AggressiveCaching);
            staticText.setText(m_outputPorts[i].GetName());
            staticText.prepare(QTransform(), m_portNameFont);
        }
    }


    // remove all node connections
    void GraphNode::RemoveAllConnections()
    {
        for (NodeConnection* connection : m_connections)
        {
            delete connection;
        }

        m_connections.clear();
    }


    // set the name of the node
    void GraphNode::SetName(const char* name, bool updatePixmap)
    {
        m_name = name;
        m_elidedName = m_headerFontMetrics->elidedText(name, Qt::ElideMiddle, MAX_NODEWIDTH);

        if (updatePixmap)
        {
            UpdateNameAndPorts();
            UpdateRects();
            UpdateTextPixmap();
        }
    }


    void GraphNode::SetSubTitle(const char* subTitle, bool updatePixmap)
    {
        m_subTitle = subTitle;
        m_elidedSubTitle = m_subTitleFontMetrics->elidedText(subTitle, Qt::ElideMiddle, MAX_NODEWIDTH);

        if (updatePixmap)
        {
            UpdateNameAndPorts();
            UpdateRects();
            UpdateTextPixmap();
        }
    }


    void GraphNode::SetNodeInfo(const AZStd::string& info)
    {
        m_nodeInfo = info;
        m_elidedNodeInfo = m_infoFontMetrics->elidedText(m_nodeInfo.c_str(), Qt::ElideMiddle, MAX_NODEWIDTH - m_maxInputWidth - m_maxOutputWidth);

        UpdateNameAndPorts();
        UpdateRects();
        UpdateTextPixmap();
    }


    void GraphNode::UpdateRects()
    {
        // calc window rect
        m_rect.setWidth(CalcRequiredWidth());
        m_rect.setHeight(CalcRequiredHeight());

        // calc the rect in screen space (after scrolling and zooming)
        m_finalRect = m_parentGraph->GetTransform().mapRect(m_rect);
    }


    // adjust the collapsed state
    void GraphNode::SetIsCollapsed(bool collapsed)
    {
        m_isCollapsed = collapsed;
        UpdateRects();
        UpdateTextPixmap();
    }


    // update the rects
    void GraphNode::Update(const QRect& visibleRect, const QPoint& mousePos)
    {
        UpdateRects();

        // check if this rect is visible
        m_isVisible = m_finalRect.intersects(visibleRect);

        // check if the node is visible and skip some calculations in case its not
        m_isHighlighted = false;
        m_visualizeHighlighted = false;
        // check if the mouse is over the node, if yes highlight the node
        if (m_isVisible && m_rect.contains(mousePos))
        {
            m_isHighlighted = true;
        }

        // set the arrow rect
        m_arrowRect.setCoords(m_rect.left() + 5, m_rect.top() + 9, m_rect.left() + 17, m_rect.top() + 20);

        // set the visualize rect
        m_visualizeRect.setCoords(m_rect.right() - 13, m_rect.top() + 6, m_rect.right() - 5, m_rect.top() + 14);

        // update the input ports and reset the port highlight flags
        const AZ::u16 numInputPorts = aznumeric_caster(m_inputPorts.size());
        for (AZ::u16 i = 0; i < numInputPorts; ++i)
        {
            m_inputPorts[i].SetRect(CalcInputPortRect(i));
            m_inputPorts[i].SetIsHighlighted(false);
        }

        // update the output ports and reset the port highlight flags
        const AZ::u16 numOutputPorts = aznumeric_caster(m_outputPorts.size());
        for (AZ::u16 i = 0; i < numOutputPorts; ++i)
        {
            m_outputPorts[i].SetRect(CalcOutputPortRect(i));
            m_outputPorts[i].SetIsHighlighted(false);
        }

        // update the visualize highlight flag, only do this in case:
        // the mouse position is inside the node and we haven't zoomed too much out
        if (m_isHighlighted && m_parentGraph->GetScale() > 0.3f)
        {
            if (m_canVisualize && GetIsInsideVisualizeRect(mousePos))
            {
                m_visualizeHighlighted = true;
            }
        }

        // update the port highlight flags, only do this in case:
        // 1. the node is NOT collapsed
        // 2. we haven't zoomed too much out so that the ports aren't visible anymore
        // 3. the mouse position is inside the adjusted node rect, adjusted because the ports stand bit out of the node
        if (m_isCollapsed == false && m_parentGraph->GetScale() > 0.5f && m_rect.adjusted(-6, 0, 6, 0).contains(mousePos))
        {
            // set the set highlight flags for the input ports
            bool highlightedPortFound = false;
            for (NodePort& inputPort : m_inputPorts)
            {
                // get the input port and the corresponding rect
                const QRect& portRect = inputPort.GetRect();

                // check if the mouse position is inside the port rect and break the loop in this case, as the mouse can be only over one port at the time
                if (portRect.contains(mousePos))
                {
                    inputPort.SetIsHighlighted(true);
                    highlightedPortFound = true;
                    break;
                }
            }

            // only check the output ports if the mouse is not over any of the input ports as the mouse can't be at two ports at the same time (this is an optimization)
            if (highlightedPortFound == false)
            {
                // set the set highlight flags for the output ports
                for (NodePort& outputPort : m_outputPorts)
                {
                    // get the output port and the corresponding rect
                    const QRect& portRect = outputPort.GetRect();

                    // check if the mouse position is inside the port rect and break the loop in this case, as the mouse can be only over one port at the time
                    if (portRect.contains(mousePos))
                    {
                        outputPort.SetIsHighlighted(true);
                        break;
                    }
                }
            }
        }
        
        // Update the connections
        const size_t numConnections = GetNumConnections();
        for (size_t c = 0; c < numConnections; ++c)
        {
            GetConnection(c)->Update(visibleRect, mousePos);
        }
    }


    void GraphNode::Render(QPainter& painter, QPen* pen, bool renderShadow)
    {
        // only render if the given node is visible
        if (m_isVisible == false)
        {
            return;
        }

        // this line should never be reached in our anim graph plugin!
        MCORE_ASSERT(false);

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

        // border color
        QColor borderColor;
        pen->setWidth(1);
        const bool isSelected = GetIsSelected();

        if (isSelected)
        {
            borderColor.setRgb(255, 128, 0);

            if (m_parentGraph->GetScale() > 0.75f)
            {
                pen->setWidth(2);
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
                bgColor = m_baseColor;
            }
            else
            {
                bgColor.setRgbF(0.3f, 0.3f, 0.3f, 1.0f);
            }
        }

        QColor bgColor2;
        QColor headerBgColor;
        bgColor2      = bgColor.lighter(30);// make darker actually, 30% of the old color, same as bgColor * 0.3f;
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
            QLinearGradient bgGradient(0, m_rect.top(), 0, m_rect.bottom());
            bgGradient.setColorAt(0.0f, bgColor);
            bgGradient.setColorAt(1.0f, bgColor2);
            painter.setBrush(bgGradient);
            painter.setPen(borderColor);
            painter.drawRoundedRect(m_rect, BORDER_RADIUS, BORDER_RADIUS);

            // if the scale is so small that we can't see those small things anymore
            QRect fullHeaderRect(m_rect.left(), m_rect.top(), m_rect.width(), 25);
            QRect headerRect(m_rect.left(), m_rect.top(), m_rect.width(), 15);
            QRect subHeaderRect(m_rect.left(), m_rect.top() + 13, m_rect.width(), 10);

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
                QRect textRect;

                // draw the info text
                CalcInfoTextRect(textRect);
                painter.setPen(QColor(255, 128, 0));
                painter.setFont(m_infoTextFont);
                painter.drawText(textRect, m_elidedNodeInfo, m_textOptionsCenterHv);

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

                    // draw the text
                    CalcInputPortTextRect(i, textRect);
                    painter.setPen(textColor);
                    painter.setFont(m_portNameFont);
                    painter.drawText(textRect, inputPort->GetName(), m_textOptionsAlignLeft);
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

                        // draw the text
                        CalcOutputPortTextRect(i, textRect);
                        painter.setPen(textColor);
                        painter.setFont(m_portNameFont);
                        painter.drawText(textRect, outputPort->GetName(), m_textOptionsAlignRight);
                    }
                }
            }
        }
        else
        {
            // is highlighted/hovered (on-mouse-over effect)
            if (m_isHighlighted)
            {
                bgColor = bgColor.lighter(160);
                headerBgColor = headerBgColor.lighter(160);
            }

            // if the scale is so small that we can't see those small things anymore
            QRect fullHeaderRect(m_rect.left(), m_rect.top(), m_rect.width(), 25);
            QRect headerRect(m_rect.left(), m_rect.top(), m_rect.width(), 15);
            QRect subHeaderRect(m_rect.left(), m_rect.top() + 13, m_rect.width(), 10);

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

            // draw header text
            QTextOption textOptions;
            textOptions.setAlignment(Qt::AlignCenter);
            painter.setPen(textColor);
            painter.setFont(m_headerFont);
            painter.drawText(headerRect, m_elidedName, textOptions);

            painter.setFont(m_subTitleFont);
            painter.drawText(subHeaderRect, m_elidedSubTitle, textOptions);
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

            // render the marker which indicates that you can go inside this node
            RenderHasChildsIndicator(painter, pen, borderColor, bgColor2);
        }
    }


    // render the can-go-into and has-child-nodes indicator
    void GraphNode::RenderHasChildsIndicator(QPainter& painter, QPen* pen, QColor borderColor, QColor bgColor)
    {
        MCORE_UNUSED(pen);

        // render the marker which indicates that you can go inside this node
        if (m_canHaveChildren || m_hasVisualGraph)
        {
            const int indicatorSize = 13;
            QRect childIndicatorRect(aznumeric_cast<int>(m_rect.right() - indicatorSize - 2 * BORDER_RADIUS), m_rect.top(), aznumeric_cast<int>(indicatorSize + 2 * BORDER_RADIUS + 1), aznumeric_cast<int>(indicatorSize + 2 * BORDER_RADIUS));

            // set the border color to the same one as the node border
            painter.setPen(borderColor);

            // set the background color for the indicator
            if (GetIsSelected())
            {
                painter.setBrush(bgColor);
            }
            else
            {
                painter.setBrush(GetHasChildIndicatorColor());
            }

            // construct the clipping polygon
            m_substPoly[0] = QPointF(childIndicatorRect.right() - indicatorSize,               childIndicatorRect.top());            // top right
            m_substPoly[1] = QPointF(childIndicatorRect.right() - 5 * indicatorSize,             childIndicatorRect.top());          // top left
            m_substPoly[2] = QPointF(childIndicatorRect.right() + 1,                           childIndicatorRect.top() + 5 * indicatorSize);// bottom down
            m_substPoly[3] = QPointF(childIndicatorRect.right() + 1,                           childIndicatorRect.top() + indicatorSize);// bottom up

            // matched mini rounded rect on top of the node rect
            QPainterPath path;
            path.addRoundedRect(childIndicatorRect, BORDER_RADIUS, BORDER_RADIUS);

            // substract the clipping polygon from the mini rounded rect
            QPainterPath substPath;
            substPath.addPolygon(m_substPoly);
            QPainterPath finalPath = path.subtracted(substPath);

            // draw the indicator
            painter.drawPath(finalPath);
        }
    }


    // get the border and background color for a node port
    void GraphNode::GetNodePortColors(NodePort* nodePort, const QColor& borderColor, const QColor& headerBgColor, QColor* outBrushColor, QColor* outPenColor)
    {
        if (GetIsSelected())
        {
            *outPenColor    = borderColor;
            *outBrushColor  = headerBgColor;
        }
        else
        {
            if (nodePort->GetIsHighlighted() == false)
            {
                *outPenColor    = borderColor;
                *outBrushColor  = nodePort->GetColor();
            }
            else
            {
                *outPenColor    = s_portHighlightColo;
                *outBrushColor  = s_portHighlightBGColor;
            }
        }
    }


    // render the shadow for this node
    void GraphNode::RenderShadow(QPainter& painter)
    {
        float opacityFactor = m_opacity;
        if (m_isEnabled == false)
        {
            opacityFactor = 0.10f;
        }

        painter.setOpacity(opacityFactor);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 70));

        // normal
        if (m_isCollapsed == false)
        {
            QRect shadowRect = m_rect;
            shadowRect.translate(3, 4);

            // draw the shadow rect
            painter.drawRoundedRect(shadowRect, 7.0, 7.0);
        }
        else // collapsed
        {
            QRect shadowRect(m_rect.left(), m_rect.top(), m_rect.width(), 25);
            shadowRect.translate(3, 4);

            // draw the shadow rect
            painter.drawRoundedRect(shadowRect, 7.0, 7.0);
        }
    }


    // render all connections of this node
    void GraphNode::RenderConnections(const QItemSelectionModel& selectionModel, QPainter& painter, QPen* pen, QBrush* brush, const QRect& invMappedVisibleRect, int32 stepSize)
    {
        const bool alwaysColor = GetAlwaysColor();

        // for all connections
        for (NodeConnection* nodeConnection : m_connections)
        {
            if (nodeConnection->GetIsVisible())
            {
                float opacity = 1.0f;
                if (!m_isEnabled)
                {
                    opacity = 0.25f;
                }
                GraphNode* source = nodeConnection->GetSourceNode();
                if (source)
                {
                    if (!source->GetIsEnabled())
                    {
                        opacity = 0.25f;
                    }
                    if (source->GetOpacity() < 0.35f)
                    {
                        opacity = 0.15f;
                    }
                }
                nodeConnection->Render(selectionModel, painter, pen, brush, stepSize, invMappedVisibleRect, opacity, alwaysColor);
            }
        }

        painter.setOpacity(1.0f);
    }


    // render the visualize rect
    void GraphNode::RenderVisualizeRect(QPainter& painter, const QColor& bgColor, const QColor& bgColor2)
    {
        QColor vizBorder;
        QColor vizBackGround = bgColor2.lighter(110);
        if (m_visualize)
        {
            vizBorder = Qt::black;
        }
        else
        {
            vizBorder = bgColor.darker(180);
        }

        painter.setPen(m_visualizeHighlighted ? QColor(255, 128, 0) : vizBorder);
        if (!GetIsSelected())
        {
            painter.setBrush(m_visualize ? m_visualizeColor : vizBackGround);
        }
        else
        {
            painter.setBrush(m_visualize ? QColor(255, 128, 0) : bgColor);
        }

        painter.drawRect(m_visualizeRect);
    }


    // test if a point is inside the node
    bool GraphNode::GetIsInside(const QPoint& globalPoint) const
    {
        return m_finalRect.contains(globalPoint);
    }


    // check if we are selected
    bool GraphNode::GetIsSelected() const
    {
        return m_parentGraph->GetAnimGraphModel().GetSelectionModel().isSelected(m_modelIndex);
    }


    // move the node relatively
    void GraphNode::MoveRelative(const QPoint& deltaMove)
    {
        m_rect.translate(deltaMove);
    }


    // move absolute
    void GraphNode::MoveAbsolute(const QPoint& newUpperLeft)
    {
        const int32 width = m_rect.width();
        const int32 height = m_rect.height();
        m_rect = QRect(newUpperLeft.x(), newUpperLeft.y(), width, height);
    }


    // calculate the height (including title and bottom)
    int32 GraphNode::CalcRequiredHeight() const
    {
        if (m_isCollapsed == false)
        {
            int32 numPorts = aznumeric_caster(AZStd::max(m_inputPorts.size(), m_outputPorts.size()));
            int32 result = (numPorts * 15) + 34;
            return MCore::Math::Align(result, 10);
        }
        else
        {
            return 30;
        }
    }


    // calc the max input port width
    int GraphNode::CalcMaxInputPortWidth() const
    {
        // calc the maximum input port width
        int maxInputWidth = 0;
        for (const NodePort& nodePort : m_inputPorts)
        {
            maxInputWidth = AZStd::max(maxInputWidth, m_portFontMetrics->horizontalAdvance(nodePort.GetName()));
        }

        return maxInputWidth;
    }

    // calculate the max output port width
    int GraphNode::CalcMaxOutputPortWidth() const
    {
        // calc the maximum output port width
        int maxOutputWidth = 0;
        for (const NodePort& nodePort : m_outputPorts)
        {
            maxOutputWidth = AZStd::max(maxOutputWidth, m_portFontMetrics->horizontalAdvance(nodePort.GetName()));
        }

        return maxOutputWidth;
    }

    // calculate the width
    int32 GraphNode::CalcRequiredWidth()
    {
        if (m_nameAndPortsUpdated)
        {
            return m_requiredWidth;
        }

        // calc the maximum input port width
        m_maxInputWidth = CalcMaxInputPortWidth();
        m_maxOutputWidth = CalcMaxOutputPortWidth();

        const int infoWidth = m_infoFontMetrics->horizontalAdvance(m_elidedNodeInfo);
        const int totalPortWidth = m_maxInputWidth + m_maxOutputWidth + 40 + infoWidth;

        // make sure the node is at least 100 units in width
        const int headerWidth = AZStd::max(m_headerFontMetrics->horizontalAdvance(m_elidedName) + 40, 100);

        m_requiredWidth = AZStd::max(headerWidth, totalPortWidth);
        m_requiredWidth = MCore::Math::Align(m_requiredWidth, 10);

        m_nameAndPortsUpdated = true;

        return m_requiredWidth;
    }

    // get the rect for a given input port
    QRect GraphNode::CalcInputPortRect(AZ::u16 portNr)
    {
        return QRect(m_rect.left() - 5, m_rect.top() + 35 + portNr * 15, 8, 8);
    }


    // get the rect for a given output port
    QRect GraphNode::CalcOutputPortRect(AZ::u16 portNr)
    {
        return QRect(m_rect.right() - 5, m_rect.top() + 35 + portNr * 15, 8, 8);
    }


    // calculate the info text rect
    void GraphNode::CalcInfoTextRect(QRect& outRect, bool local)
    {
        if (local == false)
        {
            outRect = QRect(m_rect.left() + 15 + m_maxInputWidth, m_rect.top() + 24, m_rect.width() - 20 - m_maxInputWidth - m_maxOutputWidth, 20);
        }
        else
        {
            outRect = QRect(15 + m_maxInputWidth, 24, m_rect.width() - 20 - m_maxInputWidth - m_maxOutputWidth, 20);
        }
    }


    // calculate the text rect for the input port
    void GraphNode::CalcInputPortTextRect(AZ::u16 portNr, QRect& outRect, bool local)
    {
        if (local == false)
        {
            outRect = QRect(m_rect.left() + 10, m_rect.top() + 24 + portNr * 15, m_rect.width() - 20, 20);
        }
        else
        {
            outRect = QRect(10, 24 + portNr * 15, m_rect.width() - 20, 20);
        }
    }


    // calculate the text rect for the input port
    void GraphNode::CalcOutputPortTextRect(AZ::u16 portNr, QRect& outRect, bool local)
    {
        if (local == false)
        {
            outRect = QRect(m_rect.left() + 10, m_rect.top() + 24 + portNr * 15, m_rect.width() - 20, 20);
        }
        else
        {
            outRect = QRect(10, 24 + portNr * 15, m_rect.width() - 20, 20);
        }
    }


    // remove all input ports
    void GraphNode::RemoveAllInputPorts()
    {
        m_inputPorts.clear();
    }


    // remove all output ports
    void GraphNode::RemoveAllOutputPorts()
    {
        m_outputPorts.clear();
    }


    // add a new input port
    NodePort* GraphNode::AddInputPort(bool updateTextPixMap)
    {
        m_inputPorts.emplace_back();
        m_inputPorts.back().SetNode(this);
        if (updateTextPixMap)
        {
            UpdateTextPixmap();
        }
        return &m_inputPorts.back();
    }


    // add a new output port
    NodePort* GraphNode::AddOutputPort(bool updateTextPixMap)
    {
        m_outputPorts.emplace_back();
        m_outputPorts.back().SetNode(this);
        if (updateTextPixMap)
        {
            UpdateTextPixmap();
        }
        return &m_outputPorts.back();
    }


    // remove all input ports
    NodePort* GraphNode::FindPort(int32 x, int32 y, AZ::u16* outPortNr, bool* outIsInputPort, bool includeInputPorts)
    {
        // if the node is not visible at all skip directly
        if (m_isVisible == false)
        {
            return nullptr;
        }

        // if the node is collapsed we can skip directly, too
        if (m_isCollapsed)
        {
            return nullptr;
        }

        // check the input ports
        if (includeInputPorts)
        {
            const AZ::u16 numInputPorts = aznumeric_caster(m_inputPorts.size());
            for (AZ::u16 i = 0; i < numInputPorts; ++i)
            {
                QRect rect = CalcInputPortRect(i);
                if (rect.contains(QPoint(x, y)))
                {
                    *outPortNr      = i;
                    *outIsInputPort = true;
                    return &m_inputPorts[i];
                }
            }
        }

        // check the output ports
        const AZ::u16 numOutputPorts = aznumeric_caster(m_outputPorts.size());
        for (AZ::u16 i = 0; i < numOutputPorts; ++i)
        {
            QRect rect = CalcOutputPortRect(i);
            if (rect.contains(QPoint(x, y)))
            {
                *outPortNr      = i;
                *outIsInputPort = false;
                return &m_outputPorts[i];
            }
        }

        return nullptr;
    }

    // remove a given connection
    bool GraphNode::RemoveConnection(const void* connection, bool removeFromMemory)
    {
        const auto foundConnection = AZStd::find_if(begin(m_connections), end(m_connections), [match = connection](const NodeConnection* connection)
        {
            return connection->GetModelIndex().data(AnimGraphModel::ROLE_POINTER).value<void*>() == match;
        });

        if (foundConnection == end(m_connections))
        {
            return false;
        }

        if (removeFromMemory)
        {
            delete *foundConnection;
        }
        m_connections.erase(foundConnection);
        return true;
    }

    
    // Remove a given connection by model index
    bool GraphNode::RemoveConnection(const QModelIndex& modelIndex, bool removeFromMemory)
    {
        const auto foundConnection = AZStd::find_if(begin(m_connections), end(m_connections), [match = modelIndex](const NodeConnection* connection)
        {
            return connection->GetModelIndex() == match;
        });

        if (foundConnection == end(m_connections))
        {
            return false;
        }

        if (removeFromMemory)
        {
            delete *foundConnection;
        }
        m_connections.erase(foundConnection);
        return true;
    }


    // called when the name of a port got changed
    void NodePort::OnNameChanged()
    {
        if (m_node == nullptr)
        {
            return;
        }

        m_node->UpdateNameAndPorts();
        m_node->UpdateRects();
        m_node->UpdateTextPixmap();
    }


    // new temporary helper function for text drawing
    void GraphNode::RenderText(QPainter& painter, const QString& text, const QColor& textColor, const QFont& font, const QFontMetrics& fontMetrics, Qt::Alignment textAlignment, const QRect& rect)
    {
        painter.setFont(font);
        painter.setPen(Qt::NoPen);
        painter.setBrush(textColor);

        const float textWidth       = aznumeric_cast<float>(fontMetrics.horizontalAdvance(text));
        const float halfTextWidth   = aznumeric_cast<float>(textWidth * 0.5 + 0.5);
        const float halfTextHeight  = aznumeric_cast<float>(fontMetrics.height() * 0.5 + 0.5);
        const QPoint rectCenter     = rect.center();

        QPoint textPos;
        textPos.setY(aznumeric_cast<int>(rectCenter.y() + halfTextHeight - 1));

        switch (textAlignment)
        {
        case Qt::AlignLeft:
        {
            textPos.setX(rect.left() - 2);
            break;
        }

        case Qt::AlignRight:
        {
            textPos.setX(aznumeric_cast<int>(rect.right() - textWidth + 1));
            break;
        }

        default:
        {
            textPos.setX(aznumeric_cast<int>(rectCenter.x() - halfTextWidth + 1));
        }
        }

        QPainterPath path;
        path.addText(textPos, font, text);
        painter.drawPath(path);
    }
}   // namespace EMotionFX
