// Modifications copyright Amazon.com, Inc. or its affiliates.

/**
*  wWidgets - Lightweight UI Toolkit.
*  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
*                          Alexander Kotliar <alexander.kotliar@gmail.com>
*
*  This code is distributed under the MIT License:
*                          http://www.opensource.org/licenses/MIT
*/


#include "EditorCommon_precompiled.h"
#include "QPropertyTree.h"
#include "PropertyDrawContext.h"
#include "Serialization.h"
#include "Serialization/Decorators/Range.h"
using Serialization::Range;
#include "PropertyTreeModel.h"
#include "QPropertyTreeStyle.h"

#include "PropertyOArchive.h"
#include "PropertyIArchive.h"
#include "Unicode.h"

#include <QRect>
#include <QTimer>
#include <QMimeData>
#include <QMenu>
#include <QMouseEvent>
#include <QScrollBar>
#include <QLineEdit>
#include <QPainter>
#include <QElapsedTimer>
#include "PropertyTreeMenuHandler.h"

#include "MathUtils.h"

#include <QToolTip>
// only for clipboard:
#include <QClipboard>
#include <QApplication>
#include "PropertyRowPointer.h"
#include "PropertyRowContainer.h"
// ^^^
#include "PropertyRowObject.h"
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

using Serialization::SStructs;

void PropertyTreeMenuHandler::onMenuFilter()
{
    tree->startFilter("");
}

void PropertyTreeMenuHandler::onMenuFilterByName()
{
    tree->startFilter(filterName.c_str());
}

void PropertyTreeMenuHandler::onMenuFilterByValue()
{
    tree->startFilter(filterValue.c_str());
}

void PropertyTreeMenuHandler::onMenuFilterByType()
{
    tree->startFilter(filterType.c_str());
}

void PropertyTreeMenuHandler::onMenuUndo()
{
    tree->model()->undo();
}

void PropertyTreeMenuHandler::onMenuRedo()
{
    tree->model()->redo();
}

static PropertyRow* findFirstLeafPulledRow(PropertyRow* row)
{
    if (row->isLeaf() &&
        row->widgetPlacement() != PropertyRow::WIDGET_ICON &&
        row->widgetPlacement() != PropertyRow::WIDGET_NONE)
        return row;

    for (int i = 0; i < row->count(); ++i)
    {
        PropertyRow* child = row->childByIndex(i);
        if (!child)
            continue;
        if (!child->pulledUp())
            continue;
        PropertyRow* leaf = findFirstLeafPulledRow(child);
        if (leaf)
            return leaf;
    }

    return 0;
}

static QMimeData* propertyRowToMimeData(PropertyRow* row, ConstStringList* constStrings)
{
    PropertyRow::setConstStrings(constStrings);
    SharedPtr<PropertyRow> clonedRow(row->clone(constStrings));
    Serialization::BinOArchive oa;
    PropertyRow::setConstStrings(constStrings);
    if (!oa(clonedRow, "row", "Row")) {
        PropertyRow::setConstStrings(0);
        return 0;
    }
    PropertyRow::setConstStrings(0);

    QByteArray byteArray(oa.buffer(), (int)oa.length());
    QMimeData* mime = new QMimeData;
    mime->setData("binary/crypropertytree", byteArray);
    if (clonedRow)
    {
        PropertyRow* textRow = findFirstLeafPulledRow(row);
        if (textRow)
            mime->setText(QString::fromWCharArray(textRow->valueAsWString().c_str()));
    }
    return mime;
}

static bool smartPaste(PropertyRow* dest, SharedPtr<PropertyRow>& source, PropertyTreeModel* model, bool onlyCheck)
{
    bool result = false;
    // content of the pulled container has a priority over the node itself
    PropertyRowContainer* destPulledContainer = static_cast<PropertyRowContainer*>(dest->pulledContainer());
    if ((destPulledContainer && strcmp(destPulledContainer->elementTypeName(), source->typeName()) == 0)) {
        PropertyRow* elementRow = model->defaultType(destPulledContainer->elementTypeName());
        YASLI_ESCAPE(elementRow, return false);
        if (strcmp(elementRow->typeName(), source->typeName()) == 0){
            result = true;
            if (!onlyCheck){
                PropertyRow* dest = elementRow;
                if (dest->isPointer() && !source->isPointer()){
                    PropertyRowPointer* d = static_cast<PropertyRowPointer*>(dest);
                    SharedPtr<PropertyRowPointer> newSourceRoot = static_cast<PropertyRowPointer*>(d->clone(model->constStrings()).get());
                    source->swapChildren(newSourceRoot, model);
                    source = newSourceRoot;
                }
                destPulledContainer->add(source.get());
            }
        }
    }
    else if ((source->isContainer() && dest->isContainer() &&
        strcmp(static_cast<PropertyRowContainer*>(source.get())->elementTypeName(),
        static_cast<PropertyRowContainer*>(dest)->elementTypeName()) == 0) ||
        (!source->isContainer() && !dest->isContainer() && strcmp(source->typeName(), dest->typeName()) == 0)){
        result = true;
        if (!onlyCheck){
            if (dest->isPointer() && !source->isPointer()){
                PropertyRowPointer* d = static_cast<PropertyRowPointer*>(dest);
                SharedPtr<PropertyRowPointer> newSourceRoot = static_cast<PropertyRowPointer*>(d->clone(model->constStrings()).get());
                source->swapChildren(newSourceRoot, model);
                source = newSourceRoot;
            }
            const char* name = dest->name();
            const char* nameAlt = dest->label();
            source->setName(name);
            source->setLabel(nameAlt);
            if (dest->parent())
                dest->parent()->replaceAndPreserveState(dest, source, model);
            else{
                dest->swapChildren(source, model);
                source->clear();
            }
            source->setLabelChanged();
        }
    }
    else if (dest->isContainer()){
        if (model){
            PropertyRowContainer* container = static_cast<PropertyRowContainer*>(dest);
            PropertyRow* elementRow = model->defaultType(container->elementTypeName());
            YASLI_ESCAPE(elementRow, return false);
            if (strcmp(elementRow->typeName(), source->typeName()) == 0){
                result = true;
                if (!onlyCheck){
                    PropertyRow* dest = elementRow;
                    if (dest->isPointer() && !source->isPointer()){
                        PropertyRowPointer* d = static_cast<PropertyRowPointer*>(dest);
                        SharedPtr<PropertyRowPointer> newSourceRoot = static_cast<PropertyRowPointer*>(d->clone(model->constStrings()).get());
                        source->swapChildren(newSourceRoot, model);
                        source = newSourceRoot;
                    }

                    container->add(source.get());
                }
            }
            container->setLabelChanged();
        }
    }

    return result;
}

static bool propertyRowFromMimeData(SharedPtr<PropertyRow>& row, const QMimeData* mimeData, ConstStringList* constStrings)
{
    PropertyRow::setConstStrings(constStrings);
    QStringList formats = mimeData->formats();
    QByteArray array = mimeData->data("binary/crypropertytree");
    if (array.isEmpty())
        return 0;
    Serialization::BinIArchive ia;
    if (!ia.open(array.data(), array.size()))
        return 0;

    if (!ia(row, "row", "Row"))
        return false;

    PropertyRow::setConstStrings(0);
    return true;

}

bool propertyRowFromClipboard(SharedPtr<PropertyRow>& row, ConstStringList* constStrings)
{
    const QMimeData* mime = QApplication::clipboard()->mimeData();
    if (!mime)
        return false;
    return propertyRowFromMimeData(row, mime, constStrings);
}

void PropertyTreeMenuHandler::onMenuCopy()
{
    QMimeData* mime = propertyRowToMimeData(row, tree->model()->constStrings());
    if (mime)
        QApplication::clipboard()->setMimeData(mime);
}

void PropertyTreeMenuHandler::onMenuPaste()
{
    if (!tree->canBePasted(row))
        return;
    PropertyRow* parent = row->parent();

    tree->model()->rowAboutToBeChanged(row);

    SharedPtr<PropertyRow> source;
    if (!propertyRowFromClipboard(source, tree->model()->constStrings()))
        return;

    if (!smartPaste(row, source, tree->model(), false))
        return;

    tree->model()->rowChanged(parent ? parent : tree->model()->root());
}

class FilterEntry : public QLineEdit
{
public:
    FilterEntry(QPropertyTree* tree)
        : QLineEdit(tree)
        , tree_(tree)
    {
    }
protected:

    void keyPressEvent(QKeyEvent * ev)
    {
        if (ev->key() == Qt::Key_Escape || ev->key() == Qt::Key_Return)
        {
            ev->accept();
            tree_->setFocus();
            tree_->keyPressEvent(ev);
        }

        if (ev->key() == Qt::Key_Backspace && text().isEmpty())
        {
            tree_->setFilterMode(false);
        }
        QLineEdit::keyPressEvent(ev);
    }
private:
    QPropertyTree* tree_;
};

// ---------------------------------------------------------------------------

DragWindow::DragWindow(QPropertyTree* tree)
    : tree_(tree)
    , offset_(0, 0)
{
    QWidget::setWindowFlags(Qt::ToolTip);
    QWidget::setWindowOpacity(192.0f / 256.0f);
}

void DragWindow::set(QPropertyTree* tree, PropertyRow* row, const QRect& rowRect)
{
    QRect rect = tree->rect();
    rect.setTopLeft(tree->mapToGlobal(rect.topLeft()));

    offset_ = rect.topLeft();

    row_ = row;
    rect_ = rowRect;
}

void DragWindow::setWindowPos([[maybe_unused]] bool visible)
{
    QWidget::move(rect_.left() + offset_.x() - 3, rect_.top() + offset_.y() - 3 + tree_->area_.top());
    QWidget::resize(rect_.width() + 5, rect_.height() + 5);
}

void DragWindow::show()
{
    setWindowPos(true);
    QWidget::show();
}

void DragWindow::move(int deltaX, int deltaY)
{
    offset_ += QPoint(deltaX, deltaY);
    setWindowPos(isVisible());
}

void DragWindow::hide()
{
    setWindowPos(false);
    QWidget::hide();
}

struct DrawRowVisitor
{
    DrawRowVisitor(QPainter& painter) : painter_(painter) {}

    ScanResult operator()(PropertyRow* row, QPropertyTree* tree, int index)
    {
        if (row->pulledUp() && row->visible(tree)) {
            row->drawRow(painter_, tree, index, true);
            row->drawRow(painter_, tree, index, false);
        }

        return SCAN_CHILDREN_SIBLINGS;
    }

protected:
    QPainter& painter_;
};

void DragWindow::drawRow(QPainter& p)
{
    QRect entireRowRect(0, 0, rect_.width() + 4, rect_.height() + 4);

    p.setBrush(tree_->palette().button());
    p.setPen(QPen(tree_->palette().color(QPalette::WindowText)));
    p.drawRect(entireRowRect);

    QPoint leftTop = row_->rect().topLeft();
    int offsetX = aznumeric_cast<int>(-leftTop.x() - tree_->treeStyle().firstLevelIndent * tree_->_defaultRowHeight() + 3);
    int offsetY = -leftTop.y() + 3;
    p.translate(offsetX, offsetY);
    int rowIndex = 0;
    if (row_->parent())
        rowIndex = row_->parent()->childIndex(row_);
    row_->drawRow(p, tree_, 0, true);
    row_->drawRow(p, tree_, 0, false);
    DrawRowVisitor visitor(p);
    row_->scanChildren(visitor, tree_);
    p.translate(-offsetX, -offsetY);
}

void DragWindow::paintEvent([[maybe_unused]] QPaintEvent* ev)
{
    QPainter p(this);

    drawRow(p);
}

// ---------------------------------------------------------------------------

