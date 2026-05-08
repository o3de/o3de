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
#include <AzCore/Math/Color.h>
#include <AzCore/Math/ColorGradient.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QDialog>
#include <QWidget>
AZ_POP_DISABLE_WARNING
#endif

class QDoubleSpinBox;
class QPushButton;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;

namespace AzToolsFramework
{
    class PropertyColorCtrl;
    // =======================================================================
    // GradientBarWidget
    // =======================================================================
    // Paints a live RGBA gradient preview over a checkerboard backdrop.
    // Read-only: does not handle input.

    class AZTF_API GradientBarWidget : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GradientBarWidget, AZ::SystemAllocator);

        explicit GradientBarWidget(QWidget* parent = nullptr);

        void setGradient(const AZ::ColorGradient& gradient);
        void setAlphaEnabled(bool enabled);
        void setMixed(bool mixed);

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        AZ::ColorGradient m_gradient;
        bool              m_alphaEnabled { true };
        bool              m_mixed { false };
    };

    // =======================================================================
    // GradientStopsTrack
    // =======================================================================
    // One stops track (color or alpha). Hit-test, drag, add-on-empty-click,
    // highlight selection, Del to remove.

    class AZTF_API GradientStopsTrack : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GradientStopsTrack, AZ::SystemAllocator);

        enum class Kind { Color, Alpha };

        GradientStopsTrack(Kind kind, QWidget* parent = nullptr);

        void setColorStops(const AZStd::vector<AZ::ColorGradientMarker>& stops);
        void setAlphaStops(const AZStd::vector<AZ::AlphaGradientMarker>& stops);

        const AZStd::vector<AZ::ColorGradientMarker>& colorStops() const { return m_colorStops; }
        const AZStd::vector<AZ::AlphaGradientMarker>& alphaStops() const { return m_alphaStops; }

        AZStd::optional<size_t> selectedIndex() const { return m_selected; }
        void setSelectedIndex(AZStd::optional<size_t> index);

        //! Samples the provided tracks to get the blended value at normalized t.
        AZ::Color sampleColorAt(float t) const;
        float     sampleAlphaAt(float t) const;

        //! Track arrow triangles point toward the gradient bar.
        Kind kind() const { return m_kind; }

        //! Duplicate the currently selected stop at a position offset, clamped.
        void duplicateSelected();
        //! Remove the selected stop if safe (respects minimum 1 stop).
        void removeSelected();

        //! Commit the position/value of the currently selected stop.
        void setSelectedPosition(float t);
        void setSelectedColor(const AZ::Color& rgb);
        void setSelectedAlpha(float alpha);

    signals:
        void selectionChanged();
        //! Live value change; fires during drags and field edits, used for preview repaint.
        void stopsChanged();
        //! Discrete commit point; fires after drag release, add, remove, duplicate, and color pick.
        //! Used by the dialog to push an undo snapshot.
        void editCommitted();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        QRect triangleRect(float position) const;
        int   positionToPixel(float position) const;
        float pixelToPosition(int pixel) const;
        AZStd::optional<size_t> hitTest(const QPoint& point) const;
        void  addStopAt(float t);
        size_t stopCount() const;
        float stopPositionAt(size_t index) const;

        Kind m_kind;
        AZStd::vector<AZ::ColorGradientMarker> m_colorStops;
        AZStd::vector<AZ::AlphaGradientMarker> m_alphaStops;
        AZStd::optional<size_t> m_selected;
        bool m_dragging { false };
        bool m_pendingDelete { false };
    };

    // =======================================================================
    // ColorGradientEditorDialog
    // =======================================================================
    // Popup dialog that assembles the preview bar, alpha track (top), color
    // track (bottom), and the selected-stop inspector. Commits on OK.

    class AZTF_API ColorGradientEditorDialog : public QDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ColorGradientEditorDialog, AZ::SystemAllocator);

        ColorGradientEditorDialog(const AZ::ColorGradient& initial, bool alphaEnabled, bool multiEdit, QWidget* parent = nullptr);

        const AZ::ColorGradient& result() const { return m_working; }

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;

    public Q_SLOTS:
        // Override to intercept the "cancel" action regardless of trigger
        // path (Esc keypress, Esc shortcut on Cancel button, X close, etc.).
        // When a spin or line-edit field inside the dialog has focus, revert
        // its typed value and release focus instead of closing the dialog.
        void reject() override;

    private slots:
        void onAlphaTrackChanged();
        void onColorTrackChanged();
        void onSelectionChanged();
        void onPositionChanged(double normalized);
        void onColorFieldLiveChanged();
        void onColorFieldCommitted();
        void onColorFieldRejected();
        void onAlphaSpinChanged(double value);

    private:
        // In-dialog granular undo snapshot. Stores track vectors directly so
        // undo preserves exact insertion order and selection index.
        struct UndoSnapshot
        {
            AZStd::vector<AZ::ColorGradientMarker> colorStops;
            AZStd::vector<AZ::AlphaGradientMarker> alphaStops;
            GradientStopsTrack::Kind selectedKind { GradientStopsTrack::Kind::Color };
            AZStd::optional<size_t>  selectedIndex;
        };

        void rebuildFromTracks();
        void refreshInspector();
        void refreshPreview();

        // In-dialog granular undo. Snapshots are pushed at discrete commit
        // points (drag release, field edits, add, remove, duplicate, color
        // pick). On dialog accept the outer RPE sees a single Pre/Post write.
        void pushUndoSnapshot();
        void undo();
        void redo();
        void applyUndoSnapshot(const UndoSnapshot& snapshot);
        UndoSnapshot captureSnapshot() const;

        AZ::ColorGradient  m_working;
        bool               m_alphaEnabled { true };

        AZStd::vector<UndoSnapshot> m_undoStack;
        size_t m_undoCursor { 0 };

        GradientBarWidget* m_preview { nullptr };
        GradientStopsTrack* m_alphaTrack { nullptr };
        GradientStopsTrack* m_colorTrack { nullptr };

        // Inspector row
        QLabel*            m_valueLabel { nullptr };
        PropertyColorCtrl* m_colorField { nullptr };
        QDoubleSpinBox*    m_alphaSpin { nullptr };
        QDoubleSpinBox*    m_positionSpin { nullptr };

        // Tracks which track owns the selection (color or alpha)
        GradientStopsTrack* m_activeTrack { nullptr };
    };

    // =======================================================================
    // PropertyColorGradientCtrl
    // =======================================================================
    // Inline widget shown in the RPE row. A preview bar + "Edit..." button
    // that opens the editor dialog. Commits the new gradient when the dialog
    // closes with OK.

    class AZTF_API PropertyColorGradientCtrl : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyColorGradientCtrl, AZ::SystemAllocator);

        explicit PropertyColorGradientCtrl(QWidget* parent = nullptr);

        const AZ::ColorGradient& value() const { return m_value; }
        void setValue(const AZ::ColorGradient& v);

        void setAlphaEnabled(bool enabled);
        bool alphaEnabled() const { return m_alphaEnabled; }

        //! RPE multi-edit support. The first instance of a refresh pass calls
        //! beginReadPass() with its value; subsequent instances call
        //! addReadInstance() so we can detect divergence without overwriting.
        void beginReadPass(const AZ::ColorGradient& firstInstance);
        void addReadInstance(const AZ::ColorGradient& instance);
        bool isMultiEdit() const { return m_multiEdit; }

    signals:
        void valueChanged();
        void editingFinished();

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;

    private:
        void openEditorDialog();
        void copyToClipboard();
        void pasteFromClipboard();

        GradientBarWidget* m_preview { nullptr };
        AZ::ColorGradient  m_value;
        bool               m_alphaEnabled { true };
        bool               m_multiEdit { false };
    };

    // =======================================================================
    // Property Handlers
    // =======================================================================
    // Registered by PropertyManagerComponent so any reflected AZ::ColorGradient
    // or AZ::ColorGradientRGB field picks up the gradient editor automatically.

    class AZTF_API AZColorGradientPropertyHandler
        : public QObject
        , public PropertyHandler<AZ::ColorGradient, PropertyColorGradientCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AZColorGradientPropertyHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override { return AZ::Edit::UIHandlers::ColorGradient; }
        bool    IsDefaultHandler() const override { return true; }

        QWidget* CreateGUI(QWidget* parent) override;
        void ConsumeAttribute(PropertyColorGradientCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyColorGradientCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyColorGradientCtrl* GUI, const property_t& instance, InstanceDataNode* node) override;
    };

    class AZTF_API AZColorGradientRGBPropertyHandler
        : public QObject
        , public PropertyHandler<AZ::ColorGradientRGB, PropertyColorGradientCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AZColorGradientRGBPropertyHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override { return AZ::Edit::UIHandlers::ColorGradientRGB; }
        bool    IsDefaultHandler() const override { return true; }

        QWidget* CreateGUI(QWidget* parent) override;
        void ConsumeAttribute(PropertyColorGradientCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyColorGradientCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyColorGradientCtrl* GUI, const property_t& instance, InstanceDataNode* node) override;
    };

    AZTF_API void RegisterColorGradientPropertyHandlers();
}
