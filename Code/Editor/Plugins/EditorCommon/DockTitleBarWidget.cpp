/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "DockTitleBarWidget.h"
#include <QStyle>
#include <QStyleOptionToolButton>
#include <AzCore/Casting/numeric_cast.h>

namespace DockTitleBarInterpolate
{
    static QColor Interpolate(const QColor& a, const QColor& b, float k)
    {
        float mk = 1.0f - k;
        return QColor(aznumeric_cast<int>(a.red() * mk + b.red() * k),
            aznumeric_cast<int>(a.green() * mk + b.green() * k),
            aznumeric_cast<int>(a.blue() * mk + b.blue() * k),
            aznumeric_cast<int>(a.alpha() * mk + b.alpha() * k));
    }
}

class CDockWidgetTitleButton
    : public QAbstractButton
{
public:
    CDockWidgetTitleButton(QWidget* parent);

    QSize sizeHint() const;
    QSize minimumSizeHint() const { return sizeHint(); }

protected:
    void enterEvent(QEvent* ev);
    void leaveEvent(QEvent* ev);
    void paintEvent(QPaintEvent* ev);
};

class CTitleBarText
    : public QWidget
{
public:

    CTitleBarText(QWidget* parent, QDockWidget* dockWidget)
        : QWidget(parent)
        , m_dockWidget(dockWidget)
    {
        QFont font;
        font.setBold(true);
        setFont(font);
    }

    void paintEvent([[maybe_unused]] QPaintEvent* ev) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRect r = rect().adjusted(2, 2, -3, -3);
        p.translate(0.5f, 0.5f);
        QColor color = DockTitleBarInterpolate::Interpolate(palette().color(QPalette::Window), palette().color(QPalette::Shadow), 0.2f);
        p.setBrush(QBrush(color));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, 4, 4, Qt::AbsoluteSize);
        p.setPen(QPen(palette().color(QPalette::WindowText)));
        QTextOption textOption(Qt::AlignLeft | Qt::AlignVCenter);
        textOption.setWrapMode(QTextOption::NoWrap);
        p.drawText(r.adjusted(4, 0, 0, 0), m_dockWidget->windowTitle(), textOption);
    }
private:
    QDockWidget* m_dockWidget;
};


// ---------------------------------------------------------------------------

CDockWidgetTitleButton::CDockWidgetTitleButton(QWidget* parent)
    : QAbstractButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
}

QSize CDockWidgetTitleButton::sizeHint() const
{
    ensurePolished();

    int size = 2 * style()->pixelMetric(QStyle::PM_DockWidgetTitleBarButtonMargin, 0, this);
    if (!icon().isNull())
    {
        int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
        QSize sz = icon().actualSize(QSize(iconSize, iconSize));
        size += qMax(sz.width(), sz.height());
    }

    return QSize(size, size);
}

void CDockWidgetTitleButton::enterEvent(QEvent* ev)
{
    if (isEnabled())
    {
        update();
    }
    QAbstractButton::enterEvent(ev);
}

void CDockWidgetTitleButton::leaveEvent(QEvent* ev)
{
    if (isEnabled())
    {
        update();
    }
    QAbstractButton::leaveEvent(ev);
}

void CDockWidgetTitleButton::paintEvent([[maybe_unused]] QPaintEvent* ev)
{
    QPainter painter(this);

    QStyleOptionToolButton opt;
    opt.state = QStyle::State_AutoRaise;
    opt.init(this);
    opt.state |= QStyle::State_AutoRaise;

    if (isEnabled() && underMouse() && !isChecked() && !isDown())
    {
        opt.state |= QStyle::State_Raised;
    }
    if (isChecked())
    {
        opt.state |= QStyle::State_On;
    }
    if (isDown())
    {
        opt.state |= QStyle::State_Sunken;
    }
    if (opt.state & (QStyle::State_Raised | QStyle::State_Sunken))
    {
        style()->drawPrimitive(QStyle::PE_PanelButtonTool, &opt, &painter, this);
    }

    opt.icon = icon();
    opt.subControls = QStyle::SubControls();
    opt.activeSubControls = QStyle::SubControls();
    opt.features = QStyleOptionToolButton::None;
    opt.arrowType = Qt::NoArrow;
    int size = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
    opt.iconSize = QSize(size, size);
    style()->drawComplexControl(QStyle::CC_ToolButton, &opt, &painter, this);
}

// ---------------------------------------------------------------------------

CDockTitleBarWidget::CDockTitleBarWidget(QDockWidget* dockWidget)
    : m_dockWidget(dockWidget)
{
    CTitleBarText* textWidget = new CTitleBarText(this, dockWidget);

    m_layout = new QBoxLayout(QBoxLayout::LeftToRight);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(textWidget, 1);

    QStyleOptionDockWidget opt;
    opt.initFrom(dockWidget);
    opt.closable = dockWidget->features() & QDockWidget::DockWidgetClosable;
    opt.movable = dockWidget->features() & QDockWidget::DockWidgetMovable;
    opt.floatable = dockWidget->features() & QDockWidget::DockWidgetFloatable;

    m_buttonLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    m_buttonLayout->setContentsMargins(0, 0, 0, 0);
    m_buttonLayout->setSpacing(0);
    m_layout->addLayout(m_buttonLayout, 0);

    m_floatButton = new CDockWidgetTitleButton(dockWidget);
    m_floatButton->setIcon(QIcon("Icons/float.png"));
    m_floatButton->setVisible(opt.floatable);
    m_floatButton->setToolTip("Toggle Floating");
    connect(m_floatButton, SIGNAL(clicked()), SLOT(OnFloatButtonPressed()));
    m_layout->addWidget(m_floatButton, 0);

    m_closeButton = new CDockWidgetTitleButton(dockWidget);
    // close.png is a standard icon that looks similar to one in Fusion theme but
    // uses alpha so it can be used on dark theme as well.
    // style()->standardIcon(QStyle::SP_TitleBarCloseButton, &opt, dockWidget)
    QIcon closeIcon("Icons/close.png");
    m_closeButton->setIcon(closeIcon);
    m_closeButton->setVisible(opt.closable);
    m_closeButton->setToolTip("Close");
    connect(m_closeButton, SIGNAL(clicked()), SLOT(OnCloseButtonPressed()));
    m_layout->addWidget(m_closeButton, 0);

    setLayout(m_layout);
}

void CDockTitleBarWidget::AddCustomButton(const QIcon& icon, const char* tooltip, int id)
{
    SCustomButton slot;
    slot.button = new CDockWidgetTitleButton(m_dockWidget);
    slot.button->setIcon(icon);
    slot.button->setToolTip(tooltip);
    connect(slot.button, SIGNAL(clicked()), SLOT(OnCustomButtonPressed()));
    slot.id = id;
    m_buttonLayout->addWidget(slot.button, 0);
    m_customButtons.push_back(slot);
}

void CDockTitleBarWidget::OnFloatButtonPressed()
{
    m_dockWidget->setFloating(!m_dockWidget->isFloating());
}

void CDockTitleBarWidget::OnCloseButtonPressed()
{
    m_dockWidget->close();
}

void CDockTitleBarWidget::OnCustomButtonPressed()
{
    QObject* button = sender();
    for (size_t i = 0; i < m_customButtons.size(); ++i)
    {
        if (button == m_customButtons[i].button)
        {
            SignalCustomButtonPressed(m_customButtons[i].id);
            break;
        }
    }
}


#include <moc_DockTitleBarWidget.cpp>
