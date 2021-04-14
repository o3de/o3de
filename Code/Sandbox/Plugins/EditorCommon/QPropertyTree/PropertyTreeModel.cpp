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
#include "PropertyTreeModel.h"
#include "QPropertyTree.h"
#include "Serialization.h"
#include "Serialization/ClassFactory.h"
#include "Serialization/Callback.h"

PropertyTreeModel::PropertyTreeModel()
: expandLevels_(0)
, undoEnabled_(true)
, fullUndo_(false)
{
    clear();
}

PropertyTreeModel::~PropertyTreeModel()
{
    root_ = 0;
    defaultTypes_.clear();
    defaultTypesPoly_.clear();
}

TreePath PropertyTreeModel::pathFromRow(PropertyRow* row)
{
    TreePath result;
    if(row)
    {
        while(row->parent()){
            int childIndex = row->parent()->childIndex(row);
            YASLI_ESCAPE(childIndex >= 0, return TreePath());
            result.insert(result.begin(), childIndex);
            row = row->parent();
        }
    }
    return result;
}

void PropertyTreeModel::selectRow(PropertyRow* row, bool select, bool exclusive)
{
    if(exclusive)
        deselectAll();

    row->setSelected(select);

    Selection::iterator it = std::find(selection_.begin(), selection_.end(), pathFromRow(row));
    if(select){
        if(it == selection_.end())
            selection_.push_back(pathFromRow(row));
        setFocusedRow(row);
    }
    else if(it != selection_.end()){
#if !defined(NDEBUG)
        PropertyRow* it_row = rowFromPath(*it);
#endif
        YASLI_ASSERT(it_row->refCount() > 0 && it_row->refCount() < 0xFFFF);
        selection_.erase(it);
    }
}

void PropertyTreeModel::deselectAll()
{
    Selection::iterator it;
    for(it = selection_.begin(); it != selection_.end(); ++it){
        PropertyRow* row = rowFromPath(*it);
        row->setSelected(false);
    }
    selection_.clear();
}

PropertyRow* PropertyTreeModel::rowFromPath(const TreePath& path)
{
    PropertyRow* row = root();
    if (!root())
        return 0;
    TreePath::const_iterator it;
    for(it = path.begin(); it != path.end(); ++it){
        int index = it->index;
        if(index < int(row->count()) && index >= 0){
            PropertyRow* nextRow = row->childByIndex(index);
            if(!nextRow)
                return row;
            else
                row = nextRow;
        }
        else
            return row;
    }
    return row;
}

void PropertyTreeModel::setSelection(const Selection& selection)
{
    deselectAll();
    Selection::const_iterator it;
    for(it = selection.begin(); it != selection.end(); ++it){
        const TreePath& path = *it;
        PropertyRow* row = rowFromPath(path);
        if(row)
            selectRow(row, true, false);
    }
}

void PropertyTreeModel::clear()
{
    if(root_)
        root_->clear();
    root_ = 0;
    setRoot(new PropertyRow());
    root_->setNames("", "root", "");
    selection_.clear();
}

void PropertyTreeModel::onUpdated(const PropertyRows& rows, bool needApply)
{
    signalUpdated(rows, needApply);
}

void PropertyTreeModel::applyOperator(PropertyTreeOperator* op)
{
    YASLI_ESCAPE(op, return);
    PropertyRow *dest = rowFromPath(op->path_);
    YASLI_ESCAPE(dest && "Unable to apply operator!", return);
    if(op->type_ == PropertyTreeOperator::NONE)
        return;
    YASLI_ESCAPE(op->row_, return);
    if(dest->parent())
        dest->parent()->replaceAndPreserveState(dest, op->row_, 0);
    else{
        op->row_->assignRowProperties(root_);
        root_ = op->row_;
    }
    PropertyRow* newRow = op->row_;
    op->row_ = 0;
    rowChanged(newRow);
}

void PropertyTreeModel::undo()
{
    YASLI_ESCAPE(!undoOperators_.empty(), return);

    auto op = &undoOperators_.back();
    PropertyRow *dest = rowFromPath(op->path_);
    PropertyTreeOperator redoOp = getCurrentStateTreeOperator(dest);

    applyOperator(op);
    undoOperators_.pop_back();

    pushRedo(redoOp);
}