class QPropertyTree::DragController
{
public:
    DragController(QPropertyTree* tree)
        : tree_(tree)
        , captured_(false)
        , dragging_(false)
        , before_(false)
        , row_(0)
        , clickedRow_(0)
        , window_(tree)
        , hoveredRow_(0)
        , destinationRow_(0)
    {
    }

    void beginDrag(PropertyRow* clickedRow, PropertyRow* draggedRow, QPoint pt)
    {
        row_ = draggedRow;
        clickedRow_ = clickedRow;
        startPoint_ = pt;
        lastPoint_ = pt;
        captured_ = true;
        dragging_ = false;
    }

    bool dragOn(QPoint screenPoint)
    {
        if (dragging_)
            window_.move(screenPoint.x() - lastPoint_.x(), screenPoint.y() - lastPoint_.y());

        bool needCapture = false;
        if (!dragging_ && (startPoint_ - screenPoint).manhattanLength() >= 5)
            if (row_->canBeDragged()){
                needCapture = true;
                QRect rect = row_->rect();
                rect = QRect(rect.topLeft() - tree_->offset_ + QPoint(aznumeric_cast<int>(tree_->treeStyle().firstLevelIndent * tree_->_defaultRowHeight()), 0),
                    rect.bottomRight() - tree_->offset_);

                window_.set(tree_, row_, rect);
                window_.move(screenPoint.x() - startPoint_.x(), screenPoint.y() - startPoint_.y());
                window_.show();
                dragging_ = true;
            }

        if (dragging_){
            QPoint point = tree_->mapFromGlobal(screenPoint);
            trackRow(point);
        }
        lastPoint_ = screenPoint;
        return needCapture;
    }

    void interrupt()
    {
        captured_ = false;
        dragging_ = false;
        row_ = 0;
        window_.hide();
    }

    void trackRow(QPoint pt)
    {
        hoveredRow_ = 0;
        destinationRow_ = 0;

        QPoint point = pt;
        PropertyRow* row = tree_->rowByPoint(point);
        if (!row || !row_)
            return;

        row = row->nonPulledParent();
        if (!row->parent() || row->isChildOf(row_) || row == row_)
            return;

        float pos = (point.y() - row->rect().top()) / float(row->rect().height());
        if (row_->canBeDroppedOn(row->parent(), row, tree_)){
            if (pos < 0.25f){
                destinationRow_ = row->parent();
                hoveredRow_ = row;
                before_ = true;
                return;
            }
            if (pos > 0.75f){
                destinationRow_ = row->parent();
                hoveredRow_ = row;
                before_ = false;
                return;
            }
        }
        if (row_->canBeDroppedOn(row, 0, tree_))
            hoveredRow_ = destinationRow_ = row;
    }

    void drawUnder(QPainter& painter)
    {
        if (dragging_ && destinationRow_ == hoveredRow_ && hoveredRow_){
            QRect rowRect = hoveredRow_->rect();
            rowRect.setLeft(aznumeric_cast<int>(rowRect.left() + tree_->treeStyle().firstLevelIndent * tree_->_defaultRowHeight()));
            QBrush brush(true ? tree_->palette().highlight() : tree_->palette().shadow());
            QColor brushColor = brush.color();
            QColor borderColor(brushColor.alpha() / 4, brushColor.red(), brushColor.green(), brushColor.blue());
            fillRoundRectangle(painter, brush, rowRect, borderColor, 6);
        }
    }

    void drawOver(QPainter& painter)
    {
        if (!dragging_)
            return;

        QRect rowRect = row_->rect();

        if (destinationRow_ != hoveredRow_ && hoveredRow_){
            const int tickSize = 4;
            QRect hoveredRect = hoveredRow_->rect();
            hoveredRect.setLeft(aznumeric_cast<int>(hoveredRect.left() + tree_->treeStyle().firstLevelIndent * tree_->_defaultRowHeight()));

            if (!before_){ // previous
                QRect rect(hoveredRect.left() - 1, hoveredRect.bottom() - 1, hoveredRect.width(), 2);
                QRect rectLeft(hoveredRect.left() - 1, hoveredRect.bottom() - tickSize, 2, tickSize * 2);
                QRect rectRight(hoveredRect.right() - 1, hoveredRect.bottom() - tickSize, 2, tickSize * 2);
                painter.fillRect(rect, tree_->palette().highlight());
                painter.fillRect(rectLeft, tree_->palette().highlight());
                painter.fillRect(rectRight, tree_->palette().highlight());
            }
            else{ // next
                QRect rect(hoveredRect.left() - 1, hoveredRect.top() - 1, hoveredRect.width(), 2);
                QRect rectLeft(hoveredRect.left() - 1, hoveredRect.top() - tickSize, 2, tickSize * 2);
                QRect rectRight(hoveredRect.right() - 1, hoveredRect.top() - tickSize, 2, tickSize * 2);
                painter.fillRect(rect, tree_->palette().highlight());
                painter.fillRect(rectLeft, tree_->palette().highlight());
                painter.fillRect(rectRight, tree_->palette().highlight());
            }
        }
    }

    bool drop([[maybe_unused]] QPoint screenPoint)
    {
        bool rowLayoutChanged = false;
        if (row_ && hoveredRow_){
            YASLI_ASSERT(destinationRow_);
            clickedRow_->setSelected(false);
            row_->dropInto(destinationRow_, destinationRow_ == hoveredRow_ ? 0 : hoveredRow_, tree_, before_);
            rowLayoutChanged = true;
        }

        captured_ = false;
        dragging_ = false;
        row_ = 0;
        window_.hide();
        hoveredRow_ = 0;
        destinationRow_ = 0;
        return rowLayoutChanged;
    }

    bool captured() const{ return captured_; }
    bool dragging() const{ return dragging_; }
    PropertyRow* draggedRow() { return row_; }
protected:
    DragWindow window_;
    QPropertyTree* tree_;
    PropertyRow* row_;
    PropertyRow* clickedRow_;
    PropertyRow* hoveredRow_;
    PropertyRow* destinationRow_;
    QPoint startPoint_;
    QPoint lastPoint_;
    bool captured_;
    bool dragging_;
    bool before_;
};

// ---------------------------------------------------------------------------

AZ_PUSH_DISABLE_WARNING(4335, "-Wunknown-warning-option")
QPropertyTree::QPropertyTree(QWidget* parent)
    : QWidget(parent)
    , sizeHint_(180, 180)
    , model_(0)
    , cursorX_(0)
    , attachedPropertyTree_(0)
    , autoHideAttachedPropertyTree_(false)
    , autoRevert_(true)
    , dragController_(new DragController(this))
    , leftBorder_(0)
    , rightBorder_(0)
    , filterMode_(false)

    , applyTime_(0)
    , revertTime_(0)
    , updateHeightsTime_(0)
    , paintTime_(0)
    , pressPoint_(-1, -1)
    , pressDelta_(0, 0)
    , pointerMovedSincePress_(false)
    , lastStillPosition_(-1, -1)
    , pressedRow_(0)
    , capturedRow_(0)
    , iconCache_(new IconXPMCache())
    , dragCheckMode_(false)
    , dragCheckValue_(false)
    , archiveContext_(0)
    , outlineMode_(false)
    , sizeToContent_(false)
    , hideSelection_(false)
    , zoomLevel_(10)
    , validatorBlock_(new ValidatorBlock)
    , style_(new QPropertyTreeStyle())

    , aggregateMouseEvents_(false)
    , aggregatedMouseEventCount_(0)
{
    setFocusPolicy(Qt::WheelFocus);
    setMouseTracking(true); // need to receive mouseMoveEvent to update mouse cursor and tooltip

    scrollBar_ = new QScrollBar(Qt::Vertical, this);
    connect(scrollBar_, SIGNAL(valueChanged(int)), this, SLOT(onScroll(int)));

    model_.reset(new PropertyTreeModel());
    model_->setExpandLevels(config_.expandLevels);
    model_->setUndoEnabled(config_.undoEnabled);
    model_->setFullUndo(config_.fullUndo);

    connect(model_.data(), SIGNAL(signalUpdated(const PropertyRows&, bool)), this, SLOT(onModelUpdated(const PropertyRows&, bool)));
    connect(model_.data(), SIGNAL(signalPushUndo(PropertyTreeOperator*, bool*)), this, SLOT(onModelPushUndo(PropertyTreeOperator*, bool*)));
    connect(model_.data(), SIGNAL(signalPushRedo(PropertyTreeOperator*, bool*)), this, SLOT(onModelPushRedo(PropertyTreeOperator*, bool*)));
    //model_->signalPushUndo().connect(this, &QPropertyTree::onModelPushUndo);

    filterEntry_.reset(new FilterEntry(this));
    QObject::connect(filterEntry_.data(), SIGNAL(textChanged(const QString&)), this, SLOT(onFilterChanged(const QString&)));
    filterEntry_->hide();

    mouseStillTimer_ = new QTimer(this);
    mouseStillTimer_->setSingleShot(true);
    connect(mouseStillTimer_, SIGNAL(timeout()), this, SLOT(onMouseStillTimeout()));

    boldFont_.setBold(true);
    backgroundColor_ = palette().color(QPalette::Window);
}
AZ_POP_DISABLE_WARNING

QPropertyTree::~QPropertyTree()
{
    clearMenuHandlers();
}

