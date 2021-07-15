/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include <Controls/QToolTipWidget.h>

#include "QBitmapPreviewDialogImp.h"
#include "qcoreapplication.h"
#include "qguiapplication.h"
#include "qapplication.h"
#include <QDesktopWidget>
#include <QPainter>
#include <QtGlobal>
#include <qgraphicseffect.h>

void QToolTipWidget::RebuildLayout()
{
    if (m_title != nullptr)
    {
        m_title->hide();
    }
    if (m_content != nullptr)
    {
        m_content->hide();
    }
    if (m_specialContent != nullptr)
    {
        m_specialContent->hide();
    }

    //empty layout
    while (m_layout->count() > 0)
    {
        m_layout->takeAt(0);
    }
    qDeleteAll(m_currentShortcuts);
    m_currentShortcuts.clear();
    if (m_includeTextureShortcuts)
    {
        m_currentShortcuts.append(new QLabel(tr("Alt - Alpha"), this));
        m_currentShortcuts.back()->setProperty("tooltipLabel", "Shortcut");
        m_currentShortcuts.append(new QLabel(tr("Shift - RGBA"), this));
        m_currentShortcuts.back()->setProperty("tooltipLabel", "Shortcut");
    }

    if (m_title != nullptr && !m_title->text().isEmpty())
    {
        m_layout->addWidget(m_title);
        m_title->show();
    }

    for (QLabel* var : m_currentShortcuts)
    {
        if (var != nullptr)
        {
            m_layout->addWidget(var);
            var->show();
        }
    }
    if (m_specialContent != nullptr)
    {
        m_layout->addWidget(m_specialContent);
        m_specialContent->show();
    }
    if (m_content != nullptr && !m_content->text().isEmpty())
    {
        m_layout->addWidget(m_content);
        m_content->show();
    }
    m_background->adjustSize();
    adjustSize();
}

void QToolTipWidget::Hide()
{
    m_currentShortcuts.clear();
    hide();
}

void QToolTipWidget::Show(QPoint pos, ArrowDirection dir)
{
    if (!IsValid())
    {
        return;
    }
    m_arrow->m_direction = dir;
    pos = AdjustTipPosByArrowSize(pos, dir);
    m_normalPos = pos;
    move(pos);
    RebuildLayout();
    show();
    m_arrow->show();
}

void QToolTipWidget::Display(QRect targetRect, ArrowDirection preferredArrowDir)
{
    if (!IsValid())
    {
        return;
    }

    KeepTipOnScreen(targetRect, preferredArrowDir);

    RebuildLayout();
    show();
    m_arrow->show();
}

void QToolTipWidget::TryDisplay(QPoint mousePos, const QRect& rect, [[maybe_unused]] ArrowDirection preferredArrowDir)
{
    if (rect.contains(mousePos))
    {
        Display(rect, QToolTipWidget::ArrowDirection::ARROW_RIGHT);
    }
    else
    {
        hide();
    }
   }

void QToolTipWidget::TryDisplay(QPoint mousePos, const QWidget* widget, ArrowDirection preferredArrowDir)
{
    const QRect rect(widget->mapToGlobal(QPoint(0,0)), widget->size());
    TryDisplay(mousePos, rect, preferredArrowDir);
}

void QToolTipWidget::SetTitle(QString title)
{
    if (!title.isEmpty())
    {
        m_title->setText(title);
    }
    m_title->setProperty("tooltipLabel", "Title");

    setWindowTitle("ToolTip - " + title);
}

void QToolTipWidget::SetContent(QString content)
{
    m_content->setWordWrap(true);

    m_content->setProperty("tooltipLabel", "Content");
    //line-height is not supported via stylesheet so we use the html rich-text subset in QT for it.
    m_content->setText(QString("<span style=\"line-height: 14px;\">%1</span>").arg(content));
}

void QToolTipWidget::AppendContent(QString content)
{
    m_content->setText(m_content->text() + "\n\n" + content);
    update();
    RebuildLayout();
    m_content->update();
    m_content->repaint();
}

