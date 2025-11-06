/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QToolTip>
#include <QMenu>
#include <QSpacerItem>
#include <QApplication>

#include <Window/Controls/CurveEditor.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/tuple.h>

namespace OpenParticleSystemEditor
{
    constexpr AZStd::string_view INTERPMODE_LINEAR = "Linear";
    constexpr AZStd::string_view INTERPMODE_STEP = "Step";
    constexpr AZStd::string_view INTERPMODE_CUBICIN = "CubicIn";
    constexpr AZStd::string_view INTERPMODE_CUBICOUT = "CubicOut";

    AZStd::string ConvertInterpModeToString(const OpenParticle::KeyPointInterpMode& mode)
    {
        const AZStd::unordered_map<OpenParticle::KeyPointInterpMode, AZStd::string> table =
        {
            { OpenParticle::KeyPointInterpMode::LINEAR, "Linear" },
            { OpenParticle::KeyPointInterpMode::STEP, "Step" },
            { OpenParticle::KeyPointInterpMode::CUBIC_IN, "CubicIn" },
            { OpenParticle::KeyPointInterpMode::CUBIC_OUT, "CubicOut" }
        };
        auto it = table.find(mode);
        if (it != table.end())
        {
            return it->second;
        }
        return "";
    }

    OpenParticle::KeyPointInterpMode ConvertStringToInterpMode(const AZStd::string& mode)
    {
        const AZStd::unordered_map<AZStd::string, OpenParticle::KeyPointInterpMode> table = {
            { "Linear", OpenParticle::KeyPointInterpMode::LINEAR },
            { "Step", OpenParticle::KeyPointInterpMode::STEP },
            { "CubicIn", OpenParticle::KeyPointInterpMode::CUBIC_IN },
            { "CubicOut", OpenParticle::KeyPointInterpMode::CUBIC_OUT }
        };
        auto it = table.find(mode);
        if (it != table.end())
        {
            return it->second;
        }
        return OpenParticle::KeyPointInterpMode::LINEAR;
    }

    QPoint TransformPointToScreen(float x, float y, int width, int height, int offsetX, int offsetY)
    {
        QPoint transformedPoint;
        transformedPoint.setX(static_cast<int>(x * width + offsetX));
        transformedPoint.setY(static_cast<int>(height * (1 - y) + offsetY));
        return transformedPoint;
    }

    QPainterPath CreatePathFromCurve(
        const OpenParticle::KeyPoint& start,
        const OpenParticle::KeyPoint& end,
        float curveStep,
        int width,
        int height,
        int offsetX,
        int offsetY)
    {
        QPainterPath path;
        QPointF pt = TransformPointToScreen(start.time, start.value, width, height, offsetX, offsetY);
        SimuCore::ParticleCore::KeyPointInterpMode interpMode = OpenParticle::DataConvertor::InterpModeToRuntime(end.interpMode);
        std::function<float(const float)> interpFunc = SimuCore::ParticleCore::ParticleCurve::GetInterpFunc(interpMode);
        path.moveTo(pt);
        float value = 0;
        float time = 0;
        for (float step = 0; step < 1; step += curveStep)
        {
            value = interpFunc(step);
            time = start.time + (end.time - start.time) * step;
            value = start.value + (end.value - start.value) * value;
            pt = TransformPointToScreen(time, value, width, height, offsetX, offsetY);
            path.lineTo(pt);
        }
        return path;
    }

    OpenParticle::KeyPoint ToKeyPoint(const QPointF& point, const OpenParticle::KeyPointInterpMode& interpMode)
    {
        OpenParticle::KeyPoint keyNew;
        keyNew.time = static_cast<float>(point.x());
        keyNew.value = static_cast<float>(point.y());
        keyNew.interpMode = interpMode;
        return keyNew;
    }

    CurveEditor::CurveEditor(AZStd::string busIDName, QWidget* parent)
        : QWidget(parent)
        , m_busIDName(busIDName)
        , m_layout(this)
    {
        SetupUi();

        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, m_busIDName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            m_distribution = &(sourceData->m_distribution);
            m_sourceData = sourceData;
        }
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
            connect(preset, &PresetCurveWidget::OnClicked, this, [this, preset]()
                {
                    m_currentCurve->keyPoints.clear();
                    m_currentCurve->keyPoints.emplace_back(preset->m_start);
                    m_currentCurve->keyPoints.emplace_back(preset->m_stop);
                    update();

                    const AZStd::string key = m_sourceData->GetModuleKey(m_moduleId, m_paramId);
                    const int activeCurveIdx = m_sourceData->m_distribution.curveCaches[key].activeAxis;
                    m_sourceData->m_distribution.curveCaches[key].curves[activeCurveIdx] = m_currentCurve;

                    EBUS_EVENT_ID(m_busIDName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
                });
            m_presetCurveWidget.emplace_back(preset);
        }

