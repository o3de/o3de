/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "NodeConnection.h"
#include "NodeGraph.h"
#include "GraphNode.h"
#include <EMotionFX/Source/BlendTreeConnection.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <MCore/Source/FastMath.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/Vector.h>


namespace EMStudio
{
    // constructor
    NodeConnection::NodeConnection(NodeGraph* parentGraph, const QModelIndex& modelIndex, GraphNode* targetNode, AZ::u16 portNr, GraphNode* sourceNode, AZ::u16 sourceOutputPortNr)
        : m_modelIndex(modelIndex)
        , m_parentGraph(parentGraph)
    {
        m_sourceNode     = sourceNode;
        m_sourcePortNr   = sourceOutputPortNr;
        m_targetNode     = targetNode;
        m_portNr         = portNr;
        m_isVisible      = false;
        m_isProcessed    = false;
        m_isDashed       = false;
        m_isDisabled     = false;
        m_isHeadHighlighted = false;
        m_isTailHighlighted = false;
        m_isSynced       = false;
        m_color          = QColor(128, 255, 128);
    }


    // destructor
    NodeConnection::~NodeConnection()
    {
    }


    // update the connection
    void NodeConnection::Update(const QRect& visibleRect, const QPoint& mousePos)
    {
        MCORE_UNUSED(mousePos);

        // calculate the rects
        m_rect       = CalcRect();
        m_finalRect  = CalcFinalRect();

        // check for visibility
        m_isVisible  = m_finalRect.intersects(visibleRect);

        // reset the is highlighted flags
        m_isHighlighted          = false;
        m_isConnectedHighlighted = false;
    }


    // update the painter path
    void NodeConnection::UpdatePainterPath()
    {
        AZStd::vector<QPoint>& points = m_pathScratch;
        CalcPolyline(points);

        // Every stretch between two points is the usual horizontal curve, so a wire bent at waypoints still
        // leaves its output port and enters its input port sideways and passes through each waypoint level.
        m_painterPath = QPainterPath();
        m_painterPath.moveTo(points.front());
        const size_t lastIndex = points.size() - 1;
        for (size_t i = 1; i <= lastIndex; ++i)
        {
            const QPoint from = points[i - 1];
            const QPoint to = points[i];
            const int fromX = i == 1 ? from.x() + 3 : from.x();
            const int toX = i == lastIndex ? to.x() - 3 : to.x();
            if (i == 1)
            {
                m_painterPath.lineTo(fromX, from.y());
            }

            const float width = aznumeric_cast<float>(abs(toX - fromX));
            m_painterPath.cubicTo(from.x() + (width / 2), from.y(), to.x() - (width / 2), to.y(), toX, to.y());
            if (i == lastIndex)
            {
                m_painterPath.lineTo(to);
            }
        }
    }


