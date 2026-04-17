/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "PropertyCurveCtrl.hxx"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/sort.h>

#include <QMenu>
#include <QSpacerItem>

namespace AzToolsFramework
{
    constexpr AZStd::string_view INTERPMODE_LINEAR = "Linear";
    constexpr AZStd::string_view INTERPMODE_STEP = "Step";
    constexpr AZStd::string_view INTERPMODE_CUBICIN = "CubicIn";
    constexpr AZStd::string_view INTERPMODE_CUBICOUT = "CubicOut";

    AZStd::string ConvertInterpModeToString(const AzFramework::EasingCurve::PointInterpolationMode& mode)
    {
        switch(mode)
        {
        case AzFramework::EasingCurve::PointInterpolationMode::LINEAR:
            return INTERPMODE_LINEAR;
        case AzFramework::EasingCurve::PointInterpolationMode::STEP:
            return INTERPMODE_STEP;
        case AzFramework::EasingCurve::PointInterpolationMode::CUBIC_IN:
            return INTERPMODE_CUBICIN;
        case AzFramework::EasingCurve::PointInterpolationMode::CUBIC_OUT:
            return INTERPMODE_CUBICOUT;
        default:
            return "";
        }
    }

    AzFramework::EasingCurve::PointInterpolationMode ConvertStringToInterpMode(const AZStd::string& mode)
    {
        if (mode == INTERPMODE_LINEAR)
        {
            return AzFramework::EasingCurve::PointInterpolationMode::LINEAR;
        }
        else if (mode == INTERPMODE_STEP)
        {
            return AzFramework::EasingCurve::PointInterpolationMode::STEP;
        }
        else if (mode == INTERPMODE_CUBICIN)
        {
            return AzFramework::EasingCurve::PointInterpolationMode::CUBIC_IN;
        }
        else if (mode == INTERPMODE_CUBICOUT)
        {
            return AzFramework::EasingCurve::PointInterpolationMode::CUBIC_OUT;
        }
        return AzFramework::EasingCurve::PointInterpolationMode::LINEAR;
    }

    QPoint TransformPointToScreen(float x, float y, int width, int height, int offsetX, int offsetY)
    {
        QPoint transformedPoint;
        transformedPoint.setX(static_cast<int>(x * width + offsetX));
        transformedPoint.setY(static_cast<int>(height * (1 - y) + offsetY));
        return transformedPoint;
    }

    QPainterPath CreatePathFromCurve(
        const AzFramework::EasingCurve::Point& start,
        const AzFramework::EasingCurve::Point& end,
        float curveStep,
        int width,
        int height,
        int offsetX,
        int offsetY)
    {
        QPainterPath path;
        QPointF pt = TransformPointToScreen(start.m_time, start.m_value, width, height, offsetX, offsetY);
        path.moveTo(pt);
        float value = 0;
        float time = 0;
        for (float step = 0; step < 1; step += curveStep)
        {
            time = start.m_time + (end.m_time - start.m_time) * step;
            value = AzFramework::EasingCurve::Interpolate(start, end, time);
            pt = TransformPointToScreen(time, value, width, height, offsetX, offsetY);
            path.lineTo(pt);
        }
        return path;
    }

    AzFramework::EasingCurve::Point ToKeyPoint(const QPointF& point, const AzFramework::EasingCurve::PointInterpolationMode& interpMode)
    {
        AzFramework::EasingCurve::Point keyNew;
        keyNew.m_time = static_cast<float>(point.x());
        keyNew.m_value = static_cast<float>(point.y());
        keyNew.m_interpMode = interpMode;
        return keyNew;
    }

    CurveEditor::CurveEditor(QWidget* parent)
        : QWidget(parent)
        , m_layout(this)
    {
        setFixedHeight(200);
        m_currentCurve.SetDefaultValue();
        SetupUi();
    }

    CurveEditor::~CurveEditor()
    {
    }

