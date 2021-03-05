/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates.

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYTREEMODEL_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYTREEMODEL_H
#pragma once


#if !defined(Q_MOC_RUN)
#include <map>
#include "PropertyRow.h"
#include "PropertyTreeOperator.h"
#include "Serialization/Pointers.h"
#endif

using std::vector;
using std::map;

struct TreeSelection : vector<TreePath>
{
    bool operator==(const TreeSelection& rhs){
        if(size() != rhs.size())
            return false;
        for(int i = 0; i < int(size()); ++i)
            if((*this)[i] != rhs[i])
                return false;
        return true;
    }
};

struct PropertyDefaultDerivedTypeValue
{
    string registeredName;
    Serialization::SharedPtr<PropertyRow> root;
    Serialization::IClassFactory* factory;
    int factoryIndex;
    std::string label;

    PropertyDefaultDerivedTypeValue()
    : factoryIndex(-1)
    , factory(0)
    {
    }
};

struct PropertyDefaultTypeValue
{
    Serialization::TypeID type;
    string registedName;
    Serialization::SharedPtr<PropertyRow> root;
    Serialization::IClassFactory* factory;
    int factoryIndex;
    std::string label;

    PropertyDefaultTypeValue()
    : factoryIndex(-1)
    , factory(0)
    {
    }
};

// ---------------------------------------------------------------------------

class PropertyTreeModel : public QObject
{
    Q_OBJECT
public:
    class LockedUpdate : public Serialization::RefCounter{
    public:
        LockedUpdate(PropertyTreeModel* model)
        : model_(model)
        , apply_(false)
        {}
        void requestUpdate(const PropertyRows& rows, bool apply) {
            for (size_t i = 0; i < rows.size(); ++i) {
                PropertyRow* row = rows[i];
                if (std::find(rows_.begin(), rows_.end(), row) == rows_.end())
                    rows_.push_back(row);
            }
            if (apply)
                apply_ = true;
        }
        void dismissUpdate(){ rows_.clear(); }
        ~LockedUpdate(){
            model_->updateLock_ = 0;
            if(!rows_.empty())
                model_->signalUpdated(rows_, apply_);
        }
    protected:
        PropertyTreeModel* model_;
        PropertyRows rows_;
        bool apply_;
    };
    typedef Serialization::SharedPtr<LockedUpdate> UpdateLock;

    typedef TreeSelection Selection;

    PropertyTreeModel();
    ~PropertyTreeModel();

    void clear();
    bool canUndo() const{ return !undoOperators_.empty(); }
    void undo();
    bool canRedo() const{ return !redoOperators_.empty(); }
    void redo();
    void clearUndo();

    TreePath pathFromRow(PropertyRow* node);
    PropertyRow* rowFromPath(const TreePath& path);
    void setFocusedRow(PropertyRow* row) { focusedRow_ = pathFromRow(row); }
    PropertyRow* focusedRow() { return rowFromPath(focusedRow_); }

    const Selection& selection() const{ return selection_; }
    void setSelection(const Selection& selection);

    void setRoot(PropertyRow* root) { root_ = root; }
    PropertyRow* root() { return root_; }
    const PropertyRow* root() const { return root_; }

    void Serialize(Serialization::IArchive& ar, QPropertyTree* tree);

    UpdateLock lockUpdate();
    void requestUpdate(const PropertyRows& rows, bool needApply);
    void dismissUpdate();

    void selectRow(PropertyRow* row, bool selected, bool exclusive = true);
    void deselectAll();

    void rowAboutToBeChanged(PropertyRow* row);
    void callRowCallback(PropertyRow* row);
    void rowChanged(PropertyRow* row, bool apply = true); // be careful: it can destroy 'row'

    void setUndoEnabled(bool enabled) { undoEnabled_ = enabled; }
    void setFullUndo(bool fullUndo) { fullUndo_ = fullUndo; }
    void setExpandLevels(int levels) { expandLevels_ = levels; }
    int expandLevels() const{ return expandLevels_; }

    void onUpdated(const PropertyRows& rows, bool needApply);

    // for defaultArchive
    const Serialization::StringList& typeStringList(const Serialization::TypeID& baseType) const;

    bool defaultTypeRegistered(const char* typeName) const;
    void addDefaultType(PropertyRow* propertyRow, const char* typeName);
    PropertyRow* defaultType(const char* typeName) const;

    bool defaultTypeRegistered(const Serialization::TypeID& baseType, const char* derivedRegisteredName) const;
    void addDefaultType(const Serialization::TypeID& baseType, const PropertyDefaultDerivedTypeValue& value);
    const PropertyDefaultDerivedTypeValue* defaultType(const Serialization::TypeID& baseType, int index) const;
    ConstStringList* constStrings() { return &constStrings_; }

signals:
    void signalUpdated(const PropertyRows& rows, bool needApply);
    void signalPushUndo(PropertyTreeOperator* op, bool* result);
    void signalPushRedo(PropertyTreeOperator* op, bool* result);

    void signalUndoRedoStackChanged(bool undosAvailable, bool redosAvailable);
private:
    void applyOperator(PropertyTreeOperator* op);
    void pushUndo(const PropertyTreeOperator& op);
    void pushRedo(const PropertyTreeOperator& op);

    void clearObjectReferences();
    PropertyTreeOperator getCurrentStateTreeOperator(PropertyRow* row);

    TreePath focusedRow_;
    Selection selection_;

    Serialization::SharedPtr<PropertyRow> root_;
    UpdateLock updateLock_;

    typedef std::map<string, Serialization::SharedPtr<PropertyRow> > DefaultTypes;
    DefaultTypes defaultTypes_;


    typedef vector<PropertyDefaultDerivedTypeValue> DerivedTypes;
    struct BaseClass{
        Serialization::TypeID type;
        std::string name;
        Serialization::StringList strings;
        DerivedTypes types;
    };
    typedef map<Serialization::TypeID, BaseClass> DefaultTypesPoly;
    DefaultTypesPoly defaultTypesPoly_;

    int expandLevels_;
    bool undoEnabled_;
    bool fullUndo_;

    std::vector<PropertyTreeOperator> undoOperators_;
    std::vector<PropertyTreeOperator> redoOperators_;

    ConstStringList constStrings_;

    friend class TreeImpl;
};

bool Serialize(Serialization::IArchive& ar, TreeSelection &selection, const char* name, const char* label);
// vim:ts=4 sw=4:

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYTREEMODEL_H