    // render the connection
    void NodeConnection::Render([[maybe_unused]] const QItemSelectionModel& selectionModel, QPainter& painter, QPen* pen, QBrush* brush, int32 stepSize, const QRect& visibleRect, float opacity, bool alwaysColor)
    {
        AZ_UNUSED(stepSize);
        AZ_UNUSED(visibleRect);

        // used when relinking
        if (m_isDashed)
        {
            return;
        }

        painter.setOpacity(opacity);

        const float scale = m_sourceNode->GetParentGraph()->GetScale();
        QColor penColor;

        // draw some small horizontal lines that go outside of the connection port
        if (GetIsSelected())
        {
            penColor.setRgb(255, 128, 0);

            if (scale > 0.75f)
            {
                pen->setWidth(2);
            }
            else
            {
                pen->setWidth(1);
            }
        }
        else // unselected
        {
            // don't make it bold when not selected
            if (m_isProcessed == false && alwaysColor == false)
            {
                if (m_sourceNode)
                {
                    penColor.setRgb(75, 75, 75);
                }
                else
                {
                    uint32 col = aznumeric_cast<uint32>(scale * 200);
                    col = MCore::Clamp<uint32>(col, 130, 130);
                    penColor.setRgb(col, col, col);
                }
            }
            else
            {
                if (m_sourceNode)
                {
                    if (alwaysColor == false)
                    {
                        pen->setWidthF(1.5f);
                    }

                    penColor = m_sourceNode->GetOutputPort(m_sourcePortNr)->GetColor();
                }
                else
                {
                    penColor.setRgb(75, 75, 75);
                }
            }
            //penColor.setRgb(255,225,0);
            //          penColor.setRgb(255,0,255);
        }

        // lighten the color in case the transition is highlighted
        if (m_isHighlighted)
        {
            penColor = penColor.lighter(160);
        }

        // lighten the color in case the transition is connected to the currently selected node
        if (m_isConnectedHighlighted)
        {
            const float minInput = 0.1f;
            const float maxInput = 1.0f;
            const float minOutput = 3.0;
            const float maxOutput = 1.0;

            // clamp it so that the value is in the valid input range
            float x = MCore::Clamp<float>(scale, minInput, maxInput);

            // apply the simple linear conversion
            float result;
            if (MCore::Math::Abs(maxInput - minInput) > MCore::Math::epsilon)
            {
                result = ((x - minInput) / (maxInput - minInput)) * (maxOutput - minOutput) + minOutput;
            }
            else
            {
                result = minOutput;
            }

            pen->setWidth(aznumeric_cast<int>(result));
            penColor = penColor.lighter(160);
        }

        // blinking red error color
        if (m_sourceNode && m_sourceNode->GetHasError() && !GetIsSelected())
        {
            NodeGraph* parentGraph = m_targetNode->GetParentGraph();
            if (parentGraph->GetUseAnimation())
            {
                penColor = parentGraph->GetErrorBlinkColor();
            }
            else
            {
                penColor = Qt::red;
            }
        }

        // set the pen
        pen->setColor(penColor);
        if (m_isProcessed)
        {
            NodeGraph* parentGraph = m_targetNode->GetParentGraph();
            if (parentGraph->GetScale() > 0.5f && parentGraph->GetUseAnimation())
            {
                pen->setStyle(Qt::PenStyle::DashLine);
                pen->setDashOffset(parentGraph->GetDashOffset());
            }
            else
            {
                pen->setStyle(Qt::PenStyle::SolidLine);
                //pen->setDashOffset( -parentGraph->GetDashOffset() );
            }

            pen->setWidth(2);
        }
        else
        {
            pen->setStyle(Qt::SolidLine);
        }

        painter.setPen(*pen);

        // set the brush
        //brush->setColor(penColor);
        //painter.setBrush( *brush );
        painter.setBrush(Qt::NoBrush);

        // draw the curve
        UpdatePainterPath();
        painter.drawPath(m_painterPath);

        // The waypoint handles take the wire's colour, so they read as part of it.
        if (GetNumWaypoints() > 0)
        {
            RenderWaypoints(painter, *brush, *pen, penColor);
            painter.setBrush(Qt::NoBrush);
        }

        // restore opacity and width
        painter.setOpacity(1.0f);
        pen->setWidth(1);
    }


    // get the source rect
    QRect NodeConnection::GetSourceRect() const
    {
        if (m_sourceNode)
        {
            if (m_sourceNode->GetIsCollapsed() == false)
            {
                return m_sourceNode->GetOutputPort(m_sourcePortNr)->GetRect();
            }
            else
            {
                return CalcCollapsedSourceRect();
            }
        }

        QRect defaultRect = GetTargetRect();
        defaultRect.setLeft(defaultRect.left() - WILDCARDTRANSITION_SIZE);
        defaultRect.setTop(defaultRect.top() - WILDCARDTRANSITION_SIZE);
        return defaultRect;
    }


    // get the target rect
    QRect NodeConnection::GetTargetRect() const
    {
        if (m_targetNode->GetIsCollapsed() == false)
        {
            return m_targetNode->GetInputPort(m_portNr)->GetRect();
        }
        else
        {
            return CalcCollapsedTargetRect();
        }
    }


    // intersects this connection?
    bool NodeConnection::Intersects(const QRect& rect)
    {
        if (m_rect.intersects(rect) == false)
        {
            return false;
        }

        // get the port rects
        //QRect sourceRect = GetSourceRect();
        //QRect targetRect = GetTargetRect();

        //return NodeGraph::RectIntersectsSmoothedLine(rect, sourceRect.center().x(), sourceRect.center().y(), targetRect.center().x(), targetRect.center().y());

        //QPainterPath testPath;
        //testPath.addRect( rect );

        UpdatePainterPath();
        return m_painterPath.intersects(rect);
    }


    // is it close to this connection?
    bool NodeConnection::CheckIfIsCloseTo(const QPoint& point)
    {
        // if we're not visible don't check
        if (m_isVisible == false)
        {
            return false;
        }

        if (m_rect.contains(point) == false)
        {
            return false;
        }

        // get the port rects
        //QRect sourceRect = GetSourceRect();
        //QRect targetRect = GetTargetRect();

        //return NodeGraph::PointCloseToSmoothedLine(sourceRect.center().x(), sourceRect.center().y(), targetRect.center().x(), targetRect.center().y(), point.x(), point.y());

        int32 size = 6;
        int32 halfSize = size / 2;
        QRect testRect(point.x() - halfSize, point.y() - halfSize, size, size);
        return Intersects(testRect);
    }


