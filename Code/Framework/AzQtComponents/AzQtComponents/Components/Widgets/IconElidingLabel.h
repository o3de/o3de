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
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

#include <QIcon>
#include <QSize>
#endif

namespace AzQtComponents
{
    //! An ElidingLabel with a small icon before its text, in one widget. It stands in for a QToolButton and an ElidingLabel side by
    //! side in a QHBoxLayout without margins or spacing, and looks and sizes as they did: the icon's area is the size the tool button
    //! asks for, at the left and centred (QWidgetItem::setGeometry); the style draws it as a tool button through one hidden
    //! QToolButton in the label's window (whose sheet rules reach it); the text starts after it (QLabel's indent) and is elided to
    //! what is left. The area is not a widget: it takes no focus (as the DPE's Tab skipped the button) and a click on it is the label's.
    class AZ_QT_COMPONENTS_API IconElidingLabel : public ElidingLabel
    {
        Q_OBJECT
    public:
        explicit IconElidingLabel(QWidget* parent = nullptr);

        //! The icon before the text, at the tool button's icon size. A null icon draws as the button without an icon did.
        void setIcon(const QIcon& icon, const QSize& iconSize);

        //! The hints of the button and the label side by side, as their QBoxLayout gave them
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        //! The text rect of the label beside the button: ElidingLabel's, less the button's width at the left
        QRect TextRect() override;
        bool event(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        //! The size the layout gave the button: its hint within its limits (QWidgetItem::sizeHint)
        QSize iconButtonSize() const;
        //! The text starts after the button's area
        void updateIndent();

        QIcon m_icon;
        QSize m_iconSize;
        mutable QSize m_buttonSize; //!< iconButtonSize, until the style or the window changes
    };
} // namespace AzQtComponents