bool QPropertyTree::onRowKeyDown(PropertyRow* row, const QKeyEvent* ev)
{
    PropertyTreeMenuHandler handler;
    handler.row = row;
    handler.tree = this;

    if (row->onKeyDown(this, ev))
        return true;
    if (row->pulledContainer() && static_cast<PropertyRowContainer*>(row->pulledContainer())->onKeyDownContainer(this, ev))
        return true;

    // NOTE: If you add a new key here, you also have to check for it in rowProcessesKey

    switch (ev->key()){
    case Qt::Key_C:
        if (!row->userNonCopyable() && ev->modifiers() == Qt::CTRL)
            handler.onMenuCopy();
        return true;
    case Qt::Key_V:
        if (!row->userNonCopyable() && ev->modifiers() == Qt::CTRL)
            handler.onMenuPaste();
        return true;
    case Qt::Key_Z:
        if (config_.undoEnabled)
        {
            if (ev->modifiers() == (Qt::SHIFT | Qt::CTRL))
            {
                if (model()->canRedo())
                {
                    handler.onMenuRedo();
                }
                return true;
            }
            else if (ev->modifiers() == Qt::CTRL)
            {
                if (model()->canUndo())
                {
                    handler.onMenuUndo();
                }
                return true;
            }
        }
        else
        {
            if (ev->modifiers() == (Qt::SHIFT | Qt::CTRL))
            {
                emit signalRedo();
            }
            else if (ev->modifiers() == Qt::CTRL)
            {
                emit signalUndo();
            }

            return true;
        }
        break;
    case Qt::Key_Y:
        if (!config_.undoEnabled)
        {
            if (model()->canRedo())
            {
                handler.onMenuRedo();
            }
        }
        else
        {
            if (ev->modifiers() == Qt::CTRL)
            {
                emit signalRedo();
            }
        }
        return true;
        break;

    case Qt::Key_F2:
        if (ev->modifiers() == Qt::NoModifier) {
            if (selectedRow()) {
                PropertyActivationEvent act;
                act.tree = this;
                act.force = true;
                act.reason = PropertyActivationEvent::REASON_KEYBOARD;
                selectedRow()->onActivate(act);
            }
        }
        break;
    case Qt::Key_Menu:
    {
        if (ev->modifiers() == Qt::NoModifier) {
            QMenu menu(this);

            if (onContextMenu(row, menu)){
                QRect rect(row->rect());
                QPoint pt = _toScreen(QPoint(rect.left() + rect.height(), rect.bottom()));
                menu.exec(pt);
            }
            return true;
        }
        break;
    }
    }

    PropertyRow* focusedRow = model()->focusedRow();
    if (!focusedRow)
        return false;
    PropertyRow* parentRow = focusedRow->nonPulledParent();
    int x = parentRow->horizontalIndex(this, focusedRow);
    int y = model()->root()->verticalIndex(this, parentRow);
    PropertyRow* selectedRow = 0;
    switch (ev->key()){
    case Qt::Key_Up:
        if (filterMode_ && y == 0) {
            setFilterMode(true);
        }
        else {
            selectedRow = model()->root()->rowByVerticalIndex(this, --y);
            if (selectedRow)
                selectedRow = selectedRow->rowByHorizontalIndex(this, cursorX_);
        }
        break;
    case Qt::Key_Down:
        if (filterMode_ && filterEntry_->hasFocus()) {
            setFocus();
        }
        else {
            selectedRow = model()->root()->rowByVerticalIndex(this, ++y);
            if (selectedRow)
                selectedRow = selectedRow->rowByHorizontalIndex(this, cursorX_);
        }
        break;
    case Qt::Key_Left:
        selectedRow = parentRow->rowByHorizontalIndex(this, cursorX_ = --x);
        if (selectedRow == focusedRow && parentRow->canBeToggled(this) && parentRow->expanded()){
            expandRow(parentRow, false);
            selectedRow = model()->focusedRow();
        }
        break;
    case Qt::Key_Right:
        selectedRow = parentRow->rowByHorizontalIndex(this, cursorX_ = ++x);
        if (selectedRow == focusedRow && parentRow->canBeToggled(this) && !parentRow->expanded()){
            expandRow(parentRow, true);
            selectedRow = model()->focusedRow();
        }
        break;
    case Qt::Key_Home:
        if (ev->modifiers() == Qt::CTRL) {
            selectedRow = parentRow->rowByHorizontalIndex(this, cursorX_ = INT_MIN);
        }
        else {
            selectedRow = model()->root()->rowByVerticalIndex(this, 0);
            if (selectedRow)
                selectedRow = selectedRow->rowByHorizontalIndex(this, cursorX_);
        }
        break;
    case Qt::Key_End:
        if (ev->modifiers() == Qt::CTRL) {
            selectedRow = parentRow->rowByHorizontalIndex(this, cursorX_ = INT_MAX);
        }
        else {
            selectedRow = model()->root()->rowByVerticalIndex(this, INT_MAX);
            if (selectedRow)
                selectedRow = selectedRow->rowByHorizontalIndex(this, cursorX_);
        }
        break;
    case Qt::Key_Space:
        if (config_.filterWhenType)
            break;
    case Qt::Key_Return:
        if (focusedRow->canBeToggled(this))
            expandRow(focusedRow, !focusedRow->expanded());
        else {
            PropertyActivationEvent e;
            e.tree = this;
            e.reason = e.REASON_KEYBOARD;
            e.force = false;
            focusedRow->onActivate(e);
        }
        break;
    }
    if (selectedRow){
        onRowSelected(std::vector<PropertyRow*>(1, selectedRow), false, false);
        return true;
    }
    return false;
}

bool QPropertyTree::rowProcessesKey(PropertyRow* row, const QKeyEvent* ev)
{
    if (row->processesKey(this, ev))
    {
        return true;
    }

    if (row->pulledContainer() && static_cast<PropertyRowContainer*>(row->pulledContainer())->processesKeyContainer(this, ev))
    {
        return true;
    }

    int modifiedKey = ev->key() | ev->modifiers();

    switch (modifiedKey)
    {
        case (Qt::CTRL | Qt::Key_Z):
        case (Qt::CTRL | Qt::SHIFT | Qt::Key_Z) :
        case Qt::Key_Y:
        case (Qt::CTRL | Qt::Key_V):
        case (Qt::CTRL | Qt::Key_C):
        case (Qt::CTRL | Qt::Key_F):
        case Qt::Key_Menu:
        case Qt::Key_F2:
            return true;
            break;

        default:
            break;
    }

    switch (ev->key())
    {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Return:
            return true;
            break;

        default:
            break;
    }

    return false;
}

struct FirstIssueVisitor
{
    ValidatorEntryType entryType_;
    PropertyRow* startRow_;
    PropertyRow* result;

    FirstIssueVisitor(ValidatorEntryType type, PropertyRow* startRow)
        : entryType_(type)
        , startRow_(startRow)
        , result()
    {
    }

    ScanResult operator()(PropertyRow* row, QPropertyTree* tree, int)
    {
        if ((row->pulledUp() || row->pulledBefore()) && row->nonPulledParent() == startRow_)
            return SCAN_SIBLINGS;
        if (row->validatorCount()) {
            if (const ValidatorEntry* validatorEntries = tree->_validatorBlock()->GetEntry(row->validatorIndex(), row->validatorCount())) {
                for (int i = 0; i < row->validatorCount(); ++i) {
                    const ValidatorEntry* validatorEntry = validatorEntries + i;
                    if (validatorEntry->type == entryType_) {
                        result = row;
                        return SCAN_FINISHED;
                    }
                }
            }
        }
        return SCAN_CHILDREN_SIBLINGS;
    }
};

void QPropertyTree::jumpToNextHiddenValidatorIssue(bool isError, PropertyRow* start)
{
    FirstIssueVisitor op(isError ? VALIDATOR_ENTRY_ERROR : VALIDATOR_ENTRY_WARNING, start);
    start->scanChildren(op, this);

    PropertyRow* row = op.result;

    vector<PropertyRow*> parents;
    while (row && row->parent())  {
        parents.push_back(row);
        row = row->parent();
    }
    for (int i = (int)parents.size() - 1; i >= 0; --i) {
        if (!parents[i]->visible(this))
            break;
        row = parents[i];
    }
    if (row)
        setSelectedRow(row);

    updateValidatorIcons();
    updateHeights();
}

static void rowsInBetween(vector<PropertyRow*>* rows, PropertyRow* a, PropertyRow* b)
{
    if (!a)
        return;
    if (!b)
        return;
    vector<PropertyRow*> pathA;
    PropertyRow* rootA = a;
    while (rootA->parent()) {
        pathA.push_back(rootA);
        rootA = rootA->parent();
    }

    vector<PropertyRow*> pathB;
    PropertyRow* rootB = b;
    while (rootB->parent()) {
        pathB.push_back(rootB);
        rootB = rootB->parent();
    }

    if (rootA != rootB)
        return;

    const PropertyRow* commonParent = rootA;
    int maxDepth = min((int)pathA.size(), (int)pathB.size());
    for (int i = 0; i < maxDepth; ++i) {
        PropertyRow* parentA = pathA[(int)pathA.size() - 1 - i];
        PropertyRow* parentB = pathB[(int)pathB.size() - 1 - i];
        if (parentA != parentB) {
            int indexA = commonParent->childIndex(parentA);
            int indexB = commonParent->childIndex(parentB);
            int minIndex = min(indexA, indexB);
            int maxIndex = max(indexA, indexB);
            for (int j = minIndex; j <= maxIndex; ++j)
                rows->push_back((PropertyRow*)commonParent->childByIndex(j));
            return;
        }
        commonParent = parentA;
    }
}

bool QPropertyTree::onRowLMBDown(PropertyRow* row, [[maybe_unused]] const QRect& rowRect, QPoint point, bool controlPressed, bool shiftPressed)
{
    pressPoint_ = point;
    pressDelta_ = QPoint(0, 0);
    pointerMovedSincePress_ = false;
    row = model()->root()->hit(this, point);
    if (row){
        if (!row->isRoot()) {
            if (row->plusRect(this).contains(point) && toggleRow(row))
                return true;
            if (row->validatorWarningIconRect(this).contains(point)) {
                jumpToNextHiddenValidatorIssue(false, row);
                return true;
            }
            if (row->validatorErrorIconRect(this).contains(point)) {
                jumpToNextHiddenValidatorIssue(true, row);
                return true;
            }
        }

        PropertyRow* rowToSelect = row;
        while (rowToSelect && !rowToSelect->isSelectable())
            rowToSelect = rowToSelect->parent();

        if (rowToSelect) {
            if (!shiftPressed || !multiSelectable()) {
                onRowSelected(std::vector<PropertyRow*>(1, rowToSelect), multiSelectable() && controlPressed, true);
                lastSelectedRow_ = rowToSelect;
            }
            else {
                vector<PropertyRow*> rowsToSelect;

                rowsInBetween(&rowsToSelect, lastSelectedRow_, rowToSelect);
                onRowSelected(rowsToSelect, false, true);
            }
        }
    }

    PropertyTreeModel::UpdateLock lock = model()->lockUpdate();
    row = model()->root()->hit(this, point);
    if (row && !row->isRoot()){
        bool changed = false;
        if (row->widgetRect(this).contains(point)) {
            DragCheckBegin dragCheck = row->onMouseDragCheckBegin();
            if (dragCheck != DRAG_CHECK_IGNORE) {
                dragCheckValue_ = dragCheck == DRAG_CHECK_SET;
                dragCheckMode_ = true;
                changed = row->onMouseDragCheck(this, dragCheckValue_);
            }
        }

        if (!dragCheckMode_) {
            bool capture = row->onMouseDown(this, point, changed);
            if (!changed){
                if (capture)
                    return true;
                else if (row->widgetRect(this).contains(point)){
                    if (row->widgetPlacement() != PropertyRow::WIDGET_ICON)
                        interruptDrag();
                    PropertyActivationEvent e;
                    e.force = false;
                    e.tree = this;
                    e.clickPoint = point;
                    row->onActivate(e);
                    return false;
                }
            }
        }
    }
    return false;
}

void QPropertyTree::onRowLMBUp(PropertyRow* row, [[maybe_unused]] const QRect& rowRect, QPoint point)
{
    onMouseStill(point);
    row->onMouseUp(this, point);

    if (!pointerMovedSincePress_ && (pressPoint_ - point).manhattanLength() < 1 && row->widgetRect(this).contains(point)) {
        PropertyActivationEvent e;
        e.tree = this;
        e.clickPoint = point;
        e.reason = e.REASON_RELEASE;
        row->onActivate(e);
    }
}

void QPropertyTree::onRowRMBDown(PropertyRow* row, [[maybe_unused]] const QRect& rowRect, QPoint point)
{
    SharedPtr<PropertyRow> handle = row;
    PropertyRow* menuRow = 0;

    if (row->isSelectable()){
        menuRow = row;
    }
    else{
        if (row->parent() && row->parent()->isSelectable())
            menuRow = row->parent();
    }

    if (menuRow) {
        onRowSelected(std::vector<PropertyRow*>(1, menuRow), false, true);
        QMenu menu(this);
        clearMenuHandlers();
        if (onContextMenu(menuRow, menu))
            menu.exec(point);
    }
}

void QPropertyTree::expandParents(PropertyRow* row)
{
    bool hasChanges = false;
    typedef std::vector<PropertyRow*> Parents;
    Parents parents;
    PropertyRow* p = row->nonPulledParent()->parent();
    while (p){
        parents.push_back(p);
        p = p->parent();
    }
    Parents::iterator it;
    for (it = parents.begin(); it != parents.end(); ++it) {
        PropertyRow* row = *it;
        row->_setExpanded(true);
        hasChanges = true;
    }
    if (hasChanges) {
        updateValidatorIcons();
        updateHeights();
    }
}


void QPropertyTree::expandAll(PropertyRow* root)
{
    if (!root){
        root = model()->root();
        for (PropertyRows::iterator it = root->begin(); it != root->end(); ++it){
            PropertyRow* row = *it;
            row->setExpandedRecursive(this, true);
        }
        root->setLayoutChanged();
    }
    else
        root->setExpandedRecursive(this, true);

    for (PropertyRow* r = root; r != 0; r = r->parent())
        r->setLayoutChanged();

    updateHeights();
}

