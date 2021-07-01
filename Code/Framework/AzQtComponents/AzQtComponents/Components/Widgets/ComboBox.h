/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QColor>
#include <QPointer>
#include <QStyle>
#include <QValidator>
#endif
AZ_POP_DISABLE_WARNING

class QComboBox;
class QSettings;

namespace AzQtComponents
{
    class Style;
    class ComboBoxWatcher;

    //! Default validator for the ComboBox control. Always returns true to all inputs.
    class AZ_QT_COMPONENTS_API ComboBoxValidator
        : public QValidator
    {
        Q_OBJECT

    public:
        explicit ComboBoxValidator(QObject* parent = nullptr);
        virtual QValidator::State validateIndex(int) const;
        QValidator::State validate(QString&, int&) const override;
    };

    //! Adds functionality to ComboBox controls by offering configuration, styling and validation options.
    class AZ_QT_COMPONENTS_API ComboBox
    {
    public:
        //! Style configuration for the ComboBox class.
        struct Config
        {
            QColor placeHolderTextColor;    //!< Color for the placeholder text that is shown when no value is selected.
            QColor framelessTextColor;      //!< Default text color.
            QString errorImage;             //!< Path to the icon. Svg images recommended.
            QSize errorImageSize;           //!< Size of the error image. Size must be proportional to the image's ratio.
            int errorImageSpacing;          //!< Spacing around the error image in pixels.
            int itemPadding;                //!< Total horizontal padding (left + right) of items in the dropdown, in pixels.
            int leftAdjust;                 //!< Adjust the left position of the dropdown, in pixels.
        };

        //! Sets the ComboBox style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the ComboBox.
        static Config loadConfig(QSettings& settings);
        //! Gets the default ComboBox style configuration.
        static Config defaultConfig();

        //! Adds a validator to the ComboBox.
        //! This function binds a validator to the control itself instead of the underlying
        //! LineEdit, meaning it won't be deleted until the QComboBox itself is destroyed.
       static void setValidator(QComboBox* cb, QValidator* validator);

        //! Forces the ComboBox to set its internal error state to "error"
       static void setError(QComboBox* cb, bool error);

        //! Applies the CustomCheckState styling to a QCheckBox.
        //! With this style applied, the checkmark will be defined by the model,
        //! and won't be automatically checked for the current item.
        //! Same as AzQtComponents::Style::addClass(comboBox, "CustomCheckState")
        static void addCustomCheckStateStyle(QComboBox* cb);

    private:
        friend class Style;

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
        static QPointer<ComboBoxWatcher> s_comboBoxWatcher;
        AZ_POP_DISABLE_WARNING
        
        static unsigned int s_watcherReferenceCount;

        static void initializeWatcher();
        static void uninitializeWatcher();

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);
        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const ComboBox::Config& config);
        static QRect comboBoxListBoxPopupRect(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static bool drawComboBox(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawComboBoxLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawIndicatorArrow(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawItemCheckIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static void addErrorButton(QComboBox* cb, const Config& config);
    };

} // namespace AzQtComponents

Q_DECLARE_METATYPE(AzQtComponents::ComboBoxValidator*)
