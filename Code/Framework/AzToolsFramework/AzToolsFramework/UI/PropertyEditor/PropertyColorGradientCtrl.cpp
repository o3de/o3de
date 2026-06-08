/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyColorGradientCtrl.hxx"
#include "PropertyColorCtrl.hxx"
#include "PropertyQTConstants.h"

#include <AzCore/Debug/Trace.h>
#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QBoxLayout>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSignalBlocker>
#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QKeySequence>
#include <QMenu>
#include <QShortcut>
#include <QString>
#include <QStringList>
AZ_POP_DISABLE_WARNING

#include <algorithm>

namespace AzToolsFramework
{
    // =======================================================================
    // Local Constants
    // =======================================================================

    namespace GradientCtrlConstants
    {
        static constexpr int    kBarHeight       = 48;
        static constexpr int    kAlphaTrackHeight = 24;   // cube (10) + gap (2) + triangle (10) + 2 padding
        static constexpr int    kColorTrackHeight = 24;   // triangle (10) + gap (2) + cube (10) + 2 padding
        static constexpr int    kTriangleWidth   = 10;
        static constexpr int    kTriangleHeight  = 10;
        static constexpr int    kCubeSize        = 10;    // outer frame
        static constexpr int    kCubeInnerSize   = 8;     // inner colored fill
        static constexpr int    kCubeGap         = 2;     // gap between triangle base and cube top
        static constexpr int    kInlineHeight    = 20;
        static constexpr int    kDialogWidth     = 600;
        static constexpr int    kCheckerSize     = 8;
        static constexpr int    kDragOffThreshold = 25;   // vertical distance to drag a marker off to delete it
        static constexpr float  kDuplicateOffset = 0.05f;
    }

    // =======================================================================
    // GradientBarWidget
    // =======================================================================

    GradientBarWidget::GradientBarWidget(QWidget* parent)
        : QWidget(parent)
    {
        setMinimumHeight(GradientCtrlConstants::kInlineHeight);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    }

    void GradientBarWidget::setGradient(const AZ::ColorGradient& gradient)
    {
        m_gradient = gradient;
        m_gradient.SortGradients();
        update();
    }

    void GradientBarWidget::setAlphaEnabled(bool enabled)
    {
        m_alphaEnabled = enabled;
        update();
    }

    void GradientBarWidget::setMixed(bool mixed)
    {
        if (m_mixed == mixed) { return; }
        m_mixed = mixed;
        update();
    }

    void GradientBarWidget::paintEvent(QPaintEvent* /*event*/)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        const QRect area = rect();

        // Multi-edit with divergent values: show a neutral grey field crossed
        // with an X to indicate the displayed gradient is not representative
        // of every selected entity.
        if (m_mixed)
        {
            painter.fillRect(area, QColor(90, 90, 90));
            painter.setPen(QPen(QColor(30, 30, 30), 2));
            painter.drawLine(area.topLeft(),    area.bottomRight());
            painter.drawLine(area.topRight(),   area.bottomLeft());
            painter.setPen(QColor(30, 30, 30));
            painter.drawRect(area.adjusted(0, 0, -1, -1));
            return;
        }

        // Checkerboard backdrop to visualize transparency.
        const int cs = GradientCtrlConstants::kCheckerSize;
        for (int y = area.top(); y < area.bottom(); y += cs)
        {
            for (int x = area.left(); x < area.right(); x += cs)
            {
                const bool dark = ((x / cs) + (y / cs)) % 2 == 0;
                painter.fillRect(QRect(x, y, cs, cs), dark ? QColor(80, 80, 80) : QColor(120, 120, 120));
            }
        }

        if (area.width() <= 0)
        {
            return;
        }

        // Sample the gradient once per pixel column along the bar.
        for (int x = 0; x < area.width(); ++x)
        {
            const float t = static_cast<float>(x) / static_cast<float>(area.width() - 1);
            AZ::Color sampled = m_alphaEnabled ? m_gradient.EvaluateGradient(t) : m_gradient.EvaluateColor(t);
            QColor c;
            c.setRedF(static_cast<qreal>(sampled.GetR()));
            c.setGreenF(static_cast<qreal>(sampled.GetG()));
            c.setBlueF(static_cast<qreal>(sampled.GetB()));
            c.setAlphaF(m_alphaEnabled ? static_cast<qreal>(sampled.GetA()) : 1.0);
            painter.fillRect(QRect(area.left() + x, area.top(), 1, area.height()), c);
        }

