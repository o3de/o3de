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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/CurveData.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QDialog>
#include <QRect>
#include <QWidget>
#endif

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QHBoxLayout;
class QLabel;

namespace AzToolsFramework
{
    // =====================================================================
    // CurvePresetButton
    // =====================================================================

    //! A small clickable thumbnail that paints a fixed preset curve. Clicking it
    //! replaces the editor's working curve with the preset.
    class AZTF_API CurvePresetButton
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit CurvePresetButton(const AZ::CurveData& curve, const QString& tooltip, QWidget* parent = nullptr);
        ~CurvePresetButton() override = default;

        const AZ::CurveData& curve() const { return m_curve; }
        void setRemovable(bool removable) { m_removable = removable; }

    signals:
        void clicked(const AZ::CurveData& curve);
        void removeRequested(); //!< emitted from the right-click menu of a removable (custom) preset

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;

    private:
        AZ::CurveData m_curve;
        bool m_pressed = false;
        bool m_removable = false;
    };

    // =====================================================================
    // CurveCanvas
    // =====================================================================

    //! The interactive plotting area: draws the curve and its keys, and handles
    //! key selection, dragging, adding and deleting. The view is auto-fitted to
    //! the curve in this version; manual zoom/pan is layered on later.
    class AZTF_API CurveCanvas
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit CurveCanvas(QWidget* parent = nullptr);
        ~CurveCanvas() override = default;

        const AZ::CurveData& curve() const { return m_curve; }
        void setCurve(const AZ::CurveData& curve);

        AZStd::optional<int64_t> selectedKey() const { return m_selected; }
        void setSelectedKey(AZStd::optional<int64_t> index);

        // Inspector-driven edits to the selected key.
        void setSelectedKeyTime(float time);
        void setSelectedKeyValue(float value);
        void setSelectedKeyTangents(
            AZ::CurveData::TangentMode inMode,
            AZ::CurveData::TangentMode outMode,
            bool broken);
        void deleteSelectedKey();

    signals:
        void curveChanged();     //!< Live change (repaint / preview).
        void selectionChanged(); //!< Selected key changed.
        void editCommitted();    //!< A discrete edit finished (undo commit point).

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;

    private:
        //! What the current left/middle drag is manipulating.
        enum class Drag { None, Key, InHandle, OutHandle, Pan, Marquee, Move, Scale };

        void recomputePlotRect();
        void fitViewToCurve();
        QPointF dataToScreen(float time, float value) const;
        QPointF screenToData(const QPointF& screen) const;
        AZStd::optional<int64_t> hitTestKey(const QPoint& screen) const;
        Drag hitTestHandle(const QPoint& screen) const;
        float outArmTimeSpan(int64_t index) const;
        float inArmTimeSpan(int64_t index) const;
        QPointF outHandleScreen(int64_t index) const;
        QPointF inHandleScreen(int64_t index) const;
        void dragHandle(int64_t index, bool inArm, const QPointF& cursorData);
        void addKeyAt(const QPoint& screen);

        // Group move / scale.
        bool selectionBounds(float& t0, float& t1, float& v0, float& v1) const;
        int  hitTestScaleHandle(const QPoint& screen) const; //!< returns corner 0..3 (TL,TR,BR,BL) or -1
        void beginGroupOp();
        void groupMove(const QPointF& deltaData);
        void groupScale(float scaleX, float scaleY, const QPointF& pivot);
        void commitSelectedPositions(const AZStd::vector<QPointF>& newPositions);

        AZ::CurveData m_curve;
        AZStd::optional<int64_t> m_selected;       //!< primary key (drives the inspector + tangent handles).
        AZStd::vector<int64_t> m_selection;        //!< highlighted group (ascending); includes the primary.
        Drag m_drag = Drag::None;
        bool m_userAdjustedView = false; //!< once true, setCurve keeps the user's zoom/pan instead of auto-fitting.
        QPoint m_lastPanPos;
        QPoint m_marqueeStart;
        QRect m_marqueeRect;

        // Snapshot captured when a group move / scale begins.
        AZStd::vector<AZ::CurveData::Point> m_opSnapshot;
        AZStd::vector<int64_t> m_opSelected;
        QPointF m_moveStartData;
        int m_scaleCorner = -1;
        float m_opT0 = 0.0f;
        float m_opT1 = 0.0f;
        float m_opV0 = 0.0f;
        float m_opV1 = 0.0f;

        // View bounds in data space (auto-fitted to the curve for now).
        float m_timeMin = 0.0f;
        float m_timeMax = 1.0f;
        float m_valueMin = 0.0f;
        float m_valueMax = 1.0f;

        QRect m_plotRect; //!< Drawable area inside the axis margins.
    };

    // =====================================================================
    // CurveEditorDialog
    // =====================================================================

    //! Modal popout that owns a working copy of the curve, a preset palette, the
    //! canvas, a selected-key inspector and a self-contained undo stack. The
    //! outer property editor receives the result only on accept.
    class AZTF_API CurveEditorDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        CurveEditorDialog(const AZ::CurveData& initial, bool multiEdit, QWidget* parent = nullptr);
        ~CurveEditorDialog() override = default;

        const AZ::CurveData& result() const { return m_working; }

    protected:
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        void buildPresetPalette(QLayout* parent);
        void addPresetButton(const AZStd::string& name, const AZ::CurveData& curve, bool removable);
        void loadCustomPresets();   //!< loads project-unique presets from the Settings Registry
        void saveCurrentAsPreset(); //!< prompts for a name and saves the working curve as a project preset
        void removeCustomPreset(const AZStd::string& name, QWidget* button); //!< deletes a custom preset + its button
        void persistPresets();      //!< writes the preset registry subtree to <project>/Registry/*.setreg
        void refreshInspector();
        void applyPreset(const AZ::CurveData& preset);

        // Inspector slots.
        void onCanvasChanged();
        void onCanvasSelectionChanged();
        void onInspectorTimeChanged(double value);
        void onInspectorValueChanged(double value);
        void onInspectorTangentChanged();

        // In-dialog undo stack.
        struct UndoSnapshot
        {
            AZ::CurveData m_curve;
            AZStd::optional<int64_t> m_selected;
        };
        UndoSnapshot captureSnapshot() const;
        void pushUndoSnapshot();
        void undo();
        void redo();
        void applyUndoSnapshot(const UndoSnapshot& snapshot);

        AZ::CurveData m_working;

        CurveCanvas* m_canvas = nullptr;
        QHBoxLayout* m_presetLayout = nullptr; //!< row that holds the preset thumbnails
        int m_presetCount = 0;                 //!< number of preset buttons (kept ahead of the trailing stretch)
        QLabel* m_selectionLabel = nullptr;
        QDoubleSpinBox* m_timeSpin = nullptr;
        QDoubleSpinBox* m_valueSpin = nullptr;
        QComboBox* m_inModeCombo = nullptr;
        QComboBox* m_outModeCombo = nullptr;
        QCheckBox* m_brokenCheck = nullptr;

        AZStd::vector<UndoSnapshot> m_undoStack;
        size_t m_undoCursor = 0;
    };

    // =====================================================================
    // PropertyCurveCtrl (inline preview)
    // =====================================================================

    //! Inline property widget: paints a small preview of the curve and opens the
    //! popout editor when clicked. Right-click offers copy / paste.
    class AZTF_API PropertyCurveCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyCurveCtrl, AZ::SystemAllocator);
        explicit PropertyCurveCtrl(QWidget* parent = nullptr);
        ~PropertyCurveCtrl() override = default;

        AZ::CurveData value() const { return m_value; }
        void setValue(const AZ::CurveData& value);

        // Multi-edit read aggregation across a selection of entities.
        void beginReadPass(const AZ::CurveData& firstInstance);
        void addReadInstance(const AZ::CurveData& instance);

    signals:
        void valueChanged();
        void editingFinished();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;

    private:
        void openEditorDialog();
        void copyToClipboard();
        void pasteFromClipboard();

        AZ::CurveData m_value;
        bool m_multiEdit = false;
        bool m_mixed = false;
    };

    // =====================================================================
    // Handler
    // =====================================================================

    class AZTF_API PropertyCurveEditHandler
        : QObject
        , public PropertyHandler<AZ::CurveData, PropertyCurveCtrl>
    {
        // QObject purely so it can connect to slots with context.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyCurveEditHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override { return AZ::Edit::UIHandlers::Curve; }
        bool IsDefaultHandler() const override { return true; }

        QWidget* CreateGUI(QWidget* parent) override;
        void ConsumeAttribute(PropertyCurveCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyCurveCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyCurveCtrl* GUI, const property_t& instance, InstanceDataNode* node) override;
    };

    AZTF_API void RegisterCurveEditHandler();
}
