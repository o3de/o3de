/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYOARCHIVE_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYOARCHIVE_H
#pragma once


#include "Serialization/IArchive.h"
#include "Serialization/Pointers.h"

namespace Serialization
{ 
    class CEnumDescription; 
    class Object; 
    struct ICallback;
}

class PropertyRow;
class PropertyTreeModel;
class ValidatorBlock;

using Serialization::SharedPtr;

class PropertyOArchive : public Serialization::IArchive{
public:
    PropertyOArchive(PropertyTreeModel* model, PropertyRow* root, ValidatorBlock* validator);
    ~PropertyOArchive();

    void SetOutlineMode(bool outlineMode);

    inline const SharedPtr<PropertyRow>& currentNode() const {
        return currentNode_;
    }

protected:
    bool operator()(Serialization::IString& value, const char* name, const char* label);
    bool operator()(Serialization::IWString& value, const char* name, const char* label);
    bool operator()(bool& value, const char* name, const char* label);
    bool operator()(char& value, const char* name, const char* label);

    bool operator()(int8& value, const char* name, const char* label);
    bool operator()(int16& value, const char* name, const char* label);
    bool operator()(int32& value, const char* name, const char* label);
    bool operator()(int64& value, const char* name, const char* label);

    bool operator()(uint8& value, const char* name, const char* label);
    bool operator()(uint16& value, const char* name, const char* label);
    bool operator()(uint32& value, const char* name, const char* label);
    bool operator()(uint64& value, const char* name, const char* label);

    bool operator()(float& value, const char* name, const char* label);
    bool operator()(double& value, const char* name, const char* label);

    bool operator()(const Serialization::SStruct& ser, const char* name, const char* label);
    bool operator()(Serialization::IPointer& ptr, const char *name, const char *label);
    bool operator()(Serialization::IContainer& ser, const char *name, const char *label);
    bool operator()(Serialization::Object& obj, const char *name, const char *label);
    bool operator()(Serialization::ICallback& ser, const char *name, const char *label);
    using Serialization::IArchive::operator();

    bool OpenBlock(const char* name, const char* label);
    void CloseBlock();
    void ValidatorMessage(bool error, const void* handle, const Serialization::TypeID& type, const char* message);
    void DocumentLastField(const char* docString);

protected:
    PropertyOArchive(PropertyTreeModel* model, bool forDefaultType);

private:
    struct Level {
        std::vector<SharedPtr<PropertyRow> > oldRows;
        int rowIndex;
        Level() : rowIndex(0) {}
    };
    std::vector<Level> stack_;

    template<class RowType, class ValueType>
    PropertyRow* updateRowPrimitive(const char* name, const char* label, const char* typeName, const ValueType& value, const void* handle, const Serialization::TypeID& typeId);

    template<class RowType, class ValueType>
    RowType* updateRow(const char* name, const char* label, const char* typeName, const ValueType& value);

    void enterNode(PropertyRow* row); // sets currentNode
    void closeStruct(const char* name);
    PropertyRow* defaultValueRootNode();

    bool updateMode_;
    bool defaultValueCreationMode_;
    PropertyTreeModel* model_;
    ValidatorBlock* validator_;
    SharedPtr<PropertyRow> currentNode_;
    SharedPtr<PropertyRow> lastNode_;

    // for defaultArchive
    SharedPtr<PropertyRow> rootNode_;
    std::string typeName_;
    const char* derivedTypeName_;
    std::string derivedTypeNameAlt_;
    bool outlineMode_;
};

// vim:ts=4 sw=4:

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYOARCHIVE_H
