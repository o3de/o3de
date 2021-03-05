/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
 // Modifications copyright Amazon.com, Inc. or its affiliates

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_CONSTSTRINGLIST_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_CONSTSTRINGLIST_H
#pragma once


#include <list>
#include <string>
#include "EditorCommonAPI.h"

class ConstStringWrapper;

namespace Serialization { class IArchive; }

bool Serialize(Serialization::IArchive& ar, ConstStringWrapper &wrapper, const char* name, const char* label);

class ConstStringList{
public:
    const char* findOrAdd(const char* string);
protected:
    typedef std::list<std::string> Strings;
    Strings strings_;
};

class ConstStringWrapper {
public:
    ConstStringWrapper(ConstStringList* list, const char*& string);
protected:
    ConstStringList* list_;
    const char*& string_;
    friend bool ::Serialize(Serialization::IArchive& ar, ConstStringWrapper &wrapper, const char* name, const char* label);
};

extern ConstStringList globalConstStringList;


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_CONSTSTRINGLIST_H