    void CurveEditor::SetupUi()
    {
        using namespace AZStd;
        using Key = pair<AZ::Vector2, string>;
        using Preset = vector<pair<Key, Key>>;
        Preset presetCurves;
        const AZ::Vector2 POINT_LEFT_BOTTOM = AZ::Vector2(0.0, 0.0);
        const AZ::Vector2 POINT_LEFT_TOP = AZ::Vector2(0.0, 1.0);
        const AZ::Vector2 POINT_RIGHT_BOTTOM = AZ::Vector2(1.0, 0.0);
        const AZ::Vector2 POINT_RIGHT_TOP = AZ::Vector2(1.0, 1.0);
        presetCurves.emplace_back(make_pair(make_pair(POINT_LEFT_TOP, INTERPMODE_LINEAR), make_pair(POINT_RIGHT_TOP, INTERPMODE_LINEAR)));
        presetCurves.emplace_back(make_pair(make_pair(POINT_LEFT_BOTTOM, INTERPMODE_LINEAR), make_pair(POINT_RIGHT_TOP, INTERPMODE_LINEAR)));
        presetCurves.emplace_back(make_pair(make_pair(POINT_LEFT_TOP, INTERPMODE_LINEAR), make_pair(POINT_RIGHT_BOTTOM, INTERPMODE_LINEAR)));
        presetCurves.emplace_back(make_pair(make_pair(POINT_LEFT_BOTTOM, INTERPMODE_LINEAR), make_pair(POINT_RIGHT_TOP, INTERPMODE_CUBICIN)));
        presetCurves.emplace_back(make_pair(make_pair(POINT_LEFT_TOP, INTERPMODE_LINEAR), make_pair(POINT_RIGHT_BOTTOM, INTERPMODE_CUBICOUT)));
        presetCurves.emplace_back(make_pair(make_pair(POINT_LEFT_BOTTOM, INTERPMODE_LINEAR), make_pair(POINT_RIGHT_TOP, INTERPMODE_CUBICOUT)));
        presetCurves.emplace_back(make_pair(make_pair(POINT_LEFT_TOP, INTERPMODE_LINEAR), make_pair(POINT_RIGHT_BOTTOM, INTERPMODE_CUBICIN)));

        for (size_t i = 0; i < presetCurves.size(); i++)
        {
            if (i == 0)
            {
                m_layout.addSpacing(m_scaleWidth);
            }
            else
            {
                m_layout.addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
            }
            auto preset = new PresetCurveWidget();
            auto start = presetCurves[i].first;
            auto stop = presetCurves[i].second;
            preset->SetKeyPoint(
                start.first.GetX(), start.first.GetY(), ConvertStringToInterpMode(start.second),
                stop.first.GetX(), stop.first.GetY(), ConvertStringToInterpMode(stop.second));
            m_layout.addWidget(preset);
            connect(preset, &PresetCurveWidget::onClick, this, [this, preset]()
                {
                    m_currentCurve.Clear();
                    m_currentCurve.AddPoint(preset->m_start);
                    m_currentCurve.AddPoint(preset->m_stop);
                    update();
                    emit curveChanged();
                });
            m_presetCurveWidget.emplace_back(preset);
        }

        m_layout.setContentsMargins(0, 0, 0, 0);
        m_layout.setAlignment(Qt::AlignTop);

        setMouseTracking(true);
    }

    const AzFramework::EasingCurve& CurveEditor::GetCurrentCurve() const
    {
        return m_currentCurve;
    }

    void CurveEditor::SetCurrentCurve(const AzFramework::EasingCurve& curve)
    {
        m_currentCurve = curve;
        update();
    }

    void CurveEditor::CheckMinMax(float& value, float min, float max) const
    {
        if (value < min)
        {
            value = min;
        }
        else if (value > max)
        {
            value = max;
        }
    }

    QPointF CurveEditor::TransformPointFromScreen(float ptx, float pty) const
    {
        float x = ptx - m_scaleWidth;
        float y = pty - m_mainRect.top();

        CheckMinMax(x, 0.0f, static_cast<float>(m_mainRect.width()));
        CheckMinMax(y, 0.0f, static_cast<float>(m_mainRect.height()));

        x = x / m_mainRect.width();
        y = (m_mainRect.height() - y) / m_mainRect.height();

        return QPointF(x, y);
    }