void PropertyTreeModel::redo()
{
    YASLI_ESCAPE(!redoOperators_.empty(), return);

    auto op = &redoOperators_.back();
    PropertyRow *dest = rowFromPath(op->path_);
    PropertyTreeOperator undoOp = getCurrentStateTreeOperator(dest);

    applyOperator(op);
    redoOperators_.pop_back();

    pushUndo(undoOp);
}

void PropertyTreeModel::clearUndo()
{
    undoOperators_.clear();
    redoOperators_.clear();

    Q_EMIT signalUndoRedoStackChanged(false, false);
}

PropertyTreeModel::UpdateLock PropertyTreeModel::lockUpdate()
{
    if(updateLock_)
        return updateLock_;
    else {
        UpdateLock lock = new PropertyTreeModel::LockedUpdate(this);;
        updateLock_ = lock;
        lock->release();
        return lock;
    }
}

void PropertyTreeModel::dismissUpdate()
{
    if(updateLock_)
        updateLock_->dismissUpdate();
}

void PropertyTreeModel::requestUpdate(const PropertyRows& rows, bool apply)
{
    if(updateLock_)
        updateLock_->requestUpdate(rows, apply);
    else
        onUpdated(rows, apply);
}

struct RowObtainer {
    RowObtainer(std::vector<char>& states) : states_(states) {}
    ScanResult operator()(PropertyRow* row)
    {
        states_.push_back(row->expanded() ? 1 : 0);
        return row->expanded() ? SCAN_CHILDREN_SIBLINGS : SCAN_SIBLINGS;
    }
protected:
    std::vector<char>& states_;
};

struct RowExpander {
    RowExpander(const std::vector<char>& states) : states_(states), index_(0) {}
    ScanResult operator()(PropertyRow* row, QPropertyTree* tree, [[maybe_unused]] int index)
    {
        if(size_t(index_) >= states_.size())
            return SCAN_FINISHED;

        if(states_[index_++]){
            if(row->canBeToggled(tree))
                row->_setExpanded(true);
            return SCAN_CHILDREN_SIBLINGS;
        }
        else{
            row->_setExpanded(false);
            return SCAN_SIBLINGS;
        }
    }
protected:
    int index_;
    const std::vector<char>& states_;
};

void PropertyTreeModel::Serialize(Serialization::IArchive& ar, QPropertyTree* tree)
{
    ar(focusedRow_, "focusedRow", 0);       
    ar(selection_, "selection", 0);

    if (root()) {
        std::vector<char> expanded;
        if(ar.IsOutput()) {
            RowObtainer op(expanded);
            root()->scanChildren(op);
        }
        ar(expanded, "expanded", 0);
        if(ar.IsInput()){
            Selection sel = selection_;
            setSelection(sel);
            RowExpander op(expanded);
            root()->scanChildren(op, tree);
            root()->setLayoutChanged();
            root()->setLayoutChangedToChildren();
        }
    }
}

void PropertyTreeModel::pushUndo(const PropertyTreeOperator& op)
{
    PropertyTreeOperator oper = op;
    bool handled = false;
    signalPushUndo(&oper, &handled);
    if(!handled && oper.row_ != 0)
        undoOperators_.push_back(oper);

    Q_EMIT signalUndoRedoStackChanged(!undoOperators_.empty(), !redoOperators_.empty());
}

void PropertyTreeModel::pushRedo(const PropertyTreeOperator& op)
{
    PropertyTreeOperator oper = op;
    bool handled = false;
    signalPushRedo(&oper, &handled);
    if (!handled && oper.row_ != 0)
        redoOperators_.push_back(oper);

    Q_EMIT signalUndoRedoStackChanged(!undoOperators_.empty(), !redoOperators_.empty());
}

PropertyTreeOperator PropertyTreeModel::getCurrentStateTreeOperator(PropertyRow* row)
{
    if (fullUndo_){
        if (undoEnabled_){
            SharedPtr<PropertyRow> clonedRow = root()->clone(constStrings());
            clonedRow->assignRowState(*root(), true);
            return PropertyTreeOperator(TreePath(), clonedRow);
        }
        else{
            return PropertyTreeOperator(TreePath(), 0);
        }
    }
    else{
        if (undoEnabled_){
            SharedPtr<PropertyRow> clonedRow = row->clone(constStrings());
            clonedRow->assignRowState(*row, true);
            return PropertyTreeOperator(pathFromRow(row), clonedRow);
        }
        else{
            return PropertyTreeOperator(pathFromRow(row), 0);
        }
    }
}