void QPropertyTree::collapseAll(PropertyRow* root)
{
    if (!root){
        root = model()->root();

        for (PropertyRows::iterator it = root->begin(); it != root->end(); ++it){
            PropertyRow* row = *it;
            row->setExpandedRecursive(this, false);
        }
    }
    else{
        root->setExpandedRecursive(this, false);
        PropertyRow* row = model()->focusedRow();
        while (row){
            if (root == row){
                model()->selectRow(row, true);
                break;
            }
            row = row->parent();
        }
    }

    for (PropertyRow* r = root; r != 0; r = r->parent())
        r->setLayoutChanged();

    updateHeights();
}


void QPropertyTree::expandRow(PropertyRow* row, bool expanded, bool updateHeights)
{
    bool hasChanges = false;
    if (row->expanded() != expanded) {
        row->_setExpanded(expanded);
        hasChanges = true;
    }

    for (PropertyRow* r = row; r != 0; r = r->parent())
        r->setLayoutChanged();

    if (!row->expanded()){
        PropertyRow* f = model()->focusedRow();
        while (f){
            if (row == f){
                model()->selectRow(row, true);
                break;
            }
            f = f->parent();
        }
    }

    if (hasChanges)
        updateValidatorIcons();
    if (hasChanges && updateHeights)
        this->updateHeights();
}

void QPropertyTree::interruptDrag()
{
    dragController_->interrupt();
}

void QPropertyTree::updateHeights(bool recalculateTextSize)
{
    QFontMetrics fm(font());
    defaultRowHeight_ = max(16, int(fm.lineSpacing() * 1.666f)); // to fit at least 16x16 icons

    QElapsedTimer timer;
    timer.start();

    model()->root()->updateLabel(this, 0, false);

    QRect widgetRect = this->rect();

    int scrollBarW = 16;
    int lb = 1;
    int rb = widgetRect.right() - lb - scrollBarW - 2;
    int availableWidth = widgetRect.width() - 4 - scrollBarW;
    bool force = recalculateTextSize || lb != leftBorder_ || rb != rightBorder_;
    leftBorder_ = lb;
    rightBorder_ = rb;
    model()->root()->calculateMinimalSize(this, leftBorder_, availableWidth, force, 0, 0, 0);

    updateValidatorIcons();

    int totalHeight = 0;
    model()->root()->adjustVerticalPosition(this, totalHeight);
    totalHeight += 4;
    QPoint oldSize = size_;
    size_.setY(totalHeight);

    updateScrollBar();

    area_.setLeft(widgetRect.left() + 2);
    area_.setRight(widgetRect.right() - 2 - scrollBarW);
    area_.setTop(widgetRect.top() + 2);
    area_.setBottom(widgetRect.bottom() - 2);
    size_.setX(area_.width());

    int filterAreaHeight = 0;
    if (filterMode_)
    {
        filterAreaHeight = filterEntry_ ? filterEntry_->height() : 0;
        area_.setTop(area_.top() + filterAreaHeight + 2 + 2);
    }

    _arrangeChildren();

    int contentHeight = totalHeight + filterAreaHeight + 4;
    if (sizeToContent_)
    {
        setMaximumHeight(contentHeight);
        setMinimumHeight(contentHeight);
    }
    else
    {
        setMaximumHeight(QWIDGETSIZE_MAX);
        setMinimumHeight(0);
    }

    update();
    updateHeightsTime_ = aznumeric_cast<int>(timer.elapsed());

    QSize contentSize = QSize(area_.width(), contentHeight);
    if (contentSize_.height() != contentSize.height())
    {
        contentSize_ = contentSize;
        signalSizeChanged();
    }
    else
    {
        contentSize_ = contentSize;
    }
}

void QPropertyTree::setSizeToContent(bool sizeToContent)
{
    if (sizeToContent != sizeToContent_)
    {
        sizeToContent_ = sizeToContent;
        updateHeights();
    }
}


bool QPropertyTree::updateScrollBar()
{
    int pageSize = rect().height();
    offset_.setX(max(0, min(offset_.x(), max(0, size_.x() - area_.right() - 1))));
    offset_.setY(max(0, min(offset_.y(), max(0, size_.y() - pageSize))));

    if (pageSize < size_.y())
    {
        scrollBar_->setRange(0, size_.y() - pageSize);
        scrollBar_->setSliderPosition(offset_.y());
        scrollBar_->setPageStep(pageSize);
        scrollBar_->show();
        scrollBar_->move(rect().right() - scrollBar_->width(), 0);
        scrollBar_->resize(scrollBar_->width(), height());
        return true;
    }
    else
    {
        scrollBar_->hide();
        return false;
    }
}

QPoint QPropertyTree::treeSize() const
{
    return size_ + (compact() ? QPoint(0, 0) : QPoint(8, 8));
}

void QPropertyTree::onScroll([[maybe_unused]] int pos)
{
    offset_.setY(scrollBar_->sliderPosition());
    _arrangeChildren();
    repaint();
}

void QPropertyTree::Serialize(IArchive& ar)
{
    model()->Serialize(ar, this);

    if (ar.IsInput()){
        ensureVisible(model()->focusedRow());
        updateAttachedPropertyTree(false);
        updateHeights();
        signalSelected();
    }
}

void QPropertyTree::ensureVisible(PropertyRow* row, bool update, bool considerChildren)
{
    if (row == 0)
        return;
    if (row->isRoot())
        return;

    expandParents(row);

    QRect rect = considerChildren ? row->rectIncludingChildren(this) : row->rect();
    if (rect.bottom() > area_.bottom() + offset_.y()){
        offset_.setY(max(0, rect.bottom() - area_.bottom()));
    }
    if (rect.top() < area_.top() + offset_.y()){
        offset_.setY(max(0, rect.top() - area_.top()));
    }
    updateScrollBar();
    if (update)
        this->update();
}

void QPropertyTree::onRowSelected(const std::vector<PropertyRow*>& rows, bool addSelection, bool adjustCursorPos)
{
    for (size_t i = 0; i < rows.size(); ++i) {
        PropertyRow* row = rows[i];
        if (!row->isRoot()) {
            bool addRowToSelection = !(addSelection && row->selected() && model()->selection().size() > 1) || i > 0;
            bool exclusiveSelection = !addSelection && i == 0;
            model()->selectRow(row, addRowToSelection, exclusiveSelection);
        }
    }
    if (!rows.empty()) {
        ensureVisible(rows.back(), true, false);
        if (adjustCursorPos)
            cursorX_ = rows.back()->nonPulledParent()->horizontalIndex(this, rows.back());
    }
    updateAttachedPropertyTree(false);
    signalSelected();
}

bool QPropertyTree::attach(const Serialization::SStructs& serializers)
{
    bool changed = false;
    if (attached_.size() != serializers.size())
        changed = true;
    else {
        for (size_t i = 0; i < serializers.size(); ++i) {
            if (attached_[i].serializer() != serializers[i]) {
                changed = true;
                break;
            }
        }
    }

    // We can't perform plain copying here, as it was before:
    //   attached_ = serializers;
    // ...as move forwarder calls copying constructor with non-const argument
    // which invokes second templated constructor of Serializer, which is not what we need.
    if (changed) {
        attached_.assign(serializers.begin(), serializers.end());
        model_->clearUndo();
    }

    revertNoninterrupting();

    return changed;
}

void QPropertyTree::attach(const Serialization::SStruct& serializer)
{
    if (attached_.size() != 1 || attached_[0].serializer() != serializer) {
        attached_.clear();
        attached_.push_back(Serialization::Object(serializer));
        model_->clearUndo();
    }
    revert();
}

void QPropertyTree::attach(const Serialization::Object& object)
{
    attached_.clear();
    attached_.push_back(object);

    revert();
}

void QPropertyTree::detach()
{
    if (widget_)
        widget_.reset();
    attached_.clear();
    model()->root()->clear();
    update();
}

int QPropertyTree::revertObjects(vector<void*> objectAddresses)
{
    int result = 0;
    for (size_t i = 0; i < objectAddresses.size(); ++i) {
        if (revertObject(objectAddresses[i]))
            ++result;
    }
    return result;
}

bool QPropertyTree::revertObject(void* objectAddress)
{
    PropertyRow* row = model()->root()->findByAddress(objectAddress);
    if (row && row->isObject()) {
        // TODO:
        // revertObjectRow(row);
        return true;
    }
    return false;
}


void QPropertyTree::revert()
{
    interruptDrag();
    widget_.reset();
    capturedRow_ = 0;

    if (!attached_.empty()) {
        validatorBlock_->Clear();

        QElapsedTimer timer;
        timer.start();

        PropertyOArchive oa(model_.data(), model_->root(), validatorBlock_.data());
        oa.SetOutlineMode(outlineMode_);
        if (archiveContext_)
            oa.SetInnerContext(archiveContext_);
        oa.SetFilter(config_.filter);

        Objects::iterator it = attached_.begin();
        signalAboutToSerialize(oa);
        (*it)(oa);
        signalSerialized(oa);

        PropertyTreeModel model2;
        if (it != attached_.end()) {
            while (++it != attached_.end()){
                PropertyOArchive oa2(&model2, model2.root(), validatorBlock_.data());
                oa2.SetOutlineMode(outlineMode_);
                Serialization::SContext<QPropertyTree> treeContext(oa2, this);
                if (archiveContext_)
                    oa2.SetInnerContext(archiveContext_);
                oa2.SetFilter(config_.filter);
                signalAboutToSerialize(oa2);
                (*it)(oa2);
                signalSerialized(oa2);
                model_->root()->intersect(model2.root());
            }
        }
        revertTime_ = int(timer.elapsed());

        if (attached_.size() != 1)
            validatorBlock_->Clear();
        applyValidation();
    }
    else
        model_->clear();

    if (filterMode_) {
        if (model_->root())
            model_->root()->updateLabel(this, 0, false);
        onFilterChanged(QString());
    }
    else {
        updateHeights();
    }

    update();
    updateAttachedPropertyTree(true);

    signalReverted();
}

struct ValidatorVisitor
{
    ValidatorVisitor(ValidatorBlock* validator)
        : validator_(validator)
    {
    }

    ScanResult operator()(PropertyRow* row, [[maybe_unused]] QPropertyTree* tree, int)
    {
        const void* rowHandle = row->searchHandle();
        int index = 0;
        int count = 0;
        Serialization::TypeID typeID = row->typeId();
        if (validator_->FindHandleEntries(&index, &count, rowHandle, typeID))
        {
            validator_->MarkAsUsed(index, count);
            if (row->setValidatorEntry(index, count))
                row->setLabelChanged();
        }
        else
        {
            if (row->setValidatorEntry(0, 0))
                row->setLabelChanged();
        }

        return SCAN_CHILDREN_SIBLINGS;
    }

protected:
    ValidatorBlock* validator_;
};

void QPropertyTree::applyValidation()
{
    if (!validatorBlock_->IsEnabled())
        return;

    ValidatorVisitor visitor(validatorBlock_.data());
    model()->root()->scanChildren(visitor, this);

    int rootFirst = 0;
    int rootCount = 0;
    Serialization::TypeID typeID = model()->root()->typeId();
    // Gather all the items with unknown handle/type pair at root level.
    validatorBlock_->MergeUnusedItemsWithRootItems(&rootFirst, &rootCount, model()->root()->searchHandle(), typeID);
    model()->root()->setValidatorEntry(rootFirst, rootCount);
    model()->root()->setLabelChanged();
}

void QPropertyTree::revertNoninterrupting()
{
    if (!capturedRow_)
        revert();
}

