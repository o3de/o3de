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
#include <MCore/Source/Color.h>


namespace EMStudio
{
    // statics
    QColor GraphNode::mPortHighlightColor   = QColor(255, 128, 0);
    QColor GraphNode::mPortHighlightBGColor = QColor(128, 64, 0);


    // constructor
    GraphNode::GraphNode(const QModelIndex& modelIndex, const char* name, AZ::u16 numInputs, AZ::u16 numOutputs)
        : m_modelIndex(modelIndex)
    {
        mRect                   = QRect(0, 0, 200, 128);
        mBaseColor              = QColor(74, 63, 238);
        mVisualizeColor         = QColor(0, 255, 0);
        mOpacity                = 1.0f;
        mFinalRect              = mRect;
        mIsDeletable            = true;
        mIsHighlighted          = false;
        mConFromOutputOnly      = false;
        mIsCollapsed            = false;
        mIsProcessed            = false;
        mIsUpdated              = false;
        mIsEnabled              = true;
        mVisualize              = false;
        mCanVisualize           = false;
        mVisualizeHighlighted   = false;
        mNameAndPortsUpdated    = false;
        mCanHaveChildren        = false;
        mHasVisualGraph         = false;
        mHasVisualOutputPorts   = true;
        mMaxInputWidth          = 0;
        mMaxOutputWidth         = 0;

        mHeaderFont.setPixelSize(12);
        mHeaderFont.setBold(true);
        mPortNameFont.setPixelSize(9);
        mInfoTextFont.setPixelSize(10);
        mInfoTextFont.setBold(true);
        mSubTitleFont.setPixelSize(10);

        // has child node indicator
        mSubstPoly.resize(4);

        mTextOptionsCenter.setAlignment(Qt::AlignCenter);
        mTextOptionsCenterHV.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        mTextOptionsAlignRight.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        mTextOptionsAlignLeft.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        mInputPorts.resize(numInputs);
        mOutputPorts.resize(numOutputs);

        // initialize the port metrics
        mPortFontMetrics    = new QFontMetrics(mPortNameFont);
        mHeaderFontMetrics  = new QFontMetrics(mHeaderFont);
        mInfoFontMetrics    = new QFontMetrics(mInfoTextFont);
        mSubTitleFontMetrics = new QFontMetrics(mSubTitleFont);

        SetName(name, false);
        ResetBorderColor();
    }


    // destructor
    GraphNode::~GraphNode()
    {
        // delete the font metrics
        delete mPortFontMetrics;
        delete mHeaderFontMetrics;
        delete mInfoFontMetrics;
        delete mSubTitleFontMetrics;

        RemoveAllConnections();
    }


    // update the transparent pixmap that contains all text for the node
    void GraphNode::UpdateTextPixmap()
    {
        // init the title text
        mTitleText.setTextOption(mTextOptionsCenter);
        mTitleText.setTextFormat(Qt::PlainText);
        mTitleText.setPerformanceHint(QStaticText::AggressiveCaching);
        mTitleText.setTextWidth(mRect.width());
        mTitleText.setText(mElidedName);
        mTitleText.prepare(QTransform(), mHeaderFont);

        // init the title text
        mSubTitleText.setTextOption(mTextOptionsCenter);
        mSubTitleText.setTextFormat(Qt::PlainText);
        mSubTitleText.setPerformanceHint(QStaticText::AggressiveCaching);
        mSubTitleText.setTextWidth(mRect.width());
        mSubTitleText.setText(mElidedSubTitle);
        mSubTitleText.prepare(QTransform(), mSubTitleFont);

        // draw the info text
        QRect textRect;
        CalcInfoTextRect(textRect, true);
        mInfoText.setTextOption(mTextOptionsCenterHV);
        mInfoText.setTextFormat(Qt::PlainText);
        mInfoText.setPerformanceHint(QStaticText::AggressiveCaching);
        mInfoText.setTextWidth(mRect.width());
        mInfoText.setText(mElidedNodeInfo);
        mInfoText.prepare(QTransform(), mSubTitleFont);

        // input ports
        const size_t numInputs = mInputPorts.size();
        mInputPortText.resize(numInputs);
        for (size_t i = 0; i < numInputs; ++i)
        {
            QStaticText& staticText = mInputPortText[i];
            staticText.setTextFormat(Qt::PlainText);
            staticText.setPerformanceHint(QStaticText::AggressiveCaching);
            //      staticText.setTextWidth( mRect.width() );
            staticText.setText(mInputPorts[i].GetName());
            staticText.prepare(QTransform(), mPortNameFont);
        }

        // output ports
        const size_t numOutputs = mOutputPorts.size();
        mOutputPortText.resize(numOutputs);
        for (size_t i = 0; i < numOutputs; ++i)
        {
            QStaticText& staticText = mOutputPortText[i];
            staticText.setTextFormat(Qt::PlainText);
            staticText.setPerformanceHint(QStaticText::AggressiveCaching);
            //      staticText.setTextWidth( mRect.width() );
            staticText.setText(mOutputPorts[i].GetName());
            staticText.prepare(QTransform(), mPortNameFont);
        }
    }


