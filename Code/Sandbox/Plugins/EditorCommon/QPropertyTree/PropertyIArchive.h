/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates.

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYIARCHIVE_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYIARCHIVE_H
#pragma once


#include "Serialization/IArchive.h"

namespace Serialization{
    class CEnumDescription;
    class Object;
}

class PropertyRow;
class PropertyTreeModel;

class PropertyIArchive : public Serialization::IArchive{
public:
    PropertyIArchive(PropertyTreeModel* model, PropertyRow* root);

protected:
    bool operator()(Serialization::IString& value, const char* name, const char* label);
    bool operator()(Serialization::IWString& value, const char* name, const char* label);
    bool operator()(bool& value, const char* name, const char* label);
    bool operator()(char& value, const char* name, const char* label);

    // Signed types
    bool operator()(int8& value, const char* name, const char* label);
    bool operator()(int16& value, const char* name, const char* label);
    bool operator()(int32& value, const char* name, const char* label);
    bool operator()(int64& value, const char* name, const char* label);
    // Unsigned types
    bool operator()(uint8& value, const char* name, const char* label);
    bool operator()(uint16& value, const char* name, const char* label);
    bool operator()(uint32& value, const char* name, const char* label);
    bool operator()(uint64& value, const char* name, const char* label);

    bool operator()(float& value, const char* name, const char* label);
    bool operator()(double& value, const char* name, const char* label);

    bool operator()(const Serialization::SStruct& ser, const char* name, const char* label);
    bool operator()(Serialization::IPointer& ser, const char* name, const char* label);
    bool operator()(Serialization::IContainer& ser, const char* name, const char* label);
    bool operator()(Serialization::Object& obj, const char* name, const char* label);
    bool operator()(Serialization::ICallback& callback, const char* name, const char* label);
    using Serialization::IArchive::operator();

    bool OpenBlock(const char* name, const char* label);
    void CloseBlock();

protected:
    bool needDefaultArchive([[maybe_unused]] const char* baseName) const { return false; }
private:
    bool openRow(const char* name, const char* label, const char* typeName);
    void closeRow(const char* name);

    struct Level {
        int rowIndex;
        Level() : rowIndex(0) {}
    };

    vector<Level> stack_;

    PropertyTreeModel* model_;
    PropertyRow* currentNode_;
    PropertyRow* lastNode_;
    PropertyRow* root_;
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYIARCHIVE_H
