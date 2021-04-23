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

#ifndef LUAEDITOR_LUACOMPLETER_H
#define LUAEDITOR_LUACOMPLETER_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QCompleter>
#endif

#pragma once

namespace LUAEditor
{

    class Completer : public QCompleter
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(Completer, AZ::SystemAllocator, 0);

        Completer(QAbstractItemModel* model, QObject* parent);
        virtual ~Completer();

        int GetCompletionPrefixTailLength();

    private:
        const char* c_luaSplit{R"([.:])"};

        QStringList splitPath(const QString & path) const override;

        QStringList m_keywords;
    };
}

#endif