    QRect CurveEditor::CenterPointToRect(QPoint pt) const
    {
        const int KEY_WIDTH = 8;
        const int PART_TWO = 2;
        float x = float(pt.x() - KEY_WIDTH / PART_TWO);
        float y = float(pt.y() - KEY_WIDTH / PART_TWO);

        CheckMinMax(x, static_cast<float>(m_scaleWidth), static_cast<float>(rect().width() - KEY_WIDTH));
        CheckMinMax(y, static_cast<float>(m_mainRect.top()), static_cast<float>(m_mainRect.top() + m_mainRect.height() - KEY_WIDTH));

        return QRect(static_cast<int>(x), static_cast<int>(y), KEY_WIDTH, KEY_WIDTH);
    }

    void CurveEditor::DrawGrid(QPainter& painter) const
    {
        // CONSTANTS
        const AZStd::string_view AXIS_INFO_ONE = "1.0";
        const AZStd::string_view AXIS_INFO_ZERO = "0.0";
        const AZStd::string_view AXIS_INFO_Y = "0.5";
        const AZStd::string_view AXIS_INFO_X1 = "0.25";
        const AZStd::string_view AXIS_INFO_X2 = "0.50";
        const AZStd::string_view AXIS_INFO_X3 = "0.75";

        painter.setPen(QColor(43, 43, 43));
        // horizon
        painter.drawLine(
            m_scaleWidth, m_mainRect.height() / 2 + m_mainRect.top(), m_mainRect.right(),
            m_mainRect.height() / 2 + m_mainRect.top());
        // vertical
        float width = static_cast<float>(m_mainRect.width()) / 4.0f;

        QPointF begin;
        QPointF end;
        for (int i = 0; i < 3; i++)  // draw a line every 25% of the widget width
        {
            begin.setX(m_mainRect.left() + width * (i + 1));
            begin.setY(m_mainRect.top());
            end.setX(begin.x());
            end.setY(m_mainRect.bottom());
            painter.drawLine(begin, end);
        }

        QPen pen(Qt::gray);
        int offset = m_scaleWidth / 2;
        painter.setPen(pen);
        painter.drawText(0, m_mainRect.top() + 12, AXIS_INFO_ONE.data());
        painter.drawText(0, m_mainRect.top() + m_mainRect.height() / 2 + offset / 2, AXIS_INFO_Y.data());
        painter.drawText(0, m_mainRect.bottom(), AXIS_INFO_ZERO.data());

        painter.drawText(m_scaleWidth, rect().bottom(), AXIS_INFO_ZERO.data());
        painter.drawText(m_scaleWidth + static_cast<int>(width) - offset, rect().bottom(), AXIS_INFO_X1.data());
        painter.drawText(m_scaleWidth + static_cast<int>(width * 2) - offset, rect().bottom(), AXIS_INFO_X2.data());
        painter.drawText(m_scaleWidth + static_cast<int>(width * 3) - offset, rect().bottom(), AXIS_INFO_X3.data());
        painter.drawText(rect().right() - static_cast<int>(offset * 1.5), rect().bottom(), AXIS_INFO_ONE.data());
    }

    void CurveEditor::paintEvent(QPaintEvent* event)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(m_mainRect, QBrush(QColor(56, 56, 56)));

        // draw grids
        DrawGrid(painter);

        // draw keys
        QPen pen(Qt::red);
        QPen unselectedPen = QPen(Qt::gray);
        QPen selectedPen = QPen(Qt::yellow);
        painter.setPen(pen);
        for (int64_t index = 0; index < m_currentCurve.GetNumPoints(); index++)
        {
            AzFramework::EasingCurve::Point keyPoint = m_currentCurve.GetPoint(index);
            QPoint center = TransformPointToScreen(
                keyPoint.m_time, keyPoint.m_value, m_mainRect.width(), m_mainRect.height(), m_scaleWidth, m_mainRect.top());
            QRect rc = CenterPointToRect(center);
            painter.setPen((m_currentKeyIndex == index) ? selectedPen : unselectedPen);
            painter.setBrush(QBrush(Qt::gray));
            painter.drawRect(rc);
        }

        // draw curves
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        AzFramework::EasingCurve::Point keyStart;
        AzFramework::EasingCurve::Point keyEnd;
        QPainterPath path;
        const float CURVE_STEP = 0.005f;

        if (m_currentCurve.GetNumPoints() >= 2)
        {
            for (int64_t index = 0; index < m_currentCurve.GetNumPoints() - 1; index++)
            {
                keyStart = m_currentCurve.GetPoint(index);
                keyEnd = m_currentCurve.GetPoint(index+1);
                path = CreatePathFromCurve(keyStart, keyEnd, CURVE_STEP, m_mainRect.width(), m_mainRect.height(), m_scaleWidth, m_mainRect.top());
                painter.drawPath(path);
            }
        }

