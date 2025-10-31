/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientColorPickerWidget.h"
#include "GradientWidget.h"

#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/sort.h>
#include <Window/Controls/CommonDefs.h>

namespace OpenParticleSystemEditor
{
    GradientColorPickerWidget::GradientColorPickerWidget(QWidget* parent)
        : QWidget(parent)
        , m_gradientWidget(new GradientWidget(this))
        , m_gradientSize(QSize(20, 12))
        , m_colorInactive(Qt::gray)
        , m_colorHovered(Qt::yellow)
        , m_colorSelected(Qt::blue)
        , m_addKey(true)
    {
        setLayout(&m_layout);

        m_gradientWidget->AddGradient(new QLinearGradient(0, 0, 0, 1), QPainter::CompositionMode::CompositionMode_SourceOver);
        m_gradientWidget->AddGradient(new QLinearGradient(0, 0, 1, 0), QPainter::CompositionMode::CompositionMode_SourceAtop);

        m_layout.addWidget(m_gradientWidget, 0, 0, 1, 1);
        m_layout.setContentsMargins(0, 0, 0, 0);
        m_layout.addItem(new QSpacerItem(0, 16), 1, 0);

        OnGradientChanged();
        setMouseTracking(true);
    }

    GradientColorPickerWidget::~GradientColorPickerWidget()
    {
    }

    unsigned int GradientColorPickerWidget::AddKey(float stop, QColor color)
    {
        m_gradientWidget->AddKey((unsigned int)Gradients::HUE_RANGE, stop, color);
        unsigned int index = m_keys.count();
        m_keys.push_back(GradientKey(QGradientStop(stop, color), GetRegion(), m_gradientSize));
        // set the stop to the selected
        for (int i = 0; i < m_keys.count(); i++)
        {
            m_keys[i].SetSelected(false);
        }
        m_keys.back().SetSelected(true);
        return index;
    }

    void GradientColorPickerWidget::RemoveKey(const QGradientStop& stop)
    {
        for (int i = 0; i < m_keys.count(); i++)
        {
            if (m_keys[i].GetStop().first == stop.first && m_keys[i].GetStop().second == stop.second)
            {
                m_keys.removeAt(i);
                OnGradientChanged();
                if (m_defaultLocationChangedCB != nullptr)
                {
                    m_defaultLocationChangedCB();
                }
                return;
            }
        }
    }

    void GradientColorPickerWidget::SetKeys(QGradientStops stops)
    {
        m_keys.clear();
        QRect rect = GetRegion();
        for (QGradientStop stop : stops)
        {
            m_keys.push_back(GradientKey(stop, rect, m_gradientSize));
        }
        OnGradientChanged();
    }

    QGradientStops GradientColorPickerWidget::GetKeys()
    {
        QGradientStops stops;
        // loop through the stops and then sort them
        for (int i = 0; i < m_keys.count(); i++)
        {
            stops.push_back(m_keys[i].GetStop());
        }

        AZStd::sort(stops.begin(), stops.end(), [=](const QGradientStop& s1, const QGradientStop& s2) -> bool
            {
                return s1.first < s2.first;
            });

        return stops;
    }

    void GradientColorPickerWidget::paintEvent([[maybe_unused]] QPaintEvent* event)
    {
        QPainter painter(this);
        for (GradientKey stop : m_keys)
        {
            stop.Paint(painter, m_colorInactive, m_colorHovered, m_colorSelected);
        }
    }

