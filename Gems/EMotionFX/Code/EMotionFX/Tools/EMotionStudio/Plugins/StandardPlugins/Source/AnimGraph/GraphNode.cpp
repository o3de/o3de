/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    GraphNode::GraphNode(const QModelIndex& modelIndex, const char* name, uint32 numInputs, uint32 numOutputs)
        : m_modelIndex(modelIndex)
    {
        mConnections.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        mInputPorts.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        mOutputPorts.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

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

        mInputPorts.Resize(numInputs);
        mOutputPorts.Resize(numOutputs);

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
        const uint32 numInputs = mInputPorts.GetLength();
        mInputPortText.Resize(numInputs);
        for (uint32 i = 0; i < numInputs; ++i)
        {
            QStaticText& staticText = mInputPortText[i];
            staticText.setTextFormat(Qt::PlainText);
            staticText.setPerformanceHint(QStaticText::AggressiveCaching);
            //      staticText.setTextWidth( mRect.width() );
            staticText.setText(mInputPorts[i].GetName());
            staticText.prepare(QTransform(), mPortNameFont);
        }

        // output ports
        const uint32 numOutputs = mOutputPorts.GetLength();
        mOutputPortText.Resize(numOutputs);
        for (uint32 i = 0; i < numOutputs; ++i)
        {
            QStaticText& staticText = mOutputPortText[i];
            staticText.setTextFormat(Qt::PlainText);
            staticText.setPerformanceHint(QStaticText::AggressiveCaching);
            //      staticText.setTextWidth( mRect.width() );
            staticText.setText(mOutputPorts[i].GetName());
            staticText.prepare(QTransform(), mPortNameFont);
        }

        //-------------------------------------------
        /*
            // create a new pixmap with the new and correct resolution
            const uint32 nodeWidth  = mRect.width();
            const uint32 nodeHeight = mRect.height();
            mTextPixmap = QPixmap(nodeWidth, nodeHeight);

            // make the pixmap fully transparent
            mTextPixmap.fill(Qt::transparent);

            mTextPainter.begin( &mTextPixmap );

            // setup colors
            QColor textColor;
            if (!GetIsSelected())
            {
                if (mIsEnabled)
                    textColor = Qt::white;
                else
                    textColor = QColor( 100, 100, 100 );
            }
            else
                textColor = QColor(255,128,0);

            // some rects we need for the text
            QRect fullHeaderRect( 0, 0, mRect.width(), 25 );
            QRect headerRect( 0, 0, mRect.width(), 15 );
            QRect subHeaderRect( 0, 13, mRect.width(), 10 );

            // draw header text
            mTextPainter.setBrush( Qt::NoBrush );
            mTextPainter.setPen( textColor );
            mTextPainter.setFont( mHeaderFont );
            mTextPainter.drawText( headerRect, mElidedName, mTextOptionsCenter );

            mTextPainter.setFont( mSubTitleFont );
            mTextPainter.drawText( subHeaderRect, mElidedSubTitle, mTextOptionsCenter );

            if (mIsCollapsed == false)
            {
                // draw the info text
                QRect textRect;
                CalcInfoTextRect( textRect, true );
                mTextPainter.setPen( QColor(255,128,0) );
                mTextPainter.setFont( mInfoTextFont );
                mTextPainter.drawText( textRect, mElidedNodeInfo, mTextOptionsCenterHV );

                mTextPainter.setPen( textColor );

                // draw the input ports
                mTextPainter.setPen( textColor );
                mTextPainter.setFont( mPortNameFont );
                const uint32 numInputs = mInputPorts.GetLength();
                for (uint32 i=0; i<numInputs; ++i)
                {
                    // get the input port and the corresponding rect
                    NodePort* inputPort = &mInputPorts[i];
                    const QRect& portRect = inputPort->GetRect();

                    if (inputPort->GetNameID() == MCORE_INVALIDINDEX32)
                        continue;

                    // draw the text
                    CalcInputPortTextRect(i, textRect, true);
                    mTextPainter.drawText( textRect, inputPort->GetName(), mTextOptionsAlignLeft );
                }

                // draw the output ports
                const uint32 numOutputs = mOutputPorts.GetLength();
                for (uint32 i=0; i<numOutputs; ++i)
                {
                    // get the output port and the corresponding rect
                    NodePort* outputPort = &mOutputPorts[i];

                    if (outputPort->GetNameID() == MCORE_INVALIDINDEX32)
                        continue;

                    const QRect& portRect = outputPort->GetRect();

                    // draw the text
                    CalcOutputPortTextRect(i, textRect, true);
                    mTextPainter.drawText( textRect, outputPort->GetName(), mTextOptionsAlignRight );
                }
            }

            mTextPainter.end();
        */
    }


    // remove all node connections
    void GraphNode::RemoveAllConnections()
    {
        const uint32 numConnections = mConnections.GetLength();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            delete mConnections[i];
        }

        mConnections.Clear();
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
        uint32 i;
        const uint32 numInputPorts = mInputPorts.GetLength();
        for (i = 0; i < numInputPorts; ++i)
        {
            mInputPorts[i].SetRect(CalcInputPortRect(i));
            mInputPorts[i].SetIsHighlighted(false);
        }

        // update the output ports and reset the port highlight flags
        const uint32 numOutputPorts = mOutputPorts.GetLength();
        for (i = 0; i < numOutputPorts; ++i)
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
            for (i = 0; i < numInputPorts; ++i)
            {
                // get the input port and the corresponding rect
                NodePort* inputPort = &mInputPorts[i];
                const QRect& portRect = inputPort->GetRect();

                // check if the mouse position is inside the port rect and break the loop in this case, as the mouse can be only over one port at the time
                if (portRect.contains(mousePos))
                {
                    inputPort->SetIsHighlighted(true);
                    highlightedPortFound = true;
                    break;
                }
            }

            // only check the output ports if the mouse is not over any of the input ports as the mouse can't be at two ports at the same time (this is an optimization)
            if (highlightedPortFound == false)
            {
                // set the set highlight flags for the output ports
                for (i = 0; i < numOutputPorts; ++i)
                {
                    // get the output port and the corresponding rect
                    NodePort* outputPort = &mOutputPorts[i];
                    const QRect& portRect = outputPort->GetRect();

                    // check if the mouse position is inside the port rect and break the loop in this case, as the mouse can be only over one port at the time
                    if (portRect.contains(mousePos))
                    {
                        outputPort->SetIsHighlighted(true);
                        break;
                    }
                }
            }
        }
        
        // Update the connections
        const uint32 numConnections = GetNumConnections();
        for (uint32 c = 0; c < numConnections; ++c)
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
                const uint32 numInputs = mInputPorts.GetLength();
                for (uint32 i = 0; i < numInputs; ++i)
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
                    const uint32 numOutputs = mOutputPorts.GetLength();
                    for (uint32 i = 0; i < numOutputs; ++i)
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
        const uint32 numConnections = mConnections.GetLength();
        for (uint32 c = 0; c < numConnections; ++c)
        {
            NodeConnection* nodeConnection = mConnections[c];
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
            uint32 numPorts = MCore::Max<uint32>(mInputPorts.GetLength(), mOutputPorts.GetLength());
            uint32 result = (numPorts * 15) + 34;
            return MCore::Math::Align(result, 10);
        }
        else
        {
            return 30;
        }
    }


    // calc the max input port width
    uint32 GraphNode::CalcMaxInputPortWidth() const
    {
        // calc the maximum input port width
        uint32 maxInputWidth = 0;
        uint32 width;
        const uint32 numInputPorts = mInputPorts.GetLength();
        for (uint32 i = 0; i < numInputPorts; ++i)
        {
            const NodePort* nodePort = &mInputPorts[i];
            width = mPortFontMetrics->horizontalAdvance(nodePort->GetName());
            maxInputWidth = MCore::Max<uint32>(maxInputWidth, width);
        }

        return maxInputWidth;
    }

    // calculate the max output port width
    uint32 GraphNode::CalcMaxOutputPortWidth() const
    {
        // calc the maximum output port width
        uint32 width;
        uint32 maxOutputWidth = 0;
        const uint32 numOutputPorts = mOutputPorts.GetLength();
        for (uint32 i = 0; i < numOutputPorts; ++i)
        {
            width = mPortFontMetrics->horizontalAdvance(mOutputPorts[i].GetName());
            maxOutputWidth = MCore::Max<uint32>(maxOutputWidth, width);
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

        const uint32 infoWidth = mInfoFontMetrics->horizontalAdvance(mElidedNodeInfo);
        const uint32 totalPortWidth = mMaxInputWidth + mMaxOutputWidth + 40 + infoWidth;

        // make sure the node is at least 100 units in width
        uint32 headerWidth = mHeaderFontMetrics->horizontalAdvance(mElidedName) + 40;
        headerWidth = MCore::Max<uint32>(headerWidth, 100);

        mRequiredWidth = MCore::Max<uint32>(headerWidth, totalPortWidth);
        mNameAndPortsUpdated = true;

        mRequiredWidth = MCore::Math::Align(mRequiredWidth, 10);

        return mRequiredWidth;
    }

    // get the rect for a given input port
    QRect GraphNode::CalcInputPortRect(uint32 portNr)
    {
        return QRect(mRect.left() - 5, mRect.top() + 35 + portNr * 15, 8, 8);
    }


    // get the rect for a given output port
    QRect GraphNode::CalcOutputPortRect(uint32 portNr)
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
    void GraphNode::CalcInputPortTextRect(uint32 portNr, QRect& outRect, bool local)
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
    void GraphNode::CalcOutputPortTextRect(uint32 portNr, QRect& outRect, bool local)
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
        mInputPorts.Clear(false);
    }


    // remove all output ports
    void GraphNode::RemoveAllOutputPorts()
    {
        mOutputPorts.Clear(false);
    }


    // add a new input port
    NodePort* GraphNode::AddInputPort(bool updateTextPixMap)
    {
        mInputPorts.AddEmpty();
        mInputPorts.GetLast().SetNode(this);
        if (updateTextPixMap)
        {
            UpdateTextPixmap();
        }
        return &mInputPorts.GetLast();
    }


    // add a new output port
    NodePort* GraphNode::AddOutputPort(bool updateTextPixMap)
    {
        mOutputPorts.AddEmpty();
        mOutputPorts.GetLast().SetNode(this);
        if (updateTextPixMap)
        {
            UpdateTextPixmap();
        }
        return &mOutputPorts.GetLast();
    }

    /*
    // update port text path
    void GraphNode::UpdatePortTextPath()
    {
        mPortTextPath = QPainterPath();

        QRect textRect;
        const uint32 numInputs = mInputPorts.GetLength();
        for (uint32 i=0; i<numInputs; ++i)
        {
            // get the input port and the corresponding rect
            NodePort* inputPort = &mInputPorts[i];

            // add the text
            CalcInputPortTextRect( i, textRect );
            //painter.drawText( textRect, QString::fromWCharArray(inputPort->GetName()), mTextOptionsAlignLeft );
            mPortTextPath.addText( textRect.left(), textRect.center().y(), mPortNameFont, QString::fromWCharArray(inputPort->GetName()));
        }
    }
    */

    // remove all input ports
    NodePort* GraphNode::FindPort(int32 x, int32 y, uint32* outPortNr, bool* outIsInputPort, bool includeInputPorts)
    {
        uint32 i;

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
            const uint32 numInputPorts = mInputPorts.GetLength();
            for (i = 0; i < numInputPorts; ++i)
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
        const uint32 numOutputPorts = mOutputPorts.GetLength();
        for (i = 0; i < numOutputPorts; ++i)
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
        const uint32 numConnections = mConnections.GetLength();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            // if this is the connection we're searching for
            if (mConnections[i]->GetModelIndex().data(AnimGraphModel::ROLE_POINTER).value<void*>() == connection)
            {
                if (removeFromMemory)
                {
                    delete mConnections[i];
                }
                mConnections.Remove(i);
                return true;
            }
        }
        return false;
    }

    
    // Remove a given connection by model index
    bool GraphNode::RemoveConnection(const QModelIndex& modelIndex, bool removeFromMemory)
    {
        const uint32 numConnections = mConnections.GetLength();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            // if this is the connection we're searching for
            if (mConnections[i]->GetModelIndex() == modelIndex)
            {
                if (removeFromMemory)
                {
                    delete mConnections[i];
                }
                mConnections.Remove(i);
                return true;
            }
        }
        return false;
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
