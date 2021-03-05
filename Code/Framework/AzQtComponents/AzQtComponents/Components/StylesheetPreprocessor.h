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