QToolTipWidget::QToolTipWidget(QWidget* parent)
    : QWidget(parent)
{
    m_background = new QWidget(this);
    m_background->setProperty("tooltip", "Background");
    m_background->stackUnder(this);
    m_title = new QLabel(this);
    m_currentShortcuts = QVector<QLabel*>();
    m_content = new QLabel(this);
    m_specialContent = nullptr;
    setWindowTitle("ToolTip");
    setObjectName("ToolTip");
    m_layout = new QVBoxLayout(this);
    m_normalPos = QPoint(0, 0);
    m_arrow = new QArrow(m_background);
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    m_arrow->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    m_arrow->setAttribute(Qt::WA_TranslucentBackground, true);
    m_background->setLayout(m_layout);
    m_arrow->setObjectName("ToolTipArrow");
    m_background->setObjectName("ToolTipBackground");

    //we need a drop shadow for the background
    QGraphicsDropShadowEffect* dropShadow = new QGraphicsDropShadowEffect(this);
    dropShadow->setBlurRadius(m_shadowRadius);
    dropShadow->setColor(Qt::black);
    dropShadow->setOffset(0);
    dropShadow->setEnabled(true);
    m_background->setGraphicsEffect(dropShadow);
    //we need a second drop shadow effect for the arrow
    dropShadow = new QGraphicsDropShadowEffect(m_arrow);
    dropShadow->setBlurRadius(m_shadowRadius);
    dropShadow->setColor(Qt::black);
    dropShadow->setOffset(0);
    dropShadow->setEnabled(true);
    m_arrow->setGraphicsEffect(dropShadow);
}

QToolTipWidget::~QToolTipWidget()
{
}

void QToolTipWidget::AddSpecialContent(QString type, QString dataStream)
{
    if (type.isEmpty())
    {
        m_includeTextureShortcuts = false;
        if (m_specialContent != nullptr)
        {
            delete m_specialContent;
            m_specialContent = nullptr;
        }
        return;
    }
    if (type == "TEXTURE")
    {
        if (m_specialContent == nullptr)
        {
            QCoreApplication::instance()->installEventFilter(this); //grab the event filter while displaying the advanced texture tooltip
            m_specialContent = new QBitmapPreviewDialogImp(this);
        }
        QString path(dataStream);
        qobject_cast<QBitmapPreviewDialogImp*>(m_specialContent)->setImage(path);
        // set default showmode to RGB
        qobject_cast<QBitmapPreviewDialogImp*>(m_specialContent)->setShowMode(QBitmapPreviewDialogImp::EShowMode::ESHOW_RGB);
        QString dir = (path.split("/").count() > path.split("\\").count()) ? path.split("/").back() : path.split("\\").back();
        SetTitle(dir);
        //always use default size but not image size
        qobject_cast<QBitmapPreviewDialogImp*>(m_specialContent)->setOriginalSize(false);
        m_includeTextureShortcuts = true;
    }
    else if (type == "ADD TO CONTENT")
    {
        AppendContent(dataStream);

        m_includeTextureShortcuts = false;
        if (m_specialContent != nullptr)
        {
            delete m_specialContent;
            m_specialContent = nullptr;
        }
    }
    else if (type == "REPLACE TITLE")
    {
        SetTitle(dataStream);
        m_includeTextureShortcuts = false;
        if (m_specialContent != nullptr)
        {
            delete m_specialContent;
            m_specialContent = nullptr;
        }
    }
    else if (type == "REPLACE CONTENT")
    {
        SetContent(dataStream);
        m_includeTextureShortcuts = false;
        if (m_specialContent != nullptr)
        {
            delete m_specialContent;
            m_specialContent = nullptr;
        }
    }
    else
    {
        m_includeTextureShortcuts = false;
        if (m_specialContent != nullptr)
        {
            delete m_specialContent;
            m_specialContent = nullptr;
        }
        return;
    }

    m_special = type;
}