        QWidget::paintEvent(event);
    }

    void CurveEditor::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_keyPointSelected)
        {
            auto localPos = event->pos();
            UpdateCurveKey(localPos);
            update();
        }
        event->accept(); 
    }

    void CurveEditor::mousePressEvent(QMouseEvent* event)
    {
        QPoint currPos = event->pos();
        for (int64_t index = 0; index < m_currentCurve.GetNumPoints(); index++)
        {
            AzFramework::EasingCurve::Point k = m_currentCurve.GetPoint(index);
            QPoint center = TransformPointToScreen(k.m_time, k.m_value, m_mainRect.width(), m_mainRect.height(), m_scaleWidth, m_mainRect.top());
            QRect rc = CenterPointToRect(center);
            if (rc.contains(currPos))
            {
                m_keyPointSelected = true;
                m_currentKeyIndex = (int)index;
                AZStd::string toolTip = AZStd::string::format("time:%f\nvalue:%f\nmode:%s",
                    k.m_time, k.m_value, ConvertInterpModeToString(k.m_interpMode).c_str());
                setToolTip(toolTip.c_str());

                update();
                break;
            }
        }
        if (event->button() == Qt::RightButton)
        {
            UpdateMenu(event->pos());
        }
        event->accept(); 
    }


    void CurveEditor::mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_keyPointSelected)
        {
            if (m_keyPointDragged)
            {
                m_keyPointDragged = false;

                AzFramework::EasingCurve::Point keyPoint = m_currentCurve.GetPoint(m_currentKeyIndex);
                AZStd::string toolTip = AZStd::string::format("time:%f\nvalue:%f\nmode:%s",
                    keyPoint.m_time, keyPoint.m_value, ConvertInterpModeToString(keyPoint.m_interpMode).c_str());
                setToolTip(toolTip.c_str());
                update();
                emit curveChanged();
            }
            m_keyPointSelected = false;
        }

        event->accept(); 
        emit mouseRelease();
    }

    void CurveEditor::AddKey(QPoint pos)
    {
        QPointF pt = TransformPointFromScreen(static_cast<float>(pos.x()), static_cast<float>(pos.y()));
        auto keyNew = ToKeyPoint(pt, AzFramework::EasingCurve::PointInterpolationMode::CUBIC_IN);
        m_currentKeyIndex = m_currentCurve.AddPoint(keyNew);
        emit curveChanged();
    }

    void CurveEditor::UpdateMenu(QPoint pos)
    {
        QMenu menu;
        QMenu subMenu;
        if (m_keyPointSelected)
        {
            m_keyPointSelected = false;
            menu.addAction(tr("Delete Key"), this, &CurveEditor::DeleteKey);
            subMenu.setTitle(tr("Interpolation Mode"));
            subMenu.addAction(INTERPMODE_LINEAR.data(), this, &CurveEditor::UpdateKeyInterpMode);
            subMenu.addAction(INTERPMODE_STEP.data(), this, &CurveEditor::UpdateKeyInterpMode);
            subMenu.addAction(INTERPMODE_CUBICIN.data(), this, &CurveEditor::UpdateKeyInterpMode);
            subMenu.addAction(INTERPMODE_CUBICOUT.data(), this, &CurveEditor::UpdateKeyInterpMode);
            menu.addMenu(&subMenu);
        }
        else
        {
            menu.addAction(
                tr("Add Key"), this,
                [this, pos]()
                {
                    AddKey(pos);
                });
        }

        menu.exec(QCursor::pos());
    }

    void CurveEditor::DeleteKey()
    {
        m_currentCurve.RemovePoint(m_currentKeyIndex);
        m_currentKeyIndex = 0;
        emit curveChanged();
    }

    void CurveEditor::UpdateKeyInterpMode()
    {
        QAction* action = (QAction*)sender();

        auto interpMode = action->text();
        auto keyPoint = m_currentCurve.GetPoint(m_currentKeyIndex);
        keyPoint.m_interpMode = ConvertStringToInterpMode(interpMode.toStdString().c_str());
        m_currentCurve.UpdatePoint(m_currentKeyIndex, keyPoint);

        emit curveChanged();
        update();
    }

    void CurveEditor::resizeEvent(QResizeEvent* event)
    {
        m_mainRect = rect();
        m_mainRect.setTop(m_widgetWidth);
        m_mainRect.setLeft(rect().x() + m_scaleWidth);
        m_mainRect.setBottom(rect().bottom() - m_scaleHeight);

        QWidget::resizeEvent(event);
    }

    void CurveEditor::UpdateCurveKey(QPointF pos)
    {
        AzFramework::EasingCurve::Point keyPoint = m_currentCurve.GetPoint(m_currentKeyIndex);
        QPointF pt = TransformPointFromScreen(static_cast<float>(pos.x()), static_cast<float>(pos.y()));
        keyPoint.m_time = static_cast<float>(pt.x());
        keyPoint.m_value = static_cast<float>(pt.y());
        m_currentKeyIndex = m_currentCurve.UpdatePoint(m_currentKeyIndex, keyPoint);
        // update m_currentKeyIndex since the point index might have changed
        m_keyPointDragged = true;

        update();
    }

    PresetCurveWidget::PresetCurveWidget(QWidget* parent)
        : QWidget(parent)
        , m_pressed(false)
    {
        setFixedSize(20, 14);
    }

    PresetCurveWidget::~PresetCurveWidget()
    {
    }

    void PresetCurveWidget::SetKeyPoint(
        float startTime,
        float startValue,
        const AzFramework::EasingCurve::PointInterpolationMode& startMode,
        float stopTime,
        float stopValue,
        const AzFramework::EasingCurve::PointInterpolationMode& stopMode)
    {
        m_start.m_time = startTime;
        m_start.m_value = startValue;
        m_start.m_interpMode = startMode;
        m_stop.m_time = stopTime;
        m_stop.m_value = stopValue;
        m_stop.m_interpMode = stopMode;
    }

    void PresetCurveWidget::paintEvent(QPaintEvent* event)
    {
        // draw background
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.drawRect(rect());
        painter.fillRect(rect(), m_pressed ? QBrush(Qt::darkYellow) : QBrush(Qt::black));

        // draw curve
        QPen pen(Qt::white);
        painter.setPen(pen);
        QPainterPath path;
        const float CURVE_STEP = 0.05f;
        path = CreatePathFromCurve(m_start, m_stop, CURVE_STEP, rect().width(), rect().height(), 0, 0);
        painter.drawPath(path);

        QWidget::paintEvent(event);
    }

    void PresetCurveWidget::mousePressEvent(QMouseEvent* event)
    {
        m_pressed = true;
        update();
        emit onClick();
        QWidget::mousePressEvent(event);
    }

    void PresetCurveWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_pressed)
        {
            m_pressed = false;
            update();
        }

        QWidget::mouseReleaseEvent(event);
    }

    QWidget* PropertyCurveEditHandler::CreateGUI(QWidget* pParent)
    {
        CurveEditor* newCtrl = aznew CurveEditor(pParent);
        connect(
            newCtrl,
            &CurveEditor::curveChanged,
            this,
            [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });
        return newCtrl;
    }

    void PropertyCurveEditHandler::ConsumeAttribute(CurveEditor* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        Q_UNUSED(GUI);
        Q_UNUSED(attrib);
        Q_UNUSED(attrValue);
        Q_UNUSED(debugName);
    }

    void PropertyCurveEditHandler::WriteGUIValuesIntoProperty(size_t index, CurveEditor* GUI, property_t& instance, InstanceDataNode* node)
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        instance = GUI->GetCurrentCurve();
    }

    bool PropertyCurveEditHandler::ReadValuesIntoGUI(size_t index, CurveEditor* GUI, const property_t& instance, InstanceDataNode* node)
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        GUI->SetCurrentCurve(instance);
        return true;
    }

    void RegisterCurveEditHandler()
    {
        PropertyTypeRegistrationMessageBus::Broadcast(&PropertyTypeRegistrationMessages::RegisterPropertyType, aznew PropertyCurveEditHandler());
    }
} // namespace AzToolsFramework

#include "UI/PropertyEditor/moc_PropertyCurveCtrl.cpp"