void PropertyTreeModel::rowAboutToBeChanged(PropertyRow* row)
{
    YASLI_ESCAPE(row, return);
    pushUndo(getCurrentStateTreeOperator(row));

    // clear the redo stack now
    redoOperators_.clear();
    Q_EMIT signalUndoRedoStackChanged(true, false);
}

void PropertyTreeModel::callRowCallback(PropertyRow* row)
{
    PropertyRow* current = row;
    while (true) {
        Serialization::ICallback* callback = current->callback();
        if (callback) {
        auto applyFunc = [=](void* arg, [[maybe_unused]] const TypeID& type) {
                current->assignToByPointer(arg, callback->Type());
        };
        callback->Call(applyFunc);
            return;
        }
        current = current->parent();
        if (current)
            current->handleChildrenChange();
        else
            break;
    }
}

void PropertyTreeModel::rowChanged(PropertyRow* row, bool apply)
{
    callRowCallback(row);

    YASLI_ESCAPE(row, return);
        row->setLabelChanged();
    row->setLayoutChanged();

    PropertyRow* parentObj = row;
    while (parentObj->parent() && !parentObj->isObject())
        parentObj = parentObj->parent();

    row->setMultiValue(false);

    PropertyRows rows;
    rows.push_back(parentObj);
    requestUpdate(rows, apply);
}

bool PropertyTreeModel::defaultTypeRegistered(const char* typeName) const
{
    return defaultTypes_.find(typeName) != defaultTypes_.end();
}

void PropertyTreeModel::addDefaultType(PropertyRow* row, const char* typeName)
{
    YASLI_ESCAPE(typeName != 0, return);
    defaultTypes_[typeName] = row;
}

PropertyRow* PropertyTreeModel::defaultType(const char* typeName) const
{
    DefaultTypes::const_iterator it = defaultTypes_.find(typeName);
    YASLI_ESCAPE(it != defaultTypes_.end(), return 0);
    return it->second;
}

void PropertyTreeModel::addDefaultType(const TypeID& type, const PropertyDefaultDerivedTypeValue& value)
{
    YASLI_ASSERT(type != TypeID());

    BaseClass& base = defaultTypesPoly_[type];
    for (DerivedTypes::iterator it = base.types.begin(); it != base.types.end(); ++it){
        if (it->registeredName == value.registeredName) {
            YASLI_ASSERT(it->root == 0);
            *it = value;
            return;
        }
    }

    base.types.push_back(value);
    base.strings.push_back(value.label.c_str());
}

const PropertyDefaultDerivedTypeValue* PropertyTreeModel::defaultType(const TypeID& baseType, int derivedIndex) const
{
    DefaultTypesPoly::const_iterator it = defaultTypesPoly_.find(baseType);
    YASLI_ESCAPE(it != defaultTypesPoly_.end(), return 0);
    const BaseClass& base = it->second;
    YASLI_ESCAPE(size_t(derivedIndex) < base.types.size(), return 0);
    return &base.types[derivedIndex];
}

bool PropertyTreeModel::defaultTypeRegistered(const TypeID& baseType, const char* derivedRegisteredName) const
{
    if (!derivedRegisteredName)
        derivedRegisteredName = "";
    DefaultTypesPoly::const_iterator it = defaultTypesPoly_.find(baseType);

    if (it == defaultTypesPoly_.end())
        return false;

    const BaseClass& base = it->second;
    DerivedTypes::const_iterator dit;
    for (dit = base.types.begin(); dit != base.types.end(); ++dit){
        if (dit->registeredName == derivedRegisteredName)
            return true;
    }
    return false;
}

const Serialization::StringList& PropertyTreeModel::typeStringList(const TypeID& baseType) const
{
    DefaultTypesPoly::const_iterator it = defaultTypesPoly_.find(baseType);

    static Serialization::StringList empty;
    YASLI_ESCAPE(it != defaultTypesPoly_.end(), return empty);
    const BaseClass& base = it->second;
    return base.strings;
}

// ----------------------------------------------------------------------------------

bool Serialize(Serialization::IArchive& ar, TreePathLeaf& value, const char* name, const char* label)
{
    return ar(value.index, name, label);
}

bool Serialize(Serialization::IArchive& ar, TreeSelection& value, const char* name, const char* label)
{
    return ar(static_cast<std::vector<TreePath>&>(value), name, label);
}

#include <QPropertyTree/moc_PropertyTreeModel.cpp>
