/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QPoint>
#include <QPointF>
#include <QPainterPath>
#include <QRect>
#include <QPainter>
#include <QHBoxLayout>

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <Document/ParticleDocumentBus.h>
#endif

namespace OpenParticleSystemEditor
{
    class PresetCurveWidget : public QWidget
    {
        Q_OBJECT
    public:
        PresetCurveWidget(QWidget* parent = nullptr);
        ~PresetCurveWidget() override;

        virtual void paintEvent(QPaintEvent* event) override;
        virtual void mousePressEvent(QMouseEvent* event) override;
        virtual void mouseReleaseEvent(QMouseEvent* event) override;

        void SetKeyPoint(
            float startTime,
            float startValue,
            const OpenParticle::KeyPointInterpMode& startMode,
            float stopTime,
            float stopValue,
            const OpenParticle::KeyPointInterpMode& stopMode);
        OpenParticle::KeyPoint m_start;
        OpenParticle::KeyPoint m_stop;

    Q_SIGNALS:
        void OnClicked();

    protected:
        bool m_pressed;
    };

    class CurveEditor : public QWidget
    {
        Q_OBJECT
    public:
        static constexpr float EPSILON = 0.01f;
        explicit CurveEditor(AZStd::string busIDName, QWidget* parent = nullptr);
        ~CurveEditor() override;

        virtual void paintEvent(QPaintEvent* event) override;
        virtual void mouseMoveEvent(QMouseEvent* event) override;
        virtual void mousePressEvent(QMouseEvent* event) override;
        virtual void mouseReleaseEvent(QMouseEvent* event) override;
        virtual void resizeEvent(QResizeEvent* event) override;

        void SetCurrentCurve(int index);
        void UpdateKeyInterpMode();
        void DeleteKey();
        float GetValueFactor() const;
        int GetTickMode() const;
        void SetValueFactor(float value);
        void SetTickMode(const OpenParticle::CurveTickMode& mode);
        float GetTimeFactor() const;
        void SetTimeFactor(float value);
        float GetCurrentKeyPointTime();
        float GetCurrentKeyPointValue();
        void SimulateLeftButtonPressDragRelease(float oldX, float oldY, float newX, float newY, bool needMove = true);
        AZ::TypeId m_moduleId;
        AZ::TypeId m_paramId;
        AZStd::string m_busIDName;

    signals:
        void mouseRelease();
        void afterCurveSet();

    private:
        void SetupUi();
        QPointF TransformPointFromScreen(float x, float y) const;
        QRect CenterPointToRect(QPoint pt) const;
        void UpdateCurveKey(QPointF pos);
        void UpdateMenu(QPoint pt);
        void CheckMinMax(float& value, float min, float max) const;
        void AddKey(QPoint pos);

        void DrawGrid(QPainter& painter) const;

        bool m_buttonPressed = false;
        bool m_buttonDragged = false;

        int m_currentKeyIndex = 0;
        float m_currentKeyPointTime = 0.f;
        float m_currentKeyPointValue = 0.f;
        OpenParticle::Distribution* m_distribution = nullptr;
        OpenParticle::Curve* m_currentCurve = nullptr;

        QRect m_mainRect;
        const int m_scaleWidth = 20;
        const int m_scaleHeight = 16;
        const int m_widgetWidth = 20;

        QHBoxLayout m_layout;
        AZStd::vector<PresetCurveWidget*> m_presetCurveWidget;

        OpenParticle::ParticleSourceData* m_sourceData = nullptr;
    };
} // namespace OpenParticleSystemEditor