        m_layout.setContentsMargins(0, 0, 0, 0);
        m_layout.setAlignment(Qt::AlignTop);

        setMouseTracking(true);
    }

    float CurveEditor::GetValueFactor() const
    {
        if (m_currentCurve == nullptr)
        {
            return .0f;
        }

        return m_currentCurve->valueFactor;
    }

    int CurveEditor::GetTickMode() const
    {
        if (m_currentCurve == nullptr)
        {
            return 0;
        }
        return m_currentCurve->tickMode == OpenParticle::CurveTickMode::EMIT_DURATION ? 0 : 1;
    }

    void CurveEditor::SetValueFactor(float value)
    {
        if (m_currentCurve)
        {
            m_currentCurve->valueFactor = value;
            const AZStd::string key = m_sourceData->GetModuleKey(m_moduleId, m_paramId);
            const int activeCurveIdx = m_sourceData->m_distribution.curveCaches[key].activeAxis;
            m_sourceData->m_distribution.curveCaches[key].curves[activeCurveIdx] = m_currentCurve;
        }
    }

    void CurveEditor::SetTickMode(const OpenParticle::CurveTickMode& mode)
    {
        if (m_currentCurve)
        {
            m_currentCurve->tickMode = mode;
            const AZStd::string key = m_sourceData->GetModuleKey(m_moduleId, m_paramId);
            const int activeCurveIdx = m_sourceData->m_distribution.curveCaches[key].activeAxis;
            m_sourceData->m_distribution.curveCaches[key].curves[activeCurveIdx] = m_currentCurve;
        }
    }

    float CurveEditor::GetTimeFactor() const
    {
        if (m_currentCurve == nullptr)
        {
            return .0f;
        }

        return m_currentCurve->timeFactor;
    }

    void CurveEditor::SetTimeFactor(float value)
    {
        if (m_currentCurve)
        {
            m_currentCurve->timeFactor = value;
        }
    }

    void CurveEditor::SetCurrentCurve(int index)
    {
        if (index == 0 || index > m_distribution->curves.size())
        {
            return;
        }
        m_currentCurve = m_distribution->curves[index - 1];
        emit afterCurveSet();
    }

    float CurveEditor::GetCurrentKeyPointTime()
    {
        return m_currentKeyPointTime;
    }

    float CurveEditor::GetCurrentKeyPointValue()
    {
        return m_currentKeyPointValue;
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
        const int PART_TWO = 2;
        const int PART_THREE = 3;
        const int PART_FOUR = 4;
        const AZStd::string_view AXIS_INFO_ONE = "1.0";
        const AZStd::string_view AXIS_INFO_ZERO = "0.0";
        const AZStd::string_view AXIS_INFO_Y = "0.5";
        const AZStd::string_view AXIS_INFO_X1 = "0.25";
        const AZStd::string_view AXIS_INFO_X2 = "0.50";
        const AZStd::string_view AXIS_INFO_X3 = "0.75";

        painter.setPen(QColor(43, 43, 43));
        // horizon
        painter.drawLine(
            m_scaleWidth, m_mainRect.height() / PART_TWO + m_mainRect.top(), m_mainRect.right(),
            m_mainRect.height() / PART_TWO + m_mainRect.top());
        // vertical
        float width = static_cast<float>(m_mainRect.width() / static_cast<float>(PART_FOUR));

        QPointF begin;
        QPointF end;
        for (int i = 0; i < PART_THREE; i++)
        {
            begin.setX(m_mainRect.left() + width * (i + 1));
            begin.setY(m_mainRect.top());
            end.setX(begin.x());
            end.setY(m_mainRect.bottom());
            painter.drawLine(begin, end);
        }

        QPen pen(Qt::gray);
        int offset = m_scaleWidth / PART_TWO;
        painter.setPen(pen);
        painter.drawText(0, m_mainRect.top() + 12, AXIS_INFO_ONE.data());
        painter.drawText(0, m_mainRect.top() + m_mainRect.height() / PART_TWO + offset / PART_TWO, AXIS_INFO_Y.data());
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
        time_t index = 0;
        for (auto iter = m_currentCurve->keyPoints.begin(); iter != m_currentCurve->keyPoints.end(); iter++, index++)
        {
            OpenParticle::KeyPoint& keyPoint = (*iter);
            QPoint center = TransformPointToScreen(
                keyPoint.time, keyPoint.value, m_mainRect.width(), m_mainRect.height(), m_scaleWidth, m_mainRect.top());
            QRect rc = CenterPointToRect(center);
            painter.setPen((m_currentKeyIndex == index) ? selectedPen : unselectedPen);
            painter.setBrush(QBrush(Qt::gray));
            painter.drawRect(rc);
            if (m_currentKeyIndex == index)
            {
                m_currentKeyPointTime = keyPoint.time;
                m_currentKeyPointValue = keyPoint.value;
            }
        }

        // draw curves
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        OpenParticle::KeyPoint keyStart;
        OpenParticle::KeyPoint keyEnd;
        QPainterPath path;
        const float CURVE_STEP = 0.005f;
        for (auto iter = m_currentCurve->keyPoints.begin(); iter != m_currentCurve->keyPoints.end(); iter++)
        {
            if (iter == m_currentCurve->keyPoints.begin())
            {
                keyStart = (*iter);
                continue;
            }
            keyEnd = (*iter);
            path = CreatePathFromCurve(keyStart, keyEnd, CURVE_STEP, m_mainRect.width(), m_mainRect.height(), m_scaleWidth, m_mainRect.top());
            painter.drawPath(path);
            keyStart = keyEnd;
        }

        QWidget::paintEvent(event);
    }

    void CurveEditor::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_buttonPressed)
        {
            auto localPos = event->pos();
            UpdateCurveKey(localPos);
        }

        QWidget::mouseMoveEvent(event);
    }

    void CurveEditor::mousePressEvent(QMouseEvent* event)
    {
        QPoint currPos = event->pos();
        time_t index = 0;
        for (auto iter = m_currentCurve->keyPoints.begin(); iter != m_currentCurve->keyPoints.end(); iter++, index++)
        {
            OpenParticle::KeyPoint k = (*iter);
            QPoint center = TransformPointToScreen(k.time, k.value, m_mainRect.width(), m_mainRect.height(), m_scaleWidth, m_mainRect.top());
            QRect rc = CenterPointToRect(center);
            if (rc.contains(currPos))
            {
                m_buttonPressed = true;
                m_currentKeyIndex = (int)index;
                AZStd::string toolTip = AZStd::string::format("time:%f\nvalue:%f\nmode:%s",
                    k.time, k.value, ConvertInterpModeToString(k.interpMode).c_str());
                setToolTip(toolTip.c_str());

                m_currentKeyPointTime = k.time;
                m_currentKeyPointValue = k.value;
                update();
                break;
            }
        }
        if (event->button() == Qt::RightButton)
        {
            UpdateMenu(event->pos());
        }

        QWidget::mousePressEvent(event);
    }

    void CurveEditor::SimulateLeftButtonPressDragRelease(float oldTime, float oldValue, float newTime, float newValue, bool needMove)
    {
        // when the point is at the edge of the curve editor,
        // we move the click point a little bit to make sure we can click at it
        if (AZ::IsClose(oldTime, 0))
        {
            oldTime += EPSILON;
        }
        if (AZ::IsClose(oldTime, 1))
        {
            oldTime -= EPSILON;
        }
        if (AZ::IsClose(oldValue, 0))
        {
            oldValue += EPSILON;
        }
        if (AZ::IsClose(oldValue, 1))
        {
            oldValue -= EPSILON;
        }
        auto oldPoint = TransformPointToScreen(oldTime, oldValue, m_mainRect.width(), m_mainRect.height(), m_scaleWidth, m_mainRect.top());
        auto newPoint = TransformPointToScreen(newTime, newValue, m_mainRect.width(), m_mainRect.height(), m_scaleWidth, m_mainRect.top());
        QMouseEvent* pressEvent = nullptr;
        QMouseEvent* releaseEvent = nullptr;
        QMouseEvent* moveEvent = nullptr;

        pressEvent = new QMouseEvent(QEvent::MouseButtonPress, oldPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::postEvent(this, pressEvent);

        if (needMove)
        {
            moveEvent = new QMouseEvent(QEvent::MouseMove, newPoint, Qt::NoButton, Qt::MouseButtons(Qt::LeftButton), Qt::NoModifier);
            QCoreApplication::postEvent(this, moveEvent);
        }

        releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, newPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::postEvent(this, releaseEvent);
    }

    void CurveEditor::AddKey(QPoint pos)
    {
        QPointF pt = TransformPointFromScreen(static_cast<float>(pos.x()), static_cast<float>(pos.y()));
        auto keyNew = ToKeyPoint(pt, OpenParticle::KeyPointInterpMode::CUBIC_IN);
        m_currentCurve->keyPoints.emplace_back(keyNew);
        AZStd::sort(
            m_currentCurve->keyPoints.begin(), m_currentCurve->keyPoints.end(),
            [](OpenParticle::KeyPoint& left, OpenParticle::KeyPoint& right)
            {
                return left.time < right.time;
            });
        
        const AZStd::string key = m_sourceData->GetModuleKey(m_moduleId, m_paramId);
        const int activeCurveIdx = m_sourceData->m_distribution.curveCaches[key].activeAxis;
        m_sourceData->m_distribution.curveCaches[key].curves[activeCurveIdx] = m_currentCurve;

        // to change the key point editor's target to current added key point
        SimulateLeftButtonPressDragRelease(keyNew.time, keyNew.value, keyNew.time, keyNew.value);
    }

    void CurveEditor::UpdateMenu(QPoint pos)
    {
        QMenu menu;
        QMenu subMenu;
        if (m_buttonPressed)
        {
            m_buttonPressed = false;
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
        if (m_currentCurve == nullptr)
        {
            return;
        }

        int index = 0;
        for (auto iter = m_currentCurve->keyPoints.begin(); iter != m_currentCurve->keyPoints.end(); iter++, index++)
        {
            if (index == m_currentKeyIndex)
            {
                m_currentCurve->keyPoints.erase(iter);

                EBUS_EVENT_ID(m_busIDName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
                break;
            }
        }
        const AZStd::string key = m_sourceData->GetModuleKey(m_moduleId, m_paramId);
        const int activeCurveIdx = m_sourceData->m_distribution.curveCaches[key].activeAxis;
        m_sourceData->m_distribution.curveCaches[key].curves[activeCurveIdx] = m_currentCurve;
    }

    void CurveEditor::UpdateKeyInterpMode()
    {
        QAction* action = (QAction*)sender();
        if (m_currentCurve == nullptr)
        {
            return;
        }

        int index = 0;
        auto interpMode = action->text();
        for (auto iter = m_currentCurve->keyPoints.begin(); iter != m_currentCurve->keyPoints.end(); iter++, index++)
        {
            if (index == m_currentKeyIndex)
            {
                iter->interpMode = ConvertStringToInterpMode(interpMode.toStdString().c_str());

                update();
                EBUS_EVENT_ID(m_busIDName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
                break;
            }
        }
    }

    void CurveEditor::mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_buttonPressed)
        {
            if (m_buttonDragged)
            {
                m_buttonDragged = false;

                size_t index = 0;
                for (auto iter = m_currentCurve->keyPoints.begin(); iter != m_currentCurve->keyPoints.end(); iter++, index++)
                {
                    if (index == m_currentKeyIndex)
                    {
                        OpenParticle::KeyPoint keyPoint = (*iter);
                        m_currentKeyPointTime = keyPoint.time;
                        m_currentKeyPointValue = keyPoint.value;
                        AZStd::string toolTip = AZStd::string::format("time:%f\nvalue:%f\nmode:%s",
                            keyPoint.time, keyPoint.value, ConvertInterpModeToString(keyPoint.interpMode).c_str());
                        setToolTip(toolTip.c_str());
                        update();
                        EBUS_EVENT_ID(m_busIDName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
                        break;
                    }
                }
            }
            m_buttonPressed = false;
        }

        QWidget::mouseReleaseEvent(event);
        emit mouseRelease();
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
        size_t index = 0;
        for (auto iter = m_currentCurve->keyPoints.begin(); iter != m_currentCurve->keyPoints.end(); iter++, index++)
        {
            if (m_currentKeyIndex == index)
            {
                QPointF pt = TransformPointFromScreen(static_cast<float>(pos.x()), static_cast<float>(pos.y()));
                iter->time = static_cast<float>(pt.x());
                iter->value = static_cast<float>(pt.y());
                break;
            }
        }

        const AZStd::string key = m_sourceData->GetModuleKey(m_moduleId, m_paramId);
        const int activeCurveIdx = m_sourceData->m_distribution.curveCaches[key].activeAxis;
        m_sourceData->m_distribution.curveCaches[key].curves[activeCurveIdx] = m_currentCurve;

        m_buttonDragged = true;

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
        const OpenParticle::KeyPointInterpMode& startMode,
        float stopTime,
        float stopValue,
        const OpenParticle::KeyPointInterpMode& stopMode)
    {
        m_start.time = startTime;
        m_start.value = startValue;
        m_start.interpMode = startMode;
        m_stop.time = stopTime;
        m_stop.value = stopValue;
        m_stop.interpMode = stopMode;
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
        emit OnClicked();
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
} // namespace OpenParticleSystemEditor
