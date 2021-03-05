/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 *
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

// Modifications copyright Amazon.com, Inc. or its affiliates.

#include "EditorCommon_precompiled.h"
#include "QPropertyTree.h"
#include "QPropertyTreeStyle.h"
#include "PropertyTreeModel.h"
#include "PropertyRowNumberField.h"
#include "PropertyDrawContext.h"
#include "MathUtils.h"
#include <QStyleOption>
#include <QDesktopWidget>
#include <QApplication>
#include <QPainter>
#include <QBitmap>

PropertyRowNumberField::PropertyRowNumberField()
    : pressed_(false)
    , dragStarted_(false)
{
}

PropertyRowWidget* PropertyRowNumberField::createWidget(QPropertyTree* tree)
{
    return new PropertyRowWidgetNumber(tree->model(), this, tree);
}

QColor interpolateColor(const QColor& a, const QColor& b, float k);

void PropertyRowNumberField::redraw(const PropertyDrawContext& context)
{
    if (multiValue())
        context.drawEntry(L" ... ", false, true, 0);
    else if (userReadOnly())
        context.drawValueText(pulledSelected(), valueAsWString().c_str());
    else
    {
        QPainter* painter = context.painter;
        const QPropertyTree* tree = context.tree;

        QRect rt = context.widgetRect;
        rt.adjust(0, 0, 0, -1);

#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
        QStyleOptionFrameV2 option;
        option.features = QStyleOptionFrameV2::None;
#else
        QStyleOptionFrame option;
        option.features = QStyleOptionFrame::None;
#endif

        option.state = QStyle::State_Sunken;

        // We require a widget to use as context, so that the style sheet can work.
        QLineEdit widgetForContext;
        option.lineWidth = tree->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, &widgetForContext);

        option.midLineWidth = 0;

        if (context.captured) {
            option.state |= QStyle::State_HasFocus;
            option.state |= QStyle::State_Active;
            option.state |= QStyle::State_MouseOver;
        }
        else if (!userReadOnly()) {
            option.state |= QStyle::State_Enabled;
        }
        option.rect = rt; // option.rect is the rectangle to be drawn on.
        option.palette = tree->palette();
        option.fontMetrics = tree->fontMetrics();
        QRect textRect = tree->style()->subElementRect(QStyle::SE_LineEditContents, &option, &widgetForContext);
        if (!textRect.isValid()) {
            textRect = rt;
            textRect.adjust(3, 1, -3, -2);
        }
        else {
            textRect.adjust(2, 1, -2, -1);
        }


        widgetForContext.ensurePolished();
        option.palette = widgetForContext.palette();

        painter->setPen(QPen(widgetForContext.palette().color(QPalette::WindowText)));
        painter->setBrush(QBrush(widgetForContext.palette().color(QPalette::Base)));
        tree->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, painter, &widgetForContext);

        double sliderPos = sliderPosition();
        if (sliderPos != 0.0)
        {
            QRect r = textRect.adjusted(-2, -1, 2, 1);
            QRect sliderOverlayRect(r.left(), r.top(), int(r.width() * sliderPos), r.height());
            QColor sliderOverlayColor = interpolateColor(tree->palette().color(QPalette::Window), tree->palette().color(QPalette::Highlight), tree->treeStyle().sliderSaturation);
            sliderOverlayColor.setAlpha(192);
            painter->setBrush(QBrush(sliderOverlayColor));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(sliderOverlayRect, 1, 1);

            if (pressed_) {
                painter->setPen(QColor(255, 255, 255));
                painter->setBrush(QBrush(QColor(255, 255, 255)));
                painter->drawLine(sliderOverlayRect.right(), sliderOverlayRect.top(), sliderOverlayRect.right(), sliderOverlayRect.bottom());
                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->translate(0.5f, 0.5f);
                int r2 = sliderOverlayRect.right();
                int t = sliderOverlayRect.top();
                int h = sliderOverlayRect.height();
                QPoint points[3] = {
                    QPoint(r2 - 1 - h / 8 - h / 3, t + h / 2),
                    QPoint(r2 - 1 - h / 8, t + h * 1 / 4),
                    QPoint(r2 - 1 - h / 8, t + h * 3 / 4)
                };
                QPoint pointsR[3] = {
                    QPoint(r2 + 1 + h / 8 + h / 3, t + h / 2),
                    QPoint(r2 + 1 + h / 8, t + h * 1 / 4),
                    QPoint(r2 + 1 + h / 8, t + h * 3 / 4)
                };
                painter->drawPolygon(points, 3);
                painter->drawPolygon(pointsR, 3);
                painter->setRenderHint(QPainter::Antialiasing, false);
                painter->translate(-0.5f, -0.5f);
            }
        }

        painter->setPen(QPen(widgetForContext.palette().color(QPalette::WindowText)));
        painter->setBrush(QBrush(widgetForContext.palette().color(QPalette::Base)));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, QString(valueAsString().c_str()));


    }
}

