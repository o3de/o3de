/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>

#include <QColor>
#include <QFrame>
#include <QIcon>
#include <QString>
#endif

class QSettings;
class QLineEdit;
class QValidator;
class QStyleOption;

namespace AzQtComponents
{
    class Style;

    //! A widget combining a Line Edit and a Push Button. Use this widget when you want to extend a Line Edit with support for user interaction.
    class AZ_QT_COMPONENTS_API BrowseEdit
        : public QFrame
    {
        Q_OBJECT

        //! Read-only value indicating whether or not user input satisfies the inputMask condition and any validator. Default value is true.
        Q_PROPERTY(bool acceptableInput READ hasAcceptableInput NOTIFY hasAcceptableInputChanged)
    public:
        typedef LineEdit::Config Config;

        BrowseEdit(QWidget* parent = nullptr);
        ~BrowseEdit();

        //! Sets the Icon for the button attached to this BrowseEdit.
        void setAttachedButtonIcon(const QIcon& icon);
        //! Returns the Icon currently being used by the button attached to this BrowseEdit.
        QIcon attachedButtonIcon() const;

        //! Enables the clear button on the Browse Edit.
        void setClearButtonEnabled(bool enable);
        //! Returns true if the clear button is currently enabled on this Browse Edit.
        bool isClearButtonEnabled() const;

        //! Renders the BrowseEdit read-only. Double clicking on the read-only text will trigger the attached button.
        void setLineEditReadOnly(bool readOnly);
        //! Returns true if the text of this BrowseEdit is currently read-only.
        bool isLineEditReadOnly() const;

        //! Sets the placeholder text for the BrowseEdit.
        //! This is the message shown when the current text value is an empty string.
        void setPlaceholderText(const QString& placeholderText);
        //! Returns the current placeholder text for the BrowseEdit.
        QString placeholderText() const;

        //! Sets the text value for the BrowseEdit.
        void setText(const QString& text);
        //! Gets the text value from the BrowseEdit.
        QString text() const;

        //! Sets the error message shown when hovering on the warning icon
        //! when the current text value is not acceptable.
        void setErrorToolTip(const QString& toolTipText);
        //! Returns the current error message for this BrowseEdit.
        QString errorToolTip() const;

        //! Sets a validator on the BrowseEdit.
        void setValidator(const QValidator* validator);
        //! Returns a pointer to the validator object on the BrowseEdit, if set.
        const QValidator* validator() const;

        //! Returns true if the current text value is accepted by the currently set validator.
        bool hasAcceptableInput() const;

        //! Returns a pointer to the underlying LineEdit.
        QLineEdit* lineEdit() const;

        //! Sets the BrowseEdit style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the BrowseEdit.
        static Config loadConfig(QSettings& settings);
        //! Gets the default BrowseEdit style configuration.
        static Config defaultConfig();

        //! Applies the drop target styling to the BrowseEdit.
        //! @param browseEdit Pointer to the BrowseEdit instance to apply the style on.
        //! @param valid Whether the BrowseEdit is a valid drop target or not.
        static void applyDropTargetStyle(BrowseEdit* browseEdit, bool valid);

        //! Remove the drop target styling from the BrowseEdit.
        //! @param browseEdit Pointer to the BrowseEdit instance to remove the style from.
        static void removeDropTargetStyle(BrowseEdit* browseEdit);

    Q_SIGNALS:
        //! Triggered when the button attached to the BrowseEdit is triggered.
        void attachedButtonTriggered();
        //! Triggered when the user has finished editing the BrowseEdit's value.
        void editingFinished();
        //! Triggered when the return button is pressed while the BrowseEdit is focused.
        void returnPressed();
        //! Triggered when the text value in the BrowseEdit is changed,
        //! either by the user or programmatically.
        void textChanged(const QString& text);
        //! Triggered when the text value in the BrowseEdit is edited by the user.
        void textEdited(const QString& text);
        //! Triggered when the text value changes from acceptable to non acceptable
        //! and vice versa, based on the current validator.
        void hasAcceptableInputChanged(bool acceptable);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        bool event(QEvent* event) override;
        void setHasAcceptableInput(bool acceptable);

    private Q_SLOTS:
        void onTextChanged(const QString& text);

    private:
        friend class Style;
        struct InternalData;

        static bool polish(Style* style, QWidget* widget, const Config& config, const LineEdit::Config& lineEditConfig);
        static bool unpolish(Style* style, QWidget* widget, const Config& config, const LineEdit::Config& lineEditConfig);

        static bool drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::BrowseEdit::m_data': class 'QScopedPointer<AzQtComponents::BrowseEdit::InternalData,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'AzQtComponents::BrowseEdit'
        QScopedPointer<InternalData> m_data;
        AZ_POP_DISABLE_WARNING
    };

} // namespace AzQtComponents
