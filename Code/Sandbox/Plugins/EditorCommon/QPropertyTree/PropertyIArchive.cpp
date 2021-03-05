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
#include "Serialization.h"
#include "Serialization/Enum.h"
#include "Serialization/Callback.h"
#include "PropertyTreeModel.h"
#include "PropertyIArchive.h"
#include "PropertyRowBool.h"
#include "PropertyRowString.h"
#include "PropertyRowNumber.h"
#include "PropertyRowPointer.h"
#include "PropertyRowObject.h"
#include "Unicode.h"

using Serialization::TypeID;

PropertyIArchive::PropertyIArchive(PropertyTreeModel* model, PropertyRow* root)
: IArchive(INPUT | EDIT)
, model_(model)
, currentNode_(0)
, lastNode_(0)
, root_(root)
{
    stack_.push_back(Level());

    if (!root_)
        root_ = model_->root();
    else
        currentNode_ = root;
}

bool PropertyIArchive::operator()(Serialization::IString& value, const char* name, const char* label)
{
    if(openRow(name, label, "string")){
        if(PropertyRowString* row = static_cast<PropertyRowString*>(currentNode_))
            value.set(fromWideChar(row->value().c_str()).c_str());
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(Serialization::IWString& value, const char* name, const char* label)
{
    if(openRow(name, label, "string")){
        if(PropertyRowString* row = static_cast<PropertyRowString*>(currentNode_)) {
            value.set(row->value().c_str());
        }
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(bool& value, const char* name, const char* label)
{
    if(openRow(name, label, "bool")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(char& value, const char* name, const char* label)
{
    if(openRow(name, label, "char")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

// Signed types
bool PropertyIArchive::operator()(int8& value, const char* name, const char* label)
{
    if(openRow(name, label, "int8")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(int16& value, const char* name, const char* label)
{
    if(openRow(name, label, "int16")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(int32& value, const char* name, const char* label)
{
    if(openRow(name, label, "int32")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(int64& value, const char* name, const char* label)
{
    if(openRow(name, label, "int64")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

// Unsigned types
bool PropertyIArchive::operator()(uint8& value, const char* name, const char* label)
{
    if(openRow(name, label, "uint8")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(uint16& value, const char* name, const char* label)
{
    if(openRow(name, label, "uint16")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(uint32& value, const char* name, const char* label)
{
    if(openRow(name, label, "uint32")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(uint64& value, const char* name, const char* label)
{
    if(openRow(name, label, "uint64")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(float& value, const char* name, const char* label)
{
    if(openRow(name, label, "float")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(double& value, const char* name, const char* label)
{
    if(openRow(name, label, "double")){
        currentNode_->assignToPrimitive(&value, sizeof(value));
        closeRow(name);
        return true;
    }
    else
        return false;
}

bool PropertyIArchive::operator()(Serialization::IContainer& ser, const char* name, const char* label)
{
    const char* typeName = ser.containerType().name();
    if(!openRow(name, label, typeName))
        return false;

    size_t size = 0;
    if(currentNode_->multiValue())
        size = ser.size();
    else{
        size = currentNode_->count();
        size = ser.resize(size);
    }

    stack_.push_back(Level());

    size_t index = 0;
    if(ser.size() > 0)
        while(index < size)
        {
            ser(*this, "", "<");
            ser.next();
            ++index;
        }

    stack_.pop_back();

    closeRow(name);
    return true;
}

bool PropertyIArchive::operator()(const Serialization::SStruct& ser, const char* name, const char* label)
{
    PropertyRow* nonLeafNode = 0;
    if(openRow(name, label, ser.type().name())){
        if (currentNode_->isLeaf()) {
            if(!currentNode_->isRoot()){
                currentNode_->assignTo(ser);
                closeRow(name);
                return true;
            }
        }
        else 
            nonLeafNode = currentNode_;
    }
    else
        return false;

    stack_.push_back(Level());

    ser(*this);

    stack_.pop_back();

    if (nonLeafNode)
        nonLeafNode->closeNonLeaf(ser, *this);
    closeRow(name);
    return true;
}


bool PropertyIArchive::operator()(Serialization::IPointer& ser, const char* name, const char* label)
{
    const char* baseName = ser.baseType().name();

    if(openRow(name, label, baseName)){
        if (!currentNode_->isPointer()) {
            closeRow(name);
            return false;
        }

        YASLI_ASSERT(currentNode_);
        PropertyRowPointer* row = static_cast<PropertyRowPointer*>(currentNode_);
        if(!row){
            closeRow(name);
            return false;
        }
        row->assignTo(ser);
    }
    else
        return false;

    stack_.push_back(Level());

    if(ser.get() != 0)
        ser.serializer()( *this );

    stack_.pop_back();

    closeRow(name);
    return true;
}

bool PropertyIArchive::operator()(Serialization::ICallback& callback, const char* name, const char* label)
{
    return callback.SerializeValue(*this, name, label);
}

bool PropertyIArchive::operator()(Serialization::Object& obj, const char* name, const char* label)
{
    if(openRow(name, label, obj.type().name())){
        bool result = false;
        if (currentNode_->isObject()) {
            PropertyRowObject* rowObj = static_cast<PropertyRowObject*>(currentNode_);
            result = rowObj->assignTo(&obj);
        }
        closeRow(name);
        return result;
    }
    else
        return false;
}

bool PropertyIArchive::OpenBlock(const char* name, const char* label)
{
    if(openRow(name, label, "block")){
        stack_.push_back(Level());
        return true;
    }
    else
        return false;
}

void PropertyIArchive::CloseBlock()
{
    closeRow("block");
    stack_.pop_back();
}

bool PropertyIArchive::openRow(const char* name, [[maybe_unused]] const char* label, const char* typeName)
{
    if(!name)
        return false;

    if(!currentNode_){
        lastNode_ = currentNode_ = model_->root();
        YASLI_ASSERT(currentNode_);
        if (currentNode_ && strcmp(currentNode_->typeName(), typeName) != 0)
            return false;
        return true;
    }

    YASLI_ESCAPE(currentNode_, return false);
    
    if(currentNode_->empty())
        return false;

    Level& level = stack_.back();

    PropertyRow* node = 0;
    if(currentNode_->isContainer()){
        if (level.rowIndex < int(currentNode_->children_.size()))
            node = currentNode_->children_[level.rowIndex];
        ++level.rowIndex;
    }
    else {
        node = currentNode_->findFromIndex(&level.rowIndex, name, typeName, level.rowIndex);
        ++level.rowIndex;
    }

    if(node){
        lastNode_ = node;
        if(node->isContainer() || !node->multiValue()){
            currentNode_ = node;
            if (currentNode_ && strcmp(currentNode_->typeName(), typeName) != 0)
                return false;
            return true;
        }
    }
    return false;
}

void PropertyIArchive::closeRow([[maybe_unused]] const char* name)
{
    YASLI_ESCAPE(currentNode_, return);
    currentNode_ = currentNode_->parent();
}