    // remove all node connections
    void GraphNode::RemoveAllConnections()
    {
        for (NodeConnection* connection : mConnections)
        {
            delete connection;
        }

        mConnections.clear();
    }


    // set the name of the node
    void GraphNode::SetName(const char* name, bool updatePixmap)
    {
        mName = name;
        mElidedName = mHeaderFontMetrics->elidedText(name, Qt::ElideMiddle, MAX_NODEWIDTH);

        if (updatePixmap)
        {
            UpdateNameAndPorts();
            UpdateRects();
            UpdateTextPixmap();
        }
    }


    void GraphNode::SetSubTitle(const char* subTitle, bool updatePixmap)
    {
        mSubTitle = subTitle;
        mElidedSubTitle = mSubTitleFontMetrics->elidedText(subTitle, Qt::ElideMiddle, MAX_NODEWIDTH);

        if (updatePixmap)
        {
            UpdateNameAndPorts();
            UpdateRects();
            UpdateTextPixmap();
        }
    }


    void GraphNode::SetNodeInfo(const AZStd::string& info)
    {
        mNodeInfo = info;
        mElidedNodeInfo = mInfoFontMetrics->elidedText(mNodeInfo.c_str(), Qt::ElideMiddle, MAX_NODEWIDTH - mMaxInputWidth - mMaxOutputWidth);

        UpdateNameAndPorts();
        UpdateRects();
        UpdateTextPixmap();
    }


    void GraphNode::UpdateRects()
    {
        // calc window rect
        mRect.setWidth(CalcRequiredWidth());
        mRect.setHeight(CalcRequiredHeight());

        // calc the rect in screen space (after scrolling and zooming)
        mFinalRect = mParentGraph->GetTransform().mapRect(mRect);
    }


    // adjust the collapsed state
    void GraphNode::SetIsCollapsed(bool collapsed)
    {
        mIsCollapsed = collapsed;
        UpdateRects();
        UpdateTextPixmap();
    }


