/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QColor>
#include <QPointer>
#include <QSize>
#include <QStyleOption>

class QLineEdit;
class QPainter;
class QProxyStyle;
class QSettings;
class QToolButton;

namespace AzQtComponents
{
    class BrowseEdit;
    class LineEditWatcher;
    class Style;

    //! Class to handle styling and painting of QLineEdit widgets.
    class AZ_QT_COMPONENTS_API LineEdit
    {
    public:
        //! Style configuration for the LineEdit class.
        struct Config
        {
            int borderRadius;                   //!< Border radius in pixels. All corners get the same border radius.
            QColor borderColor;
            QColor placeHolderTextColor;
            QColor hoverBorderColor;
            QColor hoverBackgroundColor;
            int hoverLineWidth;                 //!< Border thickness for the hover state, in pixels.
            QColor focusedBorderColor;
            int focusedLineWidth;               //!< Border thickness for the focused state, in pixels.
            QColor errorBorderColor;
            int errorLineWidth;                 //!< Border thickness for the error state, in pixels.
            QString clearImage;                 //!< Path to the clear icon. Svg images recommended.
            QString clearImageDisabled;         //!< Path to the disabled clear icon. Svg images recommended.
            QSize clearImageSize;               //!< Size of the clear icon. Size must be proportional to the icon's ratio.
            QString errorImage;                 //!< Path to the error icon. Svg images recommended.
            QSize errorImageSize;               //!< Size of the error icon. Size must be proportional to the icon's ratio.
            int iconSpacing;                    //!< Spacing value for the icons, in pixels. All directions get the same spacing.
            int iconMargin;                     //!< Margin value for the icons, in pixels. All directions get the same margin.
            bool autoSelectAllOnClickFocus;     //!< Determines if all the text is selected when the LineEdit gains focus.
            int dropFrameOffset;                //!< Offset around the frame drawn when the LineEdit is a drop target, in pixels.
            int dropFrameRadius;                //!< Radius of the frame drawn when the LineEdit is a drop target, in pixels.

            //! Returns the current LineEdit border width.
            //! @param option The QStyleOption containing the style parameters for the LineEdit.
            //! @param hasError Whether the LineEdit is in an error state.
            //! @param dropTarget Whether the LineEdit is currently a drop target.
            int getLineWidth(const QStyleOption* option, bool hasError, bool dropTarget) const;
            //! Returns the current LineEdit border color.
            //! @param option The QStyleOption containing the style parameters for the LineEdit.
            //! @param hasError Whether the LineEdit is in an error state.
            //! @param dropTarget Whether the LineEdit is currently a drop target.
            QColor getBorderColor(const QStyleOption* option, bool hasError, bool dropTarget) const;
            //! Returns the LineEdit background color.
            //! @param option The QStyleOption containing the style parameters for the LineEdit.
            //! @param hasError Whether the LineEdit is in an error state.
            //! @param isDropTarget Whether the LineEdit is currently a drop target.
            //! @param widget Pointer to the LineEdit widget.
            QColor getBackgroundColor(const QStyleOption* option, bool hasError, bool isDropTarget, const QWidget* widget) const;
        };

        //! Applies the "Search" style class to a QLineEdit.
        static void applySearchStyle(QLineEdit* lineEdit);
        //! Removes the "Search" style class from a QLineEdit.
        static void removeSearchStyle(QLineEdit* lineEdit);

        //! Applies the "DropTarget" style class to a QLineEdit.
        //! @param valid Whether the QLineEdit is a valid drop target or not.
        static void applyDropTargetStyle(QLineEdit* lineEdit, bool valid);
        //! Removes the "DropTarget" style class from a QLineEdit.
        static void removeDropTargetStyle(QLineEdit* lineEdit);

        //! Sets the LineEdit style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the LineEdit.
        static Config loadConfig(QSettings& settings);
        //! Gets the default LineEdit style configuration.
        static Config defaultConfig();

        //! Sets the message to display in a tooltip if the QLineEdit validator
        //! detects an error.
        static void setErrorMessage(QLineEdit* lineEdit, const QString& error);

        //! Set external error state.
        //! The global error state is calculated by an OR operation between
        //! external error state, validator and inputMask.
        static void setExternalError(QLineEdit* lineEdit, bool hasExternalError);

        //! Sets whether an error icon and message should be displayed when the
        //! LineEdit is in an error state. Default value is true.
        static void setErrorIconEnabled(QLineEdit* lineEdit, bool enabled);
        //! Returns true if the error icon will be shown if an error state is detected.
        static bool errorIconEnabled(QLineEdit* lineEdit);

        //! Returns a pointer to the QToolButton created when QLineEdit::setClearButtonEnabled(true)
        //! is called. Returns nullptr if the clear button has not been created yet.
        static QToolButton* getClearButton(const QLineEdit* lineEdit);

        //! Enables the clear button to be displayed on read-only LineEdits. Default value is false.
        static void setEnableClearButtonWhenReadOnly(QLineEdit* lineEdit, bool enabled);

    private:
        friend class BrowseEdit;
        friend class Style;

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // needs to have dll-interface to be used by clients of class 'AzQtComponents::LineEdit'
        static QPointer<LineEditWatcher> s_lineEditWatcher;
        AZ_POP_DISABLE_WARNING
        static unsigned int s_watcherReferenceCount;

        static void initializeWatcher();
        static void uninitializeWatcher();

        static bool drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static bool polish(QProxyStyle* style, QWidget* widget, const Config& config);
        static bool unpolish(QProxyStyle* style, QWidget* widget, const Config& config);

        static void applyErrorStyle(QLineEdit* lineEdit, const Config& config);
        static void removeErrorStyle(QLineEdit* lineEdit);

        static QIcon clearButtonIcon(const QStyleOption* option, const QWidget* widget, const Config& config);

        static QRect lineEditContentsRect(const Style* style, QStyle::SubElement element, const QStyleOption* option, const QWidget* widget, const Config& config);
    };

} // namespace AzQtComponents
