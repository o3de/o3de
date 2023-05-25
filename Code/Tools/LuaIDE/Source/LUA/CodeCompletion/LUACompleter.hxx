/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(Completer, AZ::SystemAllocator);

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
