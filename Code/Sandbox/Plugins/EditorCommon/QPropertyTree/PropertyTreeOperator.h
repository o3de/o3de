/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYTREEOPERATOR_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYTREEOPERATOR_H
#pragma once

#include <vector>
#include "Serialization/Pointers.h"

namespace Serialization{ class IArchive; }

class PropertyRow;

struct TreePathLeaf
{
    int index;

    TreePathLeaf(int _index = -1)
        : index(_index)
    {
    }
    bool operator==(const TreePathLeaf& rhs) const{
        return index == rhs.index;
    }
    bool operator!=(const TreePathLeaf& rhs) const{
        return index != rhs.index;
    }
};
bool Serialize(Serialization::IArchive& ar, TreePathLeaf& value, const char* name, const char* label);

typedef std::vector<TreePathLeaf> TreePath;
typedef std::vector<TreePath> TreePathes;

class PropertyTreeOperator
{
public:
    enum Type{
        NONE,
        REPLACE,
        ADD,
        REMOVE
    };

    PropertyTreeOperator();
    ~PropertyTreeOperator();
    PropertyTreeOperator(const TreePath& path, PropertyRow* row);
    void Serialize(Serialization::IArchive& ar);
private:
    Type type_;
    TreePath path_;
    Serialization::SharedPtr<PropertyRow> row_;
    int index_;
    friend class PropertyTreeModel;
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYTREEOPERATOR_H