    // get the collapsed source rect
    QRect NodeConnection::CalcCollapsedSourceRect() const
    {
        QRect tempRect = m_sourceNode->GetRect();
        //  QPoint a = QPoint(tempRect.right(), tempRect.top() + tempRect.height() / 2);
        QPoint a = QPoint(tempRect.right(), tempRect.top() + 13);
        return QRect(a - QPoint(1, 1), a);
    }


    // get the collapsed target rect
    QRect NodeConnection::CalcCollapsedTargetRect() const
    {
        QRect tempRect = m_targetNode->GetRect();
        //  QPoint a = QPoint(tempRect.left(), tempRect.top() + tempRect.height() / 2);
        QPoint a = QPoint(tempRect.left(), tempRect.top() + 13);
        return QRect(a, a + QPoint(1, 1));
    }


    // calculate the bounds
    QRect NodeConnection::CalcRect() const
    {
        QRect sourceRect = GetSourceRect();
        QRect targetRect = GetTargetRect();
        QRect rect = sourceRect.united(targetRect);

        // The path bends through the waypoints, so they have to be inside for culling and hit testing.
        if (const AZStd::vector<AZ::Vector2>* waypoints = GetWaypoints())
        {
            for (const AZ::Vector2& waypoint : *waypoints)
            {
                rect = rect.united(QRect(
                    aznumeric_cast<int>(waypoint.GetX()) - s_waypointRadius,
                    aznumeric_cast<int>(waypoint.GetY()) - s_waypointRadius,
                    2 * s_waypointRadius + 1,
                    2 * s_waypointRadius + 1));
            }
        }

        return rect;
    }


    // calc the final rect
    QRect NodeConnection::CalcFinalRect() const
    {
        if (m_sourceNode)
        {
            return m_sourceNode->GetParentGraph()->GetTransform().mapRect(CalcRect());
        }

        if (m_targetNode)
        {
            return m_targetNode->GetParentGraph()->GetTransform().mapRect(CalcRect());
        }

        MCORE_ASSERT(false);
        return QRect();
    }

    bool NodeConnection::GetIsSelected() const
    {
        return m_parentGraph->GetAnimGraphModel().GetSelectionModel().isSelected(m_modelIndex);
    }

    const AZStd::vector<AZ::Vector2>* NodeConnection::GetWaypoints() const
    {
        const EMotionFX::BlendTreeConnection* connection =
            m_modelIndex.data(AnimGraphModel::ROLE_CONNECTION_POINTER).value<EMotionFX::BlendTreeConnection*>();
        return connection ? &connection->GetWaypoints() : nullptr;
    }

    void NodeConnection::SetWaypoints(const AZStd::vector<AZ::Vector2>& waypoints)
    {
        EMotionFX::BlendTreeConnection* connection =
            m_modelIndex.data(AnimGraphModel::ROLE_CONNECTION_POINTER).value<EMotionFX::BlendTreeConnection*>();
        if (connection)
        {
            connection->SetWaypoints(waypoints);
        }
    }

    void NodeConnection::SetWaypoint(size_t index, const AZ::Vector2& position)
    {
        const AZStd::vector<AZ::Vector2>* waypoints = GetWaypoints();
        if (!waypoints || index >= waypoints->size())
        {
            return;
        }

        AZStd::vector<AZ::Vector2> newWaypoints = *waypoints;
        newWaypoints[index] = position;
        SetWaypoints(newWaypoints);
    }

    size_t NodeConnection::GetNumWaypoints() const
    {
        const AZStd::vector<AZ::Vector2>* waypoints = GetWaypoints();
        return waypoints ? waypoints->size() : 0;
    }

    void NodeConnection::CalcPolyline(AZStd::vector<QPoint>& outPoints) const
    {
        outPoints.clear();

        const AZStd::vector<AZ::Vector2>* waypoints = GetWaypoints();
        outPoints.reserve((waypoints ? waypoints->size() : 0) + 2);

        // The wire runs one pixel below the port centres, as it always has.
        outPoints.push_back(GetSourceRect().center() + QPoint(0, 1));
        if (waypoints)
        {
            for (const AZ::Vector2& waypoint : *waypoints)
            {
                outPoints.push_back(QPoint(aznumeric_cast<int>(waypoint.GetX()), aznumeric_cast<int>(waypoint.GetY())));
            }
        }
        outPoints.push_back(GetTargetRect().center() + QPoint(0, 1));
    }

    QPoint NodeConnection::CalcPathMidpoint() const
    {
        AZStd::vector<QPoint>& points = m_pathScratch;
        CalcPolyline(points);

        AZ::Vector2 mid = AZ::Vector2::CreateZero();
        AZ::Vector2 dir = AZ::Vector2::CreateAxisX();
        float segmentLength = 0.0f;
        if (!FindPathMidpoint(points.data(), points.size(), mid, dir, segmentLength))
        {
            return points.front();
        }

        return QPoint(aznumeric_cast<int>(mid.GetX()), aznumeric_cast<int>(mid.GetY()));
    }

