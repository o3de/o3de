/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_UNICODE_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_UNICODE_H
#pragma once

#include "Strings.h"
#include <stdio.h>

string fromWideChar(const wchar_t* wideCharString);
wstring toWideChar(const char* multiByteString);
wstring fromANSIToWide(const char* ansiString);
string toANSIFromWide(const wchar_t* wstr);


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_UNICODE_H
