/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QCursor>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPointer>
#endif

class QAction;
class QLineEdit;
class QMenu;
class QPainter;
class QProxyStyle;
class QSettings;
class QStyleOption;
class QStyleOptionComplex;

namespace AzQtComponents
{
    class SpinBoxWatcher;
    class Style;

    namespace internal
    {
        class SpinBoxLineEdit;
    }

    //! Control for integer number selection.
    //! Supports value editing via direct input, dragging, mouse scrollwheel and UI arrows.
    class AZ_QT_COMPONENTS_API SpinBox
        : public QSpinBox
    {
        Q_OBJECT

        //! Whether an undo value is available.
        Q_PROPERTY(bool undoAvailable READ isUndoAvailable)
        //! Whether a redo value is available.
        Q_PROPERTY(bool redoAvailable READ isRedoAvailable)
    public:
        static unsigned int s_watcherReferenceCount;

        //! Style configuration for the SpinBox class.
        struct Config
        {
            int pixelsPerStep;                  //!< The distance the cursor needs to be dragged to increase the value one step, in pixels.
            QCursor scrollCursor;               //!< Default mouse cursor used on hover in draggable areas. Must be an svg file.
            QCursor scrollCursorLeft;           //!< Mouse cursor used when dragging to the left. Must be an svg file.
            QCursor scrollCursorLeftMax;        //!< Mouse cursor used when dragging to the left and the value hit the minimum. Must be an svg file.
            QCursor scrollCursorRight;          //!< Mouse cursor used when dragging to the right. Must be an svg file.
            QCursor scrollCursorRightMax;       //!< Mouse cursor used when dragging to the right and the value hit the maximum. Must be an svg file.
            int labelSize;                      //!< Default label size. Used in derived classes like VectorInput.
            bool autoSelectAllOnClickFocus;     //!< Select the full value when the control is focused.
        };

        explicit SpinBox(QWidget* parent = nullptr);

        //! Sets the SpinBox style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the SpinBox.
        static Config loadConfig(QSettings& settings);
        //! Gets the default SpinBox style configuration.
        static Config defaultConfig();

        //! Sets the "HasError" property of the spinbox, displaying it with a red border.
        static void setHasError(QAbstractSpinBox* spinbox, bool hasError);

        //! Sets the value of the SpinBox, with undo/redo support.
        //! Note that this is an override to a non-virtual function, so an explicit pointer
        //! to the SpinBox class must be used in place of QSpinBox for this to work properly.
        void setValue(int value);

        //! Returns the recommended minimum size for this control.
        QSize minimumSizeHint() const override;

        //! Returns whether an undo value is available.
        bool isUndoAvailable() const;
        //! Returns whether a redo value is available.
        bool isRedoAvailable() const;

        //! Sets whether the initial value was initialized. False prompts reinitialization.
        void setInitialValueWasSetting(bool b);
    Q_SIGNALS:
        //! Triggered when the value begins changing, for example when the control starts being dragged.
        void valueChangeBegan();
        //! Triggered when the value stops changing, for example when the control is released.
        void valueChangeEnded();

        //! Triggered when the global undo key sequence is pressed by the user while this control is focused.
        void globalUndoTriggered();
        //! Triggered when the global redo key sequence is pressed by the user while this control is focused.
        void globalRedoTriggered();

        //! Triggered when the Cut key sequence is pressed by the user while this control is focused.
        void cutTriggered();
        //! Triggered when the Copy key sequence is pressed by the user while this control is focused.
        void copyTriggered();
        //! Triggered when the Paste key sequence is pressed by the user while this control is focused.
        void pasteTriggered();
        //! Triggered when the Delete key sequence is pressed by the user while this control is focused.
        void deleteTriggered();

        //! Triggered before the context menu is shown.
        //! Connect to this signal in the main UI thread, with a direct connection. Do not use Qt::QueuedConnection or
        //! Qt::BlockingQueuedConnection as the parameters will only be valid for a short time
        void contextMenuAboutToShow(QMenu* menu, QAction* undoAction, QAction* redoAction);

    protected:
        friend class SpinBoxWatcher;
        friend class Style;
        friend class VectorElement;

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::SpinBox::s_spinBoxWatcher': class 'QPointer<AzQtComponents::SpinBoxWatcher>' needs to have dll-interface to be used by clients of class 'AzQtComponents::SpinBox'
        static QPointer<SpinBoxWatcher> s_spinBoxWatcher;
        AZ_POP_DISABLE_WARNING
        
        static void initializeWatcher();
        static void uninitializeWatcher();

        static bool drawSpinBox(const QProxyStyle* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static QRect editFieldRect(const QProxyStyle* style, const QStyleOptionComplex* option, const QWidget* widget, const Config& config);

        static bool polish(QProxyStyle* style, QWidget* widget, const Config& config);
        static bool unpolish(QProxyStyle* style, QWidget* widget, const Config& config);

        static void setButtonSymbolsForStyle(QAbstractSpinBox* spinBox);

        void focusInEvent(QFocusEvent* e) override;
        void focusOutEvent(QFocusEvent* e) override;
        void contextMenuEvent(QContextMenuEvent* ev) override;
        virtual void setLastValue(int v);

        internal::SpinBoxLineEdit* m_lineEdit = nullptr;
        int m_lastValue = 0;
        QString m_lastSuffix;
        QSize m_lastMinimumSize;

        friend class DoubleSpinBox;
    };

    //! Control for decimal number selection.
    //! Supports value editing via direct input, dragging, mouse scroll-wheel and UI arrows.
    class AZ_QT_COMPONENTS_API DoubleSpinBox
        : public QDoubleSpinBox
    {
        Q_OBJECT

    public:
        //! Display options for the DoubleSpinBox class.
        enum Option
        {
            SHOW_ONE_DECIMAL_PLACE_ALWAYS,  //!< Show zeros even for values with no decimal part.
        };
        Q_DECLARE_FLAGS(Options, Option)
        Q_FLAG(Options)

        //! Whether an undo value is available.
        Q_PROPERTY(bool undoAvailable READ isUndoAvailable)
        //! Whether a redo value is available.
        Q_PROPERTY(bool redoAvailable READ isRedoAvailable)
        //! Display options for the control.
        Q_PROPERTY(Options options READ options WRITE setOptions)
        //! The maximum number of decimal digits to display when the value is truncated for display.
        Q_PROPERTY(int displayDecimals READ displayDecimals WRITE setDisplayDecimals)

        explicit DoubleSpinBox(QWidget* parent = nullptr);

        //! Sets the value of the SpinBox, with undo/redo support.
        //! Note that this is an override to a non-virtual function, so an explicit pointer
        //! to the DoubleSpinBox class must be used in place of QSpinBox for this to work properly.
        void setValue(double value);

        //! Returns the recommended minimum size for this control.
        QSize minimumSizeHint() const override;

        //! Formats the value into a string according to the display options.
        QString textFromValue(double value) const override;

        //! Returns whether an undo value is available.
        bool isUndoAvailable() const;
        //! Returns whether a redo value is available.
        bool isRedoAvailable() const;

        //! Returns this control's display options.
        Options options() const { return m_options; }
        //! Sets this control's display options.
        void setOptions(Options options) { m_options = options; }

        //! Returns the setting for the number of decimals to display.
        int displayDecimals() const { return m_displayDecimals; }
        //! Sets the setting for the number of decimals to display.
        void SetDisplayDecimals(int displayDecimals) { setDisplayDecimals(displayDecimals); }
        //! Sets the setting for the number of decimals to display.
        void setDisplayDecimals(int displayDecimals) { m_displayDecimals = displayDecimals; }

        //! Returns whether this SpinBox is currently being edited.
        bool isEditing() const;

        //! Sets whether the initial value was initialized. False prompts reinitialization.
        void setInitialValueWasSetting(bool b);

    Q_SIGNALS:
        //! Triggered when the value begins changing, for example when the control starts being dragged.
        void valueChangeBegan();
        //! Triggered when the value stops changing, for example when the control is released.
        void valueChangeEnded();

        //! Triggered when the global undo key sequence is pressed by the user while this control is focused.
        void globalUndoTriggered();
        //! Triggered when the global redo key sequence is pressed by the user while this control is focused.
        void globalRedoTriggered();

        //! Triggered when the Cut key sequence is pressed by the user while this control is focused.
        void cutTriggered();
        //! Triggered when the Copy key sequence is pressed by the user while this control is focused.
        void copyTriggered();
        //! Triggered when the Paste key sequence is pressed by the user while this control is focused.
        void pasteTriggered();
        //! Triggered when the Delete key sequence is pressed by the user while this control is focused.
        void deleteTriggered();

        //! Triggered before the context menu is shown.
        //! Connect to this signal in the main UI thread, with a direct connection. Do not use Qt::QueuedConnection or
        //! Qt::BlockingQueuedConnection as the parameters will only be valid for a short time
        void contextMenuAboutToShow(QMenu* menu, QAction* undoAction, QAction* redoAction);

    protected:
        friend class SpinBoxWatcher;

        void focusInEvent(QFocusEvent* e) override;
        void focusOutEvent(QFocusEvent* e) override;
        void contextMenuEvent(QContextMenuEvent* ev) override;
        QString stringValue(double value, bool truncated = false) const;
        void updateToolTip(double value);
        virtual void setLastValue(double v);

        internal::SpinBoxLineEdit* m_lineEdit = nullptr;
        double m_lastValue = 0.0;
        QString m_lastSuffix;
        QSize m_lastMinimumSize;
        int m_displayDecimals;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::DoubleSpinBox::m_options': class 'QFlags<AzQtComponents::DoubleSpinBox::Option>' needs to have dll-interface to be used by clients of class 'AzQtComponents::DoubleSpinBox'
        Options m_options;
        AZ_POP_DISABLE_WARNING
    };

    namespace internal
    {
        //! Internal class to add support to undo and redo for SpinBox classes.
        //! Must not be used outside of the SpinBox classes.
        class SpinBoxLineEdit : public QLineEdit
        {
            Q_OBJECT

        public:
            explicit SpinBoxLineEdit(QWidget* parent = nullptr);

            bool event(QEvent* ev) override;
            void keyPressEvent(QKeyEvent* ev) override;
            void paintEvent(QPaintEvent* event) override;

            bool overrideUndoRedo() const;

        Q_SIGNALS:
            void globalUndoTriggered();
            void globalRedoTriggered();

            void selectAllTriggered();
            void cutTriggered();
            void copyTriggered();
            void pasteTriggered();
            void deleteTriggered();
        };
    } // namespace internal
} // namespace AzQtComponents

Q_DECLARE_OPERATORS_FOR_FLAGS(AzQtComponents::DoubleSpinBox::Options)