void QPropertyTree::apply(bool continuousUpdate)
{
    QElapsedTimer timer;
    timer.start();

    if (!attached_.empty()) {
        Objects::iterator it;
        for (it = attached_.begin(); it != attached_.end(); ++it) {
            PropertyIArchive ia(model_.data(), model_->root());
            Serialization::SContext<QPropertyTree> treeContext(ia, this);
            ia.SetFilter(config_.filter);
            if (archiveContext_)
                ia.SetInnerContext(archiveContext_);
            signalAboutToSerialize(ia);
            (*it)(ia);
            signalSerialized(ia);
        }
    }

    if (!continuousUpdate)
        signalChanged();
    else
        signalContinuousChange();
    applyTime_ = aznumeric_cast<int>(timer.elapsed());
}

void QPropertyTree::applyInplaceEditor()
{
    if (widget_)
        widget_->commit();
}

bool QPropertyTree::spawnWidget(PropertyRow* row, bool ignoreReadOnly)
{
    if (!widget_ || widget_->row() != row || !widget_->actualWidget()->isVisible()){
        interruptDrag();
        setWidget(0);
        PropertyRowWidget* newWidget = 0;
        if ((ignoreReadOnly && row->userReadOnlyRecurse()) || !row->userReadOnly())
            newWidget = row->createWidget(this);
        setWidget(newWidget);
        return newWidget != 0;
    }
    return false;
}

void QPropertyTree::addMenuHandler(PropertyRowMenuHandler* handler)
{
    menuHandlers_.push_back(handler);
}

void QPropertyTree::clearMenuHandlers()
{
    for (size_t i = 0; i < menuHandlers_.size(); ++i)
    {
        PropertyRowMenuHandler* handler = menuHandlers_[i];
        delete handler;
    }
    menuHandlers_.clear();
}

static string quoteIfNeeded(const char* str)
{
    if (!str)
        return string();
    if (strchr(str, ' ') != 0) {
        string result;
        result = "\"";
        result += str;
        result += "\"";
        return result;
    }
    else {
        return string(str);
    }
}

bool QPropertyTree::onContextMenu(PropertyRow* r, QMenu& menu)
{
    SharedPtr<PropertyRow> row(r);
    PropertyTreeMenuHandler* handler = new PropertyTreeMenuHandler();
    addMenuHandler(handler);
    handler->tree = this;
    handler->row = row;

    PropertyRow::iterator it;
    for (it = row->begin(); it != row->end(); ++it){
        PropertyRow* child = *it;
        if (child->isContainer() && child->pulledUp())
            child->onContextMenu(menu, this);
    }
    row->onContextMenu(menu, this);
    if (config_.undoEnabled){
        if (!menu.isEmpty())
            menu.addSeparator();
        QAction* undo = menu.addAction("Undo", handler, SLOT(onMenuUndo()));
        undo->setEnabled(model()->canUndo());
        undo->setShortcut(QKeySequence("Ctrl+Z"));

        QAction* redo = menu.addAction("Redo", handler, SLOT(onMenuRedo()));
        redo->setEnabled(model()->canRedo());
        redo->setShortcut(QKeySequence("Ctrl+Shift+Z"));
    }
    if (!menu.isEmpty())
        menu.addSeparator();

    if (!row->userNonCopyable()){
        menu.addAction("Copy", handler, SLOT(onMenuCopy()), QKeySequence("Ctrl+C"));

        if(!row->userReadOnly()){
            QAction* paste = menu.addAction("Paste", handler, SLOT(onMenuPaste()), QKeySequence("Ctrl+V"));
            paste->setEnabled(canBePasted(row));
        }

        menu.addSeparator();
    }

    menu.addAction("Filter...", handler, SLOT(onMenuFilter()), QKeySequence("Ctrl+F"));
    QMenu* filter = menu.addMenu("Filter by");
    {
        string nameFilter = "#";
        nameFilter += quoteIfNeeded(row->labelUndecorated());
        handler->filterName = nameFilter;
        filter->addAction((string("Name:\t") + nameFilter).c_str(), handler, SLOT(onMenuFilterByName()));

        string valueFilter = "=";
        valueFilter += quoteIfNeeded(row->valueAsString().c_str());
        handler->filterValue = valueFilter;
        filter->addAction((string("Value:\t") + valueFilter).c_str(), handler, SLOT(onMenuFilterByValue()));

        string typeFilter = ":";
        typeFilter += quoteIfNeeded(row->typeNameForFilter(this));
        handler->filterType = typeFilter;
        filter->addAction((string("Type:\t") + typeFilter).c_str(), handler, SLOT(onMenuFilterByType()));
    }

#if 0
    menu.addSeparator();
    menu.addAction(TRANSLATE("Decompose"), row).connect(this, &QPropertyTree::onRowMenuDecompose);
#endif
    return true;
}

void QPropertyTree::onRowMouseMove(PropertyRow* row, [[maybe_unused]] const QRect& rowRect, QPoint point)
{
    PropertyDragEvent e;
    e.tree = this;
    e.pos = point;
    e.start = pressPoint_;
    e.totalDelta = pressDelta_;
    row->onMouseDrag(e);
    update();
}


bool QPropertyTree::canBePasted(PropertyRow* destination)
{
    SharedPtr<PropertyRow> source;
    if (!propertyRowFromClipboard(source, model_->constStrings()))
        return false;

    if (!smartPaste(destination, source, model(), true))
        return false;
    return true;
}

bool QPropertyTree::canBePasted(const char* destinationType)
{
    SharedPtr<PropertyRow> source;
    if (!propertyRowFromClipboard(source, model()->constStrings()))
        return false;

    bool result = strcmp(source->typeName(), destinationType) == 0;
    return result;
}

struct DecomposeProxy
{
    DecomposeProxy(SharedPtr<PropertyRow>& row) : row(row) {}

    void Serialize(IArchive& ar)
    {
        ar(row, "row", "Row");
    }

    SharedPtr<PropertyRow>& row;
};

void QPropertyTree::onRowMenuDecompose([[maybe_unused]] PropertyRow* row)
{
    // SharedPtr<PropertyRow> clonedRow = row->clone();
    // DecomposeProxy proxy(clonedRow);
    // edit(SStruct(proxy), 0, IMMEDIATE_UPDATE, this);
}

void QPropertyTree::onModelUpdated([[maybe_unused]] const PropertyRows& rows, bool needApply)
{
    if (widget_)
        widget_.reset();

    if (config_.immediateUpdate){
        if (needApply)
            apply(false);

        if (autoRevert_)
            revert();
        else {
            updateHeights();
            updateAttachedPropertyTree(true);
            if (!config_.immediateUpdate)
                onSignalChanged();
        }
    }
    else {
        update();
    }
}

void QPropertyTree::onModelPushUndo([[maybe_unused]] PropertyTreeOperator* op, [[maybe_unused]] bool* handled)
{
    signalPushUndo();
}

void QPropertyTree::onModelPushRedo([[maybe_unused]] PropertyTreeOperator* op, [[maybe_unused]] bool* handled)
{
    signalPushRedo();
}

void QPropertyTree::setWidget(PropertyRowWidget* widget)
{
    if (widget_){
        widget_->setParent(0);
    }
    widget_.reset();
    model()->dismissUpdate();

    if (widget)
    {
        QWidget* actualWidget = widget->actualWidget();
        if (actualWidget)
        {
            actualWidget->setParent(this);
            actualWidget->setFocus();
        }

        widget_.reset(widget);
        _arrangeChildren();

        if (widget_)
        {
            widget_->showPopup();
        }
    }
}

bool QPropertyTree::hasFocusOrInplaceHasFocus() const
{
    if (hasFocus())
        return true;

    if (widget_ && widget_->actualWidget() && widget_->actualWidget()->hasFocus())
        return true;

    return false;
}

void QPropertyTree::setFilterMode(bool inFilterMode)
{
    bool changed = filterMode_ != inFilterMode;
    filterMode_ = inFilterMode;

    if (filterMode_)
    {
        filterEntry_->show();
        filterEntry_->setFocus();
        filterEntry_->selectAll();
    }
    else
        filterEntry_->hide();

    if (changed)
    {
        onFilterChanged(QString());
    }
}

void QPropertyTree::startFilter(const char* filter)
{
    setFilterMode(true);
    filterEntry_->setText(filter);
    onFilterChanged(filter);
}

void QPropertyTree::_arrangeChildren()
{
    if (widget_){
        PropertyRow* row = widget_->row();
        if (row->visible(this)){
            QWidget* w = widget_->actualWidget();
            YASLI_ASSERT(w);
            if (w){
                QRect rect = row->widgetRect(this);
                rect = QRect(rect.topLeft() - offset_ + area_.topLeft(),
                    rect.bottomRight() - offset_ + area_.topLeft());
                w->move(rect.topLeft());
                w->resize(rect.size());
                if (!w->isVisible()){
                    w->show();
                    w->setFocus();
                }
            }
            else{
                //YASLI_ASSERT(w);
            }
        }
        else{
            widget_.reset();
        }
    }

    if (filterEntry_) {
        QSize size = rect().size();
        const int padding = 2;
        QRect pos(padding, padding, size.width() - padding * 2, filterEntry_->height());
        filterEntry_->move(pos.topLeft());
        filterEntry_->resize(pos.size() - QSize(scrollBar_ ? scrollBar_->width() : 0, 0));
    }
}



void QPropertyTree::setExpandLevels(int levels)
{
    config_.expandLevels = levels;
    model()->setExpandLevels(levels);
}

PropertyRow* QPropertyTree::selectedRow()
{
    const PropertyTreeModel::Selection &sel = model()->selection();
    if (sel.empty())
        return 0;
    return model()->rowFromPath(sel.front());
}

int QPropertyTree::selectedRowCount() const
{
    return (int)model()->selection().size();
}

PropertyRow* QPropertyTree::selectedRowByIndex(int index)
{
    std::vector<PropertyRow*> result;
    const PropertyTreeModel::Selection &sel = model()->selection();
    if (size_t(index) >= sel.size())
        return 0;

    return model()->rowFromPath(sel[index]);
}

bool QPropertyTree::getSelectedObject(Serialization::Object* object)
{
    const PropertyTreeModel::Selection &sel = model()->selection();
    if (sel.empty())
        return 0;
    PropertyRow* row = model()->rowFromPath(sel.front());
    while (row && !row->isObject())
        row = row->parent();
    if (!row)
        return false;

    if (row->isObject()) {
        PropertyRowObject* obj = static_cast<PropertyRowObject*>(row);
        *object = obj->object();
        return true;
    }
    else {
        return false;
    }
}

QPoint QPropertyTree::_toScreen(QPoint point) const
{
    QPoint pt(point.x() - offset_.x() + area_.left(),
        point.y() - offset_.y() + area_.top());

    return mapToGlobal(pt);
}

bool QPropertyTree::setSelectedRow(PropertyRow* row)
{
    TreeSelection sel;
    if (row)
        sel.push_back(model()->pathFromRow(row));
    if (model()->selection() != sel) {
        model()->setSelection(sel);
        if (row)
            ensureVisible(row);
        updateAttachedPropertyTree(false);
        repaint();
        return true;
    }
    return false;
}

bool QPropertyTree::selectByAddress(const void* addr, bool keepSelectionIfChildSelected)
{
    if (model()->root()) {
        PropertyRow* row = model()->root()->findByAddress(addr);

        bool keepSelection = false;
        if (keepSelectionIfChildSelected && row && !model()->selection().empty()) {
            keepSelection = true;
            TreeSelection::const_iterator it;
            for (it = model()->selection().begin(); it != model()->selection().end(); ++it){
                PropertyRow* selectedRow = model()->rowFromPath(*it);
                if (!selectedRow)
                    continue;
                if (!selectedRow->isChildOf(row)){
                    keepSelection = false;
                    break;
                }
            }
        }

        if (!keepSelection)
            return setSelectedRow(row);
    }
    return false;
}

