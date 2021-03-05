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
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include "PropertyRowNumber.h"

#define REGISTER_NUMBER_ROW(TypeName, postfix) \
    typedef PropertyRowNumber<TypeName> PropertyRow##postfix; \
    typedef Serialization::RangeDecorator<TypeName> RangeDecorator##postfix; \
    PropertyRow* TypeName##postfixFactory() { return new PropertyRow##postfix; }; \
    REGISTER_IN_FACTORY(PropertyRowFactory, Serialization::TypeID::get<RangeDecorator##postfix>().name(), PropertyRow##postfix, TypeName##postfixFactory); \
    SERIALIZATION_CLASS_NAME(PropertyRow, PropertyRow##postfix, "PropertyRow" #postfix, #TypeName);

REGISTER_NUMBER_ROW(float, Float)
REGISTER_NUMBER_ROW(double , Double)

REGISTER_NUMBER_ROW(char, Char)
REGISTER_NUMBER_ROW(int8, Int8)
REGISTER_NUMBER_ROW(uint8, Uint8)

REGISTER_NUMBER_ROW(int16, Int16)
REGISTER_NUMBER_ROW(int32, Int32)
REGISTER_NUMBER_ROW(int64, Int64)
REGISTER_NUMBER_ROW(uint16, Uint16)
REGISTER_NUMBER_ROW(uint32, Uint32)
REGISTER_NUMBER_ROW(uint64, Uint64)

#undef REGISTER_NUMBER_ROW

DECLARE_SEGMENT(PropertyRowNumber)

// ---------------------------------------------------------------------------
