/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyCurveCtrl.hxx"

#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/ByteContainerStream.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWheelEvent>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    using CurveData = AZ::CurveData;
    using TangentMode = AZ::CurveData::TangentMode;

    // =================================================================
    // Local Constants
    // =================================================================

    namespace CurveCtrlConstants
    {
        static constexpr int   kInlineHeight  = 20;
        static constexpr int   kPresetWidth   = 46;   // preset thumbnails are wide and short
        static constexpr int   kPresetHeight  = 20;
        static constexpr int   kKeySize       = 7;
        static constexpr int   kKeyHitRadius  = 6;
        static constexpr int   kMarginLeft    = 38;
        static constexpr int   kMarginBottom  = 18;
        static constexpr int   kMarginTop     = 8;
        static constexpr int   kMarginRight   = 8;
        static constexpr int   kDialogWidth   = 560;
        static constexpr int   kDialogHeight  = 460;
        static constexpr float kRangePadding  = 0.1f;
    }

    // =================================================================
    // Curve Painting + View Helpers (file-local)
    // =================================================================

    namespace
    {
        //! Computes the value (Y) range spanned by the curve over a time window,
        //! padded slightly so the shape is not flush against the frame.
        void ComputeValueRange(const CurveData& curve, float timeMin, float timeMax, float& outMin, float& outMax)
        {
            const int64_t count = curve.GetNumPoints();
            if (count == 0)
            {
                outMin = 0.0f;
                outMax = 1.0f;
                return;
            }

            outMin = AZStd::numeric_limits<float>::max();
            outMax = AZStd::numeric_limits<float>::lowest();
            for (int64_t i = 0; i < count; ++i)
            {
                const float v = curve.GetPoint(i).m_value;
                outMin = AZStd::min(outMin, v);
                outMax = AZStd::max(outMax, v);
            }

            // Sample the interior so dips/overshoots from the tangents are framed too.
            constexpr int Samples = 48;
            for (int i = 0; i <= Samples; ++i)
            {
                const float t = AZ::Lerp(timeMin, timeMax, static_cast<float>(i) / Samples);
                const float v = curve.EvaluateTime(t);
                outMin = AZStd::min(outMin, v);
                outMax = AZStd::max(outMax, v);
            }

            if (outMax - outMin < 1e-3f)
            {
                outMin -= 0.5f;
                outMax += 0.5f;
            }
            const float pad = (outMax - outMin) * CurveCtrlConstants::kRangePadding;
            outMin -= pad;
            outMax += pad;
        }

        //! Paints the curve as a polyline inside the given rect, sampling one
        //! point per pixel column across the supplied view bounds.
        void PaintCurveInRect(
            QPainter& painter,
            const CurveData& curve,
            const QRect& area,
            float timeMin,
            float timeMax,
            float valueMin,
            float valueMax,
            const QColor& lineColor)
        {
            if (curve.GetNumPoints() < 1 || area.width() <= 1 || area.height() <= 1)
            {
                return;
            }

            const float timeSpan = (timeMax > timeMin) ? (timeMax - timeMin) : 1.0f;
            const float valueSpan = (valueMax > valueMin) ? (valueMax - valueMin) : 1.0f;

            auto toScreen = [&](float t, float v)
            {
                const float nx = (t - timeMin) / timeSpan;
                const float ny = (v - valueMin) / valueSpan;
                return QPointF(area.left() + nx * area.width(), area.bottom() - ny * area.height());
            };

            QPainterPath path;
            const int samples = area.width();
            for (int i = 0; i <= samples; ++i)
            {
                const float t = timeMin + timeSpan * (static_cast<float>(i) / samples);
                const float v = curve.EvaluateTime(t);
                const QPointF p = toScreen(t, v);
                if (i == 0)
                {
                    path.moveTo(p);
                }
                else
                {
                    path.lineTo(p);
                }
            }

            painter.setPen(QPen(lineColor, 1.5));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
        }

        CurveData::Point MakePoint(float time, float value, TangentMode inMode, TangentMode outMode)
        {
            CurveData::Point point;
            point.m_time = time;
            point.m_value = value;
            point.m_inMode = inMode;
            point.m_outMode = outMode;
            return point;
        }

        int TangentModeToIndex(TangentMode mode)
        {
            return static_cast<int>(mode);
        }

        TangentMode IndexToTangentMode(int index)
        {
            switch (index)
            {
            case 1:  return TangentMode::Flat;
            case 2:  return TangentMode::Linear;
            case 3:  return TangentMode::Constant;
            case 4:  return TangentMode::Auto;
            case 0:
            default: return TangentMode::Free;
            }
        }

        //! Data-space position of a selection bounding-box corner.
        //! 0 = top-left, 1 = top-right, 2 = bottom-right, 3 = bottom-left.
        QPointF BoxCorner(int corner, float t0, float t1, float v0, float v1)
        {
            switch (corner)
            {
            case 0:  return QPointF(t0, v1);
            case 1:  return QPointF(t1, v1);
            case 2:  return QPointF(t1, v0);
            default: return QPointF(t0, v0);
            }
        }

        // Project-unique preset storage lives in the Settings Registry under this
        // root, persisted to <project>/Registry/CurveEditorPresets.setreg.
        constexpr const char* kPresetRoot = "/O3DE/CurveEditor";
        constexpr const char* kPresetNamesKey = "/O3DE/CurveEditor/PresetNames";

        AZStd::string PresetKey(const AZStd::string& name)
        {
            return AZStd::string("/O3DE/CurveEditor/Presets/") + name;
        }

        AZStd::string SanitizePresetName(const AZStd::string& in)
        {
            AZStd::string out;
            for (char c : in)
            {
                const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                                (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ' ';
                out.push_back(ok ? c : '_');
            }
            return out;
        }

        void PopulateTangentCombo(QComboBox* combo)
        {
            combo->addItem(QStringLiteral("Free"));
            combo->addItem(QStringLiteral("Flat"));
            combo->addItem(QStringLiteral("Linear"));
            combo->addItem(QStringLiteral("Constant"));
            combo->addItem(QStringLiteral("Auto"));
        }
    } // namespace

    // =================================================================
    // CurvePresetButton
    // =================================================================

    CurvePresetButton::CurvePresetButton(const CurveData& curve, const QString& tooltip, QWidget* parent)
        : QWidget(parent)
        , m_curve(curve)
    {
        setFixedSize(CurveCtrlConstants::kPresetWidth, CurveCtrlConstants::kPresetHeight);
        setCursor(Qt::PointingHandCursor);
        setToolTip(tooltip);
    }

    void CurvePresetButton::paintEvent(QPaintEvent* /*event*/)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), m_pressed ? QColor(70, 70, 40) : QColor(35, 35, 35));
        painter.setPen(QColor(20, 20, 20));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));

        float valueMin = 0.0f;
        float valueMax = 1.0f;
        ComputeValueRange(m_curve, m_curve.GetMinTime(), m_curve.GetMaxTime(), valueMin, valueMax);
        const float timeMax = (m_curve.GetMaxTime() > m_curve.GetMinTime()) ? m_curve.GetMaxTime() : m_curve.GetMinTime() + 1.0f;
        PaintCurveInRect(painter, m_curve, rect().adjusted(2, 2, -2, -2), m_curve.GetMinTime(), timeMax, valueMin, valueMax, QColor(220, 220, 220));
    }

    void CurvePresetButton::mousePressEvent(QMouseEvent* /*event*/)
    {
        m_pressed = true;
        update();
    }

    void CurvePresetButton::mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_pressed)
        {
            m_pressed = false;
            update();
            if (rect().contains(event->pos()))
            {
                emit clicked(m_curve);
            }
        }
    }

    void CurvePresetButton::contextMenuEvent(QContextMenuEvent* event)
    {
        if (!m_removable)
        {
            return; // built-in presets are code-defined and cannot be removed
        }
        QMenu menu(this);
        QAction* removeAction = menu.addAction(tr("Remove Preset"));
        if (menu.exec(event->globalPos()) == removeAction)
        {
            emit removeRequested();
        }
        event->accept();
    }

    // =================================================================
    // CurveCanvas
    // =================================================================

    CurveCanvas::CurveCanvas(QWidget* parent)
        : QWidget(parent)
    {
        setMinimumHeight(200);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        m_curve.SetDefaultValue();
        fitViewToCurve();
    }

    void CurveCanvas::setCurve(const CurveData& curve)
    {
        m_curve = curve;
        // A fresh / empty curve seeds the default ramp so there is always
        // something to grab.
        if (m_curve.GetNumPoints() == 0)
        {
            m_curve.SetDefaultValue();
        }
        if (m_selected.has_value() && *m_selected >= m_curve.GetNumPoints())
        {
            m_selected.reset();
        }
        m_selection.clear();
        if (m_selected.has_value())
        {
            m_selection.push_back(*m_selected);
        }
        if (!m_userAdjustedView)
        {
            fitViewToCurve();
        }
        update();
    }

    void CurveCanvas::setSelectedKey(AZStd::optional<int64_t> index)
    {
        m_selected = index;
        m_selection.clear();
        if (index.has_value())
        {
            m_selection.push_back(*index);
        }
        update();
        emit selectionChanged();
    }

    void CurveCanvas::fitViewToCurve()
    {
        m_timeMin = m_curve.GetMinTime();
        m_timeMax = m_curve.GetMaxTime();
        if (m_timeMax <= m_timeMin)
        {
            m_timeMax = m_timeMin + 1.0f;
        }
        ComputeValueRange(m_curve, m_timeMin, m_timeMax, m_valueMin, m_valueMax);
    }

    void CurveCanvas::recomputePlotRect()
    {
        m_plotRect = rect().adjusted(
            CurveCtrlConstants::kMarginLeft,
            CurveCtrlConstants::kMarginTop,
            -CurveCtrlConstants::kMarginRight,
            -CurveCtrlConstants::kMarginBottom);
    }

    QPointF CurveCanvas::dataToScreen(float time, float value) const
    {
        const float timeSpan = (m_timeMax > m_timeMin) ? (m_timeMax - m_timeMin) : 1.0f;
        const float valueSpan = (m_valueMax > m_valueMin) ? (m_valueMax - m_valueMin) : 1.0f;
        const float nx = (time - m_timeMin) / timeSpan;
        const float ny = (value - m_valueMin) / valueSpan;
        return QPointF(m_plotRect.left() + nx * m_plotRect.width(), m_plotRect.bottom() - ny * m_plotRect.height());
    }

    QPointF CurveCanvas::screenToData(const QPointF& screen) const
    {
        const float timeSpan = (m_timeMax > m_timeMin) ? (m_timeMax - m_timeMin) : 1.0f;
        const float valueSpan = (m_valueMax > m_valueMin) ? (m_valueMax - m_valueMin) : 1.0f;
        const float w = (m_plotRect.width() > 0) ? static_cast<float>(m_plotRect.width()) : 1.0f;
        const float h = (m_plotRect.height() > 0) ? static_cast<float>(m_plotRect.height()) : 1.0f;
        const float nx = (static_cast<float>(screen.x()) - m_plotRect.left()) / w;
        const float ny = (m_plotRect.bottom() - static_cast<float>(screen.y())) / h;
        return QPointF(m_timeMin + nx * timeSpan, m_valueMin + ny * valueSpan);
    }

    AZStd::optional<int64_t> CurveCanvas::hitTestKey(const QPoint& screen) const
    {
        const float radius = static_cast<float>(CurveCtrlConstants::kKeyHitRadius);
        for (int64_t i = 0; i < m_curve.GetNumPoints(); ++i)
        {
            const CurveData::Point point = m_curve.GetPoint(i);
            const QPointF center = dataToScreen(point.m_time, point.m_value);
            const QPointF delta = center - QPointF(screen);
            if (delta.manhattanLength() <= static_cast<qreal>(radius) * 2.0)
            {
                return i;
            }
        }
        return AZStd::nullopt;
    }

    // -----------------------------------------------------------------
    // Tangent handles
    // -----------------------------------------------------------------
    // A key's outgoing arm spans toward the next key (its in arm toward the
    // previous key). End keys have no neighbour on one side, so a fraction of
    // the visible time range is used as the arm length instead. The handle
    // endpoint is the arm's far end in data space: time offset by weight*span,
    // value offset by slope*weight*span.

    float CurveCanvas::outArmTimeSpan(int64_t index) const
    {
        if (index + 1 < m_curve.GetNumPoints())
        {
            return m_curve.GetPoint(index + 1).m_time - m_curve.GetPoint(index).m_time;
        }
        return 0.25f * (m_timeMax - m_timeMin);
    }

    float CurveCanvas::inArmTimeSpan(int64_t index) const
    {
        if (index > 0)
        {
            return m_curve.GetPoint(index).m_time - m_curve.GetPoint(index - 1).m_time;
        }
        return 0.25f * (m_timeMax - m_timeMin);
    }

    QPointF CurveCanvas::outHandleScreen(int64_t index) const
    {
        const CurveData::Point p = m_curve.GetPoint(index);
        const float span = outArmTimeSpan(index);
        const float reach = p.m_outWeight * span;
        return dataToScreen(p.m_time + reach, p.m_value + p.m_outTangent * reach);
    }

    QPointF CurveCanvas::inHandleScreen(int64_t index) const
    {
        const CurveData::Point p = m_curve.GetPoint(index);
        const float span = inArmTimeSpan(index);
        const float reach = p.m_inWeight * span;
        return dataToScreen(p.m_time - reach, p.m_value - p.m_inTangent * reach);
    }

    CurveCanvas::Drag CurveCanvas::hitTestHandle(const QPoint& screen) const
    {
        if (!m_selected.has_value())
        {
            return Drag::None;
        }
        const qreal hit = static_cast<qreal>(CurveCtrlConstants::kKeyHitRadius) * 2.0;
        if ((outHandleScreen(*m_selected) - QPointF(screen)).manhattanLength() <= hit)
        {
            return Drag::OutHandle;
        }
        if ((inHandleScreen(*m_selected) - QPointF(screen)).manhattanLength() <= hit)
        {
            return Drag::InHandle;
        }
        return Drag::None;
    }

    void CurveCanvas::dragHandle(int64_t index, bool inArm, const QPointF& cursorData)
    {
        CurveData::Point p = m_curve.GetPoint(index);
        constexpr float kEps = 1e-4f;

        if (inArm)
        {
            const float span = inArmTimeSpan(index);
            const float horiz = AZStd::max(static_cast<float>(p.m_time - cursorData.x()), kEps);
            const float slope = static_cast<float>(p.m_value - cursorData.y()) / horiz;
            p.m_inTangent = slope;
            p.m_inWeight = (span > 0.0f) ? AZ::GetClamp(horiz / span, 0.0f, 1.0f) : CurveData::DefaultWeight;
            p.m_inMode = TangentMode::Free;
            // Unified arms stay colinear; broken arms move independently.
            if (!p.m_broken)
            {
                p.m_outTangent = slope;
                p.m_outMode = TangentMode::Free;
            }
        }
        else
        {
            const float span = outArmTimeSpan(index);
            const float horiz = AZStd::max(static_cast<float>(cursorData.x() - p.m_time), kEps);
            const float slope = static_cast<float>(cursorData.y() - p.m_value) / horiz;
            p.m_outTangent = slope;
            p.m_outWeight = (span > 0.0f) ? AZ::GetClamp(horiz / span, 0.0f, 1.0f) : CurveData::DefaultWeight;
            p.m_outMode = TangentMode::Free;
            if (!p.m_broken)
            {
                p.m_inTangent = slope;
                p.m_inMode = TangentMode::Free;
            }
        }

        // Time is unchanged, so the index is stable.
        m_curve.UpdatePoint(index, p);
        update();
        emit curveChanged();
    }

    void CurveCanvas::addKeyAt(const QPoint& screen)
    {
        const QPointF dataPos = screenToData(screen);
        const float time = AZStd::max(0.0f, static_cast<float>(dataPos.x()));
        CurveData::Point point = MakePoint(time, static_cast<float>(dataPos.y()), TangentMode::Auto, TangentMode::Auto);
        const int64_t index = m_curve.AddPoint(point);
        if (index >= 0)
        {
            m_selected = index;
            m_selection.clear();
            m_selection.push_back(index);
            update();
            emit selectionChanged();
            emit curveChanged();
            emit editCommitted();
        }
    }

    // -----------------------------------------------------------------
    // Group move / scale
    // -----------------------------------------------------------------

    bool CurveCanvas::selectionBounds(float& t0, float& t1, float& v0, float& v1) const
    {
        if (m_selection.empty())
        {
            return false;
        }
        t0 = AZStd::numeric_limits<float>::max();
        v0 = AZStd::numeric_limits<float>::max();
        t1 = AZStd::numeric_limits<float>::lowest();
        v1 = AZStd::numeric_limits<float>::lowest();
        for (int64_t idx : m_selection)
        {
            const CurveData::Point p = m_curve.GetPoint(idx);
            t0 = AZStd::min(t0, p.m_time);
            t1 = AZStd::max(t1, p.m_time);
            v0 = AZStd::min(v0, p.m_value);
            v1 = AZStd::max(v1, p.m_value);
        }
        return true;
    }

    int CurveCanvas::hitTestScaleHandle(const QPoint& screen) const
    {
        if (m_selection.size() < 2)
        {
            return -1;
        }
        float t0, t1, v0, v1;
        if (!selectionBounds(t0, t1, v0, v1))
        {
            return -1;
        }
        for (int c = 0; c < 4; ++c)
        {
            const QPointF corner = BoxCorner(c, t0, t1, v0, v1);
            const QPointF sp = dataToScreen(static_cast<float>(corner.x()), static_cast<float>(corner.y()));
            if ((sp - QPointF(screen)).manhattanLength() <= 8.0)
            {
                return c;
            }
        }
        return -1;
    }

    void CurveCanvas::beginGroupOp()
    {
        const AZStd::vector<CurveData::Point>& pts = m_curve.GetPoints();
        m_opSnapshot.assign(pts.begin(), pts.end());
        m_opSelected = m_selection;
        selectionBounds(m_opT0, m_opT1, m_opV0, m_opV1);
    }

    void CurveCanvas::commitSelectedPositions(const AZStd::vector<QPointF>& newPositions)
    {
        // Overwrite the selected points' positions in a copy of the snapshot,
        // re-sort by time, and rebuild curve + selection atomically so moving a
        // key past a neighbour does not corrupt the index set mid-drag.
        AZStd::vector<CurveData::Point> all = m_opSnapshot;
        AZStd::vector<bool> selected(all.size(), false);
        for (size_t k = 0; k < m_opSelected.size() && k < newPositions.size(); ++k)
        {
            const int64_t idx = m_opSelected[k];
            all[idx].m_time = AZStd::max(0.0f, static_cast<float>(newPositions[k].x()));
            all[idx].m_value = static_cast<float>(newPositions[k].y());
            selected[idx] = true;
        }

        AZStd::vector<size_t> order(all.size());
        for (size_t i = 0; i < order.size(); ++i)
        {
            order[i] = i;
        }
        AZStd::sort(order.begin(), order.end(), [&all](size_t a, size_t b) { return all[a].m_time < all[b].m_time; });

        AZStd::vector<CurveData::Point> sorted;
        AZStd::vector<int64_t> newSelection;
        sorted.reserve(all.size());
        for (size_t i = 0; i < order.size(); ++i)
        {
            sorted.push_back(all[order[i]]);
            if (selected[order[i]])
            {
                newSelection.push_back(static_cast<int64_t>(i));
            }
        }

        m_curve.SetPoints(sorted);
        m_selection = newSelection;
        m_selected = newSelection.empty() ? AZStd::optional<int64_t>{} : AZStd::optional<int64_t>{ newSelection.front() };
        update();
        emit curveChanged();
        emit selectionChanged();
    }

    void CurveCanvas::groupMove(const QPointF& deltaData)
    {
        AZStd::vector<QPointF> positions;
        positions.reserve(m_opSelected.size());
        for (int64_t idx : m_opSelected)
        {
            positions.push_back(QPointF(
                m_opSnapshot[idx].m_time + deltaData.x(),
                m_opSnapshot[idx].m_value + deltaData.y()));
        }
        commitSelectedPositions(positions);
    }

    void CurveCanvas::groupScale(float scaleX, float scaleY, const QPointF& pivot)
    {
        AZStd::vector<QPointF> positions;
        positions.reserve(m_opSelected.size());
        const float px = static_cast<float>(pivot.x());
        const float py = static_cast<float>(pivot.y());
        for (int64_t idx : m_opSelected)
        {
            positions.push_back(QPointF(
                px + (m_opSnapshot[idx].m_time - px) * scaleX,
                py + (m_opSnapshot[idx].m_value - py) * scaleY));
        }
        commitSelectedPositions(positions);
    }

    void CurveCanvas::setSelectedKeyTime(float time)
    {
        if (!m_selected.has_value())
        {
            return;
        }
        CurveData::Point point = m_curve.GetPoint(*m_selected);
        point.m_time = AZStd::max(0.0f, time);
        m_selected = m_curve.UpdatePoint(*m_selected, point);
        update();
        emit curveChanged();
        emit selectionChanged();
    }

    void CurveCanvas::setSelectedKeyValue(float value)
    {
        if (!m_selected.has_value())
        {
            return;
        }
        CurveData::Point point = m_curve.GetPoint(*m_selected);
        point.m_value = value;
        m_selected = m_curve.UpdatePoint(*m_selected, point);
        update();
        emit curveChanged();
    }

    void CurveCanvas::setSelectedKeyTangents(TangentMode inMode, TangentMode outMode, bool broken)
    {
        if (!m_selected.has_value())
        {
            return;
        }
        CurveData::Point point = m_curve.GetPoint(*m_selected);
        point.m_inMode = inMode;
        point.m_outMode = outMode;
        point.m_broken = broken;
        if (!broken)
        {
            // Unify: make the two arms colinear immediately so the bars update
            // without needing a drag first.
            const float avg = 0.5f * (point.m_inTangent + point.m_outTangent);
            point.m_inTangent = avg;
            point.m_outTangent = avg;
        }
        m_selected = m_curve.UpdatePoint(*m_selected, point);
        update();
        emit curveChanged();
    }

    void CurveCanvas::deleteSelectedKey()
    {
        AZStd::vector<int64_t> indices = m_selection;
        if (indices.empty() && m_selected.has_value())
        {
            indices.push_back(*m_selected);
        }
        if (indices.empty())
        {
            return;
        }
        // m_selection is ascending; remove from the back so the earlier indices
        // stay valid as we go. Never drop below one point.
        for (auto it = indices.rbegin(); it != indices.rend(); ++it)
        {
            if (m_curve.GetNumPoints() <= 1)
            {
                break;
            }
            m_curve.RemovePoint(*it);
        }
        m_selected.reset();
        m_selection.clear();
        update();
        emit selectionChanged();
        emit curveChanged();
        emit editCommitted();
    }

    void CurveCanvas::resizeEvent(QResizeEvent* event)
    {
        recomputePlotRect();
        QWidget::resizeEvent(event);
    }

    void CurveCanvas::paintEvent(QPaintEvent* /*event*/)
    {
        recomputePlotRect();

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(48, 48, 48));
        painter.fillRect(m_plotRect, QColor(56, 56, 56));

        // Grid + axis labels (four divisions each way).
        painter.setPen(QColor(72, 72, 72));
        constexpr int Divisions = 4;
        for (int i = 0; i <= Divisions; ++i)
        {
            const int x = m_plotRect.left() + m_plotRect.width() * i / Divisions;
            const int y = m_plotRect.top() + m_plotRect.height() * i / Divisions;
            painter.drawLine(x, m_plotRect.top(), x, m_plotRect.bottom());
            painter.drawLine(m_plotRect.left(), y, m_plotRect.right(), y);
        }

        painter.setPen(QColor(150, 150, 150));
        for (int i = 0; i <= Divisions; ++i)
        {
            const float tValue = AZ::Lerp(m_valueMin, m_valueMax, 1.0f - static_cast<float>(i) / Divisions);
            const int y = m_plotRect.top() + m_plotRect.height() * i / Divisions;
            painter.drawText(2, y + 4, QString::number(tValue, 'g', 3));

            const float tTime = AZ::Lerp(m_timeMin, m_timeMax, static_cast<float>(i) / Divisions);
            const int x = m_plotRect.left() + m_plotRect.width() * i / Divisions;
            painter.drawText(x - 10, rect().bottom() - 4, QString::number(tTime, 'g', 3));
        }

        // Curve.
        PaintCurveInRect(painter, m_curve, m_plotRect, m_timeMin, m_timeMax, m_valueMin, m_valueMax, QColor(240, 120, 80));

        // Keys.
        const int half = CurveCtrlConstants::kKeySize / 2;
        for (int64_t i = 0; i < m_curve.GetNumPoints(); ++i)
        {
            const CurveData::Point point = m_curve.GetPoint(i);
            const QPointF center = dataToScreen(point.m_time, point.m_value);
            const QRectF keyRect(center.x() - half, center.y() - half, CurveCtrlConstants::kKeySize, CurveCtrlConstants::kKeySize);

            const bool primary = m_selected.has_value() && *m_selected == i;
            const bool highlighted = AZStd::find(m_selection.begin(), m_selection.end(), i) != m_selection.end();
            painter.setBrush(QColor(200, 200, 200));
            if (primary)          { painter.setPen(QPen(QColor(255, 200, 0), 2)); }
            else if (highlighted) { painter.setPen(QPen(QColor(255, 160, 0), 2)); }
            else                  { painter.setPen(QPen(QColor(30, 30, 30), 1)); }
            painter.drawRect(keyRect);
        }

        // Tangent handles for the selected key (single selection only; a
        // multi-selection shows the scale gizmo instead).
        if (m_selected.has_value() && m_selection.size() < 2)
        {
            const int64_t i = *m_selected;
            const CurveData::Point p = m_curve.GetPoint(i);
            const QPointF keyPos = dataToScreen(p.m_time, p.m_value);
            const QPointF inPos = inHandleScreen(i);
            const QPointF outPos = outHandleScreen(i);

            painter.setPen(QPen(QColor(120, 200, 255), 1));
            painter.drawLine(keyPos, inPos);
            painter.drawLine(keyPos, outPos);

            painter.setBrush(QColor(120, 200, 255));
            painter.setPen(QPen(QColor(20, 20, 20), 1));
            painter.drawEllipse(inPos, 3.0, 3.0);
            painter.drawEllipse(outPos, 3.0, 3.0);
        }

        // Scale gizmo: bounding box + corner handles around a multi-selection.
        if (m_selection.size() >= 2)
        {
            float t0, t1, v0, v1;
            if (selectionBounds(t0, t1, v0, v1))
            {
                painter.setPen(QPen(QColor(255, 200, 0), 1, Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(QRectF(dataToScreen(t0, v1), dataToScreen(t1, v0)).normalized());

                painter.setBrush(QColor(255, 200, 0));
                painter.setPen(QPen(QColor(20, 20, 20), 1));
                for (int c = 0; c < 4; ++c)
                {
                    const QPointF corner = BoxCorner(c, t0, t1, v0, v1);
                    const QPointF sp = dataToScreen(static_cast<float>(corner.x()), static_cast<float>(corner.y()));
                    painter.drawRect(QRectF(sp.x() - 3.0, sp.y() - 3.0, 6.0, 6.0));
                }
            }
        }

        // Marquee rubber-band.
        if (m_drag == Drag::Marquee && !m_marqueeRect.isNull())
        {
            painter.setPen(QPen(QColor(120, 200, 255), 1, Qt::DashLine));
            painter.setBrush(QColor(120, 200, 255, 40));
            painter.drawRect(m_marqueeRect);
        }
    }

    void CurveCanvas::mousePressEvent(QMouseEvent* event)
    {
        setFocus();

        if (event->button() == Qt::MiddleButton)
        {
            m_drag = Drag::Pan;
            m_lastPanPos = event->pos();
            event->accept();
            return;
        }

        if (event->button() == Qt::LeftButton)
        {
            // A handle of the selected key takes priority over key / empty hits.
            const Drag handle = hitTestHandle(event->pos());
            if (handle != Drag::None)
            {
                m_drag = handle;
                event->accept();
                return;
            }

            // Scale-gizmo corner handles take priority (only with >= 2 selected).
            const int corner = hitTestScaleHandle(event->pos());
            if (corner >= 0)
            {
                m_scaleCorner = corner;
                beginGroupOp();
                m_drag = Drag::Scale;
                event->accept();
                return;
            }

            auto hit = hitTestKey(event->pos());
            if (hit.has_value())
            {
                const bool inGroup = m_selection.size() >= 2 &&
                    AZStd::find(m_selection.begin(), m_selection.end(), *hit) != m_selection.end();
                if (inGroup)
                {
                    // Dragging any member of a multi-selection moves the whole group.
                    beginGroupOp();
                    m_moveStartData = screenToData(event->pos());
                    m_drag = Drag::Move;
                }
                else
                {
                    m_selected = hit;
                    m_selection.clear();
                    m_selection.push_back(*hit);
                    m_drag = Drag::Key;
                    update();
                    emit selectionChanged();
                }
            }
            else
            {
                // Begin a marquee in empty space. Release decides drag (group
                // select) vs click (deselect).
                m_drag = Drag::Marquee;
                m_marqueeStart = event->pos();
                m_marqueeRect = QRect(m_marqueeStart, QSize(0, 0));
            }
        }
        event->accept();
    }

    void CurveCanvas::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_drag == Drag::Marquee && (event->buttons() & Qt::LeftButton))
        {
            m_marqueeRect = QRect(m_marqueeStart, event->pos()).normalized();
            m_selection.clear();
            for (int64_t i = 0; i < m_curve.GetNumPoints(); ++i)
            {
                const CurveData::Point p = m_curve.GetPoint(i);
                if (m_marqueeRect.contains(dataToScreen(p.m_time, p.m_value).toPoint()))
                {
                    m_selection.push_back(i);
                }
            }
            m_selected = m_selection.empty() ? AZStd::optional<int64_t>{} : AZStd::optional<int64_t>{ m_selection.front() };
            update();
            emit selectionChanged();
            event->accept();
            return;
        }

        if (m_drag == Drag::Pan && (event->buttons() & Qt::MiddleButton))
        {
            const QPointF before = screenToData(m_lastPanPos);
            const QPointF after = screenToData(event->pos());
            const float dt = static_cast<float>(before.x() - after.x());
            const float dv = static_cast<float>(before.y() - after.y());
            m_timeMin += dt;
            m_timeMax += dt;
            m_valueMin += dv;
            m_valueMax += dv;
            m_userAdjustedView = true;
            m_lastPanPos = event->pos();
            update();
            event->accept();
            return;
        }

        if (m_drag == Drag::Move && (event->buttons() & Qt::LeftButton))
        {
            groupMove(screenToData(event->pos()) - m_moveStartData);
            event->accept();
            return;
        }

        if (m_drag == Drag::Scale && (event->buttons() & Qt::LeftButton))
        {
            const QPointF anchor = BoxCorner((m_scaleCorner + 2) % 4, m_opT0, m_opT1, m_opV0, m_opV1);
            const QPointF handle = BoxCorner(m_scaleCorner, m_opT0, m_opT1, m_opV0, m_opV1);
            const QPointF cur = screenToData(event->pos());
            const float denomX = static_cast<float>(handle.x() - anchor.x());
            const float denomY = static_cast<float>(handle.y() - anchor.y());
            const float sx = (AZ::GetAbs(denomX) > 1e-5f) ? (static_cast<float>(cur.x() - anchor.x()) / denomX) : 1.0f;
            const float sy = (AZ::GetAbs(denomY) > 1e-5f) ? (static_cast<float>(cur.y() - anchor.y()) / denomY) : 1.0f;
            groupScale(sx, sy, anchor);
            event->accept();
            return;
        }

        if (m_selected.has_value() && (event->buttons() & Qt::LeftButton))
        {
            if (m_drag == Drag::Key)
            {
                const QPointF dataPos = screenToData(event->pos());
                CurveData::Point point = m_curve.GetPoint(*m_selected);
                point.m_time = AZStd::max(0.0f, static_cast<float>(dataPos.x()));
                point.m_value = static_cast<float>(dataPos.y());
                m_selected = m_curve.UpdatePoint(*m_selected, point);
                m_selection.clear();
                if (m_selected.has_value())
                {
                    m_selection.push_back(*m_selected);
                }
                update();
                emit curveChanged();
                emit selectionChanged();
            }
            else if (m_drag == Drag::InHandle || m_drag == Drag::OutHandle)
            {
                dragHandle(*m_selected, m_drag == Drag::InHandle, screenToData(event->pos()));
            }
        }
        event->accept();
    }

    void CurveCanvas::mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_drag == Drag::Marquee)
        {
            // A tiny rect means a plain click in empty space: deselect.
            if (m_marqueeRect.width() < 3 && m_marqueeRect.height() < 3)
            {
                m_selected.reset();
                m_selection.clear();
                emit selectionChanged();
            }
            m_marqueeRect = QRect();
            m_drag = Drag::None;
            update();
            event->accept();
            return;
        }
        if (m_drag != Drag::None)
        {
            const bool wasEditing = (m_drag != Drag::Pan);
            m_drag = Drag::None;
            if (wasEditing)
            {
                emit editCommitted();
            }
        }
        event->accept();
    }

    void CurveCanvas::contextMenuEvent(QContextMenuEvent* event)
    {
        QMenu menu(this);
        auto hit = hitTestKey(event->pos());
        if (hit.has_value())
        {
            m_selected = hit;
            m_selection.clear();
            m_selection.push_back(*hit);
            update();
            emit selectionChanged();

            const CurveData::Point p = m_curve.GetPoint(*hit);
            menu.addAction(p.m_broken ? tr("Unify Tangents") : tr("Break Tangents"), this, [this]()
            {
                if (!m_selected.has_value())
                {
                    return;
                }
                CurveData::Point pt = m_curve.GetPoint(*m_selected);
                pt.m_broken = !pt.m_broken;
                if (!pt.m_broken)
                {
                    const float avg = 0.5f * (pt.m_inTangent + pt.m_outTangent);
                    pt.m_inTangent = avg;
                    pt.m_outTangent = avg;
                }
                m_curve.UpdatePoint(*m_selected, pt);
                update();
                emit curveChanged();
                emit selectionChanged();
                emit editCommitted();
            });

            if (m_curve.GetNumPoints() > 1)
            {
                menu.addAction(tr("Delete Key"), this, [this]() { deleteSelectedKey(); });
            }
        }
        else
        {
            const QPoint pos = event->pos();
            menu.addAction(tr("Add Key"), this, [this, pos]() { addKeyAt(pos); });
        }
        menu.exec(event->globalPos());
        event->accept();
    }

    void CurveCanvas::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_F)
        {
            // Re-fit the view to the curve (recover from zoom / pan).
            m_userAdjustedView = false;
            fitViewToCurve();
            update();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Delete)
        {
            deleteSelectedKey();
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }

    void CurveCanvas::wheelEvent(QWheelEvent* event)
    {
        recomputePlotRect();
        const QPointF cursor = screenToData(event->position());

        // Zoom about the cursor: scale the visible span while keeping the data
        // point under the cursor fixed on screen.
        const float factor = (event->angleDelta().y() > 0) ? 0.85f : (1.0f / 0.85f);
        const float cx = static_cast<float>(cursor.x());
        const float cy = static_cast<float>(cursor.y());
        m_timeMin = cx + (m_timeMin - cx) * factor;
        m_timeMax = cx + (m_timeMax - cx) * factor;
        m_valueMin = cy + (m_valueMin - cy) * factor;
        m_valueMax = cy + (m_valueMax - cy) * factor;
        m_userAdjustedView = true;
        update();
        event->accept();
    }

    // =================================================================
    // CurveEditorDialog
    // =================================================================

    CurveEditorDialog::CurveEditorDialog(const CurveData& initial, bool multiEdit, QWidget* parent)
        : QDialog(parent)
        , m_working(initial)
    {
        setFocusPolicy(Qt::StrongFocus);
        setWindowTitle(multiEdit
            ? QStringLiteral("Curve Editor (multi-edit: applies to all selected)")
            : QStringLiteral("Curve Editor"));
        setMinimumSize(CurveCtrlConstants::kDialogWidth, CurveCtrlConstants::kDialogHeight);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(8, 8, 8, 8);
        root->setSpacing(6);

        if (multiEdit)
        {
            auto* banner = new QLabel(QStringLiteral("This edit will be written to every selected entity's curve field."), this);
            banner->setStyleSheet(QStringLiteral("color: #ffcc00; font-weight: bold;"));
            banner->setWordWrap(true);
            root->addWidget(banner);
        }

        // Preset palette.
        auto* presetRow = new QHBoxLayout();
        presetRow->setSpacing(4);
        buildPresetPalette(presetRow);
        presetRow->addStretch(1);
        auto* savePresetButton = new QPushButton(tr("Save as Preset..."), this);
        savePresetButton->setToolTip(QStringLiteral("Save the current curve as a project-unique preset"));
        connect(savePresetButton, &QPushButton::clicked, this, [this]() { saveCurrentAsPreset(); });
        presetRow->addWidget(savePresetButton);
        root->addLayout(presetRow);

        // Canvas. Seed an empty curve with the default ramp so result() is
        // never empty even if the user accepts without editing.
        if (m_working.GetNumPoints() == 0)
        {
            m_working.SetDefaultValue();
        }
        m_canvas = new CurveCanvas(this);
        m_canvas->setCurve(m_working);
        root->addWidget(m_canvas, 1);

        // Selected-key inspector.
        auto* inspector = new QHBoxLayout();
        inspector->setSpacing(6);

        m_selectionLabel = new QLabel(QStringLiteral("(no selection)"), this);
        m_timeSpin = new QDoubleSpinBox(this);
        m_timeSpin->setRange(0.0, 1.0e6);
        m_timeSpin->setDecimals(4);
        m_timeSpin->setSingleStep(0.05);
        m_valueSpin = new QDoubleSpinBox(this);
        m_valueSpin->setRange(-1.0e6, 1.0e6);
        m_valueSpin->setDecimals(4);
        m_valueSpin->setSingleStep(0.05);
        m_inModeCombo = new QComboBox(this);
        PopulateTangentCombo(m_inModeCombo);
        m_outModeCombo = new QComboBox(this);
        PopulateTangentCombo(m_outModeCombo);
        m_brokenCheck = new QCheckBox(QStringLiteral("Broken"), this);

        inspector->addWidget(m_selectionLabel);
        inspector->addStretch(1);
        inspector->addWidget(new QLabel(QStringLiteral("Time"), this));
        inspector->addWidget(m_timeSpin);
        inspector->addWidget(new QLabel(QStringLiteral("Value"), this));
        inspector->addWidget(m_valueSpin);
        inspector->addWidget(new QLabel(QStringLiteral("In"), this));
        inspector->addWidget(m_inModeCombo);
        inspector->addWidget(new QLabel(QStringLiteral("Out"), this));
        inspector->addWidget(m_outModeCombo);
        inspector->addWidget(m_brokenCheck);
        root->addLayout(inspector);

        // Buttons.
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

        // Canvas wiring.
        connect(m_canvas, &CurveCanvas::curveChanged, this, &CurveEditorDialog::onCanvasChanged);
        connect(m_canvas, &CurveCanvas::selectionChanged, this, &CurveEditorDialog::onCanvasSelectionChanged);
        connect(m_canvas, &CurveCanvas::editCommitted, this, [this]() { pushUndoSnapshot(); });

        // Inspector wiring: spins live-update, commit on editingFinished.
        connect(m_timeSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CurveEditorDialog::onInspectorTimeChanged);
        connect(m_valueSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CurveEditorDialog::onInspectorValueChanged);
        connect(m_timeSpin, &QDoubleSpinBox::editingFinished, this, [this]() { pushUndoSnapshot(); });
        connect(m_valueSpin, &QDoubleSpinBox::editingFinished, this, [this]() { pushUndoSnapshot(); });
        connect(m_inModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { onInspectorTangentChanged(); });
        connect(m_outModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { onInspectorTangentChanged(); });
        connect(m_brokenCheck, &QCheckBox::toggled, this, [this](bool) { onInspectorTangentChanged(); });

        refreshInspector();

        // Seed the undo stack with the opening state.
        m_undoStack.push_back(captureSnapshot());
        m_undoCursor = 0;

        // Window-scoped undo / redo shortcuts so they take priority over the
        // editor's global Ctrl+Z while the dialog is active.
        auto installShortcut = [this](const QKeySequence& seq, auto slot)
        {
            auto* sc = new QShortcut(seq, this);
            sc->setContext(Qt::WindowShortcut);
            connect(sc, &QShortcut::activated, this, slot);
        };
        installShortcut(QKeySequence(QKeySequence::Undo), [this]() { undo(); });
        installShortcut(QKeySequence(QKeySequence::Redo), [this]() { redo(); });
        installShortcut(
            QKeySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::SHIFT) | static_cast<int>(Qt::Key_Z)),
            [this]() { redo(); });
    }

    void CurveEditorDialog::buildPresetPalette(QLayout* parent)
    {
        m_presetLayout = qobject_cast<QHBoxLayout*>(parent);

        struct PresetDef
        {
            const char* m_tooltip;
            CurveData m_curve;
        };

        AZStd::vector<PresetDef> presets;

        auto twoKey = [](TangentMode mode)
        {
            return CurveData({ MakePoint(0.0f, 0.0f, mode, mode), MakePoint(1.0f, 1.0f, mode, mode) });
        };

        // Linear ramp.
        presets.push_back({ "Linear", twoKey(TangentMode::Linear) });

        // Ease In: flat departure, steep arrival (accelerate).
        {
            CurveData::Point a = MakePoint(0.0f, 0.0f, TangentMode::Flat, TangentMode::Flat);
            CurveData::Point b = MakePoint(1.0f, 1.0f, TangentMode::Free, TangentMode::Free);
            b.m_inTangent = 2.5f;
            presets.push_back({ "Ease In", CurveData({ a, b }) });
        }
        // Ease Out: steep departure, flat arrival (decelerate).
        {
            CurveData::Point a = MakePoint(0.0f, 0.0f, TangentMode::Free, TangentMode::Free);
            a.m_outTangent = 2.5f;
            CurveData::Point b = MakePoint(1.0f, 1.0f, TangentMode::Flat, TangentMode::Flat);
            presets.push_back({ "Ease Out", CurveData({ a, b }) });
        }

        // Smooth S-curve (ease in and out).
        presets.push_back({ "Ease In-Out", twoKey(TangentMode::Flat) });

        // Sharp S-curve: flat ends with long arms, fast middle.
        {
            CurveData::Point a = MakePoint(0.0f, 0.0f, TangentMode::Free, TangentMode::Free);
            a.m_outTangent = 0.0f;
            a.m_outWeight = 0.55f;
            CurveData::Point b = MakePoint(1.0f, 1.0f, TangentMode::Free, TangentMode::Free);
            b.m_inTangent = 0.0f;
            b.m_inWeight = 0.55f;
            presets.push_back({ "Sharp", CurveData({ a, b }) });
        }

        // Anticipate: dips below the start before rising.
        presets.push_back({ "Anticipate",
            CurveData({ MakePoint(0.0f, 0.0f, TangentMode::Auto, TangentMode::Auto),
                        MakePoint(0.18f, -0.15f, TangentMode::Auto, TangentMode::Auto),
                        MakePoint(1.0f, 1.0f, TangentMode::Auto, TangentMode::Auto) }) });

        // Overshoot: passes the target then settles back.
        presets.push_back({ "Overshoot",
            CurveData({ MakePoint(0.0f, 0.0f, TangentMode::Auto, TangentMode::Auto),
                        MakePoint(0.78f, 1.12f, TangentMode::Auto, TangentMode::Auto),
                        MakePoint(1.0f, 1.0f, TangentMode::Auto, TangentMode::Auto) }) });

        // Bounce: decaying bounces settling at the target.
        presets.push_back({ "Bounce",
            CurveData({ MakePoint(0.0f, 0.0f, TangentMode::Auto, TangentMode::Auto),
                        MakePoint(0.35f, 1.0f, TangentMode::Auto, TangentMode::Auto),
                        MakePoint(0.5f, 0.7f, TangentMode::Auto, TangentMode::Auto),
                        MakePoint(0.65f, 1.0f, TangentMode::Auto, TangentMode::Auto),
                        MakePoint(0.82f, 0.88f, TangentMode::Auto, TangentMode::Auto),
                        MakePoint(1.0f, 1.0f, TangentMode::Auto, TangentMode::Auto) }) });

        // Constant: hold then step.
        presets.push_back({ "Constant", twoKey(TangentMode::Constant) });

        for (const PresetDef& def : presets)
        {
            addPresetButton(AZStd::string(def.m_tooltip), def.m_curve, false);
        }

        // Append any project-unique presets saved previously (removable).
        loadCustomPresets();
    }

    void CurveEditorDialog::applyPreset(const CurveData& preset)
    {
        m_working = preset;
        m_canvas->setCurve(m_working);
        m_canvas->setSelectedKey(AZStd::nullopt);
        refreshInspector();
        pushUndoSnapshot();
    }

    void CurveEditorDialog::addPresetButton(const AZStd::string& name, const CurveData& curve, bool removable)
    {
        if (!m_presetLayout)
        {
            return;
        }
        auto* button = new CurvePresetButton(curve, QString::fromUtf8(name.c_str()), this);
        button->setRemovable(removable);
        connect(button, &CurvePresetButton::clicked, this, [this](const CurveData& c) { applyPreset(c); });
        if (removable)
        {
            connect(button, &CurvePresetButton::removeRequested, this,
                [this, button, name]() { removeCustomPreset(name, button); });
        }
        // Presets occupy the first m_presetCount slots, ahead of the trailing
        // stretch and the Save button.
        m_presetLayout->insertWidget(m_presetCount, button);
        ++m_presetCount;
    }

    void CurveEditorDialog::loadCustomPresets()
    {
        auto* registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            return;
        }
        AZStd::vector<AZStd::string> names;
        if (!registry->GetObject(names, kPresetNamesKey))
        {
            return;
        }
        for (const AZStd::string& name : names)
        {
            CurveData curve;
            if (registry->GetObject(curve, PresetKey(name)))
            {
                addPresetButton(name, curve, true);
            }
        }
    }

    void CurveEditorDialog::saveCurrentAsPreset()
    {
        bool ok = false;
        const QString entered = QInputDialog::getText(
            this, tr("Save Curve Preset"), tr("Preset name:"), QLineEdit::Normal, QString(), &ok);
        if (!ok)
        {
            return;
        }
        const AZStd::string name = SanitizePresetName(AZStd::string(entered.trimmed().toUtf8().constData()));
        if (name.empty())
        {
            return;
        }

        auto* registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            return;
        }

        AZStd::vector<AZStd::string> names;
        registry->GetObject(names, kPresetNamesKey);
        if (AZStd::find(names.begin(), names.end(), name) == names.end())
        {
            names.push_back(name);
            addPresetButton(name, m_working, true); // new name -> add a thumbnail this session
        }
        registry->SetObject(kPresetNamesKey, names);
        registry->SetObject(PresetKey(name), m_working);
        persistPresets();
    }

    void CurveEditorDialog::persistPresets()
    {
        auto* registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            return;
        }
        const AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
        if (projectPath.empty())
        {
            return;
        }
        const AZStd::string filePath = AZStd::string(projectPath.c_str()) + "/Registry/CurveEditorPresets.setreg";

        // Dump the preset subtree to a string, then write it to the project
        // registry file.
        AZStd::string output;
        AZ::IO::ByteContainerStream<AZStd::string> outputStream(&output);
        AZ::SettingsRegistryMergeUtils::DumperSettings dumper;
        dumper.m_prettifyOutput = true;
        dumper.m_jsonPointerPrefix = kPresetRoot;
        if (AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*registry, kPresetRoot, outputStream, dumper))
        {
            const auto writeResult = AZ::Utils::WriteFile(output, filePath);
            AZ_Warning("CurveEditor", writeResult.IsSuccess(), "Failed to persist curve presets to %s", filePath.c_str());
        }
    }

    void CurveEditorDialog::removeCustomPreset(const AZStd::string& name, QWidget* button)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            AZStd::vector<AZStd::string> names;
            registry->GetObject(names, kPresetNamesKey);
            names.erase(AZStd::remove(names.begin(), names.end(), name), names.end());
            registry->SetObject(kPresetNamesKey, names);
            registry->Remove(PresetKey(name));
            persistPresets();
        }
        if (button && m_presetLayout)
        {
            m_presetLayout->removeWidget(button);
            button->deleteLater();
            if (m_presetCount > 0)
            {
                --m_presetCount;
            }
        }
    }

    void CurveEditorDialog::onCanvasChanged()
    {
        m_working = m_canvas->curve();
    }

    void CurveEditorDialog::onCanvasSelectionChanged()
    {
        refreshInspector();
    }

    void CurveEditorDialog::onInspectorTimeChanged(double value)
    {
        m_canvas->setSelectedKeyTime(static_cast<float>(value));
    }

    void CurveEditorDialog::onInspectorValueChanged(double value)
    {
        m_canvas->setSelectedKeyValue(static_cast<float>(value));
    }

    void CurveEditorDialog::onInspectorTangentChanged()
    {
        if (!m_canvas->selectedKey().has_value())
        {
            return;
        }
        m_canvas->setSelectedKeyTangents(
            IndexToTangentMode(m_inModeCombo->currentIndex()),
            IndexToTangentMode(m_outModeCombo->currentIndex()),
            m_brokenCheck->isChecked());
        pushUndoSnapshot();
    }

    void CurveEditorDialog::refreshInspector()
    {
        const AZStd::optional<int64_t> selected = m_canvas->selectedKey();
        const bool hasSelection = selected.has_value();

        m_timeSpin->setEnabled(hasSelection);
        m_valueSpin->setEnabled(hasSelection);
        m_inModeCombo->setEnabled(hasSelection);
        m_outModeCombo->setEnabled(hasSelection);
        m_brokenCheck->setEnabled(hasSelection);

        if (!hasSelection)
        {
            m_selectionLabel->setText(QStringLiteral("(no selection)"));
            return;
        }

        const CurveData::Point point = m_canvas->curve().GetPoint(*selected);
        m_selectionLabel->setText(QStringLiteral("Key %1").arg(static_cast<int>(*selected)));

        const QSignalBlocker blockTime(m_timeSpin);
        const QSignalBlocker blockValue(m_valueSpin);
        const QSignalBlocker blockIn(m_inModeCombo);
        const QSignalBlocker blockOut(m_outModeCombo);
        const QSignalBlocker blockBroken(m_brokenCheck);

        m_timeSpin->setValue(static_cast<double>(point.m_time));
        m_valueSpin->setValue(static_cast<double>(point.m_value));
        m_inModeCombo->setCurrentIndex(TangentModeToIndex(point.m_inMode));
        m_outModeCombo->setCurrentIndex(TangentModeToIndex(point.m_outMode));
        m_brokenCheck->setChecked(point.m_broken);
    }

    // ---- In-dialog undo stack ----

    CurveEditorDialog::UndoSnapshot CurveEditorDialog::captureSnapshot() const
    {
        UndoSnapshot snap;
        snap.m_curve = m_canvas->curve();
        snap.m_selected = m_canvas->selectedKey();
        return snap;
    }

    void CurveEditorDialog::pushUndoSnapshot()
    {
        UndoSnapshot snap = captureSnapshot();

        // Skip a no-op push (same number of points and identical key data).
        if (!m_undoStack.empty())
        {
            const CurveData& top = m_undoStack[m_undoCursor].m_curve;
            if (top.GetNumPoints() == snap.m_curve.GetNumPoints())
            {
                bool identical = true;
                for (int64_t i = 0; i < top.GetNumPoints(); ++i)
                {
                    const CurveData::Point a = top.GetPoint(i);
                    const CurveData::Point b = snap.m_curve.GetPoint(i);
                    if (a.m_time != b.m_time || a.m_value != b.m_value || a.m_inTangent != b.m_inTangent ||
                        a.m_outTangent != b.m_outTangent || a.m_inWeight != b.m_inWeight || a.m_outWeight != b.m_outWeight ||
                        a.m_inMode != b.m_inMode || a.m_outMode != b.m_outMode || a.m_broken != b.m_broken)
                    {
                        identical = false;
                        break;
                    }
                }
                if (identical)
                {
                    return;
                }
            }
        }

        if (m_undoCursor + 1 < m_undoStack.size())
        {
            m_undoStack.erase(m_undoStack.begin() + m_undoCursor + 1, m_undoStack.end());
        }
        m_undoStack.push_back(AZStd::move(snap));
        m_undoCursor = m_undoStack.size() - 1;
    }

    void CurveEditorDialog::undo()
    {
        if (m_undoCursor == 0)
        {
            return;
        }
        --m_undoCursor;
        applyUndoSnapshot(m_undoStack[m_undoCursor]);
    }

    void CurveEditorDialog::redo()
    {
        if (m_undoCursor + 1 >= m_undoStack.size())
        {
            return;
        }
        ++m_undoCursor;
        applyUndoSnapshot(m_undoStack[m_undoCursor]);
    }

    void CurveEditorDialog::applyUndoSnapshot(const UndoSnapshot& snapshot)
    {
        m_working = snapshot.m_curve;
        m_canvas->setCurve(m_working);
        m_canvas->setSelectedKey(snapshot.m_selected);
        refreshInspector();
    }

    // ---- Commit-on-close / focus handling ----

    void CurveEditorDialog::showEvent(QShowEvent* event)
    {
        QDialog::showEvent(event);
        qApp->installEventFilter(this);
    }

    void CurveEditorDialog::hideEvent(QHideEvent* event)
    {
        qApp->removeEventFilter(this);
        QDialog::hideEvent(event);
    }

    bool CurveEditorDialog::eventFilter(QObject* watched, QEvent* event)
    {
        QWidget* modal = QApplication::activeModalWidget();
        if (modal && modal != this)
        {
            return QDialog::eventFilter(watched, event);
        }

        auto isDescendant = [this](QWidget* w)
        {
            for (QWidget* p = w; p; p = p->parentWidget())
            {
                if (p == this)
                {
                    return true;
                }
            }
            return false;
        };

        if (event->type() == QEvent::MouseButtonPress)
        {
            if (auto* w = qobject_cast<QWidget*>(watched); w && !isDescendant(w))
            {
                // A click anywhere outside the dialog commits the edit.
                accept();
                return true;
            }
        }
        else if (event->type() == QEvent::KeyPress)
        {
            auto* ke = static_cast<QKeyEvent*>(event);
            const bool ctrl = (ke->modifiers() & Qt::ControlModifier) != 0;
            const bool shift = (ke->modifiers() & Qt::ShiftModifier) != 0;
            if (auto* w = qobject_cast<QWidget*>(watched); w && isDescendant(w))
            {
                const bool isUndo = ctrl && !shift && ke->key() == Qt::Key_Z;
                const bool isRedo = (ctrl && ke->key() == Qt::Key_Y) || (ctrl && shift && ke->key() == Qt::Key_Z);
                if (isUndo || isRedo)
                {
                    // Force a pending spin-box edit to commit before stepping.
                    if (auto* spin = qobject_cast<QAbstractSpinBox*>(QApplication::focusWidget()))
                    {
                        spin->clearFocus();
                    }
                    if (isUndo)
                    {
                        undo();
                    }
                    else
                    {
                        redo();
                    }
                    return true;
                }
            }
        }

        return QDialog::eventFilter(watched, event);
    }

    void CurveEditorDialog::keyPressEvent(QKeyEvent* event)
    {
        // Enter inside a spin commits the field rather than closing the dialog.
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        {
            QWidget* focused = focusWidget();
            QAbstractSpinBox* spin = qobject_cast<QAbstractSpinBox*>(focused);
            if (!spin)
            {
                for (QWidget* p = focused; p; p = p->parentWidget())
                {
                    if (auto* s = qobject_cast<QAbstractSpinBox*>(p))
                    {
                        spin = s;
                        break;
                    }
                }
            }
            if (spin == m_timeSpin || spin == m_valueSpin)
            {
                spin->interpretText();
                spin->clearFocus();
                setFocus(Qt::OtherFocusReason);
                pushUndoSnapshot();
                event->accept();
                return;
            }
            accept();
            return;
        }

        QDialog::keyPressEvent(event);
    }

    // =================================================================
    // Clipboard Serialization (file-local)
    // =================================================================

    namespace
    {
        static constexpr const char* kCurveClipboardHeader = "AZ_CURVE_DATA v1";

        QString SerializeCurve(const CurveData& curve)
        {
            QString out(kCurveClipboardHeader);
            out.append(QLatin1Char('\n'));
            for (int64_t i = 0; i < curve.GetNumPoints(); ++i)
            {
                const CurveData::Point p = curve.GetPoint(i);
                out.append(QStringLiteral("P %1 %2 %3 %4 %5 %6 %7 %8 %9\n")
                    .arg(static_cast<qreal>(p.m_time), 0, 'g', 7)
                    .arg(static_cast<qreal>(p.m_value), 0, 'g', 7)
                    .arg(static_cast<qreal>(p.m_inTangent), 0, 'g', 7)
                    .arg(static_cast<qreal>(p.m_outTangent), 0, 'g', 7)
                    .arg(static_cast<qreal>(p.m_inWeight), 0, 'g', 7)
                    .arg(static_cast<qreal>(p.m_outWeight), 0, 'g', 7)
                    .arg(static_cast<int>(p.m_inMode))
                    .arg(static_cast<int>(p.m_outMode))
                    .arg(p.m_broken ? 1 : 0));
            }
            return out;
        }

        bool DeserializeCurve(const QString& text, CurveData& out)
        {
            const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            if (lines.isEmpty() || !lines.front().startsWith(QLatin1String(kCurveClipboardHeader)))
            {
                return false;
            }

            CurveData parsed;
            for (int i = 1; i < lines.size(); ++i)
            {
                const QStringList tokens = lines[i].split(QLatin1Char(' '), Qt::SkipEmptyParts);
                if (tokens.isEmpty() || tokens[0] != QLatin1String("P") || tokens.size() != 10)
                {
                    continue;
                }
                CurveData::Point p;
                p.m_time = tokens[1].toFloat();
                p.m_value = tokens[2].toFloat();
                p.m_inTangent = tokens[3].toFloat();
                p.m_outTangent = tokens[4].toFloat();
                p.m_inWeight = tokens[5].toFloat();
                p.m_outWeight = tokens[6].toFloat();
                p.m_inMode = IndexToTangentMode(tokens[7].toInt());
                p.m_outMode = IndexToTangentMode(tokens[8].toInt());
                p.m_broken = tokens[9].toInt() != 0;
                parsed.AddPoint(p);
            }
            if (parsed.GetNumPoints() == 0)
            {
                return false;
            }
            out = parsed;
            return true;
        }

        bool CurvesEqual(const CurveData& a, const CurveData& b)
        {
            if (a.GetNumPoints() != b.GetNumPoints())
            {
                return false;
            }
            for (int64_t i = 0; i < a.GetNumPoints(); ++i)
            {
                const CurveData::Point pa = a.GetPoint(i);
                const CurveData::Point pb = b.GetPoint(i);
                if (pa.m_time != pb.m_time || pa.m_value != pb.m_value)
                {
                    return false;
                }
            }
            return true;
        }
    } // namespace

    // =================================================================
    // PropertyCurveCtrl (inline preview)
    // =================================================================

    PropertyCurveCtrl::PropertyCurveCtrl(QWidget* parent)
        : QWidget(parent)
    {
        setMinimumHeight(CurveCtrlConstants::kInlineHeight);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
        setCursor(Qt::PointingHandCursor);
        setToolTip(QStringLiteral("Click to edit curve | Right-click for copy / paste"));
        m_value.SetDefaultValue();
    }

    void PropertyCurveCtrl::setValue(const CurveData& value)
    {
        m_value = value;
        if (m_value.GetNumPoints() == 0)
        {
            m_value.SetDefaultValue();
        }
        update();
    }

    void PropertyCurveCtrl::beginReadPass(const CurveData& firstInstance)
    {
        m_multiEdit = false;
        m_mixed = false;
        m_value = firstInstance;
        if (m_value.GetNumPoints() == 0)
        {
            m_value.SetDefaultValue();
        }
        update();
    }

    void PropertyCurveCtrl::addReadInstance(const CurveData& instance)
    {
        if (!CurvesEqual(instance, m_value))
        {
            m_multiEdit = true;
            m_mixed = true;
            update();
        }
    }

    void PropertyCurveCtrl::paintEvent(QPaintEvent* /*event*/)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(40, 40, 40));

        if (m_mixed)
        {
            painter.setPen(QPen(QColor(120, 120, 120), 1));
            painter.drawLine(rect().topLeft(), rect().bottomRight());
            painter.drawLine(rect().topRight(), rect().bottomLeft());
            painter.drawRect(rect().adjusted(0, 0, -1, -1));
            return;
        }

        const QRect area = rect().adjusted(2, 2, -2, -2);

        float valueMin = 0.0f;
        float valueMax = 1.0f;
        ComputeValueRange(m_value, m_value.GetMinTime(), m_value.GetMaxTime(), valueMin, valueMax);

        // Unlike the editor canvas (which auto-fits while editing), the inspector
        // preview reports TRUE scale by always anchoring to the nominal [0,1]
        // band. A curve that stays in range fills the box; an excessive curve
        // compresses the [0,1] reference band toward a sliver as its own scale
        // takes over, so the preview visibly signals "this curve is out of
        // proportion" instead of normalizing it to look ordinary.
        valueMin = AZStd::min(valueMin, 0.0f);
        valueMax = AZStd::max(valueMax, 1.0f);

        const float timeMax = (m_value.GetMaxTime() > m_value.GetMinTime()) ? m_value.GetMaxTime() : m_value.GetMinTime() + 1.0f;

        // Faint reference lines mark the normal band (value 0 and value 1); when
        // the curve is excessive these two lines crowd together, showing how
        // little of the preview the "normal" range now occupies.
        const float valueSpan = (valueMax > valueMin) ? (valueMax - valueMin) : 1.0f;
        auto valueToY = [&](float v) { return static_cast<double>(area.bottom()) - ((v - valueMin) / valueSpan) * area.height(); };
        painter.setPen(QPen(QColor(70, 70, 70), 1, Qt::DotLine));
        const double yZero = valueToY(0.0f);
        const double yOne = valueToY(1.0f);
        painter.drawLine(QPointF(area.left(), yZero), QPointF(area.right(), yZero));
        painter.drawLine(QPointF(area.left(), yOne), QPointF(area.right(), yOne));

        PaintCurveInRect(painter, m_value, area, m_value.GetMinTime(), timeMax, valueMin, valueMax, QColor(240, 120, 80));

        painter.setPen(QColor(30, 30, 30));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }

    void PropertyCurveCtrl::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            openEditorDialog();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void PropertyCurveCtrl::openEditorDialog()
    {
        CurveEditorDialog dialog(m_value, m_multiEdit, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            m_value = dialog.result();
            update();
            emit valueChanged();
            emit editingFinished();
        }
    }

    void PropertyCurveCtrl::contextMenuEvent(QContextMenuEvent* event)
    {
        QMenu menu(this);
        QAction* copyAction = menu.addAction(QStringLiteral("Copy Curve"));
        QAction* pasteAction = menu.addAction(QStringLiteral("Paste Curve"));

        CurveData preview;
        pasteAction->setEnabled(DeserializeCurve(QApplication::clipboard()->text(), preview));

        QAction* chosen = menu.exec(event->globalPos());
        if (chosen == copyAction)
        {
            copyToClipboard();
        }
        else if (chosen == pasteAction)
        {
            pasteFromClipboard();
        }
        event->accept();
    }

    void PropertyCurveCtrl::copyToClipboard()
    {
        QApplication::clipboard()->setText(SerializeCurve(m_value));
    }

    void PropertyCurveCtrl::pasteFromClipboard()
    {
        CurveData parsed;
        if (DeserializeCurve(QApplication::clipboard()->text(), parsed))
        {
            m_value = parsed;
            update();
            emit valueChanged();
            emit editingFinished();
        }
    }

    // =================================================================
    // Handler
    // =================================================================

    QWidget* PropertyCurveEditHandler::CreateGUI(QWidget* parent)
    {
        PropertyCurveCtrl* ctrl = aznew PropertyCurveCtrl(parent);
        connect(ctrl, &PropertyCurveCtrl::valueChanged, this, [ctrl]()
        {
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::RequestWrite, ctrl);
        });
        connect(ctrl, &PropertyCurveCtrl::editingFinished, this, [ctrl]()
        {
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::OnEditingFinished, ctrl);
        });
        return ctrl;
    }

    void PropertyCurveEditHandler::ConsumeAttribute(PropertyCurveCtrl* /*GUI*/, AZ::u32 /*attrib*/, PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
    {
    }

    void PropertyCurveEditHandler::WriteGUIValuesIntoProperty(size_t /*index*/, PropertyCurveCtrl* GUI, property_t& instance, InstanceDataNode* /*node*/)
    {
        instance = GUI->value();
    }

    bool PropertyCurveEditHandler::ReadValuesIntoGUI(size_t index, PropertyCurveCtrl* GUI, const property_t& instance, InstanceDataNode* /*node*/)
    {
        if (index == 0)
        {
            GUI->beginReadPass(instance);
        }
        else
        {
            GUI->addReadInstance(instance);
        }
        return false;
    }

    void RegisterCurveEditHandler()
    {
        PropertyTypeRegistrationMessageBus::Broadcast(
            &PropertyTypeRegistrationMessages::RegisterPropertyType, aznew PropertyCurveEditHandler());
    }
} // namespace AzToolsFramework

#include "UI/PropertyEditor/moc_PropertyCurveCtrl.cpp"
