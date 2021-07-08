/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzCore/IO/Path/Path_fwd.h>

#include <QObject>
#include <QHash>
#include <QString>
#include <QScopedPointer>
#include <QSet>
#include <QStack>
#include <QMap>
#endif

class QFileSystemWatcher;
class QRegExp;

namespace AzQtComponents
{
    class StyleManager;
    class StyleSheetCacheTests;

    class AZ_QT_COMPONENTS_API StyleSheetCache
        : public QObject
    {
        Q_OBJECT

        friend StyleManager;
        friend StyleSheetCacheTests;

        explicit StyleSheetCache(QObject* parent);
        ~StyleSheetCache();

    public:
        static const QString& styleSheetExtension();

        void addSearchPaths(const QString& searchPrefix, const QString& pathOnDisk, const QString& qrcPrefix,
            const AZ::IO::PathView& engineRootPath);
        void setFallbackSearchPaths(const QString& fallbackPrefix, const QString& pathOnDisk, const QString& qrcPrefix);

        QString loadStyleSheet(QString styleFileName);

    public Q_SLOTS:
        void clearCache();

    Q_SIGNALS:
        void styleSheetsChanged();

    private Q_SLOTS:
        void fileOnDiskChanged(const QString& filePath);

    private:
        void registerPathsFoundOnDisk(const QString& pathOnDisk, const QString& qrcPrefix);

        QString preprocess(QString styleFileName, QString loadedStyleSheet);
        QString findStyleSheetPath(const QString& styleFileName);
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::StyleSheetCache::m_styleSheetCache': class 'QHash<QString,QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::StyleSheetCache'
        QHash<QString, QString> m_styleSheetCache;

        QSet<QString> m_processingFiles;
        QStack<QString> m_processingStack;

        QFileSystemWatcher* m_fileWatcher;

        QScopedPointer<QRegExp> m_importExpression;

        QSet<QString> m_prefixes;
        QMap<QString, QString> m_diskToQrcMap;

        QString m_fallbackPrefix;
        AZ_POP_DISABLE_WARNING
};

} // namespace AzQtComponents