bool QPropertyTree::selectByAddresses(const void* const* addresses, size_t addressCount, bool keepSelectionIfChildSelected)
{
    bool result = false;
    if (model()->root()) {
        bool keepSelection = false;
        vector<PropertyRow*> rows;
        for (size_t i = 0; i < addressCount; ++i) {
            const void* addr = addresses[i];
            PropertyRow* row = model()->root()->findByAddress(addr);

            if (keepSelectionIfChildSelected && row && !model()->selection().empty()) {
                keepSelection = true;
                TreeSelection::const_iterator it;
                for (it = model()->selection().begin(); it != model()->selection().end(); ++it){
                    PropertyRow* selectedRow = model()->rowFromPath(*it);
                    if (!selectedRow)
                        continue;
                    if (!selectedRow->isChildOf(row)){
                        keepSelection = false;
                        break;
                    }
                }
            }

            if (row)
                rows.push_back(row);
        }

        if (!keepSelection) {
            TreeSelection sel;
            for (size_t j = 0; j < rows.size(); ++j) {
                PropertyRow* row = rows[j];
                if (row)
                    sel.push_back(model()->pathFromRow(row));
            }
            if (model()->selection() != sel) {
                model()->setSelection(sel);
                if (!rows.empty())
                    ensureVisible(rows.back());
                update();
                result = true;
                if (attachedPropertyTree_)
                    updateAttachedPropertyTree(false);
            }
        }
    }
    return result;
}

void QPropertyTree::setUndoEnabled(bool enabled, bool full)
{
    config_.undoEnabled = enabled;
    config_.fullUndo = full;
    model()->setUndoEnabled(enabled);
    model()->setFullUndo(full);
}

void QPropertyTree::attachPropertyTree(QPropertyTree* propertyTree)
{
    if (attachedPropertyTree_)
        disconnect(attachedPropertyTree_, SIGNAL(signalChanged()), this, SLOT(onAttachedTreeChanged()));
    attachedPropertyTree_ = propertyTree;
    if (attachedPropertyTree_)
        connect(attachedPropertyTree_, SIGNAL(signalChanged()), this, SLOT(onAttachedTreeChanged()));
    updateAttachedPropertyTree(true);
}

void QPropertyTree::detachPropertyTree()
{
    attachPropertyTree(0);
}

void QPropertyTree::setAutoHideAttachedPropertyTree(bool autoHide)
{
    autoHideAttachedPropertyTree_ = autoHide;
}

void QPropertyTree::getSelectionSerializers(Serialization::SStructs* serializers)
{
    TreeSelection::const_iterator i;
    for (i = model()->selection().begin(); i != model()->selection().end(); ++i){
        PropertyRow* row = model()->rowFromPath(*i);
        if (!row)
            continue;


        while (row && ((row->pulledUp() || row->pulledBefore()) || row->isLeaf())) {
            row = row->parent();
        }
        if (outlineMode_) {
            PropertyRow* topmostContainerElement = 0;
            PropertyRow* r = row;
            while (r && r->parent()) {
                if (r->parent()->isContainer())
                    topmostContainerElement = r;
                r = r->parent();
            }
            if (topmostContainerElement != 0)
                row = topmostContainerElement;
        }
        Serialization::SStruct ser = row->serializer();

        if (ser)
            serializers->push_back(ser);
    }
}

void QPropertyTree::updateAttachedPropertyTree(bool revert)
{
    if (attachedPropertyTree_) {
        Serialization::SStructs serializers;
        getSelectionSerializers(&serializers);
        if (!attachedPropertyTree_->attach(serializers) && revert)
            attachedPropertyTree_->revertNoninterrupting();
        if (autoHideAttachedPropertyTree_)
            attachedPropertyTree_->setVisible(!serializers.empty());
    }
}

struct FilterVisitor
{
    const QPropertyTree::RowFilter& filter_;

    FilterVisitor(const QPropertyTree::RowFilter& filter)
        : filter_(filter)
    {
    }

    static void markChildrenAsBelonging(PropertyRow* row, bool belongs)
    {
        int count = int(row->count());
        for (int i = 0; i < count; ++i)
        {
            PropertyRow* child = row->childByIndex(i);
            child->setBelongsToFilteredRow(belongs);

            markChildrenAsBelonging(child, belongs);
        }
    }

    static bool hasMatchingChildren(PropertyRow* row)
    {
        int numChildren = (int)row->count();
        for (int i = 0; i < numChildren; ++i)
        {
            PropertyRow* child = row->childByIndex(i);
            if (!child)
                continue;
            if (child->matchFilter())
                return true;
            if (hasMatchingChildren(child))
                return true;
        }
        return false;
    }

    ScanResult operator()(PropertyRow* row, QPropertyTree* tree)
    {
        const char* label = row->labelUndecorated();
        Serialization::string value = row->valueAsString();

        bool matchFilter = filter_.match(label, filter_.NAME_VALUE, 0, 0) || filter_.match(value.c_str(), filter_.NAME_VALUE, 0, 0);
        if (matchFilter && filter_.typeRelevant(filter_.NAME))
            filter_.match(label, filter_.NAME, 0, 0);
        if (matchFilter && filter_.typeRelevant(filter_.VALUE))
            matchFilter = filter_.match(value.c_str(), filter_.VALUE, 0, 0);
        if (matchFilter && filter_.typeRelevant(filter_.TYPE))
            matchFilter = filter_.match(row->typeNameForFilter(tree), filter_.TYPE, 0, 0);

        int numChildren = int(row->count());
        if (matchFilter) {
            if (row->pulledBefore() || row->pulledUp()) {
                // treat pulled rows as part of parent
                PropertyRow* parent = row->parent();
                parent->setMatchFilter(true);
                markChildrenAsBelonging(parent, true);
                parent->setBelongsToFilteredRow(false);
            }
            else {
                markChildrenAsBelonging(row, true);
                row->setBelongsToFilteredRow(false);
                row->setLayoutChanged();
                row->setLabelChanged();
            }
        }
        else {
            bool belongs = hasMatchingChildren(row);
            row->setBelongsToFilteredRow(belongs);
            if (belongs) {
                tree->expandRow(row, true, false);
                for (int i = 0; i < numChildren; ++i) {
                    PropertyRow* child = row->childByIndex(i);
                    if (child->pulledUp())
                        child->setBelongsToFilteredRow(true);
                }
            }
            else {
                row->_setExpanded(false);
                row->setLayoutChanged();
            }
        }

        row->setMatchFilter(matchFilter);
        return SCAN_CHILDREN_SIBLINGS;
    }

protected:
    string labelStart_;
};



void QPropertyTree::RowFilter::parse(const char* filter)
{
    for (int i = 0; i < NUM_TYPES; ++i) {
        start[i].clear();
        substrings[i].clear();
        tillEnd[i] = false;
    }

    YASLI_ESCAPE(filter != 0, return);

    vector<char> filterBuf(filter, filter + strlen(filter) + 1);
    for (size_t i = 0; i < filterBuf.size(); ++i)
        filterBuf[i] = tolower(filterBuf[i]);

    const char* str = &filterBuf[0];

    Type type = NAME_VALUE;
    while (true)
    {
        bool fromStart = false;
        while (*str == '^') {
            fromStart = true;
            ++str;
        }

        const char* tokenStart = str;

        if (*str == '\"')
        {
            ++str;
            while (*str != '\0' && *str != '\"')
                ++str;
        }
        else
        {
            while (*str != '\0' && *str != ' ' && *str != '=' && *str != ':' && *str != '#')
                ++str;
        }
        if (str != tokenStart) {
            if (*tokenStart == '\"' && *str == '\"') {
                start[type].assign(tokenStart + 1, str);
                tillEnd[type] = true;
                ++str;
            }
            else
            {
                if (fromStart)
                    start[type].assign(tokenStart, str);
                else
                    substrings[type].push_back(string(tokenStart, str));
            }
        }
        while (*str == ' ')
            ++str;
        if (*str == '#') {
            type = NAME;
            ++str;
        }
        else if (*str == '=') {
            type = VALUE;
            ++str;
        }
        else if (*str == ':') {
            type = TYPE;
            ++str;
        }
        else if (*str == '\0')
            break;
    }
}

bool QPropertyTree::RowFilter::match(const char* textOriginal, Type type, size_t* matchStart, size_t* matchEnd) const
{
    YASLI_ESCAPE(textOriginal, return false);

    char* text;
    {
        size_t textLen = strlen(textOriginal);
        text = (char*)alloca((textLen + 1));
        memcpy(text, textOriginal, (textLen + 1));
        for (char* p = text; *p; ++p)
            *p = tolower(*p);
    }

    const string &startForType = this->start[type];
    if (tillEnd[type]){
        if (startForType == text) {
            if (matchStart)
                *matchStart = 0;
            if (matchEnd)
                *matchEnd = startForType.size();
            return true;
        }
        else
            return false;
    }

    const vector<string> &substringsForType = this->substrings[type];

    const char* startPos = text;

    if (matchStart)
        *matchStart = 0;
    if (matchEnd)
        *matchEnd = 0;
    if (!startForType.empty()) {
        if (strncmp(text, startForType.c_str(), startForType.size()) != 0){
            //_freea(text);
            return false;
        }
        if (matchEnd)
            *matchEnd = startForType.size();
        startPos += startForType.size();
    }

    size_t numSubstrings = substringsForType.size();
    for (size_t i = 0; i < numSubstrings; ++i) {
        const char* substr = strstr(startPos, substringsForType[i].c_str());
        if (!substr){
            return false;
        }
        startPos += substringsForType[i].size();
        if (matchStart && i == 0 && startForType.empty()) {
            *matchStart = substr - text;
        }
        if (matchEnd)
            *matchEnd = substr - text + substringsForType[i].size();
    }
    return true;
}

void QPropertyTree::onFilterChanged([[maybe_unused]] const QString& text)
{
    QByteArray arr = filterEntry_->text().toLocal8Bit();
    const char* filterStr = filterMode_ ? arr.data() : "";
    rowFilter_.parse(filterStr);
    FilterVisitor visitor(rowFilter_);
    model()->root()->scanChildrenBottomUp(visitor, this);
    updateHeights();
}

void QPropertyTree::drawFilteredString(QPainter& p, const wchar_t* text, RowFilter::Type type, const QFont* font, const QRect& rect, const QColor& textColor, bool pathEllipsis, bool center) const
{
    int textLen = (int)wcslen(text);

    if (textLen == 0)
        return;

    string textStr(fromWideChar(text));
    QString str(textStr.c_str());
    QFontMetrics fm(*font);
    QRect textRect = rect;
    int alignment;
    if (center)
        alignment = Qt::AlignHCenter | Qt::AlignVCenter;
    else {
        if (pathEllipsis && textRect.width() < fm.horizontalAdvance(str))
            alignment = Qt::AlignRight | Qt::AlignVCenter;
        else
            alignment = Qt::AlignLeft | Qt::AlignVCenter;
    }

    if (filterMode_) {
        size_t hiStart = 0;
        size_t hiEnd = 0;
        bool matched = rowFilter_.match(textStr.c_str(), type, &hiStart, &hiEnd) && hiStart != hiEnd;
        if (!matched && (type == RowFilter::NAME || type == RowFilter::VALUE))
            matched = rowFilter_.match(textStr.c_str(), RowFilter::NAME_VALUE, &hiStart, &hiEnd);
        if (matched && hiStart != hiEnd) {
            QRectF boxFull;
            QRectF boxStart;
            QRectF boxEnd;

            boxFull = fm.boundingRect(textRect, alignment, str);

            if (hiStart > 0)
                boxStart = fm.boundingRect(textRect, alignment, str.left(hiStart));
            else {
                boxStart = fm.boundingRect(textRect, alignment, str);
                boxStart.setWidth(0.0f);
            }
            boxEnd = fm.boundingRect(textRect, alignment, str.left(hiEnd));

            QColor highlightColor, highlightBorderColor;
            {
                highlightColor = palette().color(QPalette::Highlight);
                int h, s, v;
                highlightColor.getHsv(&h, &s, &v);
                h -= 175;
                if (h < 0)
                    h += 360;
                highlightColor.setHsv(h, min(255, int(s * 1.33f)), v, 255);
                highlightBorderColor.setHsv(h, aznumeric_cast<int>(s * 0.5f), v, 255);
            }

            int left = int(boxFull.left() + boxStart.width()) - 1;
            int top = int(boxFull.top());
            int right = int(boxFull.left() + boxEnd.width());
            int bottom = int(boxFull.top() + boxEnd.height());
            QRect highlightRect(left, top, right - left, bottom - top);
            QBrush br(highlightColor);
            p.setBrush(br);
            p.setPen(highlightBorderColor);
            bool oldAntialiasing = p.renderHints().testFlag(QPainter::Antialiasing);
            p.setRenderHint(QPainter::Antialiasing, true);

            QRect intersectedHighlightRect = rect.intersected(highlightRect);
            p.drawRoundedRect(intersectedHighlightRect, 4.0, 4.0);
            p.setRenderHint(QPainter::Antialiasing, oldAntialiasing);
        }
    }

    QBrush textBrush(textColor);
    p.setBrush(textBrush);
    p.setPen(textColor);
    QFont previousFont = p.font();
    p.setFont(*font);
    p.drawText(textRect, alignment, str, 0);
    p.setFont(previousFont);
}