bool QToolTipWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        if (m_special == "TEXTURE" && m_specialContent != nullptr)
        {
            const QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            Qt::KeyboardModifiers mods = ke->modifiers();
            if (mods & Qt::KeyboardModifier::AltModifier)
            {
                ((QBitmapPreviewDialogImp*)m_specialContent)->setShowMode(QBitmapPreviewDialogImp::ESHOW_ALPHA);
            }
            else if (mods & Qt::KeyboardModifier::ShiftModifier && !(mods & Qt::KeyboardModifier::ControlModifier))
            {
                ((QBitmapPreviewDialogImp*)m_specialContent)->setShowMode(QBitmapPreviewDialogImp::ESHOW_RGBA);
            }            
        }
    }
    if (event->type() == QEvent::KeyRelease)
    {
        if (m_special == "TEXTURE" && m_specialContent != nullptr)
        {
            const QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            Qt::KeyboardModifiers mods = ke->modifiers();
            if (!(mods& Qt::KeyboardModifier::AltModifier) && !(mods & Qt::KeyboardModifier::ShiftModifier))
            {
                ((QBitmapPreviewDialogImp*)m_specialContent)->setShowMode(QBitmapPreviewDialogImp::ESHOW_RGB);
            }            
        }
    }
    return QWidget::eventFilter(obj, event);
}

void QToolTipWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_arrow->hide();
}

void QToolTipWidget::UpdateOptionalData(QString optionalData)
{
    AddSpecialContent(m_special, optionalData);
}



QPoint QToolTipWidget::AdjustTipPosByArrowSize(QPoint pos, ArrowDirection dir)
{
    switch (dir)
    {
    case QToolTipWidget::ArrowDirection::ARROW_UP:
    {
        m_arrow->move(pos);
        pos.setY(pos.y() + 10);
        m_arrow->setFixedSize(20, 10);
        pos -= QPoint(m_shadowRadius, m_shadowRadius);
        break;
    }
    case QToolTipWidget::ArrowDirection::ARROW_LEFT:
    {
        m_arrow->move(pos);
        pos.setX(pos.x() + 10);
        m_arrow->setFixedSize(10, 20);
        pos -= QPoint(m_shadowRadius, m_shadowRadius);
        break;
    }
    case QToolTipWidget::ArrowDirection::ARROW_RIGHT:
    {
        pos.setX(pos.x() - 10);
        m_arrow->move(QPoint(pos.x() + width(), pos.y()));
        m_arrow->setFixedSize(10, 20);
        pos -= QPoint(-m_shadowRadius, m_shadowRadius);
        break;
    }
    case QToolTipWidget::ArrowDirection::ARROW_DOWN:
    {
        pos.setY(pos.y() - 10);
        m_arrow->move(QPoint(pos.x(), pos.y() + height()));
        m_arrow->setFixedSize(20, 10);
        pos -= QPoint(m_shadowRadius, -m_shadowRadius);
        break;
    }
    default:
        m_arrow->move(-10, -10);
        break;
    }
    return pos;
}

bool QToolTipWidget::IsValid()
{
    if (m_title->text().isEmpty() ||
        (m_content->text().isEmpty() && m_specialContent == nullptr))
    {
        return false;
    }
    return true;
}

