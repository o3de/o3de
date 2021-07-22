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
#include <AzCore/Math/Color.h>
#include <QWidget>
#endif

namespace AzQtComponents
{
    class ColorHexEdit;
    class Swatch;

    //! A control to select color properties via a ColorPicker dialog.
    class AZ_QT_COMPONENTS_API ColorLabel
        : public QWidget
    {
        Q_OBJECT
    public:
        ColorLabel(QWidget* parent = nullptr);
        ColorLabel(const AZ::Color& color, QWidget* parent = nullptr);
        ~ColorLabel();

        AZ::Color color() const { return m_color; }

        //! Sets the visibility of the LineEdit showing the hex value for the color.
        void setTextInputVisible(bool visible);

    Q_SIGNALS:
        //! Triggered when the color value is changed.
        void colorChanged(const AZ::Color& color);

    public Q_SLOTS:
        //! Sets the color value to the one provided.
        void setColor(const AZ::Color& color);

    protected:
        bool eventFilter(QObject* object, QEvent* event) override;

    private Q_SLOTS:
        void onHexEditColorChanged();
        void updateSwatchColor();
        void updateHexColor();

    private:
        Swatch* m_swatch;
        ColorHexEdit* m_hexEdit;

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        AZ::Color m_color;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };
} // namespace AzQtComponents
