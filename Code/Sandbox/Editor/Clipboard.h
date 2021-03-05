/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_CLIPBOARD_H
#define CRYINCLUDE_EDITOR_CLIPBOARD_H

#pragma once

#include "Include/EditorCoreAPI.h"

#include <QTimer>

class CImageEx;
class QVariant;
class QWidget;

/** Use this class to put and get stuff from windows clipboard.
*/
class EDITOR_CORE_API CClipboard
{
public:
    CClipboard(QWidget* parent);

    //! Put xml node into clipboard
    void Put(XmlNodeRef& node, const QString& title = QString());
    //! Get xml node to clipboard.
    XmlNodeRef Get() const;

    //! Put string into Windows clipboard.
    void PutString(const QString& text, const QString& title = QString());
    //! Get string from Windows clipboard.
    QString GetString() const;

    //! Return name of what is in clipboard now.
    QString GetTitle() const { return m_title; };

    //! Put image into Windows clipboard.
    void PutImage(const CImageEx& img);

    //! Get image from Windows clipboard.
    bool GetImage(CImageEx& img);

    //! Return true if clipboard is empty.
    bool IsEmpty() const;
private:
    // Resolves the last request Put operation
    void SendPendingPut();

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    static XmlNodeRef m_node;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    static QString m_title;

    static QVariant s_pendingPut;

    QWidget* m_parent;
    QTimer m_putDebounce;
};


#endif // CRYINCLUDE_EDITOR_CLIPBOARD_H