    QColor GradientColorPickerWidget::GetColorByTime(float x)
    {
        if (m_keys.count() <= 0)
        {
            return Qt::white; // return default hue if no hue is selected
        }
        if (x < 0 || x > 1.0f)
        {
            AZStd::string errInfo = AZStd::string::format("Get color by time failed! x=%f.", x);
            AZ_Assert(false, errInfo.c_str());
        }
        QPropertyAnimation interpolator;
        const qreal resolution = width();
        interpolator.setEasingCurve(QEasingCurve::Linear);
        interpolator.setDuration(static_cast<int>(resolution));
        // use the sorted stops
        QGradientStops keys = GetKeys();
        if (keys.first().first != 0)
        {
            interpolator.setKeyValueAt(0, keys.first().second);
        }
        if (keys.back().first != 1.0f)
        {
            interpolator.setKeyValueAt(1.0f, keys.back().second);
        }

        for (int i = 0; i < m_keys.count(); i++)
        {
            if (m_keys[i].GetStop().first == x)
            {
                return m_keys[i].GetStop().second;
            }
            interpolator.setKeyValueAt(m_keys[i].GetStop().first, m_keys[i].GetStop().second);
        }

        interpolator.setCurrentTime(static_cast<int>(x * resolution));

        QColor color = interpolator.currentValue().value<QColor>();

        return QColor(
            AZStd::min(AZStd::max(0, color.red()), MAXIMUM_COLOR_VALUE), AZStd::min(AZStd::max(0, color.green()), MAXIMUM_COLOR_VALUE),
            AZStd::min(AZStd::max(0, color.blue()), MAXIMUM_COLOR_VALUE), AZStd::min(AZStd::max(0, color.alpha()), MAXIMUM_COLOR_VALUE));
    }

    void GradientColorPickerWidget::mouseMoveEvent(QMouseEvent* mouseEvent)
    {
        if (!m_leftDown)
        {
            for (int i = 0; i < m_keys.count(); i++)
            {
                if (m_keys[i].PointInKey(mouseEvent->pos()))
                {
                    m_keys[i].SetHovered(true);
                }
                else
                {
                    m_keys[i].SetHovered(false);
                }
            }
        }
        else
        {
            GradientKey* selectKey = GetSelectedKey();
            if (nullptr != selectKey && m_leftDown && !RestrictedKey())
            {
                QGradientStop stop = selectKey->GetStop();
                stop.first = static_cast<float>(mouseEvent->pos().x()) / static_cast<float>(width());
                stop.first = qMax(static_cast<float>(stop.first), 0.0f);
                stop.first = qMin(static_cast<float>(stop.first), 1.0f);
                selectKey->SetStop(stop);

                if (nullptr != m_locationChangedCB)
                {
                    m_locationChangedCB(stop.first * m_percentage, stop.second);
                }
                OnGradientChanged();
            }
            else
            {
                update();
            }
        }
        QWidget::mouseMoveEvent(mouseEvent);
    }

    void GradientColorPickerWidget::mousePressEvent(QMouseEvent* mouseEvent)
    {
        QWidget::mousePressEvent(mouseEvent);
        auto clickFn = [this]()
        {
            for (int i = 0; i < m_keys.count(); i++)
            {
                m_keys[i].SetSelected(false);
            }
            for (int i = 0; i < m_keys.count(); i++)
            {
                if (m_keys[i].IsHovered())
                {
                    if (!m_keys[i].IsSelected())
                    {
                        m_keys[i].SetSelected(true);
                        setCursor(Qt::SizeHorCursor);
                        if ((bool)m_locationChangedCB)
                        {
                            m_locationChangedCB((short)(m_keys[i].GetStop().first * m_percentage), m_keys[i].GetStop().second);
                        }
                        update();
                        break;
                    }
                }
            }
        };
        switch (mouseEvent->button())
        {
        case Qt::MouseButton::LeftButton:
            m_leftDown = true;
            clickFn();
            break;
        case Qt::MouseButton::RightButton:
            m_rightDown = true;
            clickFn();
            break;
        default:
            break;
        }
    }

    void GradientColorPickerWidget::mouseReleaseEvent(QMouseEvent* mouseEvent)
    {
        QWidget::mouseReleaseEvent(mouseEvent);

        switch (mouseEvent->button())
        {
        case Qt::MouseButton::LeftButton:
            m_leftDown = false;
            setCursor(Qt::ArrowCursor);
            update();
            break;
        case Qt::MouseButton::RightButton:
            m_rightDown = false;
            setCursor(Qt::ArrowCursor);
            if (m_addKey)
            {
                ShowGradientMenu(mouseEvent->pos().x());
            }
            break;
        default:
            break;
        }
    }

    void GradientColorPickerWidget::mouseDoubleClickEvent(QMouseEvent* mouseEvent)
    {
        GradientKey* key = GetSelectedKey();

        if (key != nullptr)
        {
            if (mouseEvent->button() == Qt::MouseButton::LeftButton)
            {
                OnPickColor(key);
            }
        }
    }

