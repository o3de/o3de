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
#include "PropertyTreeOperator.h"
#include "PropertyRow.h"
#include "Serialization/Enum.h"
#include "Serialization/STL.h"
#include "Serialization/Pointers.h"
#include "Serialization/IArchive.h"
#include "Serialization/STLImpl.h"
#include "Serialization/PointersImpl.h"

SERIALIZATION_ENUM_BEGIN_NESTED(PropertyTreeOperator, Type, "PropertyTreeOp")
SERIALIZATION_ENUM_VALUE_NESTED(PropertyTreeOperator, REPLACE, "Replace")
SERIALIZATION_ENUM_VALUE_NESTED(PropertyTreeOperator, ADD, "Add")
SERIALIZATION_ENUM_VALUE_NESTED(PropertyTreeOperator, REMOVE, "Remove")
SERIALIZATION_ENUM_END()

PropertyTreeOperator::PropertyTreeOperator(const TreePath& path, PropertyRow* row)
: type_(REPLACE)
, path_(path)
, index_(-1)
, row_(row)
{
}

PropertyTreeOperator::PropertyTreeOperator()
: type_(NONE)
, index_(-1)
{
}

PropertyTreeOperator::~PropertyTreeOperator()
{
}

void PropertyTreeOperator::Serialize(Serialization::IArchive& ar)
{
    ar(type_, "type", "Type");
    ar(path_, "path", "Path");
    ar(row_, "row", "Row");
    ar(index_, "index", "Index");
}

