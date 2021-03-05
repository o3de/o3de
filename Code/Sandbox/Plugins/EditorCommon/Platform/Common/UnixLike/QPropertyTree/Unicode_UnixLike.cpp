/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

#include <QPropertyTree/Unicode.h>

#include <vector>
#include <QtCore/QString>

string fromWideChar(const wchar_t* wstr)
{
    return QString::fromWCharArray(wstr).toUtf8().data();
}

wstring toWideChar(const char* str)
{
    QString s = QString::fromUtf8(str);

    std::vector<wchar_t> result(s.size()+1);
    s.toWCharArray(&result[0]);
    return &result[0];
}