void QToolTipWidget::KeepTipOnScreen(QRect targetRect, ArrowDirection preferredArrowDir)
{
    QRect desktop = QApplication::desktop()->availableGeometry(this);

    if (this->isHidden())
    {
        setAttribute(Qt::WA_DontShowOnScreen, true);
        Show(QPoint(0, 0), preferredArrowDir);
        hide();
        setAttribute(Qt::WA_DontShowOnScreen, false);
    }
    //else assume the size is right

    //calculate initial rect
    QRect tipRect = QRect(0, 0, 0, 0);
    switch (preferredArrowDir)
    {
    case QToolTipWidget::ArrowDirection::ARROW_UP:
    {
        //tip is below the widget with a left alignment
        tipRect.setTopLeft(AdjustTipPosByArrowSize(targetRect.bottomLeft(), preferredArrowDir));
        break;
    }
    case QToolTipWidget::ArrowDirection::ARROW_LEFT:
    {
        //tip is on the right with the top being even
        tipRect.setTopLeft(AdjustTipPosByArrowSize(targetRect.topRight(), preferredArrowDir));
        break;
    }
    case QToolTipWidget::ArrowDirection::ARROW_RIGHT:
    {
        //tip is on the left with the top being even
        tipRect.setY(targetRect.top());
        tipRect.setX(targetRect.left() - width());
        tipRect.setTopLeft(AdjustTipPosByArrowSize(tipRect.topLeft(), preferredArrowDir));
        break;
    }
    case QToolTipWidget::ArrowDirection::ARROW_DOWN:
    {
        //tip is above the widget with a left alignment
        tipRect.setX(targetRect.left());
        tipRect.setY(targetRect.top() - height());
        tipRect.setTopLeft(AdjustTipPosByArrowSize(tipRect.topLeft(), preferredArrowDir));
        break;
    }
    default:
    {
        //tip is on the right with the top being even
        preferredArrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
        tipRect.setTopLeft(AdjustTipPosByArrowSize(targetRect.topRight(), QToolTipWidget::ArrowDirection::ARROW_LEFT));
        break;
    }
    }
    tipRect.setSize(size());

    //FixPositioning
    if (preferredArrowDir == ArrowDirection::ARROW_LEFT || preferredArrowDir == ArrowDirection::ARROW_RIGHT)
    {
        if (tipRect.left() <= desktop.left())
        {
            m_arrow->m_direction = ArrowDirection::ARROW_LEFT;
            tipRect.setTopLeft(AdjustTipPosByArrowSize(targetRect.topRight(), m_arrow->m_direction));
        }
        else if (tipRect.right() >= desktop.right())
        {
            m_arrow->m_direction = ArrowDirection::ARROW_RIGHT;
            tipRect.setLeft(targetRect.left() - width());
            tipRect.setTopLeft(AdjustTipPosByArrowSize(tipRect.topLeft(), m_arrow->m_direction));
        }
    }
    else if (preferredArrowDir == ArrowDirection::ARROW_UP || preferredArrowDir == ArrowDirection::ARROW_DOWN)
    {
        if (tipRect.top() <= desktop.top())
        {
            m_arrow->m_direction = ArrowDirection::ARROW_UP;
            tipRect.setTopLeft(AdjustTipPosByArrowSize(targetRect.bottomLeft(), m_arrow->m_direction));
        }
        else if (tipRect.bottom() >= desktop.bottom())
        {
            m_arrow->m_direction = ArrowDirection::ARROW_DOWN;
            tipRect.setY(targetRect.top() - height());
            tipRect.setTopLeft(AdjustTipPosByArrowSize(tipRect.topLeft(), m_arrow->m_direction));
        }
    }

    //Nudge tip without arrow
    if (preferredArrowDir == ArrowDirection::ARROW_UP || preferredArrowDir == ArrowDirection::ARROW_DOWN)
    {
        if (tipRect.left() <= desktop.left())
        {
            tipRect.setLeft(desktop.left());
        }
        else if (tipRect.right() >= desktop.right())
        {
            tipRect.setLeft(desktop.right() - width());
        }
    }
    else if (preferredArrowDir == ArrowDirection::ARROW_RIGHT || preferredArrowDir == ArrowDirection::ARROW_LEFT)
    {
        if (tipRect.top() <= desktop.top())
        {
            tipRect.setTop(desktop.top());
        }
        else if (tipRect.bottom() >= desktop.bottom())
        {
            tipRect.setTop(desktop.bottom() - height());
        }
    }


    m_normalPos = tipRect.topLeft();
    move(m_normalPos);
}