    size_t NodeConnection::FindWaypoint(const QPoint& point) const
    {
        const AZStd::vector<AZ::Vector2>* waypoints = GetWaypoints();
        if (!waypoints)
        {
            return InvalidIndex;
        }

        const AZ::Vector2 pickPoint(aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y()));
        const float pickRadius = aznumeric_cast<float>(s_waypointRadius) + 2.0f;
        for (size_t i = 0; i < waypoints->size(); ++i)
        {
            if (AZ::Vector2((*waypoints)[i] - pickPoint).GetLength() <= pickRadius)
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    size_t NodeConnection::FindClosestWaypoint(const QPoint& point) const
    {
        const AZStd::vector<AZ::Vector2>* waypoints = GetWaypoints();
        if (!waypoints || waypoints->empty())
        {
            return InvalidIndex;
        }

        const AZ::Vector2 pickPoint(aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y()));
        size_t closestIndex = 0;
        float closestDistance = -1.0f;
        for (size_t i = 0; i < waypoints->size(); ++i)
        {
            const float distance = AZ::Vector2((*waypoints)[i] - pickPoint).GetLength();
            if (closestDistance < 0.0f || distance < closestDistance)
            {
                closestDistance = distance;
                closestIndex = i;
            }
        }

        return closestIndex;
    }

    size_t NodeConnection::CalcWaypointInsertIndex(const QPoint& point) const
    {
        AZStd::vector<QPoint>& points = m_pathScratch;
        CalcPolyline(points);

        // Segment i runs from points[i] to points[i + 1], so splitting it means inserting a waypoint at index i.
        size_t closestSegment = 0;
        float closestDistance = -1.0f;
        for (size_t i = 1; i < points.size(); ++i)
        {
            const float distance = NodeGraph::DistanceToLine(
                aznumeric_cast<float>(points[i - 1].x()), aznumeric_cast<float>(points[i - 1].y()),
                aznumeric_cast<float>(points[i].x()), aznumeric_cast<float>(points[i].y()),
                aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y()));

            if (closestDistance < 0.0f || distance < closestDistance)
            {
                closestDistance = distance;
                closestSegment = i - 1;
            }
        }

        return closestSegment;
    }

    void NodeConnection::RenderWaypoints(QPainter& painter, QBrush& brush, QPen& pen, const QColor& color) const
    {
        const AZStd::vector<AZ::Vector2>* waypoints = GetWaypoints();
        if (!waypoints || waypoints->empty())
        {
            return;
        }

        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(1.0f);
        pen.setColor(color.lighter(160));
        brush.setStyle(Qt::SolidPattern);
        brush.setColor(color);
        painter.setPen(pen);
        painter.setBrush(brush);

        const qreal radius = aznumeric_cast<qreal>(s_waypointRadius);
        for (const AZ::Vector2& waypoint : *waypoints)
        {
            painter.drawEllipse(QPointF(waypoint.GetX(), waypoint.GetY()), radius, radius);
        }
    }

    bool NodeConnection::FindPathMidpoint(const QPoint* points, size_t pointCount, AZ::Vector2& outMid, AZ::Vector2& outDir, float& outSegmentLength)
    {
        if (pointCount < 2)
        {
            return false;
        }

        float totalLength = 0.0f;
        for (size_t i = 1; i < pointCount; ++i)
        {
            totalLength += AZ::Vector2(
                aznumeric_cast<float>(points[i].x() - points[i - 1].x()),
                aznumeric_cast<float>(points[i].y() - points[i - 1].y())).GetLength();
        }

        if (totalLength < MCore::Math::epsilon)
        {
            return false;
        }

        const float halfLength = totalLength * 0.5f;
        float distanceBefore = 0.0f;
        for (size_t i = 1; i < pointCount; ++i)
        {
            const AZ::Vector2 segmentStart(aznumeric_cast<float>(points[i - 1].x()), aznumeric_cast<float>(points[i - 1].y()));
            const AZ::Vector2 segment(aznumeric_cast<float>(points[i].x()) - segmentStart.GetX(),
                aznumeric_cast<float>(points[i].y()) - segmentStart.GetY());
            const float segmentLength = segment.GetLength();
            if (segmentLength < MCore::Math::epsilon)
            {
                continue;
            }

            if (distanceBefore + segmentLength >= halfLength)
            {
                outDir = segment.GetNormalized();
                outMid = segmentStart + outDir * (halfLength - distanceBefore);
                outSegmentLength = segmentLength;
                return true;
            }

            distanceBefore += segmentLength;
        }

        return false;
    }
}   // namespace EMotionFX