QCursor createSliderHoverCursor()
{
    QCursor arrow(Qt::ArrowCursor);
    QPoint hotSpot = arrow.hotSpot();
    static QImage image = arrow.pixmap().toImage();
    if (image.isNull())
        return QCursor(Qt::SizeHorCursor);

    int w = image.width();
    int h = image.height();
    QImage empty(w * 2, h, QImage::Format_ARGB32);
    empty.fill(Qt::transparent);
    QPixmap pixmap = QPixmap::fromImage(empty);
    if (pixmap.isNull())
        return QCursor(Qt::SizeHorCursor);
    QPainter p(&pixmap);
    p.drawImage(image.width() / 2, 0, image);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPoint points[3] = {
        QPoint(w / 2 - w * 2 / 8, h / 2),
        QPoint(w / 2 - w / 8, h * 3 / 8),
        QPoint(w / 2 - w / 8, h * 5 / 8)
    };
    QPoint pointsR[3] = {
        QPoint(w, h * 3 / 8),
        QPoint(w, h * 5 / 8),
        QPoint(w + w / 8, h / 2),
    };
    p.setBrush(QBrush(QColor(255, 255, 255)));
    p.setPen(QPen(QColor(0, 0, 0)));
    p.drawPolygon(points, 3);
    p.drawPolygon(pointsR, 3);
    return QCursor(pixmap, image.width() / 2 + hotSpot.x(), hotSpot.y());
}


void PropertyRowNumberField::onMouseDrag(const PropertyDragEvent& e)
{
    if (!dragStarted_) {
        e.tree->model()->rowAboutToBeChanged(this);
        dragStarted_ = true;
    }
    QSize screenSize = QApplication::desktop()->screenGeometry(e.tree).size();
    float relativeDelta = float(e.totalDelta.x()) / screenSize.width();
    int fieldRectWidth = widgetRect(e.tree).width();
    if (fieldRectWidth < 16)
        fieldRectWidth = aznumeric_cast<int>(e.tree->treeSize().x() * e.tree->valueColumnWidth());
    float valueFieldFraction = fieldRectWidth < FLT_EPSILON ? 0 : float(e.totalDelta.x()) / fieldRectWidth;
    incrementLog(relativeDelta, valueFieldFraction);
    setMultiValue(false);
}

bool PropertyRowNumberField::getHoverInfo(PropertyHoverInfo* hit, const QPoint& cursorPos, const QPropertyTree* tree) const
{
    if (pressed_ && !userReadOnly())
        hit->cursor = QCursor(Qt::BlankCursor);
    else if (widgetRect(tree).contains(cursorPos) && !userReadOnly())
        hit->cursor = QCursor(createSliderHoverCursor());
    hit->toolTip = tooltip_;
    return true;
}

void PropertyRowNumberField::onMouseStill(const PropertyDragEvent& e)
{
    e.tree->model()->callRowCallback(this);
    e.tree->apply(true);
}

bool PropertyRowNumberField::onMouseDown(QPropertyTree* tree, QPoint point, bool& changed)
{
    changed = false;
    if (widgetRect(tree).contains(point) && !userReadOnly()) {
        startIncrement();
        pressed_ = true;
        return true;
    }
    return false;
}

void PropertyRowNumberField::onMouseUp(QPropertyTree* tree, [[maybe_unused]] QPoint point)
{
    tree->unsetCursor();
    pressed_ = false;
    dragStarted_ = false;

    // endIncrement() can cause PropertyRow to be destroy, 
    // so no "this" members should be accessed after the call.
    endIncrement(tree);
}

bool PropertyRowNumberField::onActivate(const PropertyActivationEvent& e)
{
    if (e.reason == e.REASON_RELEASE || e.reason == e.REASON_DOUBLECLICK)
        return e.tree->spawnWidget(this, false);
    return false;
}

int PropertyRowNumberField::widgetSizeMin(const QPropertyTree* tree) const
{
    if (userWidgetSize() >= 0)
        return userWidgetSize();

    if (userWidgetToContent())
        return widthCache_.getOrUpdate(tree, this, 0);
    else
        return 40;
}

// ---------------------------------------------------------------------------

PropertyRowWidgetNumber::PropertyRowWidgetNumber([[maybe_unused]] PropertyTreeModel* model, PropertyRowNumberField* row, QPropertyTree* tree)
    : PropertyRowWidget(row, tree)
    , row_(row)
    , entry_(new QLineEdit())
    , tree_(tree)
{
    entry_->setText(row_->valueAsString().c_str());
    connect(entry_, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
    connect(entry_, &QLineEdit::textChanged, this, [this, tree] {
        QFontMetrics fm(entry_->font());
        int contentWidth = min((int)fm.horizontalAdvance(entry_->text()) + 8, tree->width() - entry_->x());
        if (contentWidth > entry_->width())
            entry_->resize(contentWidth, entry_->height());
    });

    entry_->selectAll();
}


void PropertyRowWidgetNumber::onEditingFinished()
{
    tree_->model()->rowAboutToBeChanged(row());
    string str = entry_->text().toLocal8Bit().data();
    if (row_->setValueFromString(str.c_str()) || row_->multiValue())
        tree_->model()->rowChanged(row());
    else
        tree_->_cancelWidget();
}

void PropertyRowWidgetNumber::commit()
{
    if (entry_)
        onEditingFinished();
}

#include <QPropertyTree/moc_PropertyRowNumberField.cpp>
