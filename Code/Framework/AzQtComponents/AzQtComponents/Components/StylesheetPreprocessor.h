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
    class StyleManagerInterface;

    class AZ_QT_COMPONENTS_API StylesheetPreprocessor
        : public QObject
    {
        Q_OBJECT

    public:
        explicit StylesheetPreprocessor(QObject* pParent);
        ~StylesheetPreprocessor();

        void Initialize();

        QString ProcessStyleSheet(const QString& stylesheetData);

    private:
        StyleManagerInterface* m_styleManagerInterface = nullptr;
    };
}
