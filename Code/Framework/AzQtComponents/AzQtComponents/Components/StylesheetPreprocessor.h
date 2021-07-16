/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QColor>
#include <QHash>
#include <QObject>
#endif

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API StylesheetPreprocessor
        : public QObject
    {
        Q_OBJECT

    public:
        explicit StylesheetPreprocessor(QObject* pParent);
        ~StylesheetPreprocessor();

        void ClearVariables();
        void ReadVariables(const QString& variables);
        QString ProcessStyleSheet(const QString& stylesheetData);

        const QColor& GetColorByName(const QString& name);

    private:
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::StylesheetPreprocessor::m_namedVariables': class 'QHash<QString,QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::StylesheetPreprocessor'
        QHash<QString, QString> m_namedVariables;
        QHash<QString, QColor> m_cachedColors;
        AZ_POP_DISABLE_WARNING

        QString GetValueByName(const QString& name);
    };
}
