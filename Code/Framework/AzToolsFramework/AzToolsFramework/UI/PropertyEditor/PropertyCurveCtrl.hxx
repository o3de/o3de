/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Math/EasingCurve.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#include <QPaintEvent>
#include <QPoint>
#include <QPointF>
#include <QPainterPath>
#include <QRect>
#include <QPainter>
#include <QHBoxLayout>
#endif

namespace AzToolsFramework
{
    class AZTF_API PresetCurveWidget : public QWidget
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
            const AzFramework::EasingCurve::PointInterpolationMode& startMode,
            float stopTime,
            float stopValue,
            const AzFramework::EasingCurve::PointInterpolationMode& stopMode);
        AzFramework::EasingCurve::Point m_start;
        AzFramework::EasingCurve::Point m_stop;

    signals:
        void onClick();

    protected:
        bool m_pressed;
    };

    class AZTF_API CurveEditor : public QWidget
    {
        Q_OBJECT
    public:
        static constexpr float EPSILON = 0.01f;
        explicit CurveEditor(QWidget* parent = nullptr);
        ~CurveEditor() override;

        virtual void paintEvent(QPaintEvent* event) override;
        virtual void mouseMoveEvent(QMouseEvent* event) override;
        virtual void mousePressEvent(QMouseEvent* event) override;
        virtual void mouseReleaseEvent(QMouseEvent* event) override;
        virtual void resizeEvent(QResizeEvent* event) override;

        const AzFramework::EasingCurve& GetCurrentCurve() const;
        void SetCurrentCurve(const AzFramework::EasingCurve& curve);
        void UpdateKeyInterpMode();
        void DeleteKey();

    signals:
        void mouseRelease();
        void curveChanged();

    private:
        void SetupUi();
        QPointF TransformPointFromScreen(float x, float y) const;
        QRect CenterPointToRect(QPoint pt) const;
        void UpdateCurveKey(QPointF pos);
        void UpdateMenu(QPoint pt);
        void CheckMinMax(float& value, float min, float max) const;
        void AddKey(QPoint pos);

        void DrawGrid(QPainter& painter) const;

        bool m_keyPointSelected = false;
        bool m_keyPointDragged = false;

        int64_t m_currentKeyIndex = 0;
        AzFramework::EasingCurve m_currentCurve;

        QRect m_mainRect;
        const int m_scaleWidth = 20;
        const int m_scaleHeight = 16;
        const int m_widgetWidth = 20;

        QHBoxLayout m_layout;
        AZStd::vector<PresetCurveWidget*> m_presetCurveWidget;
    };

    class AZTF_API PropertyCurveEditHandler
        : QObject
        , public PropertyHandler<AzFramework::EasingCurve, CurveEditor>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyCurveEditHandler, AZ::SystemAllocator);

        virtual AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::Curve; }
        virtual bool IsDefaultHandler() const override { return true; }

        virtual QWidget* CreateGUI(QWidget* pParent) override;
        virtual void ConsumeAttribute(CurveEditor* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, CurveEditor* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, CurveEditor* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    AZTF_API void RegisterCurveEditHandler();
}
