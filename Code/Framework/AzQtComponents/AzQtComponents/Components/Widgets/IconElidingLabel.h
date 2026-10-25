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

class QToolButton;

namespace AzQtComponents
{
    //! An ElidingLabel with a small icon before its text, in one widget.
    //!
    //! It stands in for a QToolButton showing the icon and an ElidingLabel beside it, in a container whose QHBoxLayout has no
    //! margins and no spacing (three widgets and a layout), and looks and sizes as they did:
    //! - the icon's area is the button: the size the tool button asks for, at the left, centred in the label's height as the
    //!   layout placed it (QWidgetItem::setGeometry);
    //! - it is drawn by the style's own tool button code, through one hidden QToolButton in the label's window (the window's style
    //!   sheet rules reach it as they reached the button), onto the label;
    //! - the text starts after the area (QLabel's indent, for the left and centred text a label has) and is elided to what is left.
    //! The area is not a widget: it takes no focus (as the DPE's Tab skipped the button) and a click on it is the label's.
    class AZ_QT_COMPONENTS_API IconElidingLabel : public ElidingLabel
    {
        Q_OBJECT
    public:
        explicit IconElidingLabel(QWidget* parent = nullptr);

        //! The icon before the text, at the tool button's icon size. A null icon draws as the button without an icon did.
        void setIcon(const QIcon& icon, const QSize& iconSize);

        //! The hint of the button and the label side by side: the widths added, the taller height
        QSize sizeHint() const override;
        //! The minimum of the button and the label side by side: the button's size, and the label's limits
        QSize minimumSizeHint() const override;

    protected:
        //! The text rect of the label beside the button: ElidingLabel's, less the button's width at the left
        QRect TextRect() override;
        bool event(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        //! The hidden tool button of the label's window, or none while the label is not in a window
        QToolButton* iconButton() const;
        //! The size the layout gave the button: its hint within its limits (QWidgetItem::sizeHint)
        QSize iconButtonSize() const;
        //! The text starts after the button's area
        void updateIndent();

        QIcon m_icon;
        QSize m_iconSize;
        mutable QSize m_buttonSize; //!< iconButtonSize, until the style or the window changes
    };
} // namespace AzQtComponents