    void GradientColorPickerWidget::leaveEvent([[maybe_unused]] QEvent* event)
    {
        for (int i = 0; i < m_keys.count(); i++)
        {
            m_keys[i].SetHovered(false);
        }

        update();
    }

    void GradientColorPickerWidget::resizeEvent(QResizeEvent* resizeEvent)
    {
        QWidget::resizeEvent(resizeEvent);
        QRect rect = GetRegion();
        for (int i = 0; i < m_keys.count(); i++)
        {
            m_keys[i].SetRegion(rect);
        }
    }

    QColor GradientColorPickerWidget::GetHueAt(float x)
    {
        if (m_keys.count() <= 0)
        {
            return Qt::white; // return default hue if no hue is selected
        }
        QPropertyAnimation interpolator;
        const qreal resolution = width();
        interpolator.setEasingCurve(QEasingCurve::Linear);
        interpolator.setDuration(resolution);
        // use the sorted stops
        QGradientStops stops = GetKeys();
        if (stops.first().first != 0)
        {
            interpolator.setKeyValueAt(0, stops.first().second);
        }
        if (stops.back().first != 1.0f)
        {
            interpolator.setKeyValueAt(1.0f, stops.back().second);
        }

        for (int i = 0; i < m_keys.count(); i++)
        {
            if (m_keys[i].GetStop().first == x)
            {
                return m_keys[i].GetStop().second;
            }
            interpolator.setKeyValueAt(m_keys[i].GetStop().first, m_keys[i].GetStop().second);
        }

        interpolator.setCurrentTime(x * resolution);

        QColor color = interpolator.currentValue().value<QColor>();

        return QColor(
            AZStd::min(AZStd::max(0, color.red()), MAXIMUM_COLOR_VALUE), AZStd::min(AZStd::max(0, color.green()), MAXIMUM_COLOR_VALUE),
            AZStd::min(AZStd::max(0, color.blue()), MAXIMUM_COLOR_VALUE), AZStd::min(AZStd::max(0, color.alpha()), MAXIMUM_COLOR_VALUE));
    }

    bool GradientColorPickerWidget::RestrictedKey() const
    {
        bool flag = false;
        for (int i = 0; i < m_keys.count(); i++)
        {
            if (m_keys[i].IsSelected())
            {
                if (m_keys[i].GetStop().first == 0.0f || m_keys[i].GetStop().first == 1.0f)
                {
                    flag = true;
                }
                break;
            }
        }
        return flag;
    }

    void GradientColorPickerWidget::ShowGradientMenu(int x)
    {
        GradientKey* key = GetSelectedKey();

        if (key == nullptr)
        {
            QMenu menu;
            menu.addAction("Add Key", this, [x, this]()
                {
                    float stop = static_cast<float>((float)x / rect().width());
                    QColor color = GetHueAt(stop);
                    AddKey(stop, color);
                    if (m_locationChangedCB != nullptr)
                    {
                        m_locationChangedCB(stop * m_percentage, color);
                    }
                    OnGradientChanged();
                });
            menu.exec(QCursor::pos());
        }
        else
        {
            if (!RestrictedKey())
            {
                QMenu menu;
                menu.addAction("Delete Key", this, [key, this]()
                    {
                        RemoveKey(key->GetStop());
                    });
                menu.exec(QCursor::pos());
            }
        }
    }

    void GradientColorPickerWidget::OnPickColor(GradientKey* key)
    {
        const QColor existingColor = key->GetStop().second;

        const AZ::Color color = AzQtComponents::ColorPicker::getColor(
            AzQtComponents::ColorPicker::Configuration::RGBA, AzQtComponents::fromQColor(existingColor), tr("Select Color"));
        QColor selectedColor = AzQtComponents::ToQColor(color);
        if (selectedColor != existingColor)
        {
            QGradientStop stop = key->GetStop();
            stop.second = selectedColor;
            key->SetStop(stop);
            OnGradientChanged();
        }
    }

    void GradientColorPickerWidget::SetSelectedKeyPosition(int location)
    {
        float loc = location / m_percentage;
        GradientKey* key = GetSelectedKey();
        if (key != nullptr)
        {
            key->SetTime(loc);
            OnGradientChanged();
        }
    }

    void GradientColorPickerWidget::SetDefaultLocationChangedCB(AZStd::function<void()> callback)
    {
        m_defaultLocationChangedCB = callback;
    }