void QPropertyTree::_drawRowLabel(QPainter& p, const wchar_t* text, const QFont* font, const QRect& rect, const QColor& textColor) const
{
    drawFilteredString(p, text, RowFilter::NAME, font, rect, textColor, false, false);
}

void QPropertyTree::_drawRowValue(QPainter& p, const wchar_t* text, const QFont* font, const QRect& rect, const QColor& textColor, bool pathEllipsis, bool center) const
{
    drawFilteredString(p, text, RowFilter::VALUE, font, rect, textColor, pathEllipsis, center);
}

struct DrawVisitor
{
    DrawVisitor(QPainter& painter, const QRect& area, int scrollOffset, bool selectionPass)
        : area_(area)
        , painter_(painter)
        , offset_(0)
        , scrollOffset_(scrollOffset)
        , lastParent_(0)
        , selectionPass_(selectionPass)
    {}

    ScanResult operator()(PropertyRow* row, QPropertyTree* tree, int index)
    {
        if (row->visible(tree) && ((!row->parent() || (row->parent()->expanded() && !lastParent_)) || row->pulledUp())){
            QRect rect = row->rect();
            if (rect.top() > scrollOffset_ + area_.height())
                lastParent_ = row->parent();

            int height = row->heightIncludingChildren();
            if ((height == USHRT_MAX || rect.top() + height > scrollOffset_) && rect.width() > 0)
                row->drawRow(painter_, tree, index, selectionPass_);

            return SCAN_CHILDREN_SIBLINGS;
        }
        else
            return SCAN_SIBLINGS;
    }

protected:
    QPainter& painter_;
    QRect area_;
    int offset_;
    int scrollOffset_;
    PropertyRow* lastParent_;
    bool selectionPass_;
};

QSize QPropertyTree::sizeHint() const
{
    if (sizeToContent_)
        return minimumSize();
    else
        return sizeHint_;
}

bool QPropertyTree::event(QEvent* ev)
{
    if (ev->type() == QEvent::ShortcutOverride)
    {
        if (!widget_)
        {
            PropertyRow* row = model()->focusedRow();
            if (row)
            {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
                bool keyWillBeProcessed = false;

                int modifiedKey = keyEvent->key() | keyEvent->modifiers();
                switch (modifiedKey)
                {
                    case (Qt::Key_F | Qt::CTRL):
                    case Qt::Key_Escape:
                        keyWillBeProcessed = true;
                        break;

                    default:
                        keyWillBeProcessed = rowProcessesKey(row, keyEvent);
                        break;
                }

                if (keyWillBeProcessed)
                {
                    ev->accept();
                    return true;
                }
            }
        }
    }

    return QWidget::event(ev);
}

void QPropertyTree::paintEvent([[maybe_unused]] QPaintEvent* ev)
{
    QElapsedTimer timer;
    timer.start();
    QPainter painter(this);
    QRect clientRect = this->rect();

    int clientHeight = clientRect.height();
    backgroundColor_ = palette().color(QPalette::Window);
    painter.fillRect(clientRect, QBrush(backgroundColor_));

    painter.translate(-offset_.x(), -offset_.y());

    if (dragController_->captured())
        dragController_->drawUnder(painter);

    painter.translate(area_.left(), area_.top());

    if (model()->root()) {
        DrawVisitor selectionOp(painter, area_, offset_.y(), true);
        model()->root()->scanChildren(selectionOp, this);

        DrawVisitor op(painter, area_, offset_.y(), false);
        op(model()->root(), this, 0);
        model()->root()->scanChildren(op, this);
    }

    painter.translate(-area_.left(), -area_.top());
    painter.translate(offset_.x(), offset_.y());

    //painter.setClipRect(rect());

    if (size_.y() > clientHeight)
    {
        const int shadowHeight = int(_defaultRowHeight() * 0.3f);
        QColor color1(0, 0, 0, 0);
        QColor color2(0, 0, 0, 96);

        int visibleAreaWidth = area_.width() + 5;

        QRect upperRect(rect().left() + 1, rect().top(), visibleAreaWidth - 2, shadowHeight);
        QLinearGradient upperGradient(upperRect.left(), upperRect.top(), upperRect.left(), upperRect.bottom());
        upperGradient.setColorAt(0.0f, color2);
        upperGradient.setColorAt(1.0f, color1);
        painter.fillRect(upperRect, QBrush(upperGradient));

        QLinearGradient upperEdgeGradient(upperRect.left(), upperRect.top(), upperRect.left(), upperRect.bottom() + shadowHeight);
        upperEdgeGradient.setColorAt(0.0f, color2);
        upperEdgeGradient.setColorAt(1.0f, color1);
        painter.fillRect(QRect(rect().left(), rect().top(), 1, shadowHeight * 2 + 1), QBrush(upperEdgeGradient));
        painter.fillRect(QRect(visibleAreaWidth - 1, rect().top(), 1, shadowHeight * 2 + 1), QBrush(upperEdgeGradient));

        QRect lowerRect(rect().left() + 1, rect().bottom() - shadowHeight / 2, visibleAreaWidth - 2, shadowHeight / 2 + 1);
        QLinearGradient lowerGradient(lowerRect.left(), lowerRect.top(), lowerRect.left(), lowerRect.bottom());
        lowerGradient.setColorAt(0.0f, color1);
        lowerGradient.setColorAt(1.0f, color2);
        QBrush lowerBrush(lowerGradient);
        painter.fillRect(lowerRect, lowerGradient);

        QLinearGradient lowerEdgeGradient(lowerRect.left(), lowerRect.top() - shadowHeight, lowerRect.left(), lowerRect.bottom());
        lowerEdgeGradient.setColorAt(0.0f, color1);
        lowerEdgeGradient.setColorAt(1.0f, color2);
        painter.fillRect(QRect(rect().left(), rect().bottom() - shadowHeight * 2, 1, shadowHeight * 2 + 1), QBrush(lowerEdgeGradient));
        painter.fillRect(QRect(visibleAreaWidth - 1, rect().bottom() - shadowHeight * 2, 1, shadowHeight * 2 + 1), QBrush(lowerEdgeGradient));
    }

    if (dragController_->captured()) {
        painter.translate(-offset_);
        dragController_->drawOver(painter);
        painter.translate(offset_);
    }
    else{
        //  if(model()->focusedRow() != 0 && model()->focusedRow()->isRoot() && tree_->hasFocus()){
        //      clientRect.left += 2; clientRect.top += 2;
        //      clientRect.right -= 2; clientRect.bottom -= 2;
        //      DrawFocusRect(dc, &clientRect);
        //  }
    }
    paintTime_ = aznumeric_cast<int>(timer.elapsed());
}

QPropertyTree::HitTest QPropertyTree::hitTest(PropertyRow* row, const QPoint& pointInWindowSpace, const QRect& rowRect)
{
    QPoint point = pointToRootSpace(pointInWindowSpace);

    if (!row->hasVisibleChildren(this) && row->plusRect(this).contains(point))
        return TREE_HIT_PLUS;

    if (row->textRect(this).contains(point))
        return TREE_HIT_TEXT;

    if (rowRect.contains(point))
        return TREE_HIT_ROW;

    return TREE_HIT_NONE;

}

PropertyRow* QPropertyTree::rowByPoint(const QPoint& pt)
{
    if (!model_->root())
        return 0;
    if (!area_.contains(pt))
        return 0;
    return model_->root()->hit(this, pointToRootSpace(pt));
}

QPoint QPropertyTree::pointToRootSpace(const QPoint& point) const
{
    return QPoint(point.x() + offset_.x() - area_.left(), point.y() + offset_.y() - area_.top());
}

QPoint QPropertyTree::pointFromRootSpace(const QPoint& point) const
{
    return QPoint(point.x() - offset_.x() + area_.left(), point.y() - offset_.y() + area_.top());
}


void QPropertyTree::moveEvent(QMoveEvent* ev)
{
    QWidget::moveEvent(ev);
}

void QPropertyTree::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);

    updateHeights();
}

void QPropertyTree::mousePressEvent(QMouseEvent* ev)
{
    //QWidget::mousePressEvent(ev);
    setFocus(Qt::MouseFocusReason);

    if (ev->button() == Qt::LeftButton)
    {
        PropertyRow* row = rowByPoint(ev->pos());
        if (row && !row->isSelectable())
            row = row->parent();
        if (row){
            if (onRowLMBDown(row, row->rect(), pointToRootSpace(ev->pos()), ev->modifiers().testFlag(Qt::ControlModifier), ev->modifiers().testFlag(Qt::ShiftModifier))) {
                capturedRow_ = row;
                lastStillPosition_ = pointToRootSpace(ev->pos());
            }
            else if (!dragCheckMode_){
                row = rowByPoint(ev->pos());
                PropertyRow* draggedRow = row;
                while (draggedRow && (!draggedRow->isSelectable() || draggedRow->pulledUp() || draggedRow->pulledBefore()))
                    draggedRow = draggedRow->parent();
                if (draggedRow && !draggedRow->userReadOnly() && !widget_){
                    dragController_->beginDrag(row, draggedRow, ev->globalPos());
                }
            }
        }
        update();
    }
    else if (ev->button() == Qt::RightButton)
    {
        QPoint point = ev->pos();
        PropertyRow* row = rowByPoint(point);
        if (row){
            model()->setFocusedRow(row);
            update();

            onRowRMBDown(row, row->rect(), _toScreen(pointToRootSpace(point)));
        }
        else{
            QRect rect = this->rect();
            onRowRMBDown(model()->root(), rect, _toScreen(pointToRootSpace(point)));
        }
    }
    else if (ev->button() == Qt::MiddleButton)
    {
        QPoint point = ev->pos();
        PropertyRow* row = rowByPoint(point);
        if (row){
            switch (hitTest(row, point, row->rect())){
            case TREE_HIT_PLUS:
                break;
            case TREE_HIT_NONE:
            default:
                model()->setFocusedRow(row);
                update();
                break;
            }

        }
    }
}

void QPropertyTree::mouseReleaseEvent(QMouseEvent* ev)
{
    QWidget::mouseReleaseEvent(ev);

    if (ev->button() == Qt::LeftButton)
    {
        if (dragController_->captured()){
            if (dragController_->drop(QCursor::pos()))
                updateHeights();
            else
                update();
        }
        if (dragCheckMode_) {
            dragCheckMode_ = false;
        }
        else {
            QPoint point = ev->pos();
            if (capturedRow_){
                QRect rowRect = capturedRow_->rect();
                onRowLMBUp(capturedRow_, rowRect, pointToRootSpace(ev->pos()));
                mouseStillTimer_->stop();
                capturedRow_ = 0;
                update();
            }
        }
    }
    else if (ev->button() == Qt::RightButton)
    {

    }

    unsetCursor();
}