    // update the rects
    void GraphNode::Update(const QRect& visibleRect, const QPoint& mousePos)
    {
        UpdateRects();

        // check if this rect is visible
        mIsVisible = mFinalRect.intersects(visibleRect);

        // check if the node is visible and skip some calculations in case its not
        mIsHighlighted = false;
        mVisualizeHighlighted = false;
        //if (mIsVisible)
        //{
        // check if the mouse is over the node, if yes highlight the node
        if (mIsVisible && mRect.contains(mousePos))
        {
            mIsHighlighted = true;
        }

        // set the arrow rect
        mArrowRect.setCoords(mRect.left() + 5, mRect.top() + 9, mRect.left() + 17, mRect.top() + 20);

        // set the visualize rect
        mVisualizeRect.setCoords(mRect.right() - 13, mRect.top() + 6, mRect.right() - 5, mRect.top() + 14);

        // update the input ports and reset the port highlight flags
        const AZ::u16 numInputPorts = aznumeric_caster(mInputPorts.size());
        for (AZ::u16 i = 0; i < numInputPorts; ++i)
        {
            mInputPorts[i].SetRect(CalcInputPortRect(i));
            mInputPorts[i].SetIsHighlighted(false);
        }

        // update the output ports and reset the port highlight flags
        const AZ::u16 numOutputPorts = aznumeric_caster(mOutputPorts.size());
        for (AZ::u16 i = 0; i < numOutputPorts; ++i)
        {
            mOutputPorts[i].SetRect(CalcOutputPortRect(i));
            mOutputPorts[i].SetIsHighlighted(false);
        }

        // update the visualize highlight flag, only do this in case:
        // the mouse position is inside the node and we haven't zoomed too much out
        if (mIsHighlighted && mParentGraph->GetScale() > 0.3f)
        {
            if (mCanVisualize && GetIsInsideVisualizeRect(mousePos))
            {
                mVisualizeHighlighted = true;
            }
        }

        // update the port highlight flags, only do this in case:
        // 1. the node is NOT collapsed
        // 2. we haven't zoomed too much out so that the ports aren't visible anymore
        // 3. the mouse position is inside the adjusted node rect, adjusted because the ports stand bit out of the node
        if (mIsCollapsed == false && mParentGraph->GetScale() > 0.5f && mRect.adjusted(-6, 0, 6, 0).contains(mousePos))
        {
            // set the set highlight flags for the input ports
            bool highlightedPortFound = false;
            for (NodePort& inputPort : mInputPorts)
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
                for (NodePort& outputPort : mOutputPorts)
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
        if (mIsVisible == false)
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

        float opacityFactor = mOpacity;
        if (mIsEnabled == false)
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

            if (mParentGraph->GetScale() > 0.75f)
            {
                pen->setWidth(2);
            }
        }
        else
        {
            /*  if (mHasError)
                    borderColor.setRgb(255,0,0);
                else if (mIsProcessed)
                    borderColor.setRgb(255,0,255);
                else
                    borderColor = mBorderColor;*/
        }

        // background and header colors
        QColor bgColor;
        if (isSelected)
        {
            bgColor.setRgbF(0.93f, 0.547f, 0.0f, 1.0f); //  rgb(72, 63, 238)
        }
        else // not selected
        {
            if (mIsEnabled)
            {
                bgColor = mBaseColor;
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
            if (mIsEnabled)
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


        if (mIsCollapsed == false)
        {
            // is highlighted/hovered (on-mouse-over effect)
            if (mIsHighlighted)
            {
                bgColor = bgColor.lighter(120);
                bgColor2 = bgColor2.lighter(120);
            }

            // draw the main rect
            QLinearGradient bgGradient(0, mRect.top(), 0, mRect.bottom());
            bgGradient.setColorAt(0.0f, bgColor);
            bgGradient.setColorAt(1.0f, bgColor2);
            painter.setBrush(bgGradient);
            painter.setPen(borderColor);
            painter.drawRoundedRect(mRect, BORDER_RADIUS, BORDER_RADIUS);

            // if the scale is so small that we can't see those small things anymore
            QRect fullHeaderRect(mRect.left(), mRect.top(), mRect.width(), 25);
            QRect headerRect(mRect.left(), mRect.top(), mRect.width(), 15);
            QRect subHeaderRect(mRect.left(), mRect.top() + 13, mRect.width(), 10);

            // if the scale is so small that we can't see those small things anymore
            if (mParentGraph->GetScale() < 0.3f)
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
            painter.drawRoundedRect(mRect, BORDER_RADIUS, BORDER_RADIUS);

            // draw header text
            // REPLACED BY PIXMAP
            /*painter.setBrush( Qt::NoBrush );
            painter.setPen( textColor );
            painter.setFont( mHeaderFont );
            painter.drawText( headerRect, mElidedName, mTextOptionsCenter );

            painter.setFont( mSubTitleFont );
            painter.setBrush( Qt::NoBrush );
            painter.setPen( textColor );
            painter.drawText( subHeaderRect, mElidedSubTitle, mTextOptionsCenter );*/
            painter.setClipping(false);

            // if the scale is so small that we can't see those small things anymore
            if (mParentGraph->GetScale() > 0.5f)
            {
                QRect textRect;

                // draw the info text
                CalcInfoTextRect(textRect);
                painter.setPen(QColor(255, 128, 0));
                painter.setFont(mInfoTextFont);
                painter.drawText(textRect, mElidedNodeInfo, mTextOptionsCenterHV);

                // draw the input ports
                QColor portBrushColor, portPenColor;
                const AZ::u16 numInputs = aznumeric_caster(mInputPorts.size());
                for (AZ::u16 i = 0; i < numInputs; ++i)
                {
                    // get the input port and the corresponding rect
                    NodePort* inputPort = &mInputPorts[i];
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
                    painter.setFont(mPortNameFont);
                    painter.drawText(textRect, inputPort->GetName(), mTextOptionsAlignLeft);
                }

                if (GetHasVisualOutputPorts())
                {
                    // draw the output ports
                    const AZ::u16 numOutputs = aznumeric_caster(mOutputPorts.size());
                    for (AZ::u16 i = 0; i < numOutputs; ++i)
                    {
                        // get the output port and the corresponding rect
                        NodePort* outputPort = &mOutputPorts[i];
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
                        painter.setFont(mPortNameFont);
                        painter.drawText(textRect, outputPort->GetName(), mTextOptionsAlignRight);
                    }
                }
            }
        }
        else
        {
            // is highlighted/hovered (on-mouse-over effect)
            if (mIsHighlighted)
            {
                bgColor = bgColor.lighter(160);
                headerBgColor = headerBgColor.lighter(160);
            }

            // if the scale is so small that we can't see those small things anymore
            QRect fullHeaderRect(mRect.left(), mRect.top(), mRect.width(), 25);
            QRect headerRect(mRect.left(), mRect.top(), mRect.width(), 15);
            QRect subHeaderRect(mRect.left(), mRect.top() + 13, mRect.width(), 10);

            // draw the header
            painter.setPen(borderColor);
            painter.setBrush(headerBgColor);
            painter.drawRoundedRect(fullHeaderRect, 7.0, 7.0);

            // if the scale is so small that we can't see those small things anymore
            if (mParentGraph->GetScale() < 0.3f)
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
            painter.setFont(mHeaderFont);
            painter.drawText(headerRect, mElidedName, textOptions);

            painter.setFont(mSubTitleFont);
            painter.drawText(subHeaderRect, mElidedSubTitle, textOptions);
            painter.setClipping(false);
        }

        if (mParentGraph->GetScale() > 0.3f)
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

            if (mIsCollapsed == false)
            {
                QPoint triangle[3];
                triangle[0].setX(mArrowRect.left());
                triangle[0].setY(mArrowRect.top());
                triangle[1].setX(mArrowRect.right());
                triangle[1].setY(mArrowRect.top());
                triangle[2].setX(mArrowRect.center().x());
                triangle[2].setY(mArrowRect.bottom());
                painter.drawPolygon(triangle, 3, Qt::WindingFill);
            }
            else
            {
                QPoint triangle[3];
                triangle[0].setX(mArrowRect.left());
                triangle[0].setY(mArrowRect.top());
                triangle[1].setX(mArrowRect.right());
                triangle[1].setY(mArrowRect.center().y());
                triangle[2].setX(mArrowRect.left());
                triangle[2].setY(mArrowRect.bottom());
                painter.drawPolygon(triangle, 3, Qt::WindingFill);
            }

            // draw the visualize area
            if (mCanVisualize)
            {
                RenderVisualizeRect(painter, bgColor, bgColor2);
            }

            // render the marker which indicates that you can go inside this node
            RenderHasChildsIndicator(painter, pen, borderColor, bgColor2);
        }

        /*  // render the text overlay with the pre-baked node name and port names etc.
            const float textOpacity = mParentGraph->GetScale();
            painter.setOpacity( textOpacity );
            painter.drawPixmap( mRect, mTextPixmap );
            painter.setOpacity( 1.0f );*/
    }


    // render the can-go-into and has-child-nodes indicator
    void GraphNode::RenderHasChildsIndicator(QPainter& painter, QPen* pen, QColor borderColor, QColor bgColor)
    {
        MCORE_UNUSED(pen);

        // render the marker which indicates that you can go inside this node
        if (mCanHaveChildren || mHasVisualGraph)
        {
            const int indicatorSize = 13;
            QRect childIndicatorRect(aznumeric_cast<int>(mRect.right() - indicatorSize - 2 * BORDER_RADIUS), mRect.top(), aznumeric_cast<int>(indicatorSize + 2 * BORDER_RADIUS + 1), aznumeric_cast<int>(indicatorSize + 2 * BORDER_RADIUS));

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
            mSubstPoly[0] = QPointF(childIndicatorRect.right() - indicatorSize,               childIndicatorRect.top());            // top right
            mSubstPoly[1] = QPointF(childIndicatorRect.right() - 5 * indicatorSize,             childIndicatorRect.top());          // top left
            mSubstPoly[2] = QPointF(childIndicatorRect.right() + 1,                           childIndicatorRect.top() + 5 * indicatorSize);// bottom down
            mSubstPoly[3] = QPointF(childIndicatorRect.right() + 1,                           childIndicatorRect.top() + indicatorSize);// bottom up

            // matched mini rounded rect on top of the node rect
            QPainterPath path;
            path.addRoundedRect(childIndicatorRect, BORDER_RADIUS, BORDER_RADIUS);

            // substract the clipping polygon from the mini rounded rect
            QPainterPath substPath;
            substPath.addPolygon(mSubstPoly);
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
                *outPenColor    = mPortHighlightColor;
                *outBrushColor  = mPortHighlightBGColor;
            }
        }
    }


