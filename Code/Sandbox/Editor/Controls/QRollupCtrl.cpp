/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include "EditorDefs.h"

#include "QRollupCtrl.h"

// Qt
#include <QMenu>
#include <QStylePainter>
#include <QVBoxLayout>
#include <QSettings>
#include <QToolButton>
#include <QStyleOptionToolButton>

//////////////////////////////////////////////////////////////////////////

class QRollupCtrlButton
    : public QToolButton
{
public:
    QRollupCtrlButton(QWidget* parent);

    inline void setSelected(bool b) { selected = b; update(); }
    inline bool isSelected() const { return selected; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    bool selected;
};

QRollupCtrlButton::QRollupCtrlButton(QWidget* parent)
    : QToolButton(parent)
    , selected(true)
{
    setBackgroundRole(QPalette::Window);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    setFocusPolicy(Qt::NoFocus);

    setStyleSheet("* {margin: 2px 5px 2px 5px; border: 1px solid #CBA457;}");
}

QSize QRollupCtrlButton::sizeHint() const
{
    QSize iconSize(8, 8);
    if (!icon().isNull())
    {
        int icone = style()->pixelMetric(QStyle::PM_SmallIconSize);
        iconSize += QSize(icone + 2, icone);
    }
    QSize textSize = fontMetrics().size(Qt::TextShowMnemonic, text()) + QSize(0, 8);

    QSize total(iconSize.width() + textSize.width(), qMax(iconSize.height(), textSize.height()));
    return total.expandedTo(QApplication::globalStrut());
}

QSize QRollupCtrlButton::minimumSizeHint() const
{
    if (icon().isNull())
    {
        return QSize();
    }
    int icone = style()->pixelMetric(QStyle::PM_SmallIconSize);
    return QSize(icone + 8, icone + 8);
}

void QRollupCtrlButton::paintEvent(QPaintEvent*)
{
    QStylePainter p(this);
    // draw the background manually, not to clash with UI 2.0 style shets
    // the numbers here are taken from the stylesheet in the constructor
    p.fillRect(QRect(5, 1, width() - 10, height() - 3), QColor(52, 52, 52));

    {
        QStyleOptionToolButton opt;
        initStyleOption(&opt);
        if (isSelected())
        {
            if (opt.state & QStyle::State_MouseOver)
            {
                opt.state |= QStyle::State_Sunken;
            }
            opt.state |= QStyle::State_MouseOver;
        }
        p.drawComplexControl(QStyle::CC_ToolButton, opt);
    }

    {
        p.setPen(QPen(QColor(132, 128, 125)));

        int top = height() / 2 - 2;
        p.drawLine(2, top, 4, top);
        p.drawLine(width() - 5, top, width() - 3, top);

        int bottom = !isSelected() ? top + 4 : height();
        p.drawLine(2, bottom, 2, top);
        p.drawLine(width() - 3, bottom, width() - 3, top);

        if (!isSelected())
        {
            p.drawLine(2, bottom, 4, bottom);
            p.drawLine(width() - 5, bottom, width() - 3, bottom);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

QRollupCtrl::Page* QRollupCtrl::page(QWidget* widget) const
{
    if (!widget)
    {
        return 0;
    }

    for (PageList::ConstIterator i = m_pageList.constBegin(); i != m_pageList.constEnd(); ++i)
    {
        if ((*i).widget == widget)
        {
            return (Page*)&(*i);
        }
    }
    return 0;
}

QRollupCtrl::Page* QRollupCtrl::page(int index)
{
    if (index >= 0 && index < m_pageList.size())
    {
        return &m_pageList[index];
    }
    return 0;
}

const QRollupCtrl::Page* QRollupCtrl::page(int index) const
{
    if (index >= 0 && index < m_pageList.size())
    {
        return &m_pageList.at(index);
    }
    return 0;
}

inline void QRollupCtrl::Page::setText(const QString& text) { button->setText(text); }
inline void QRollupCtrl::Page::setIcon(const QIcon& is) { button->setIcon(is); }
inline void QRollupCtrl::Page::setToolTip(const QString& tip) { button->setToolTip(tip); }
inline QString QRollupCtrl::Page::text() const { return button->text(); }
inline QIcon QRollupCtrl::Page::icon() const { return button->icon(); }
inline QString QRollupCtrl::Page::toolTip() const { return button->toolTip(); }

//////////////////////////////////////////////////////////////////////////

QRollupCtrl::QRollupCtrl(QWidget* parent)
    : QScrollArea(parent)
    , m_layout(0)
{
    m_body = new QWidget(this);
    m_body->setBackgroundRole(QPalette::Button);
    setWidgetResizable(true);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setWidget(m_body);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    relayout();
}

QRollupCtrl::~QRollupCtrl()
{
    foreach(const QRollupCtrl::Page & c, m_pageList)
    disconnect(c.widget, &QObject::destroyed, this, &QRollupCtrl::_q_widgetDestroyed);
}

void QRollupCtrl::readSettings(const QString& qSettingsGroup)
{
    QSettings settings;
    settings.beginGroup(qSettingsGroup);

    int i = 0;
    foreach(const QRollupCtrl::Page & c, m_pageList) {
        QString qObjectName = c.widget->objectName();

        bool bHidden = settings.value(qObjectName, true).toBool();
        setIndexVisible(i++, !bHidden);
    }

    settings.endGroup();
}

void QRollupCtrl::writeSettings(const QString& qSettingsGroup)
{
    QSettings settings;
    settings.beginGroup(qSettingsGroup);

    for (int i = 0; i < count(); i++)
    {
        QString qObjectName;
        bool bHidden = isPageHidden(i, qObjectName);

        settings.setValue(qObjectName, bHidden);
    }
}

void QRollupCtrl::updateTabs()
{
    for (auto i = m_pageList.constBegin(); i != m_pageList.constEnd(); ++i)
    {
        QRollupCtrlButton* tB = (*i).button;
        QWidget* tW = (*i).sv;
        tB->setSelected(tW->isVisible());
        tB->update();
    }
}

int QRollupCtrl::insertItem(int index, QWidget* widget, const QIcon& icon, const QString& text)
{
    if (!widget)
    {
        return -1;
    }

    auto it = std::find_if(m_pageList.cbegin(), m_pageList.cend(), [widget](const Page& page) { return page.widget == widget; });
    if (it != m_pageList.cend())
    {
        return -1;
    }

    connect(widget, &QObject::destroyed, this, &QRollupCtrl::_q_widgetDestroyed);

    QRollupCtrl::Page c;
    c.widget = widget;
    c.button = new QRollupCtrlButton(m_body);
    c.button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(c.button, &QRollupCtrlButton::clicked, this, &QRollupCtrl::_q_buttonClicked);
    connect(c.button, &QRollupCtrlButton::customContextMenuRequested, this, &QRollupCtrl::_q_custumButtonMenu);

    c.sv = new QFrame(m_body);
    c.sv->setObjectName("rollupPaneFrame");
    //  c.sv->setFixedHeight(qMax(widget->sizeHint().height(), widget->size().height()));
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setMargin(3);
    layout->addWidget(widget);
    c.sv->setLayout(layout);
    c.sv->setStyleSheet("QFrame#rollupPaneFrame {margin: 0px 2px 2px 2px; border: 1px solid #84807D; border-top:0px;}");
    c.sv->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    c.sv->show();

    c.setText(text);
    c.setIcon(icon);

    const int numPages = m_pageList.count();
    if (index < 0 || index >= numPages)
    {
        m_pageList.append(c);
        index = numPages - 1;
        m_layout->insertWidget(m_layout->count() - 1, c.button);
        m_layout->insertWidget(m_layout->count() - 1, c.sv);
    }
    else
    {
        m_pageList.insert(index, c);
        relayout();
    }

    c.button->show();

    updateTabs();
    itemInserted(index);
    return index;
}

void QRollupCtrl::_q_buttonClicked()
{
    QObject* tb = sender();
    QWidget* item = 0;
    for (auto i = m_pageList.constBegin(); i != m_pageList.constEnd(); ++i)
    {
        if ((*i).button == tb)
        {
            item = (*i).widget;
            break;
        }
    }

    if (item)
    {
        setIndexVisible(indexOf(item), !item->isVisible());
    }
}

int QRollupCtrl::count() const
{
    return m_pageList.count();
}

bool QRollupCtrl::isPageHidden(int index, QString& qObjectName) const
{
    if (index < 0 || index >= m_pageList.size())
    {
        return true;
    }
    const QRollupCtrl::Page& c = m_pageList.at(index);
    qObjectName = c.widget->objectName();
    return c.sv->isHidden();
}

void QRollupCtrl::setIndexVisible(int index, bool visible)
{
    QRollupCtrl::Page* c = page(index);
    if (!c)
    {
        return;
    }

    if (c->sv->isHidden() && visible)
    {
        c->sv->show();
    }
    else if (c->sv->isVisible() && !visible)
    {
        c->sv->hide();
    }
    updateTabs();
}

void QRollupCtrl::setWidgetVisible(QWidget* widget, bool visible)
{
    setIndexVisible(indexOf(widget), visible);
}

void QRollupCtrl::relayout()
{
    delete m_layout;
    m_layout = new QVBoxLayout(m_body);
    m_layout->setMargin(3);
    m_layout->setSpacing(0);
    for (QRollupCtrl::PageList::ConstIterator i = m_pageList.constBegin(); i != m_pageList.constEnd(); ++i)
    {
        m_layout->addWidget((*i).button);
        m_layout->addWidget((*i).sv);
    }
    m_layout->addStretch();
    updateTabs();
}

void QRollupCtrl::_q_widgetDestroyed(QObject* object)
{
    // no verification - vtbl corrupted already
    QWidget* p = (QWidget*)object;

    QRollupCtrl::Page* c = page(p);
    if (!p || !c)
    {
        return;
    }

    m_layout->removeWidget(c->sv);
    m_layout->removeWidget(c->button);
    c->sv->deleteLater(); // page might still be a child of sv
    delete c->button;

    m_pageList.removeOne(*c);
}

void QRollupCtrl::_q_custumButtonMenu([[maybe_unused]] const QPoint& pos)
{
    QMenu menu;
    menu.addAction("Expand All")->setData(-1);
    menu.addAction("Collapse All")->setData(-2);
    menu.addSeparator();
    for (int i = 0; i < m_pageList.size(); ++i)
    {
        QRollupCtrl::Page* c = page(i);
        QAction* action = menu.addAction(c->button->text());
        action->setCheckable(true);
        action->setChecked(c->sv->isVisible());
        action->setData(i);
    }

    QAction* action = menu.exec(QCursor::pos());
    if (!action)
    {
        return;
    }
    int res = action->data().toInt();
    switch (res)
    {
    case -1: // fall through
    case -2:
        expandAllPages(res == -1);
        break;
    default:
    {
        QRollupCtrl::Page* c = page(res);
        if (c)
        {
            setIndexVisible(res, !c->sv->isVisible());
        }
    }
    break;
    }
}

void QRollupCtrl::expandAllPages(bool v)
{
    for (int i = 0; i < m_pageList.size(); i++)
    {
        setIndexVisible(i, v);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void QRollupCtrl::clear()
{
    while (!m_pageList.isEmpty())
    {
        removeItem(0);
    }
}

void QRollupCtrl::removeItem(QWidget* widget)
{
    auto it = std::find_if(m_pageList.cbegin(), m_pageList.cend(), [widget](const Page& page) { return page.widget == widget; });
    if (it != m_pageList.cend())
    {
        removeItem(it - m_pageList.cbegin());
    }
}

void QRollupCtrl::removeItem(int index)
{
    if (QWidget* w = widget(index))
    {
        disconnect(w, &QObject::destroyed, this, &QRollupCtrl::_q_widgetDestroyed);
        w->setParent(this);
        // destroy internal data
        _q_widgetDestroyed(w);
        itemRemoved(index);
    }
}

QWidget* QRollupCtrl::widget(int index) const
{
    if (index < 0 || index >= (int) m_pageList.size())
    {
        return 0;
    }
    return m_pageList.at(index).widget;
}

int QRollupCtrl::indexOf(QWidget* widget) const
{
    QRollupCtrl::Page* c = page(widget);
    return c ? m_pageList.indexOf(*c) : -1;
}

void QRollupCtrl::setItemEnabled(int index, bool enabled)
{
    QRollupCtrl::Page* c = page(index);
    if (!c)
    {
        return;
    }

    c->button->setEnabled(enabled);
    if (!enabled)
    {
        int curIndexUp = index;
        int curIndexDown = curIndexUp;
        const int count = m_pageList.count();
        while (curIndexUp > 0 || curIndexDown < count - 1)
        {
            if (curIndexDown < count - 1)
            {
                if (page(++curIndexDown)->button->isEnabled())
                {
                    index = curIndexDown;
                    break;
                }
            }
            if (curIndexUp > 0)
            {
                if (page(--curIndexUp)->button->isEnabled())
                {
                    index = curIndexUp;
                    break;
                }
            }
        }
    }
}

void QRollupCtrl::setItemText(int index, const QString& text)
{
    QRollupCtrl::Page* c = page(index);
    if (c)
    {
        c->setText(text);
    }
}

void QRollupCtrl::setItemIcon(int index, const QIcon& icon)
{
    QRollupCtrl::Page* c = page(index);
    if (c)
    {
        c->setIcon(icon);
    }
}

void QRollupCtrl::setItemToolTip(int index, const QString& toolTip)
{
    QRollupCtrl::Page* c = page(index);
    if (c)
    {
        c->setToolTip(toolTip);
    }
}

bool QRollupCtrl::isItemEnabled(int index) const
{
    const QRollupCtrl::Page* c = page(index);
    return c && c->button->isEnabled();
}

QString QRollupCtrl::itemText(int index) const
{
    const QRollupCtrl::Page* c = page(index);
    return (c ? c->text() : QString());
}

QIcon QRollupCtrl::itemIcon(int index) const
{
    const QRollupCtrl::Page* c = page(index);
    return (c ? c->icon() : QIcon());
}

QString QRollupCtrl::itemToolTip(int index) const
{
    const QRollupCtrl::Page* c = page(index);
    return (c ? c->toolTip() : QString());
}

void QRollupCtrl::changeEvent(QEvent* ev)
{
    if (ev->type() == QEvent::StyleChange)
    {
        updateTabs();
    }
    QFrame::changeEvent(ev);
}

void QRollupCtrl::showEvent(QShowEvent* ev)
{
    if (isVisible())
    {
        updateTabs();
    }
    IEditor* pEditor = GetIEditor();
    pEditor->SetEditMode(EEditMode::eEditModeSelect);
    QFrame::showEvent(ev);
}

void QRollupCtrl::itemInserted(int index)
{
    Q_UNUSED(index)
}

void QRollupCtrl::itemRemoved(int index)
{
    Q_UNUSED(index)
}


#include <Controls/moc_QRollupCtrl.cpp>