QPolygonF QToolTipWidget::QArrow::CreateArrow()
{
    QVector<QPointF> vertex;
    //3 points in triangle
    vertex.reserve(3);
    //all magic number below are given in order to draw smooth transitions between tooltip and arrow
    if (m_direction == ArrowDirection::ARROW_UP)
    {
        vertex.push_back(QPointF(10, 1));
        vertex.push_back(QPointF(19, 10));
        vertex.push_back(QPointF(0, 10));
    }
    else if (m_direction == ArrowDirection::ARROW_RIGHT)
    {
        vertex.push_back(QPointF(9, 10));
        vertex.push_back(QPointF(0, 19));
        vertex.push_back(QPointF(0, 1));
    }
    else if (m_direction == ArrowDirection::ARROW_LEFT)
    {
        vertex.push_back(QPointF(1, 10));
        vertex.push_back(QPointF(10, 19));
        vertex.push_back(QPointF(10, 0));
    }
    else //ArrowDirection::ARROW_DOWN
    {
        vertex.push_back(QPointF(10, 10));
        vertex.push_back(QPointF(19, 0));
        vertex.push_back(QPointF(0, 0));
    }
    return QPolygonF(vertex);
}

void QToolTipWidget::QArrow::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    QColor color(255, 255, 255, 255);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::transparent); //force transparency
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(CreateArrow());
    //painter.setRenderHint(QPainter::Antialiasing, false);
}

QToolTipWrapper::QToolTipWrapper(QWidget* parent)
    : QObject(parent)
{
}

void QToolTipWrapper::SetTitle(QString title)
{
    m_title = title;
}

void QToolTipWrapper::SetContent(QString content)
{
    AddSpecialContent("REPLACE CONTENT", content);
}

void QToolTipWrapper::AppendContent(QString content)
{
    AddSpecialContent("ADD TO CONTENT", content);
}

void QToolTipWrapper::AddSpecialContent(QString type, QString dataStream)
{
    if (type == "REPLACE CONTENT")
    {
        m_contentOperations.clear();
    }
    m_contentOperations.push_back({type, dataStream});
}

void QToolTipWrapper::UpdateOptionalData(QString optionalData)
{
    m_contentOperations.push_back({"UPDATE OPTIONAL", optionalData});
}

void QToolTipWrapper::Display(QRect targetRect, QToolTipWidget::ArrowDirection preferredArrowDir)
{
    GetOrCreateToolTip()->Display(targetRect, preferredArrowDir);
}

void QToolTipWrapper::TryDisplay(QPoint mousePos, const QWidget * widget, QToolTipWidget::ArrowDirection preferredArrowDir)
{
    GetOrCreateToolTip()->TryDisplay(mousePos, widget, preferredArrowDir);
}

void QToolTipWrapper::TryDisplay(QPoint mousePos, const QRect & widget, QToolTipWidget::ArrowDirection preferredArrowDir)
{
    GetOrCreateToolTip()->TryDisplay(mousePos, widget, preferredArrowDir);
}

void QToolTipWrapper::hide()
{
    DestroyToolTip();
}

void QToolTipWrapper::show()
{
    GetOrCreateToolTip()->show();
}

bool QToolTipWrapper::isVisible() const
{
    return m_actualTooltip && m_actualTooltip->isVisible();
}

void QToolTipWrapper::update()
{
    if (m_actualTooltip)
    {
        m_actualTooltip->update();
    }
}

void QToolTipWrapper::ReplayContentOperations(QToolTipWidget* tooltipWidget)
{
    tooltipWidget->SetTitle(m_title);
    for (const auto& operation : m_contentOperations)
    {
        if (operation.first == "UPDATE OPTIONAL")
        {
            tooltipWidget->UpdateOptionalData(operation.second);
        }
        else
        {
            tooltipWidget->AddSpecialContent(operation.first, operation.second);
        }
    }
}

QToolTipWidget * QToolTipWrapper::GetOrCreateToolTip()
{
    if (!m_actualTooltip)
    {
        QToolTipWidget* tooltipWidget = new QToolTipWidget(static_cast<QWidget*>(parent()));
        tooltipWidget->setAttribute(Qt::WA_DeleteOnClose);
        ReplayContentOperations(tooltipWidget);
        m_actualTooltip = tooltipWidget;
    }
    return m_actualTooltip.data();
}

void QToolTipWrapper::DestroyToolTip()
{
    if (m_actualTooltip)
    {
        m_actualTooltip->deleteLater();
    }
}