    // render the shadow for this node
    void GraphNode::RenderShadow(QPainter& painter)
    {
        float opacityFactor = mOpacity;
        if (mIsEnabled == false)
        {
            opacityFactor = 0.10f;
        }

        painter.setOpacity(opacityFactor);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 70));

        // normal
        if (mIsCollapsed == false)
        {
            QRect shadowRect = mRect;
            shadowRect.translate(3, 4);

            // draw the shadow rect
            painter.drawRoundedRect(shadowRect, 7.0, 7.0);
        }
        else // collapsed
        {
            QRect shadowRect(mRect.left(), mRect.top(), mRect.width(), 25);
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
        for (NodeConnection* nodeConnection : mConnections)
        {
            if (nodeConnection->GetIsVisible())
            {
                float opacity = 1.0f;
                if (!mIsEnabled)
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
        if (mVisualize)
        {
            vizBorder = Qt::black;
        }
        else
        {
            vizBorder = bgColor.darker(180);
        }

        painter.setPen(mVisualizeHighlighted ? QColor(255, 128, 0) : vizBorder);
        if (!GetIsSelected())
        {
            painter.setBrush(mVisualize ? mVisualizeColor : vizBackGround);
        }
        else
        {
            painter.setBrush(mVisualize ? QColor(255, 128, 0) : bgColor);
        }

        painter.drawRect(mVisualizeRect);
    }


    // test if a point is inside the node
    bool GraphNode::GetIsInside(const QPoint& globalPoint) const
    {
        return mFinalRect.contains(globalPoint);
    }


    // check if we are selected
    bool GraphNode::GetIsSelected() const
    {
        return mParentGraph->GetAnimGraphModel().GetSelectionModel().isSelected(m_modelIndex);
    }


    // move the node relatively
    void GraphNode::MoveRelative(const QPoint& deltaMove)
    {
        mRect.translate(deltaMove);
    }


    // move absolute
    void GraphNode::MoveAbsolute(const QPoint& newUpperLeft)
    {
        const int32 width = mRect.width();
        const int32 height = mRect.height();
        mRect = QRect(newUpperLeft.x(), newUpperLeft.y(), width, height);
        //MCore::LOG("MoveAbsolute: (%i, %i, %i, %i)", mRect.top(), mRect.left(), mRect.bottom(), mRect.right());
    }


    // calculate the height (including title and bottom)
    int32 GraphNode::CalcRequiredHeight() const
    {
        if (mIsCollapsed == false)
        {
            int32 numPorts = aznumeric_caster(AZStd::max(mInputPorts.size(), mOutputPorts.size()));
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
        for (const NodePort& nodePort : mInputPorts)
        {
            maxInputWidth = AZStd::max(maxInputWidth, mPortFontMetrics->horizontalAdvance(nodePort.GetName()));
        }

        return maxInputWidth;
    }

    // calculate the max output port width
    int GraphNode::CalcMaxOutputPortWidth() const
    {
        // calc the maximum output port width
        int maxOutputWidth = 0;
        for (const NodePort& nodePort : mOutputPorts)
        {
            maxOutputWidth = AZStd::max(maxOutputWidth, mPortFontMetrics->horizontalAdvance(nodePort.GetName()));
        }

        return maxOutputWidth;
    }

    // calculate the width
    int32 GraphNode::CalcRequiredWidth()
    {
        if (mNameAndPortsUpdated)
        {
            return mRequiredWidth;
        }

        // calc the maximum input port width
        mMaxInputWidth = CalcMaxInputPortWidth();
        mMaxOutputWidth = CalcMaxOutputPortWidth();

        const int infoWidth = mInfoFontMetrics->horizontalAdvance(mElidedNodeInfo);
        const int totalPortWidth = mMaxInputWidth + mMaxOutputWidth + 40 + infoWidth;

        // make sure the node is at least 100 units in width
        const int headerWidth = AZStd::max(mHeaderFontMetrics->horizontalAdvance(mElidedName) + 40, 100);

        mRequiredWidth = AZStd::max(headerWidth, totalPortWidth);
        mRequiredWidth = MCore::Math::Align(mRequiredWidth, 10);

        mNameAndPortsUpdated = true;

        return mRequiredWidth;
    }

    // get the rect for a given input port
    QRect GraphNode::CalcInputPortRect(AZ::u16 portNr)
    {
        return QRect(mRect.left() - 5, mRect.top() + 35 + portNr * 15, 8, 8);
    }


    // get the rect for a given output port
    QRect GraphNode::CalcOutputPortRect(AZ::u16 portNr)
    {
        return QRect(mRect.right() - 5, mRect.top() + 35 + portNr * 15, 8, 8);
    }


    // calculate the info text rect
    void GraphNode::CalcInfoTextRect(QRect& outRect, bool local)
    {
        if (local == false)
        {
            outRect = QRect(mRect.left() + 15 + mMaxInputWidth, mRect.top() + 24, mRect.width() - 20 - mMaxInputWidth - mMaxOutputWidth, 20);
        }
        else
        {
            outRect = QRect(15 + mMaxInputWidth, 24, mRect.width() - 20 - mMaxInputWidth - mMaxOutputWidth, 20);
        }
    }


    // calculate the text rect for the input port
    void GraphNode::CalcInputPortTextRect(AZ::u16 portNr, QRect& outRect, bool local)
    {
        if (local == false)
        {
            outRect = QRect(mRect.left() + 10, mRect.top() + 24 + portNr * 15, mRect.width() - 20, 20);
        }
        else
        {
            outRect = QRect(10, 24 + portNr * 15, mRect.width() - 20, 20);
        }
    }


    // calculate the text rect for the input port
    void GraphNode::CalcOutputPortTextRect(AZ::u16 portNr, QRect& outRect, bool local)
    {
        if (local == false)
        {
            outRect = QRect(mRect.left() + 10, mRect.top() + 24 + portNr * 15, mRect.width() - 20, 20);
        }
        else
        {
            outRect = QRect(10, 24 + portNr * 15, mRect.width() - 20, 20);
        }
    }


    // remove all input ports
    void GraphNode::RemoveAllInputPorts()
    {
        mInputPorts.clear();
    }


    // remove all output ports
    void GraphNode::RemoveAllOutputPorts()
    {
        mOutputPorts.clear();
    }


    // add a new input port
    NodePort* GraphNode::AddInputPort(bool updateTextPixMap)
    {
        mInputPorts.emplace_back();
        mInputPorts.back().SetNode(this);
        if (updateTextPixMap)
        {
            UpdateTextPixmap();
        }
        return &mInputPorts.back();
    }


    // add a new output port
    NodePort* GraphNode::AddOutputPort(bool updateTextPixMap)
    {
        mOutputPorts.emplace_back();
        mOutputPorts.back().SetNode(this);
        if (updateTextPixMap)
        {
            UpdateTextPixmap();
        }
        return &mOutputPorts.back();
    }


    // remove all input ports
    NodePort* GraphNode::FindPort(int32 x, int32 y, AZ::u16* outPortNr, bool* outIsInputPort, bool includeInputPorts)
    {
        // if the node is not visible at all skip directly
        if (mIsVisible == false)
        {
            return nullptr;
        }

        // if the node is collapsed we can skip directly, too
        if (mIsCollapsed)
        {
            return nullptr;
        }

        // check the input ports
        if (includeInputPorts)
        {
            const AZ::u16 numInputPorts = aznumeric_caster(mInputPorts.size());
            for (AZ::u16 i = 0; i < numInputPorts; ++i)
            {
                QRect rect = CalcInputPortRect(i);
                if (rect.contains(QPoint(x, y)))
                {
                    *outPortNr      = i;
                    *outIsInputPort = true;
                    return &mInputPorts[i];
                }
            }
        }

        // check the output ports
        const AZ::u16 numOutputPorts = aznumeric_caster(mOutputPorts.size());
        for (AZ::u16 i = 0; i < numOutputPorts; ++i)
        {
            QRect rect = CalcOutputPortRect(i);
            if (rect.contains(QPoint(x, y)))
            {
                *outPortNr      = i;
                *outIsInputPort = false;
                return &mOutputPorts[i];
            }
        }

        return nullptr;
    }

    // remove a given connection
    bool GraphNode::RemoveConnection(const void* connection, bool removeFromMemory)
    {
        const auto foundConnection = AZStd::find_if(begin(mConnections), end(mConnections), [match = connection](const NodeConnection* connection)
        {
            return connection->GetModelIndex().data(AnimGraphModel::ROLE_POINTER).value<void*>() == match;
        });

        if (foundConnection == end(mConnections))
        {
            return false;
        }

        if (removeFromMemory)
        {
            delete *foundConnection;
        }
        mConnections.erase(foundConnection);
        return true;
    }

    
    // Remove a given connection by model index
    bool GraphNode::RemoveConnection(const QModelIndex& modelIndex, bool removeFromMemory)
    {
        const auto foundConnection = AZStd::find_if(begin(mConnections), end(mConnections), [match = modelIndex](const NodeConnection* connection)
        {
            return connection->GetModelIndex() == match;
        });

        if (foundConnection == end(mConnections))
        {
            return false;
        }

        if (removeFromMemory)
        {
            delete *foundConnection;
        }
        mConnections.erase(foundConnection);
        return true;
    }


    // called when the name of a port got changed
    void NodePort::OnNameChanged()
    {
        if (mNode == nullptr)
        {
            return;
        }

        mNode->UpdateNameAndPorts();
        mNode->UpdateRects();
        mNode->UpdateTextPixmap();
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