        painter.setPen(QColor(30, 30, 30));
        painter.drawRect(area.adjusted(0, 0, -1, -1));
    }

    // =======================================================================
    // GradientStopsTrack
    // =======================================================================

    GradientStopsTrack::GradientStopsTrack(Kind kind, QWidget* parent)
        : QWidget(parent)
        , m_kind(kind)
    {
        const int h = kind == Kind::Color ? GradientCtrlConstants::kColorTrackHeight
                                          : GradientCtrlConstants::kAlphaTrackHeight;
        setMinimumHeight(h);
        setFixedHeight(h);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFocusPolicy(Qt::StrongFocus);
    }

    void GradientStopsTrack::setColorStops(const AZStd::vector<AZ::ColorGradientMarker>& stops)
    {
        m_colorStops = stops;
        m_selected.reset();
        update();
    }

    void GradientStopsTrack::setAlphaStops(const AZStd::vector<AZ::AlphaGradientMarker>& stops)
    {
        m_alphaStops = stops;
        m_selected.reset();
        update();
    }

    void GradientStopsTrack::setSelectedIndex(AZStd::optional<size_t> index)
    {
        m_selected = index;
        update();
        emit selectionChanged();
    }

    size_t GradientStopsTrack::stopCount() const
    {
        return m_kind == Kind::Color ? m_colorStops.size() : m_alphaStops.size();
    }

    float GradientStopsTrack::stopPositionAt(size_t index) const
    {
        return m_kind == Kind::Color ? m_colorStops[index].m_markerPosition : m_alphaStops[index].m_markerPosition;
    }

    int GradientStopsTrack::positionToPixel(float position) const
    {
        const int w = width();
        if (w <= 0) { return 0; }
        return static_cast<int>(std::clamp(position, 0.f, 1.f) * (w - 1));
    }

    float GradientStopsTrack::pixelToPosition(int pixel) const
    {
        const int w = width();
        if (w <= 1) { return 0.f; }
        const float t = static_cast<float>(pixel) / static_cast<float>(w - 1);
        return std::clamp(t, 0.f, 1.f);
    }

    QRect GradientStopsTrack::triangleRect(float position) const
    {
        const int x = positionToPixel(position);
        const int half = GradientCtrlConstants::kTriangleWidth / 2;
        // Each track's triangle hugs the edge closest to the gradient bar so
        // the tip visually touches the bar. Alpha track sits above the bar:
        // triangle anchors at the BOTTOM of its widget. Color track sits below
        // the bar: triangle anchors at the TOP of its widget.
        const int y = (m_kind == Kind::Alpha)
            ? (height() - GradientCtrlConstants::kTriangleHeight)
            : 0;
        return QRect(x - half, y, GradientCtrlConstants::kTriangleWidth, GradientCtrlConstants::kTriangleHeight);
    }

    AZStd::optional<size_t> GradientStopsTrack::hitTest(const QPoint& point) const
    {
        const size_t n = stopCount();
        for (size_t i = 0; i < n; ++i)
        {
            const float pos = stopPositionAt(i);
            const QRect tri = triangleRect(pos);
            if (tri.adjusted(-2, -2, 2, 2).contains(point))
            {
                return i;
            }
            // Preview cube hit region (both tracks have a cube at the tail end).
            const int cubeX = tri.center().x() - GradientCtrlConstants::kCubeSize / 2;
            const int cubeY = (m_kind == Kind::Alpha)
                ? (tri.top() - GradientCtrlConstants::kCubeGap - GradientCtrlConstants::kCubeSize)
                : (tri.bottom() + GradientCtrlConstants::kCubeGap);
            const QRect cube(cubeX, cubeY, GradientCtrlConstants::kCubeSize, GradientCtrlConstants::kCubeSize);
            if (cube.adjusted(-2, -2, 2, 2).contains(point))
            {
                return i;
            }
        }
        return AZStd::nullopt;
    }

    AZ::Color GradientStopsTrack::sampleColorAt(float t) const
    {
        AZ::ColorGradient tmp;
        tmp.m_colorSlider = m_colorStops;
        tmp.SortGradients();
        return tmp.EvaluateColor(t);
    }

    float GradientStopsTrack::sampleAlphaAt(float t) const
    {
        AZ::ColorGradient tmp;
        tmp.m_alphaSlider = m_alphaStops;
        tmp.SortGradients();
        return tmp.EvaluateAlpha(t);
    }

    void GradientStopsTrack::addStopAt(float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        if (m_kind == Kind::Color)
        {
            AZ::ColorGradientMarker m;
            m.m_markerColor = sampleColorAt(t);
            m.m_markerPosition = t;
            m_colorStops.push_back(m);
            m_selected = m_colorStops.size() - 1;
        }
        else
        {
            AZ::AlphaGradientMarker m;
            m.m_markerAlpha = sampleAlphaAt(t);
            m.m_markerPosition = t;
            m_alphaStops.push_back(m);
            m_selected = m_alphaStops.size() - 1;
        }
        update();
        emit selectionChanged();
        emit stopsChanged();
        emit editCommitted();
    }

    void GradientStopsTrack::setSelectedPosition(float t)
    {
        if (!m_selected.has_value())
        {
            AZ_Printf("GradientCtrl", "setSelectedPosition: NO SELECTION - skip");
            return;
        }
        t = std::clamp(t, 0.f, 1.f);
        AZ_Printf("GradientCtrl",
            "setSelectedPosition: kind=%d index=%zu t=%f",
            static_cast<int>(m_kind), *m_selected, t);
        if (m_kind == Kind::Color) { m_colorStops[*m_selected].m_markerPosition = t; }
        else                       { m_alphaStops[*m_selected].m_markerPosition = t; }
        update();
        emit stopsChanged();
    }

    void GradientStopsTrack::setSelectedColor(const AZ::Color& rgb)
    {
        if (m_kind != Kind::Color || !m_selected.has_value()) { return; }
        AZ::Color opaque(rgb.GetR(), rgb.GetG(), rgb.GetB(), 1.f);
        m_colorStops[*m_selected].m_markerColor = opaque;
        update();
        emit stopsChanged();
    }

    void GradientStopsTrack::setSelectedAlpha(float alpha)
    {
        if (m_kind != Kind::Alpha || !m_selected.has_value()) { return; }
        m_alphaStops[*m_selected].m_markerAlpha = std::clamp(alpha, 0.f, 1.f);
        update();
        emit stopsChanged();
    }

    void GradientStopsTrack::duplicateSelected()
    {
        if (!m_selected.has_value()) { return; }
        const float offset = GradientCtrlConstants::kDuplicateOffset;
        if (m_kind == Kind::Color)
        {
            auto src = m_colorStops[*m_selected];
            src.m_markerPosition = std::clamp(src.m_markerPosition + offset, 0.f, 1.f);
            m_colorStops.push_back(src);
            m_selected = m_colorStops.size() - 1;
        }
        else
        {
            auto src = m_alphaStops[*m_selected];
            src.m_markerPosition = std::clamp(src.m_markerPosition + offset, 0.f, 1.f);
            m_alphaStops.push_back(src);
            m_selected = m_alphaStops.size() - 1;
        }
        update();
        emit selectionChanged();
        emit stopsChanged();
        emit editCommitted();
    }

    void GradientStopsTrack::removeSelected()
    {
        if (!m_selected.has_value()) { return; }
        if (stopCount() <= 1) { return; }
        const size_t sel = *m_selected;
        if (m_kind == Kind::Color) { m_colorStops.erase(m_colorStops.begin() + sel); }
        else                       { m_alphaStops.erase(m_alphaStops.begin() + sel); }
        m_selected.reset();
        update();
        emit selectionChanged();
        emit stopsChanged();
        emit editCommitted();
    }

    void GradientStopsTrack::paintEvent(QPaintEvent* /*event*/)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const size_t n = stopCount();
        for (size_t i = 0; i < n; ++i)
        {
            const float pos = stopPositionAt(i);
            const QRect tri = triangleRect(pos);

            // Alpha track sits above the gradient bar: tip points DOWN toward
            // the bar below (base at top of widget, tip at bottom of triangle
            // rect).
            // Color track sits below the gradient bar: tip points UP toward
            // the bar above (tip at top of triangle rect, base at bottom).
            QPainterPath path;
            if (m_kind == Kind::Alpha)
            {
                path.moveTo(tri.left(), tri.top());
                path.lineTo(tri.right(), tri.top());
                path.lineTo(tri.center().x(), tri.bottom());
            }
            else
            {
                path.moveTo(tri.center().x(), tri.top());
                path.lineTo(tri.left(), tri.bottom());
                path.lineTo(tri.right(), tri.bottom());
            }
            path.closeSubpath();

            // Triangle fill: color track uses the marker colour; alpha track
            // uses a fixed neutral (light grey) so the arrow is always legible
            // regardless of the stop's alpha value.
            QColor triFill;
            if (m_kind == Kind::Color)
            {
                const AZ::Color& c = m_colorStops[i].m_markerColor;
                triFill.setRedF(static_cast<qreal>(c.GetR()));
                triFill.setGreenF(static_cast<qreal>(c.GetG()));
                triFill.setBlueF(static_cast<qreal>(c.GetB()));
            }
            else
            {
                triFill = QColor(220, 220, 220);
            }

            const bool isSel = m_selected.has_value() && *m_selected == i;
            const bool isDeletePreview = isSel && m_pendingDelete;
            QColor outline;
            int outlineWidth;
            if (isDeletePreview)        { outline = QColor(255, 60, 60);  outlineWidth = 2; }
            else if (isSel)             { outline = QColor(255, 200, 0);  outlineWidth = 2; }
            else                        { outline = QColor(20, 20, 20);   outlineWidth = 1; }
            painter.setBrush(triFill);
            painter.setPen(QPen(outline, outlineWidth));
            painter.drawPath(path);

            // Preview cube at the tail end of the arrow. Outer frame = 10px,
            // inner display = 8px. Color track: inner is solid marker colour.
            // Alpha track: inner is a mini checkerboard overlaid with
            // semi-transparent white, so full opacity reads as white, zero
            // reads as the grid.
            const int cubeX = tri.center().x() - GradientCtrlConstants::kCubeSize / 2;
            const int cubeY = (m_kind == Kind::Alpha)
                ? (tri.top() - GradientCtrlConstants::kCubeGap - GradientCtrlConstants::kCubeSize)
                : (tri.bottom() + GradientCtrlConstants::kCubeGap);
            const QRect cubeOuter(cubeX, cubeY, GradientCtrlConstants::kCubeSize, GradientCtrlConstants::kCubeSize);
            const int innerOffset = (GradientCtrlConstants::kCubeSize - GradientCtrlConstants::kCubeInnerSize) / 2;
            const QRect cubeInner(cubeX + innerOffset, cubeY + innerOffset,
                                  GradientCtrlConstants::kCubeInnerSize, GradientCtrlConstants::kCubeInnerSize);

            painter.setRenderHint(QPainter::Antialiasing, false);

            if (m_kind == Kind::Color)
            {
                painter.fillRect(cubeInner, triFill);
            }
            else
            {
                // 4-square mini checkerboard, then semi-transparent white overlay
                // with opacity set to the stop's alpha value.
                const int half = GradientCtrlConstants::kCubeInnerSize / 2;
                const QColor dark(80, 80, 80);
                const QColor light(170, 170, 170);
                painter.fillRect(QRect(cubeInner.left(),        cubeInner.top(),        half, half), light);
                painter.fillRect(QRect(cubeInner.left() + half, cubeInner.top(),        half, half), dark);
                painter.fillRect(QRect(cubeInner.left(),        cubeInner.top() + half, half, half), dark);
                painter.fillRect(QRect(cubeInner.left() + half, cubeInner.top() + half, half, half), light);

                QColor whiteOverlay(255, 255, 255, static_cast<int>(m_alphaStops[i].m_markerAlpha * 255.f));
                painter.fillRect(cubeInner, whiteOverlay);
            }

            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(outline, outlineWidth));
            painter.drawRect(cubeOuter);
            painter.setRenderHint(QPainter::Antialiasing, true);
        }
    }

    void GradientStopsTrack::mousePressEvent(QMouseEvent* event)
    {
        AZ_Printf("GradientCtrl",
            "Track::mousePressEvent: ENTRY kind=%d (about to setFocus, prev focus=%p)",
            static_cast<int>(m_kind), (void*)QApplication::focusWidget());
        setFocus();
        AZ_Printf("GradientCtrl",
            "Track::mousePressEvent: AFTER setFocus, focus=%p",
            (void*)QApplication::focusWidget());
        if (event->button() == Qt::LeftButton)
        {
            auto hit = hitTest(event->pos());
            if (hit.has_value())
            {
                AZ_Printf("GradientCtrl", "Track::mousePressEvent: HIT existing marker idx=%zu", *hit);
                m_selected = hit;
                m_dragging = true;
                update();
                emit selectionChanged();
            }
            else
            {
                AZ_Printf("GradientCtrl", "Track::mousePressEvent: MISS - adding new stop");
                addStopAt(pixelToPosition(event->pos().x()));
                m_dragging = true;
            }
        }
    }

    void GradientStopsTrack::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_dragging && m_selected.has_value() && (event->buttons() & Qt::LeftButton))
        {
            // Drag-off-to-delete: flag the marker as pending delete when the
            // cursor leaves the widget vertically by more than the threshold.
            const int y = event->pos().y();
            const bool outside = (y < -GradientCtrlConstants::kDragOffThreshold)
                              || (y > height() + GradientCtrlConstants::kDragOffThreshold);
            if (outside != m_pendingDelete)
            {
                m_pendingDelete = outside;
                update();
            }
            setSelectedPosition(pixelToPosition(event->pos().x()));
        }
        if (!(event->buttons() & Qt::LeftButton))
        {
            m_dragging = false;
            m_pendingDelete = false;
        }
    }

    void GradientStopsTrack::mouseReleaseEvent(QMouseEvent* /*event*/)
    {
        const bool wasDragging = m_dragging;
        const bool wantDelete = wasDragging && m_pendingDelete;
        m_dragging = false;
        m_pendingDelete = false;
        if (wantDelete)
        {
            // removeSelected already emits editCommitted.
            removeSelected();
        }
        else if (wasDragging)
        {
            // A drag that moved a stop without deleting it is one commit.
            emit editCommitted();
        }
        update();
    }

    void GradientStopsTrack::mouseDoubleClickEvent(QMouseEvent* event)
    {
        auto hit = hitTest(event->pos());
        if (hit.has_value() && m_kind == Kind::Color)
        {
            m_selected = hit;
            emit selectionChanged();

            // Use the standard AzQtComponents color picker so the double-click
            // affordance matches the inline inspector swatch experience.
            const AZ::Color initial(
                m_colorStops[*m_selected].m_markerColor.GetR(),
                m_colorStops[*m_selected].m_markerColor.GetG(),
                m_colorStops[*m_selected].m_markerColor.GetB(),
                1.f);
            const AZ::Color picked = AzQtComponents::ColorPicker::getColor(
                AzQtComponents::ColorPicker::Configuration::RGB,
                initial,
                QStringLiteral("Stop Color"),
                QString(),
                QStringList(),
                this);
            setSelectedColor(AZ::Color(picked.GetR(), picked.GetG(), picked.GetB(), 1.f));
            emit editCommitted();
        }
    }

    void GradientStopsTrack::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Delete)
        {
            removeSelected();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier))
        {
            duplicateSelected();
            event->accept();
            return;
        }
        // Arrow-key nudge on the selected stop. Shift = coarse step.
        if ((event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) && m_selected.has_value())
        {
            const float step = (event->modifiers() & Qt::ShiftModifier) ? 0.10f : 0.01f;
            const float dir = event->key() == Qt::Key_Left ? -1.f : 1.f;
            setSelectedPosition(stopPositionAt(*m_selected) + dir * step);
            emit editCommitted();
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }

    // =======================================================================
    // ColorGradientEditorDialog
    // =======================================================================

    ColorGradientEditorDialog::ColorGradientEditorDialog(const AZ::ColorGradient& initial, bool alphaEnabled, bool multiEdit, QWidget* parent)
        : QDialog(parent)
        , m_working(initial)
        , m_alphaEnabled(alphaEnabled)
    {
        // Allow setFocus(this) to actually grab focus on the dialog itself.
        // QDialog's default focusPolicy is NoFocus, which would silently
        // discard our setFocus calls and leave focus null - which prevents
        // WidgetWithChildrenShortcut shortcuts from activating.
        setFocusPolicy(Qt::StrongFocus);

        setWindowTitle(multiEdit
            ? QStringLiteral("Color Gradient Editor (multi-edit: applies to all selected)")
            : QStringLiteral("Color Gradient Editor"));
        setMinimumWidth(GradientCtrlConstants::kDialogWidth);
        m_working.SortGradients();

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(8, 8, 8, 8);
        root->setSpacing(6);

        if (multiEdit)
        {
            auto* banner = new QLabel(
                QStringLiteral("This edit will be written to every selected entity's gradient field."),
                this);
            banner->setStyleSheet(QStringLiteral("color: #ffcc00; font-weight: bold;"));
            banner->setWordWrap(true);
            root->addWidget(banner);
        }

        // Alpha track (top).
        m_alphaTrack = new GradientStopsTrack(GradientStopsTrack::Kind::Alpha, this);
        m_alphaTrack->setAlphaStops(m_working.m_alphaSlider);
        m_alphaTrack->setVisible(m_alphaEnabled);
        root->addWidget(m_alphaTrack);

        // Preview bar.
        m_preview = new GradientBarWidget(this);
        m_preview->setMinimumHeight(GradientCtrlConstants::kBarHeight);
        m_preview->setAlphaEnabled(m_alphaEnabled);
        m_preview->setGradient(m_working);
        root->addWidget(m_preview);

        // Color track (bottom).
        m_colorTrack = new GradientStopsTrack(GradientStopsTrack::Kind::Color, this);
        m_colorTrack->setColorStops(m_working.m_colorSlider);
        root->addWidget(m_colorTrack);

        // Inspector row.
        auto* inspector = new QHBoxLayout();
        inspector->setSpacing(8);

        m_valueLabel = new QLabel(QStringLiteral("Color"), this);
        // Use the standard O3DE color field so the swatch + text + native
        // ColorPicker are driven by the same widget developers see everywhere.
        m_colorField = new PropertyColorCtrl(this);
        m_colorField->setAlphaChannelEnabled(false);
        m_alphaSpin = new QDoubleSpinBox(this);
        m_alphaSpin->setRange(0.0, 1.0);
        m_alphaSpin->setSingleStep(0.01);
        m_alphaSpin->setDecimals(3);
        m_alphaSpin->setVisible(false);

        auto* positionLabel = new QLabel(QStringLiteral("Position %"), this);
        m_positionSpin = new QDoubleSpinBox(this);
        m_positionSpin->setRange(0.0, 100.0);
        m_positionSpin->setSingleStep(1.0);
        m_positionSpin->setDecimals(1);

        inspector->addStretch(1);
        inspector->addWidget(m_valueLabel);
        inspector->addWidget(m_colorField);
        inspector->addWidget(m_alphaSpin);
        inspector->addSpacing(12);
        inspector->addWidget(positionLabel);
        inspector->addWidget(m_positionSpin);
        inspector->addStretch(1);
        root->addLayout(inspector);

        // OK / Cancel. Disable auto-default on both so Enter inside a spin box
        // only commits the field, rather than closing the dialog.
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        if (auto* okBtn = buttons->button(QDialogButtonBox::Ok))
        {
            okBtn->setDefault(false);
            okBtn->setAutoDefault(false);
        }
        if (auto* cancelBtn = buttons->button(QDialogButtonBox::Cancel))
        {
            cancelBtn->setDefault(false);
            cancelBtn->setAutoDefault(false);
        }
        root->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        // Wire tracks to dialog updates.
        connect(m_alphaTrack, &GradientStopsTrack::stopsChanged, this, &ColorGradientEditorDialog::onAlphaTrackChanged);
        connect(m_colorTrack, &GradientStopsTrack::stopsChanged, this, &ColorGradientEditorDialog::onColorTrackChanged);
        connect(m_alphaTrack, &GradientStopsTrack::selectionChanged, this, [this]()
        {
            if (m_alphaTrack->selectedIndex().has_value())
            {
                m_activeTrack = m_alphaTrack;
                QSignalBlocker blocker(m_colorTrack);
                m_colorTrack->setSelectedIndex(AZStd::nullopt);
            }
            onSelectionChanged();
        });
        connect(m_colorTrack, &GradientStopsTrack::selectionChanged, this, [this]()
        {
            if (m_colorTrack->selectedIndex().has_value())
            {
                m_activeTrack = m_colorTrack;
                QSignalBlocker blocker(m_alphaTrack);
                m_alphaTrack->setSelectedIndex(AZStd::nullopt);
            }
            onSelectionChanged();
        });

        connect(m_positionSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ColorGradientEditorDialog::onPositionChanged);
        // PropertyColorCtrl emits valueChanged on every picker drag frame, so
        // we only live-update the marker there (no undo snapshot). Snapshots
        // are pushed on actual commit signals (text commit / picker accepted).
        connect(m_colorField, &PropertyColorCtrl::valueChanged,     this, &ColorGradientEditorDialog::onColorFieldLiveChanged);
        connect(m_colorField, &PropertyColorCtrl::editingFinished,  this, &ColorGradientEditorDialog::onColorFieldCommitted);
        connect(m_colorField, &PropertyColorCtrl::colorSelected,    this, &ColorGradientEditorDialog::onColorFieldCommitted);
        connect(m_colorField, &PropertyColorCtrl::rejected,         this, &ColorGradientEditorDialog::onColorFieldRejected);
        connect(m_alphaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ColorGradientEditorDialog::onAlphaSpinChanged);

        // Commit-point wiring for the in-dialog undo stack. Granular steps are
        // pushed here; the outer RPE is only notified once, when the dialog
        // closes.
        connect(m_alphaTrack, &GradientStopsTrack::editCommitted, this, [this]() {
            AZ_Printf("GradientCtrl", "alphaTrack::editCommitted FIRED -> pushUndoSnapshot");
            pushUndoSnapshot();
        });
        connect(m_colorTrack, &GradientStopsTrack::editCommitted, this, [this]() {
            AZ_Printf("GradientCtrl", "colorTrack::editCommitted FIRED -> pushUndoSnapshot");
            pushUndoSnapshot();
        });
        connect(m_positionSpin, &QDoubleSpinBox::editingFinished, this, [this]() {
            AZ_Printf("GradientCtrl", "m_positionSpin::editingFinished FIRED -> pushUndoSnapshot");
            pushUndoSnapshot();
        });
        connect(m_alphaSpin, &QDoubleSpinBox::editingFinished, this, [this]() {
            AZ_Printf("GradientCtrl", "m_alphaSpin::editingFinished FIRED -> pushUndoSnapshot");
            pushUndoSnapshot();
        });

        m_activeTrack = m_colorTrack;
        refreshInspector();

        // Seed the undo stack with the opening state so the user can undo all
        // the way back to how the dialog appeared.
        m_undoStack.push_back(captureSnapshot());
        m_undoCursor = 0;

        // Widget-scoped shortcuts. These take priority over the editor's
        // global Ctrl+Z / Ctrl+Y QActions while the dialog or any of its
        // children own focus, so the keystrokes actually reach our handlers
        // instead of being eaten by the main-window menu actions.
        auto installShortcut = [this](const QKeySequence& seq, auto slot)
        {
            auto* sc = new QShortcut(seq, this);
            // WindowShortcut fires whenever the dialog is the active top-level
            // window, regardless of which descendant (or none) has focus. This
            // is what we want: while the modal gradient editor is open, the
            // user's Ctrl+Z should always go to OUR undo, not to any global
            // editor QAction.
            sc->setContext(Qt::WindowShortcut);
            connect(sc, &QShortcut::activated, this, slot);
        };

        installShortcut(QKeySequence(QKeySequence::Undo), [this]() {
            AZ_Printf("GradientCtrl", "Shortcut Undo activated focused=%p", (void*)QApplication::focusWidget());
            // If a line edit inside the dialog has focus, defer to its own
            // text-undo so typing can be reverted.
            if (auto* le = qobject_cast<QLineEdit*>(QApplication::focusWidget()))
            {
                AZ_Printf("GradientCtrl", "Shortcut Undo: line edit -> text undo");
                le->undo();
                return;
            }
            undo();
        });
        installShortcut(QKeySequence(QKeySequence::Redo), [this]() {
            AZ_Printf("GradientCtrl", "Shortcut Redo activated");
            if (auto* le = qobject_cast<QLineEdit*>(QApplication::focusWidget()))
            {
                le->redo();
                return;
            }
            redo();
        });
        installShortcut(QKeySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::SHIFT) | static_cast<int>(Qt::Key_Z)), [this]() {
            AZ_Printf("GradientCtrl", "Shortcut Ctrl+Shift+Z activated");
            if (auto* le = qobject_cast<QLineEdit*>(QApplication::focusWidget()))
            {
                le->redo();
                return;
            }
            redo();
        });
        installShortcut(QKeySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::Key_D)), [this]() {
            AZ_Printf("GradientCtrl", "Shortcut Ctrl+D activated");
            if (m_activeTrack) { m_activeTrack->duplicateSelected(); }
        });
    }

    void ColorGradientEditorDialog::rebuildFromTracks()
    {
        m_working.m_colorSlider = m_colorTrack->colorStops();
        m_working.m_alphaSlider = m_alphaTrack->alphaStops();
        m_working.m_sorted = false;
        m_working.SortGradients();
        refreshPreview();
    }

    void ColorGradientEditorDialog::refreshPreview()
    {
        m_preview->setGradient(m_working);
    }

    void ColorGradientEditorDialog::onAlphaTrackChanged()
    {
        rebuildFromTracks();
    }

    void ColorGradientEditorDialog::onColorTrackChanged()
    {
        rebuildFromTracks();
    }

    void ColorGradientEditorDialog::refreshInspector()
    {
        if (!m_activeTrack || !m_activeTrack->selectedIndex().has_value())
        {
            m_valueLabel->setText(QStringLiteral("(no selection)"));
            m_colorField->setVisible(false);
            m_alphaSpin->setVisible(false);
            m_positionSpin->setEnabled(false);
            m_positionSpin->blockSignals(true);
            m_positionSpin->setValue(0.0);
            m_positionSpin->blockSignals(false);
            return;
        }

        const size_t idx = *m_activeTrack->selectedIndex();
        m_positionSpin->setEnabled(true);

        if (m_activeTrack->kind() == GradientStopsTrack::Kind::Color)
        {
            m_valueLabel->setText(QStringLiteral("Color"));
            m_colorField->setVisible(true);
            m_alphaSpin->setVisible(false);

            const AZ::Color& cur = m_activeTrack->colorStops()[idx].m_markerColor;
            QColor q;
            q.setRedF(static_cast<qreal>(cur.GetR()));
            q.setGreenF(static_cast<qreal>(cur.GetG()));
            q.setBlueF(static_cast<qreal>(cur.GetB()));
            QSignalBlocker block(m_colorField);
            m_colorField->setValue(q);

            m_positionSpin->blockSignals(true);
            m_positionSpin->setValue(m_activeTrack->colorStops()[idx].m_markerPosition * 100.0);
            m_positionSpin->blockSignals(false);
        }
        else
        {
            m_valueLabel->setText(QStringLiteral("Alpha"));
            m_colorField->setVisible(false);
            m_alphaSpin->setVisible(true);
            m_alphaSpin->blockSignals(true);
            m_alphaSpin->setValue(static_cast<double>(m_activeTrack->alphaStops()[idx].m_markerAlpha));
            m_alphaSpin->blockSignals(false);
            m_positionSpin->blockSignals(true);
            m_positionSpin->setValue(m_activeTrack->alphaStops()[idx].m_markerPosition * 100.0);
            m_positionSpin->blockSignals(false);
        }
    }

    void ColorGradientEditorDialog::onSelectionChanged()
    {
        refreshInspector();
    }

    void ColorGradientEditorDialog::onPositionChanged(double normalized)
    {
        AZ_Printf("GradientCtrl",
            "onPositionChanged: ENTRY normalized=%f activeTrack=%p activeTrack.selected=%s",
            normalized, (void*)m_activeTrack,
            (m_activeTrack && m_activeTrack->selectedIndex().has_value())
                ? AZStd::to_string(*m_activeTrack->selectedIndex()).c_str()
                : "(none)");
        if (!m_activeTrack) { return; }
        m_activeTrack->setSelectedPosition(static_cast<float>(normalized / 100.0));
    }

    void ColorGradientEditorDialog::onColorFieldLiveChanged()
    {
        // Called on every intermediate frame while the picker is open. Just
        // push the new colour into the selected stop so the preview updates
        // live; do NOT snapshot here or the undo stack fills with per-frame
        // micro-states that erode the colour on Ctrl+Z.
        if (!m_activeTrack || m_activeTrack->kind() != GradientStopsTrack::Kind::Color) { return; }
        if (!m_activeTrack->selectedIndex().has_value()) { return; }

        const QColor q = m_colorField->value();
        AZ::Color az(
            static_cast<float>(q.redF()),
            static_cast<float>(q.greenF()),
            static_cast<float>(q.blueF()),
            1.f);
        m_activeTrack->setSelectedColor(az);
    }

    void ColorGradientEditorDialog::onColorFieldCommitted()
    {
        // Single snapshot per committed colour edit (picker OK or text edit
        // return). onColorFieldLiveChanged has already pushed the final colour
        // into the marker.
        pushUndoSnapshot();
    }

    void ColorGradientEditorDialog::onColorFieldRejected()
    {
        // Picker was cancelled. PropertyColorCtrl has already reverted its
        // internal colour to the pre-open value but does not re-emit
        // valueChanged on Cancel, so re-sync the marker manually.
        onColorFieldLiveChanged();
        // No snapshot: the undo stack already has the pre-edit state on top.
    }

    void ColorGradientEditorDialog::onAlphaSpinChanged(double value)
    {
        if (!m_activeTrack || m_activeTrack->kind() != GradientStopsTrack::Kind::Alpha) { return; }
        m_activeTrack->setSelectedAlpha(static_cast<float>(value));
    }

    void ColorGradientEditorDialog::showEvent(QShowEvent* event)
    {
        QDialog::showEvent(event);
        // Intercept application-wide mouse presses so we can commit when the
        // user clicks anywhere outside the dialog (and outside any spawned
        // child popup such as the AzQtComponents color picker).
        qApp->installEventFilter(this);
    }

    void ColorGradientEditorDialog::hideEvent(QHideEvent* event)
    {
        qApp->removeEventFilter(this);
        QDialog::hideEvent(event);
    }

    bool ColorGradientEditorDialog::eventFilter(QObject* watched, QEvent* event)
    {
        // While a child modal (e.g. the color picker) is active, let it own
        // all input. We re-engage the filter once it closes.
        QWidget* modal = QApplication::activeModalWidget();
        if (modal && modal != this)
        {
            return QDialog::eventFilter(watched, event);
        }

        auto isDescendantOfDialog = [this](QWidget* w)
        {
            for (QWidget* p = w; p; p = p->parentWidget())
            {
                if (p == this) { return true; }
            }
            return false;
        };

        if (event->type() == QEvent::KeyPress)
        {
            // Diagnostic: log every key event the filter sees so we can tell
            // when Ctrl+Z / Enter / etc. are reaching us vs being consumed
            // upstream by another widget's built-in shortcut.
            {
                auto* keyEvent = static_cast<QKeyEvent*>(event);
                AZ_Printf("GradientCtrl",
                    "eventFilter KeyPress: key=%d mods=%d watched=%p focused=%p (modal=%p this=%p)",
                    keyEvent->key(), static_cast<int>(keyEvent->modifiers()),
                    (void*)watched, (void*)QApplication::focusWidget(),
                    (void*)QApplication::activeModalWidget(), (void*)this);
            }
            // Intercept undo / redo at the application level so child widgets
            // like QLineEdit (inside PropertyColorCtrl) do not consume Ctrl+Z
            // for their own text-undo before the dialog gets a chance.
            auto* ke = static_cast<QKeyEvent*>(event);
            const bool ctrl  = (ke->modifiers() & Qt::ControlModifier)  != 0;
            const bool shift = (ke->modifiers() & Qt::ShiftModifier)    != 0;
            if (auto* w = qobject_cast<QWidget*>(watched))
            {
                if (isDescendantOfDialog(w))
                {
                    // Escape inside a focused text editor (spin box or the
                    // colour text field) reverts the typed value and exits
                    // the field, leaving the dialog open. Outside any editor
                    // it falls through to QDialog's default reject().
                    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
                    {
                        QWidget* focused = QApplication::focusWidget();
                        AZ_Printf("GradientCtrl",
                            "eventFilter Enter: watched=%p focused=%p (this=%p posSpin=%p alphaSpin=%p colorField=%p)",
                            (void*)watched, (void*)focused, (void*)this,
                            (void*)m_positionSpin, (void*)m_alphaSpin, (void*)m_colorField);
                        QAbstractSpinBox* spin = nullptr;
                        for (QWidget* p = focused; p; p = p->parentWidget())
                        {
                            if (auto* s = qobject_cast<QAbstractSpinBox*>(p)) { spin = s; break; }
                        }
                        AZ_Printf("GradientCtrl",
                            "eventFilter Enter: walk found spin=%p", (void*)spin);
                        if (spin == m_positionSpin)
                        {
                            AZ_Printf("GradientCtrl", "eventFilter Enter: position spin path - commit + grab dialog focus + push");
                            spin->interpretText();
                            onPositionChanged(m_positionSpin->value());
                            spin->clearFocus();
                            // Explicitly grab focus on the dialog so subsequent
                            // Ctrl+Z events arrive at the dialog (and our app
                            // event filter) rather than at the line edit, which
                            // would consume them as text-undo.
                            this->setFocus(Qt::OtherFocusReason);
                            AZ_Printf("GradientCtrl",
                                "eventFilter Enter: post-handoff focused=%p (this=%p)",
                                (void*)QApplication::focusWidget(), (void*)this);
                            pushUndoSnapshot();
                            return true;
                        }
                        if (spin == m_alphaSpin)
                        {
                            AZ_Printf("GradientCtrl", "eventFilter Enter: alpha spin path - commit + grab dialog focus + push");
                            spin->interpretText();
                            onAlphaSpinChanged(m_alphaSpin->value());
                            spin->clearFocus();
                            this->setFocus(Qt::OtherFocusReason);
                            AZ_Printf("GradientCtrl",
                                "eventFilter Enter: post-handoff focused=%p (this=%p)",
                                (void*)QApplication::focusWidget(), (void*)this);
                            pushUndoSnapshot();
                            return true;
                        }
                        AZ_Printf("GradientCtrl", "eventFilter Enter: falling through (not in our spin)");
                    }

                    if (ke->key() == Qt::Key_Escape)
                    {
                        QWidget* focused = QApplication::focusWidget();

                        // focusWidget() can return the spin's internal QLineEdit
                        // (focus proxy) OR the spin itself depending on Qt's
                        // focus-routing path. Walk parents from both the focused
                        // widget and the event recipient (w) so either case
                        // resolves to the spin.
                        QAbstractSpinBox* spin = nullptr;
                        for (QWidget* p = focused; p; p = p->parentWidget())
                        {
                            if (auto* s = qobject_cast<QAbstractSpinBox*>(p)) { spin = s; break; }
                        }
                        if (!spin)
                        {
                            for (QWidget* p = w; p; p = p->parentWidget())
                            {
                                if (auto* s = qobject_cast<QAbstractSpinBox*>(p)) { spin = s; break; }
                            }
                        }
                        if (spin && isDescendantOfDialog(spin))
                        {
                            // QAbstractSpinBox::lineEdit() is protected;
                            // findChild reaches it through the public API.
                            if (auto* le = spin->findChild<QLineEdit*>()) { le->undo(); }
                            spin->clearFocus();
                            return true;
                        }

                        // Plain QLineEdit (PropertyColorCtrl's text field).
                        if (auto* le = qobject_cast<QLineEdit*>(focused))
                        {
                            if (isDescendantOfDialog(le))
                            {
                                le->undo();
                                le->clearFocus();
                                return true;
                            }
                        }
                        // No focused editor: fall through to QDialog default.
                    }

                    const bool isUndo = ctrl && !shift && ke->key() == Qt::Key_Z;
                    const bool isRedo = (ctrl && ke->key() == Qt::Key_Y)
                                     || (ctrl && shift && ke->key() == Qt::Key_Z);
                    if (isUndo || isRedo)
                    {
                        // Force any in-progress spin-box edit to commit before
                        // stepping the stack. clearFocus triggers focusOutEvent
                        // which interprets pending text and emits editingFinished,
                        // which pushes the snapshot we are about to walk past.
                        if (auto* spin = qobject_cast<QAbstractSpinBox*>(QApplication::focusWidget()))
                        {
                            spin->clearFocus();
                        }
                        if (isUndo) { undo(); } else { redo(); }
                        return true;
                    }
                }
            }
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            if (auto* w = qobject_cast<QWidget*>(watched))
            {
                if (!isDescendantOfDialog(w))
                {
                    accept();
                    return true;
                }

                auto isInside = [](QWidget* container, QWidget* candidate)
                {
                    if (!container || !candidate) { return false; }
                    for (QWidget* p = candidate; p; p = p->parentWidget())
                    {
                        if (p == container) { return true; }
                    }
                    return false;
                };

                const bool inField = isInside(m_positionSpin, w)
                                  || isInside(m_alphaSpin, w)
                                  || isInside(m_colorField, w);
                const bool inTrack = isInside(m_colorTrack, w)
                                  || isInside(m_alphaTrack, w);

                if (!inField && !inTrack)
                {
                    // Empty-area click: behave like Tab-out from the active
                    // editor (clear focus -> editingFinished -> commit) AND
                    // deselect any highlighted marker so the inspector
                    // clears. Anything that is neither a track nor a field
                    // counts as empty space (preview bar, labels, dialog
                    // background, button bar).
                    QWidget* focused = QApplication::focusWidget();
                    if (focused && focused != w && isDescendantOfDialog(focused))
                    {
                        focused->clearFocus();
                    }
                    if (m_colorTrack) { m_colorTrack->setSelectedIndex(AZStd::nullopt); }
                    if (m_alphaTrack) { m_alphaTrack->setSelectedIndex(AZStd::nullopt); }
                }
            }
        }

        return QDialog::eventFilter(watched, event);
    }

    void ColorGradientEditorDialog::mousePressEvent(QMouseEvent* event)
    {
        // Mouse presses that land on a focusable child (track / spin / color
        // field / button) are accepted by that child and never reach this
        // override. Anything that bubbles up to the dialog itself - the
        // gradient bar, dialog background, labels, layout gaps - is "blank
        // space" by definition.
        //
        // Staged blur:
        //   Step 1) If a child of the dialog currently has focus, blur it
        //           (Tab-out). This commits any active editor without
        //           clearing the highlighted marker.
        //   Step 2) If no descendant has focus, drop the marker selection.
        // A second blank click is therefore needed to fully deselect when
        // the user was mid-edit.
        QWidget* focused = QApplication::focusWidget();
        bool focusedDescendant = false;
        if (focused && focused != this)
        {
            for (QWidget* p = focused; p; p = p->parentWidget())
            {
                if (p == this) { focusedDescendant = true; break; }
            }
        }

        AZ_Printf("GradientCtrl",
            "dialog::mousePressEvent: ENTRY focused=%p focusedDescendant=%d",
            (void*)focused, focusedDescendant);

        if (focusedDescendant)
        {
            AZ_Printf("GradientCtrl", "dialog::mousePressEvent: STAGE 1 - clearFocus + grab dialog focus + push");
            focused->clearFocus();
            // Hand focus to the dialog itself so subsequent Ctrl+Z reaches our
            // event filter instead of being absorbed by a still-listening line
            // edit's text-undo shortcut.
            this->setFocus(Qt::OtherFocusReason);
            AZ_Printf("GradientCtrl",
                "dialog::mousePressEvent: post-handoff focused=%p",
                (void*)QApplication::focusWidget());
            pushUndoSnapshot();
        }
        else
        {
            AZ_Printf("GradientCtrl", "dialog::mousePressEvent: STAGE 2 - deselecting markers");
            if (m_colorTrack) { m_colorTrack->setSelectedIndex(AZStd::nullopt); }
            if (m_alphaTrack) { m_alphaTrack->setSelectedIndex(AZStd::nullopt); }
        }

        QDialog::mousePressEvent(event);
    }

    void ColorGradientEditorDialog::reject()
    {
        // If a field inside the dialog is currently being edited, treat the
        // cancel as a field-revert instead of a dialog-cancel. This catches
        // every path that leads to reject() including the Cancel button's
        // implicit Esc shortcut (a QEvent::Shortcut that the KeyPress-only
        // event filter does not see).
        QWidget* focused = QApplication::focusWidget();

        auto isInside = [](QWidget* container, QWidget* candidate)
        {
            if (!container || !candidate) { return false; }
            for (QWidget* p = candidate; p; p = p->parentWidget())
            {
                if (p == container) { return true; }
            }
            return false;
        };

        if (isInside(m_positionSpin, focused))
        {
            if (auto* le = m_positionSpin->findChild<QLineEdit*>()) { le->undo(); }
            m_positionSpin->clearFocus();
            return;
        }
        if (isInside(m_alphaSpin, focused))
        {
            if (auto* le = m_alphaSpin->findChild<QLineEdit*>()) { le->undo(); }
            m_alphaSpin->clearFocus();
            return;
        }
        if (isInside(m_colorField, focused))
        {
            if (auto* le = m_colorField->findChild<QLineEdit*>())
            {
                le->undo();
                le->clearFocus();
            }
            return;
        }

        QDialog::reject();
    }

    void ColorGradientEditorDialog::keyPressEvent(QKeyEvent* event)
    {
        // Staged Enter: if focus is in a spin box, let the field commit its value
        // and swallow the key so the dialog does not accept. When no spin box is
        // focused, Enter approves the whole gradient.
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        {
            QWidget* focused = focusWidget();

            // focusWidget() may return the spin's internal QLineEdit (focus
            // proxy) rather than the QAbstractSpinBox itself, so the direct
            // qobject_cast can fail. Walk parents to find an enclosing spin
            // reliably for both routes.
            QAbstractSpinBox* spin = qobject_cast<QAbstractSpinBox*>(focused);
            if (!spin)
            {
                for (QWidget* p = focused; p; p = p->parentWidget())
                {
                    if (auto* s = qobject_cast<QAbstractSpinBox*>(p)) { spin = s; break; }
                }
            }
            if (spin == m_positionSpin || spin == m_alphaSpin)
            {
                spin->interpretText();
                if (spin == m_positionSpin) { onPositionChanged(m_positionSpin->value()); }
                else if (spin == m_alphaSpin) { onAlphaSpinChanged(m_alphaSpin->value()); }
                spin->clearFocus();
                pushUndoSnapshot();
                event->accept();
                return;
            }

            // Enter inside the colour text field: QLineEdit's returnPressed
            // already fired and PropertyColorCtrl::OnEditingFinished committed
            // the value through the existing signal chain. Just consume the
            // event so the dialog does not also approve, and release focus
            // for visual consistency with the spin path.
            if (m_colorField)
            {
                for (QWidget* p = focused; p; p = p->parentWidget())
                {
                    if (p == m_colorField)
                    {
                        if (auto* le = m_colorField->findChild<QLineEdit*>()) { le->clearFocus(); }
                        event->accept();
                        return;
                    }
                }
            }

            accept();
            return;
        }

        // In-dialog undo / redo. These never reach the outer editor; only the
        // final accepted state is written to the RPE.
        const bool ctrl = (event->modifiers() & Qt::ControlModifier) != 0;
        const bool shift = (event->modifiers() & Qt::ShiftModifier) != 0;
        if (ctrl && !shift && event->key() == Qt::Key_Z)
        {
            undo();
            event->accept();
            return;
        }
        if ((ctrl && event->key() == Qt::Key_Y) || (ctrl && shift && event->key() == Qt::Key_Z))
        {
            redo();
            event->accept();
            return;
        }

        QDialog::keyPressEvent(event);
    }

    // =======================================================================
    // In-dialog Undo Stack
    // =======================================================================

    ColorGradientEditorDialog::UndoSnapshot ColorGradientEditorDialog::captureSnapshot() const
    {
        UndoSnapshot snap;
        snap.colorStops = m_colorTrack->colorStops();
        snap.alphaStops = m_alphaTrack->alphaStops();
        snap.selectedKind = m_activeTrack ? m_activeTrack->kind() : GradientStopsTrack::Kind::Color;
        snap.selectedIndex = m_activeTrack ? m_activeTrack->selectedIndex() : AZStd::nullopt;
        AZ_Printf("GradientCtrl",
            "captureSnapshot: colorStops=%zu alphaStops=%zu activeKind=%d selectedIndex=%s",
            snap.colorStops.size(), snap.alphaStops.size(),
            static_cast<int>(snap.selectedKind),
            snap.selectedIndex.has_value()
                ? AZStd::to_string(*snap.selectedIndex).c_str()
                : "(none)");
        return snap;
    }

    void ColorGradientEditorDialog::pushUndoSnapshot()
    {
        AZ_Printf("GradientCtrl",
            "pushUndoSnapshot: ENTRY stack.size=%zu cursor=%zu",
            m_undoStack.size(), m_undoCursor);

        UndoSnapshot snap = captureSnapshot();

        // Coalesce identical-stops snapshots so a no-op editingFinished does
        // not clutter the stack. Selection-only changes are still not pushed
        // because onSelectionChanged never calls this function.
        if (!m_undoStack.empty())
        {
            const UndoSnapshot& top = m_undoStack[m_undoCursor];
            const bool sameColor = top.colorStops.size() == snap.colorStops.size()
                && std::equal(
                       top.colorStops.begin(), top.colorStops.end(),
                       snap.colorStops.begin(),
                       [](const AZ::ColorGradientMarker& a, const AZ::ColorGradientMarker& b) {
                           return a.m_markerPosition == b.m_markerPosition
                               && a.m_markerColor == b.m_markerColor;
                       });
            const bool sameAlpha = top.alphaStops.size() == snap.alphaStops.size()
                && std::equal(
                       top.alphaStops.begin(), top.alphaStops.end(),
                       snap.alphaStops.begin(),
                       [](const AZ::AlphaGradientMarker& a, const AZ::AlphaGradientMarker& b) {
                           return a.m_markerPosition == b.m_markerPosition
                               && a.m_markerAlpha == b.m_markerAlpha;
                       });
            AZ_Printf("GradientCtrl",
                "pushUndoSnapshot: coalesce check sameColor=%d sameAlpha=%d (top.color=%zu snap.color=%zu top.alpha=%zu snap.alpha=%zu)",
                sameColor, sameAlpha,
                top.colorStops.size(), snap.colorStops.size(),
                top.alphaStops.size(), snap.alphaStops.size());
            if (sameColor && sameAlpha)
            {
                AZ_Printf("GradientCtrl", "pushUndoSnapshot: SKIP (coalesced - state unchanged)");
                return;
            }
        }

        // Discard any "future" redo states beyond the cursor before pushing.
        if (m_undoCursor + 1 < m_undoStack.size())
        {
            m_undoStack.erase(m_undoStack.begin() + m_undoCursor + 1, m_undoStack.end());
        }
        m_undoStack.push_back(std::move(snap));
        m_undoCursor = m_undoStack.size() - 1;
        AZ_Printf("GradientCtrl",
            "pushUndoSnapshot: PUSHED stack.size=%zu cursor=%zu",
            m_undoStack.size(), m_undoCursor);
    }

    void ColorGradientEditorDialog::undo()
    {
        AZ_Printf("GradientCtrl",
            "undo: ENTRY stack.size=%zu cursor=%zu",
            m_undoStack.size(), m_undoCursor);
        if (m_undoCursor == 0)
        {
            AZ_Printf("GradientCtrl", "undo: AT BOTTOM, no-op");
            return;
        }
        --m_undoCursor;
        AZ_Printf("GradientCtrl", "undo: stepping to cursor=%zu", m_undoCursor);
        applyUndoSnapshot(m_undoStack[m_undoCursor]);
    }

    void ColorGradientEditorDialog::redo()
    {
        AZ_Printf("GradientCtrl",
            "redo: ENTRY stack.size=%zu cursor=%zu",
            m_undoStack.size(), m_undoCursor);
        if (m_undoCursor + 1 >= m_undoStack.size())
        {
            AZ_Printf("GradientCtrl", "redo: AT TOP, no-op");
            return;
        }
        ++m_undoCursor;
        AZ_Printf("GradientCtrl", "redo: stepping to cursor=%zu", m_undoCursor);
        applyUndoSnapshot(m_undoStack[m_undoCursor]);
    }

    void ColorGradientEditorDialog::applyUndoSnapshot(const UndoSnapshot& snapshot)
    {
        AZ_Printf("GradientCtrl",
            "applyUndoSnapshot: ENTRY snapshot.colorStops=%zu snapshot.alphaStops=%zu",
            snapshot.colorStops.size(), snapshot.alphaStops.size());
        // Preserve the user's CURRENT selection across undo - undo steps the
        // data, not the pointer to which marker the user is working on. If
        // the currently-selected index no longer exists in the restored
        // state, clear the selection.
        GradientStopsTrack::Kind currentKind = m_activeTrack ? m_activeTrack->kind() : GradientStopsTrack::Kind::Color;
        AZStd::optional<size_t> currentIndex = m_activeTrack ? m_activeTrack->selectedIndex() : AZStd::nullopt;

        QSignalBlocker blockAlpha(m_alphaTrack);
        QSignalBlocker blockColor(m_colorTrack);
        m_colorTrack->setColorStops(snapshot.colorStops);
        m_alphaTrack->setAlphaStops(snapshot.alphaStops);

        GradientStopsTrack* target = (currentKind == GradientStopsTrack::Kind::Color) ? m_colorTrack : m_alphaTrack;
        const size_t count = (currentKind == GradientStopsTrack::Kind::Color)
            ? snapshot.colorStops.size()
            : snapshot.alphaStops.size();
        if (currentIndex.has_value() && *currentIndex < count)
        {
            target->setSelectedIndex(*currentIndex);
            m_activeTrack = target;
        }
        else
        {
            m_activeTrack = m_colorTrack;
        }

        m_working.m_colorSlider = m_colorTrack->colorStops();
        m_working.m_alphaSlider = m_alphaTrack->alphaStops();
        m_working.m_sorted = false;
        m_working.SortGradients();

        refreshPreview();
        refreshInspector();
    }

    // =======================================================================
    // PropertyColorGradientCtrl
    // =======================================================================

    PropertyColorGradientCtrl::PropertyColorGradientCtrl(QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(1, 0, 1, 0);
        layout->setSpacing(0);

        m_preview = new GradientBarWidget(this);
        m_preview->setFixedHeight(GradientCtrlConstants::kInlineHeight);
        m_preview->setCursor(Qt::PointingHandCursor);
        m_preview->setToolTip(QStringLiteral("Click to edit gradient | Right-click for copy / paste"));

        layout->addWidget(m_preview, 1);
    }

    void PropertyColorGradientCtrl::setValue(const AZ::ColorGradient& v)
    {
        m_value = v;
        m_value.SortGradients();
        m_preview->setGradient(m_value);
    }

    void PropertyColorGradientCtrl::setAlphaEnabled(bool enabled)
    {
        m_alphaEnabled = enabled;
        m_preview->setAlphaEnabled(enabled);
    }

    void PropertyColorGradientCtrl::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            openEditorDialog();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void PropertyColorGradientCtrl::openEditorDialog()
    {
        ColorGradientEditorDialog dlg(m_value, m_alphaEnabled, m_multiEdit, this);
        if (dlg.exec() == QDialog::Accepted)
        {
            m_value = dlg.result();
            m_value.SortGradients();
            m_preview->setGradient(m_value);
            emit valueChanged();
            emit editingFinished();
        }
    }

    void PropertyColorGradientCtrl::beginReadPass(const AZ::ColorGradient& firstInstance)
    {
        m_multiEdit = false;
        setValue(firstInstance);
        m_preview->setMixed(false);
    }

    void PropertyColorGradientCtrl::addReadInstance(const AZ::ColorGradient& instance)
    {
        // Only flag when a subsequent instance diverges from the first one we
        // read. Structural comparison is enough for the indicator.
        auto sameColor = instance.m_colorSlider.size() == m_value.m_colorSlider.size()
                      && std::equal(
                             instance.m_colorSlider.begin(), instance.m_colorSlider.end(),
                             m_value.m_colorSlider.begin(),
                             [](const AZ::ColorGradientMarker& a, const AZ::ColorGradientMarker& b) {
                                 return a.m_markerPosition == b.m_markerPosition && a.m_markerColor == b.m_markerColor;
                             });
        auto sameAlpha = instance.m_alphaSlider.size() == m_value.m_alphaSlider.size()
                      && std::equal(
                             instance.m_alphaSlider.begin(), instance.m_alphaSlider.end(),
                             m_value.m_alphaSlider.begin(),
                             [](const AZ::AlphaGradientMarker& a, const AZ::AlphaGradientMarker& b) {
                                 return a.m_markerPosition == b.m_markerPosition && a.m_markerAlpha == b.m_markerAlpha;
                             });
        if (!sameColor || !sameAlpha)
        {
            m_multiEdit = true;
            m_preview->setMixed(true);
        }
    }

    // =======================================================================
    // Context Menu + Clipboard Serialization
    // =======================================================================
    // Plain text format so copied gradients can be pasted into other gradient
    // fields or reviewed in a text editor:
    //
    //   AZ_COLOR_GRADIENT v1
    //   C r g b position
    //   C r g b position
    //   A alpha position
    //   A alpha position

    namespace
    {
        static constexpr const char* kClipboardHeader = "AZ_COLOR_GRADIENT v1";

        QString SerializeGradient(const AZ::ColorGradient& g)
        {
            QString out(kClipboardHeader);
            out.append(QLatin1Char('\n'));
            for (const auto& c : g.m_colorSlider)
            {
                out.append(QStringLiteral("C %1 %2 %3 %4\n")
                    .arg(static_cast<qreal>(c.m_markerColor.GetR()), 0, 'g', 7)
                    .arg(static_cast<qreal>(c.m_markerColor.GetG()), 0, 'g', 7)
                    .arg(static_cast<qreal>(c.m_markerColor.GetB()), 0, 'g', 7)
                    .arg(static_cast<qreal>(c.m_markerPosition), 0, 'g', 7));
            }
            for (const auto& a : g.m_alphaSlider)
            {
                out.append(QStringLiteral("A %1 %2\n")
                    .arg(static_cast<qreal>(a.m_markerAlpha), 0, 'g', 7)
                    .arg(static_cast<qreal>(a.m_markerPosition), 0, 'g', 7));
            }
            return out;
        }

        bool DeserializeGradient(const QString& text, AZ::ColorGradient& out)
        {
            const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            if (lines.isEmpty() || !lines.front().startsWith(QLatin1String(kClipboardHeader)))
            {
                return false;
            }

            AZ::ColorGradient parsed;
            for (int i = 1; i < lines.size(); ++i)
            {
                const QStringList tokens = lines[i].split(QLatin1Char(' '), Qt::SkipEmptyParts);
                if (tokens.isEmpty()) { continue; }
                if (tokens[0] == QLatin1String("C") && tokens.size() == 5)
                {
                    AZ::ColorGradientMarker m;
                    m.m_markerColor = AZ::Color(tokens[1].toFloat(), tokens[2].toFloat(), tokens[3].toFloat(), 1.f);
                    m.m_markerPosition = tokens[4].toFloat();
                    parsed.m_colorSlider.push_back(m);
                }
                else if (tokens[0] == QLatin1String("A") && tokens.size() == 3)
                {
                    AZ::AlphaGradientMarker m;
                    m.m_markerAlpha = tokens[1].toFloat();
                    m.m_markerPosition = tokens[2].toFloat();
                    parsed.m_alphaSlider.push_back(m);
                }
            }
            parsed.SortGradients();
            out = parsed;
            return true;
        }
    }

    void PropertyColorGradientCtrl::copyToClipboard()
    {
        QApplication::clipboard()->setText(SerializeGradient(m_value));
    }

    void PropertyColorGradientCtrl::pasteFromClipboard()
    {
        AZ::ColorGradient parsed;
        if (DeserializeGradient(QApplication::clipboard()->text(), parsed))
        {
            m_value = parsed;
            m_preview->setGradient(m_value);
            emit valueChanged();
            emit editingFinished();
        }
    }

    void PropertyColorGradientCtrl::contextMenuEvent(QContextMenuEvent* event)
    {
        QMenu menu(this);
        QAction* copyAction = menu.addAction(QStringLiteral("Copy Gradient"));
        QAction* pasteAction = menu.addAction(QStringLiteral("Paste Gradient"));

        // Paste is only enabled when the clipboard carries a gradient payload.
        AZ::ColorGradient preview;
        pasteAction->setEnabled(DeserializeGradient(QApplication::clipboard()->text(), preview));

        QAction* chosen = menu.exec(event->globalPos());
        if (chosen == copyAction)       { copyToClipboard(); }
        else if (chosen == pasteAction) { pasteFromClipboard(); }

        // Consume so the component's outer context menu does not also fire.
        event->accept();
    }

    // =======================================================================
    // AZColorGradientPropertyHandler (RGBA)
    // =======================================================================

    QWidget* AZColorGradientPropertyHandler::CreateGUI(QWidget* parent)
    {
        PropertyColorGradientCtrl* ctrl = aznew PropertyColorGradientCtrl(parent);
        ctrl->setAlphaEnabled(true);
        connect(ctrl, &PropertyColorGradientCtrl::valueChanged, this, [ctrl]()
        {
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::RequestWrite, ctrl);
        });
        connect(ctrl, &PropertyColorGradientCtrl::editingFinished, this, [ctrl]()
        {
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::OnEditingFinished, ctrl);
        });
        return ctrl;
    }

    void AZColorGradientPropertyHandler::ConsumeAttribute(PropertyColorGradientCtrl* /*GUI*/, AZ::u32 /*attrib*/, PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
    {
    }

    void AZColorGradientPropertyHandler::WriteGUIValuesIntoProperty(size_t /*index*/, PropertyColorGradientCtrl* GUI, property_t& instance, InstanceDataNode* /*node*/)
    {
        instance = GUI->value();
    }

    bool AZColorGradientPropertyHandler::ReadValuesIntoGUI(size_t index, PropertyColorGradientCtrl* GUI, const property_t& instance, InstanceDataNode* /*node*/)
    {
        if (index == 0) { GUI->beginReadPass(instance); }
        else            { GUI->addReadInstance(instance); }
        return false;
    }

    // =======================================================================
    // AZColorGradientRGBPropertyHandler (RGB only)
    // =======================================================================

    QWidget* AZColorGradientRGBPropertyHandler::CreateGUI(QWidget* parent)
    {
        PropertyColorGradientCtrl* ctrl = aznew PropertyColorGradientCtrl(parent);
        ctrl->setAlphaEnabled(false);
        connect(ctrl, &PropertyColorGradientCtrl::valueChanged, this, [ctrl]()
        {
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::RequestWrite, ctrl);
        });
        connect(ctrl, &PropertyColorGradientCtrl::editingFinished, this, [ctrl]()
        {
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::OnEditingFinished, ctrl);
        });
        return ctrl;
    }

    void AZColorGradientRGBPropertyHandler::ConsumeAttribute(PropertyColorGradientCtrl* /*GUI*/, AZ::u32 /*attrib*/, PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
    {
    }

    void AZColorGradientRGBPropertyHandler::WriteGUIValuesIntoProperty(size_t /*index*/, PropertyColorGradientCtrl* GUI, property_t& instance, InstanceDataNode* /*node*/)
    {
        // RGB-only: extract only the color slider, drop alpha track.
        AZ::ColorGradient full = GUI->value();
        instance.m_colorSlider = full.m_colorSlider;
        instance.m_sorted = false;
        instance.SortGradients();
    }

    bool AZColorGradientRGBPropertyHandler::ReadValuesIntoGUI(size_t index, PropertyColorGradientCtrl* GUI, const property_t& instance, InstanceDataNode* /*node*/)
    {
        AZ::ColorGradient wrapper;
        wrapper.m_colorSlider = instance.m_colorSlider;
        wrapper.SortGradients();
        if (index == 0) { GUI->beginReadPass(wrapper); }
        else            { GUI->addReadInstance(wrapper); }
        return false;
    }

    // =======================================================================
    // Registration
    // =======================================================================

    void RegisterColorGradientPropertyHandlers()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew AZColorGradientPropertyHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew AZColorGradientRGBPropertyHandler());
    }
}

#include "UI/PropertyEditor/moc_PropertyColorGradientCtrl.cpp"
