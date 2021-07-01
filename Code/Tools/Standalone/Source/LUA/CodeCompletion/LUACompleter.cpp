/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"
#include "LUACompleter.hxx"

#include <Source/LUA/CodeCompletion/moc_LUACompleter.cpp>

#include <QRegularExpression>

namespace LUAEditor
{
    Completer::Completer(QAbstractItemModel* model, QObject* pParent)
        : QCompleter(model, pParent)
    {
        setCaseSensitivity(Qt::CaseInsensitive);
        setCompletionMode(QCompleter::CompletionMode::PopupCompletion);
        setModelSorting(QCompleter::ModelSorting::CaseSensitivelySortedModel);
    }

    Completer::~Completer()
    {
    }
    QStringList Completer::splitPath(const QString& path) const
    {
        auto result = path.split(QRegularExpression(c_luaSplit));
        return result;
    }

    int Completer::GetCompletionPrefixTailLength()
    {
        auto result = completionPrefix().split(QRegularExpression(c_luaSplit));
        return result.back().length();
    }
}