    void GradientColorPickerWidget::SetLocationChangedCB(AZStd::function<void(short, QColor)> callback)
    {
        m_locationChangedCB = callback;
    }

    void GradientColorPickerWidget::SetGradientChangedCB(AZStd::function<void(QGradientStops)> callback)
    {
        m_gradientChangedCB = callback;
    }

    QRect GradientColorPickerWidget::GetRegion()
    {
        QPoint topLeft(0, height() - m_gradientSize.height());
        QPoint bottomRight(width(), height());
        return QRect(topLeft, bottomRight);
    }

    GradientColorPickerWidget::GradientKey* GradientColorPickerWidget::GetSelectedKey()
    {
        for (int i = 0; i < m_keys.count(); i++)
        {
            if (m_keys[i].IsSelected())
            {
                return &m_keys[i];
            }
        }
        return nullptr;
    }

    void GradientColorPickerWidget::OnGradientChanged()
    {
        QGradientStops keys = GetKeys();
        if (!signalsBlocked())
        {
            if (m_gradientChangedCB)
            {
                blockSignals(true);
                m_gradientChangedCB(keys);
                blockSignals(false);
            }
            m_gradientWidget->SetKeys((unsigned int)Gradients::HUE_RANGE, keys);
            m_gradientWidget->update();
            update();
        }
    }

    GradientColorPickerWidget::GradientKey::GradientKey()
        : GradientKey(QGradientStop(0, Qt::white), QRect(0, 0, 0, 0))
    {
    }

    GradientColorPickerWidget::GradientKey::GradientKey(const GradientKey& other)
        : m_stop(other.m_stop)
        , m_rect(other.m_rect)
        , m_size(other.m_size)
        , m_selected(other.m_selected)
        , m_hovered(other.m_hovered)
    {
    }

    GradientColorPickerWidget::GradientKey::GradientKey(QGradientStop stop, QRect rect, QSize size)
        : m_stop(stop)
        , m_rect(rect)
        , m_size(size)
        , m_selected(false)
        , m_hovered(false)
    {
    }

    bool GradientColorPickerWidget::GradientKey::PointInKey(const QPoint& p)
    {
        QPolygonF poly = CreatePolygon();
        return poly.containsPoint(p, Qt::FillRule::WindingFill);
    }

    void GradientColorPickerWidget::GradientKey::Paint(
        QPainter& painter, const QColor& inactive, const QColor& hovered, const QColor& selected)
    {
        QPen pen;
        QColor corlorBorder = inactive;
        int penWidth = 2;
        if (m_selected)
        {
            corlorBorder = selected;
        }
        else if (m_hovered)
        {
            corlorBorder = hovered;
        }
        painter.setRenderHint(QPainter::Antialiasing, true);
        pen.setWidth(penWidth);
        painter.setBrush(m_stop.second);
        pen.setColor(corlorBorder);
        painter.setPen(pen);
        painter.drawPolygon(CreatePolygon());
        pen.setWidth(1);
        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing, false);
    }

    QPolygonF GradientColorPickerWidget::GradientKey::CreatePolygon()
    {
        int offsetX = static_cast<int>(m_stop.first * m_rect.width());
        int halfWidth = m_size.width() / 2;
        QVector<QPointF> vertex;
        // we're drawing a triangle, so we need 3 points
        vertex.reserve(3);
        vertex.push_back(QPointF(offsetX, m_rect.top()));
        vertex.push_back(QPointF(offsetX - halfWidth, m_rect.top() + m_size.height()));
        vertex.push_back(QPointF(offsetX + halfWidth, m_rect.top() + m_size.height()));
        return QPolygonF(vertex);
    }

    void GradientColorPickerWidget::GradientKey::SetRegion(QRect rect)
    {
        m_rect = rect;
    }

    QPoint GradientColorPickerWidget::GradientKey::LocalPos()
    {
        QPolygonF poly = CreatePolygon();
        return poly.boundingRect().topLeft().toPoint();
    }

    void GradientColorPickerWidget::GradientKey::SetTime(qreal val)
    {
        m_stop.first = val;
    }

#include <moc_GradientColorPickerWidget.cpp>
} // namespace OpenParticleSystemEditor