void QPropertyTree::focusInEvent(QFocusEvent* ev)
{
    QWidget::focusInEvent(ev);
    widget_.reset();
}

void QPropertyTree::keyPressEvent(QKeyEvent* ev)
{
    // NOTE: if you add any new key processing in here, make sure you update QPropertyTree::event() to handle
    // it in the ShortcutOverride event. Otherwise, MainWindow/other shortcuts might take priority and eat it before
    // it reaches this function.

    if (ev->key() == Qt::Key_F && ev->modifiers() == Qt::CTRL) {
        setFilterMode(true);
    }

    if (filterMode_) {
        if (ev->key() == Qt::Key_Escape && ev->modifiers() == Qt::NoModifier) {
            setFilterMode(false);
        }
    }

    bool result = false;
    if (!widget_) {
        PropertyRow* row = model()->focusedRow();
        if (row)
            onRowKeyDown(row, ev);
    }
    update();
    if (!result)
        QWidget::keyPressEvent(ev);
}


void QPropertyTree::mouseDoubleClickEvent(QMouseEvent* ev)
{
    QWidget::mouseDoubleClickEvent(ev);

    QPoint point = ev->pos();
    PropertyRow* row = rowByPoint(point);
    if (row){
        PropertyActivationEvent e;
        e.tree = this;
        e.force = true;
        e.reason = e.REASON_DOUBLECLICK;
        PropertyRow* nonPulledParent = row;
        while (nonPulledParent && nonPulledParent->pulledUp())
            nonPulledParent = nonPulledParent->parent();

        if (row->widgetRect(this).contains(pointToRootSpace(point))){
            if (!row->onActivate(e))
                toggleRow(nonPulledParent);
        }
        else if (!toggleRow(row)) {
            if (!row->onActivate(e))
                if (!toggleRow(nonPulledParent)) {
                    // activate first visible inline row
                    for (size_t i = 0; i < row->count(); ++i) {
                        PropertyRow* child = row->childByIndex(i);
                        if (child && child->pulledUp() && child->visible(this)) {
                            child->onActivate(e);
                            break;
                        }
                    }
                }
        }
    }
}

void QPropertyTree::onMouseStillTimeout()
{
    onMouseStill(mapFromGlobal(QCursor::pos()));
}

void QPropertyTree::onMouseStill(QPoint point)
{
    if (capturedRow_) {
        PropertyDragEvent e;
        e.tree = this;
        e.pos = point;
        e.start = pressPoint_;

        capturedRow_->onMouseStill(e);
        lastStillPosition_ = e.pos;
    }
}

void QPropertyTree::flushAggregatedMouseEvents()
{
    if (aggregatedMouseEventCount_ > 0) {
        bool gotPendingEvent = aggregatedMouseEventCount_ > 1;
        aggregatedMouseEventCount_ = 0;
        if (gotPendingEvent && lastMouseMoveEvent_.data())
            mouseMoveEvent(lastMouseMoveEvent_.data());
    }
}

void QPropertyTree::mouseMoveEvent(QMouseEvent* ev)
{
    if (ev->type() == QEvent::MouseMove && aggregateMouseEvents_) {
        lastMouseMoveEvent_.reset(new QMouseEvent(QEvent::MouseMove, ev->localPos(), ev->windowPos(), ev->screenPos(), ev->button(), ev->buttons(), ev->modifiers()));
        ev = lastMouseMoveEvent_.data();
        ++aggregatedMouseEventCount_;
        if (aggregatedMouseEventCount_ > 1)
            return;
    }

    QCursor newCursor = QCursor(Qt::ArrowCursor);
    QString newToolTip;
    if (dragController_->captured() && !ev->buttons().testFlag(Qt::LeftButton))
        dragController_->interrupt();
    if (dragController_->captured()){
        QPoint pos = QCursor::pos();
        if (dragController_->dragOn(pos)) {
            // SetCapture
        }
        update();
    }
    else{
        QPoint point = ev->pos();
        PropertyRow* row = rowByPoint(point);
        if (row && dragCheckMode_ && row->widgetRect(this).contains(pointToRootSpace(point))) {
            row->onMouseDragCheck(this, dragCheckValue_);
        }
        else if (capturedRow_){
            onRowMouseMove(capturedRow_, QRect(), pointToRootSpace(point));
            if (config_.sliderUpdateDelay >= 0 && !mouseStillTimer_->isActive())
                mouseStillTimer_->start(config_.sliderUpdateDelay);

            if (cursor().shape() == Qt::BlankCursor)
            {
                pressDelta_ += pointToRootSpace(ev->pos()) - pressPoint_;
                pointerMovedSincePress_ = true;
                AzQtComponents::SetCursorPos(mapToGlobal(pointFromRootSpace(pressPoint_)));
            }
            else
            {
                pressDelta_ = pointToRootSpace(ev->pos()) - pressPoint_;
            }
        }

        PropertyRow* hoverRow = row;
        if (capturedRow_)
            hoverRow = capturedRow_;
        PropertyHoverInfo hover;
        if (hoverRow) {
            QPoint pointInRootSpace = pointToRootSpace(point);
            if (hoverRow->getHoverInfo(&hover, pointInRootSpace, this)) {
                newCursor = hover.cursor;
                newToolTip = hover.toolTip;

                PropertyRow* tooltipRow = hoverRow;
                while (newToolTip.isEmpty() && tooltipRow->parent() && (tooltipRow->pulledUp() || tooltipRow->pulledBefore())) {
                    // check if parent of inlined property has a tooltip instead
                    tooltipRow = tooltipRow->parent();
                    if (tooltipRow->getHoverInfo(&hover, pointInRootSpace, this))
                        newToolTip = hover.toolTip;
                }
            }

            if (hoverRow->validatorWarningIconRect(this).contains(pointToRootSpace(point))) {
                newCursor = QCursor(Qt::PointingHandCursor);
                newToolTip = "Jump to next warning";
            }
            if (hoverRow->validatorErrorIconRect(this).contains(pointToRootSpace(point))) {
                newCursor = QCursor(Qt::PointingHandCursor);
                newToolTip = "Jump to next error";
            }
        }
    }
    setCursor(newCursor);
    if (toolTip() != newToolTip)
        setToolTip(newToolTip);
    if (newToolTip.isEmpty())
        QToolTip::hideText();
}

void QPropertyTree::wheelEvent(QWheelEvent* ev)
{
    QWidget::wheelEvent(ev);

    float delta = ev->angleDelta().ry() / 360.0f;
    if (ev->modifiers() & Qt::CTRL) {
        if (delta > 0)
            zoomLevel_ += 1;
        else
            zoomLevel_ -= 1;
        if (zoomLevel_ < 8)
            zoomLevel_ = 8;
        if (zoomLevel_ > 30)
            zoomLevel_ = 30;
        float scale = zoomLevel_ * 0.1f;
        QFont font;
        font.setPointSizeF(font.pointSizeF() * scale);
        setFont(font);
        font.setBold(true);
        boldFont_ = font;

        updateHeights(true);
    }
    else {
        if (scrollBar_->isVisible() && scrollBar_->isEnabled())
            scrollBar_->setValue(scrollBar_->value() + -ev->angleDelta().y());
    }
}

bool QPropertyTree::toggleRow(PropertyRow* row)
{
    if (!row->canBeToggled(this))
        return false;
    expandRow(row, !row->expanded());
    updateHeights();
    return true;
}

bool QPropertyTree::_isDragged(const PropertyRow* row) const
{
    if (!dragController_->dragging())
        return false;
    if (dragController_->draggedRow() == row)
        return true;
    return false;
}

bool QPropertyTree::_isCapturedRow(const PropertyRow* row) const
{
    return capturedRow_ == row;
}

void QPropertyTree::setValueColumnWidth(float valueColumnWidth)
{
    if (style_->valueColumnWidth != valueColumnWidth)
    {
        style_->valueColumnWidth = valueColumnWidth;
        updateHeights();
        update();
    }
}

QPropertyTree::QPropertyTree(const QPropertyTree&)
{
}


QPropertyTree& QPropertyTree::operator=(const QPropertyTree&)
{
    return *this;
}

void QPropertyTree::onAttachedTreeChanged()
{
    revert();
}

struct ValidatorIconVisitor
{
    ScanResult operator()(PropertyRow* row, QPropertyTree* tree, int)
    {
        row->resetValidatorIcons();
        if (row->validatorCount()) {
            bool hasErrors = false;
            bool hasWarnings = false;
            if (const ValidatorEntry* validatorEntries = tree->_validatorBlock()->GetEntry(row->validatorIndex(), row->validatorCount())) {
                for (int i = 0; i < row->validatorCount(); ++i) {
                    const ValidatorEntry* validatorEntry = validatorEntries + i;
                    if (validatorEntry->type == VALIDATOR_ENTRY_ERROR)
                        hasErrors = true;
                    else if (validatorEntry->type == VALIDATOR_ENTRY_WARNING)
                        hasWarnings = true;
                }
            }

            if (hasErrors || hasWarnings)
            {
                PropertyRow* lastClosedParent = 0;
                PropertyRow* current = row->parent();
                bool lastWasPulled = row->pulledUp() || row->pulledBefore();
                while (current && current->parent()) {
                    if (!current->expanded() && !lastWasPulled && current->visible(tree))
                        lastClosedParent = current;
                    lastWasPulled = current->pulledUp() || current->pulledBefore();
                    current = current->parent();
                }
                if (lastClosedParent)
                    lastClosedParent->addValidatorIcons(hasWarnings, hasErrors);
            }
        }
        return SCAN_CHILDREN_SIBLINGS;
    }
};

void QPropertyTree::updateValidatorIcons()
{
    if (!validatorBlock_->IsEnabled())
        return;
    ValidatorIconVisitor op;
    model()->root()->scanChildren(op, this);
    model()->root()->setLabelChangedToChildren();
}

void QPropertyTree::setTreeStyle(const QPropertyTreeStyle& style)
{
    *style_ = style;
    updateHeights(true);
}

void QPropertyTree::setPackCheckboxes(bool pack)
{
    style_->packCheckboxes = pack;
    updateHeights(true);
}

bool QPropertyTree::packCheckboxes() const
{
    return style_->packCheckboxes;
}

void QPropertyTree::setCompact(bool compact)
{
    style_->compact = compact;
    update();
}

bool QPropertyTree::compact() const
{
    return style_->compact;
}

void QPropertyTree::setRowSpacing(float rowSpacing)
{
    style_->rowSpacing = rowSpacing;
}

float QPropertyTree::rowSpacing() const
{
    return style_->rowSpacing;
}

float QPropertyTree::valueColumnWidth() const
{
    return style_->valueColumnWidth;
}

void QPropertyTree::setFullRowMode(bool fullRowMode)
{
    style_->fullRowMode = fullRowMode;
    update();
}

bool QPropertyTree::fullRowMode() const
{
    return style_->fullRowMode;
}

bool QPropertyTree::containsErrors() const
{
    return validatorBlock_->ContainsErrors();
}

void QPropertyTree::focusFirstError()
{
    jumpToNextHiddenValidatorIssue(true, model()->root());
}

void QPropertyTree::setBackgroundColor(const QColor& backgroundColor)
{
    backgroundColor_ = backgroundColor;
}

#include <QPropertyTree/moc_QPropertyTree.cpp>
#include <QPropertyTree/moc_PropertyTreeMenuHandler.cpp>

// vim:ts=4 sw=4:
