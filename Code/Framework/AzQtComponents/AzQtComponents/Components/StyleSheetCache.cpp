/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StyleSheetCache.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <QProxyStyle>
#include <QWidget>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QStringList>
#include <QRegExp>
#include <QDebug>
#include <QApplication>
#include <QQueue>

namespace AzQtComponents
{

StyleSheetCache::StyleSheetCache(QObject* parent)
: QObject(parent)
, m_fileWatcher(new QFileSystemWatcher(this))
, m_importExpression(new QRegExp("^\\s*@import\\s+\"?([^\"]+)\"?\\s*;(.*)$"))
{
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &StyleSheetCache::fileOnDiskChanged);
}

StyleSheetCache::~StyleSheetCache()
{
}

const QString& StyleSheetCache::styleSheetExtension()
{
    static const QString extension{QStringLiteral(".qss")};
    return extension;
}

void StyleSheetCache::registerPathsFoundOnDisk(const QString& pathOnDisk, const QString& qrcPrefix)
{
    // Do a sanity check to ensure there are no style-sheets on disk that don't exist in a qrc
    QDir rootDirectory(pathOnDisk);
    QQueue<QFileInfo> entriesToScan;
    entriesToScan.push_back(QFileInfo(pathOnDisk));
    while (!entriesToScan.empty())
    {
        QFileInfo entry = entriesToScan.front();
        entriesToScan.pop_front();

        if (!entry.exists())
        {
            continue;
        }

        if (entry.isDir())
        {
            for (auto subEntry : QDir(entry.absoluteFilePath()).entryInfoList({"*.qss"}, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files))
            {
                entriesToScan.push_back(subEntry);
            }
        }
        else
        {
            QString diskPath = entry.absoluteFilePath();
            QString qrcPath = QStringLiteral("%1/%2")
                .arg(qrcPrefix)
                .arg(rootDirectory.relativeFilePath(diskPath));
            QFileInfo qrcInfo(qrcPath);
            if (qrcInfo.exists())
            {
                m_diskToQrcMap[diskPath] = qrcPath;
            }
            else
            {
                AZ_Warning("StyleSheetCache", false,
                    "No QRC entry found for style sheet found on disk. Disk path: \"%s\" Expected QRC path: \"%s\"",
                    diskPath.toUtf8().constData(), qrcPath.toUtf8().constData());
            }
        }
    }
}

void StyleSheetCache::addSearchPaths(const QString& searchPrefix, const QString& pathOnDisk, const QString& qrcPrefix,
    const AZ::IO::PathView& engineRootPath)
{
    AZ_Warning("StyleSheetCache", m_prefixes.find(searchPrefix) == m_prefixes.end(),
        "Style prefix \"%s\" was already registered, ignoring...", searchPrefix.constData());

    // If pathOnDisk is a relative path, search from the engine root directory
    QString diskPathToUse = pathOnDisk;
    if (!QFileInfo(pathOnDisk).isAbsolute())
    {
        QDir rootDir = QString::fromUtf8(engineRootPath.Native().data(), aznumeric_cast<int>(engineRootPath.Native().size()));
        diskPathToUse = rootDir.absoluteFilePath(pathOnDisk);
    }

    registerPathsFoundOnDisk(diskPathToUse, qrcPrefix);

    // Specifying the path to the file on disk and the qrc prefix of the file in this order means
    // that the style will be loaded from disk if it exists, otherwise the style in the Qt Resource
    // file will be used. Styles loaded from disk will be automatically watched and re-applied if
    // changes are detected, allowing much faster style iteration.
    m_prefixes.insert(searchPrefix);
    QDir::addSearchPath(searchPrefix, diskPathToUse);
    QDir::addSearchPath(searchPrefix, qrcPrefix);
}

void StyleSheetCache::setFallbackSearchPaths(const QString& fallbackPrefix, const QString& pathOnDisk, const QString& qrcPrefix)
{
    if (m_fallbackPrefix == fallbackPrefix)
    {
        return;
    }

    if (!m_fallbackPrefix.isEmpty())
    {
        QDir::setSearchPaths(m_fallbackPrefix, {});
    }

    m_fallbackPrefix = fallbackPrefix;

    if (m_fallbackPrefix.isEmpty())
    {
        return;
    }

    registerPathsFoundOnDisk(pathOnDisk, qrcPrefix);
    QDir::setSearchPaths(m_fallbackPrefix, {pathOnDisk, qrcPrefix});
}

void StyleSheetCache::clearCache()
{
    m_styleSheetCache.clear();
}

void StyleSheetCache::fileOnDiskChanged(const QString& filePath)
{
    qDebug() << "All styleSheets reloading, triggered by " << filePath << " changing on disk.";

    // Much easier to just reload all stylesheets, instead of trying to figure out
    // which ones are affected by this file.
    // If we were to worry about just one file, we'd need to keep track of dependency
    // info when preprocessing @import's
    clearCache();
    emit styleSheetsChanged();
}

QString StyleSheetCache::loadStyleSheet(QString styleFileName)
{
    // include the file extension here; it'll make life easier when comparing file paths
    if (!styleFileName.endsWith(styleSheetExtension()))
    {
        styleFileName.append(styleSheetExtension());
    }

    // check the cache
    if (m_styleSheetCache.contains(styleFileName))
    {
        return m_styleSheetCache[styleFileName];
    }

    QString filePath = findStyleSheetPath(styleFileName);
    if (filePath.isEmpty())
    {
        return QString();
    }

    QFileInfo filePathInfo(filePath);
    // watch this file for changes now, if it's not loaded from resources
    if (filePathInfo.exists() && filePathInfo.isNativePath())
    {
        m_fileWatcher->addPath(filePath);

        QString absolutePath = filePathInfo.absoluteFilePath();
        if (m_diskToQrcMap.find(absolutePath) == m_diskToQrcMap.end())
        {
            AZ_Error("StyleSheetCache", false, "No QRC entry was found for style-sheet loaded from disk, loading has been disabled: %s", absolutePath.toUtf8().constData());
            return {};
        }
    }

    QString loadedStyleSheet;

    if (QFile::exists(filePath))
    {
        QFile styleSheetFile;
        styleSheetFile.setFileName(filePath);
        if (styleSheetFile.open(QFile::ReadOnly))
        {
            loadedStyleSheet = styleSheetFile.readAll();
        }
    }

    // pre-process stylesheet
    loadedStyleSheet = preprocess(styleFileName, loadedStyleSheet);

    m_styleSheetCache.insert(styleFileName, loadedStyleSheet);

    return loadedStyleSheet;
}

class MiniLessParser
{
public:
    MiniLessParser(StyleSheetCache* cache);

    QString process(const QString& styleSheet);

private:

    enum class State
    {
        Default,
        Comment,
    };

    void parseLine(const QString& line);
    int parseForImportStatement(const QString& line);

    State m_state = State::Default;
    QRegExp m_importStatement;
    QChar m_lastCharacter;
    QString m_result;
    int m_lineNumber = 0;
    StyleSheetCache* m_cache = nullptr;
};

MiniLessParser::MiniLessParser(StyleSheetCache* cache)
    : m_importStatement("@import\\s+\"?([^\"]+)\"?\\s*;")
    , m_cache(cache)
{
}

QString MiniLessParser::process(const QString& styleSheet)
{
    m_state = State::Default;
    m_result.reserve(styleSheet.size());
    m_lineNumber = 0;
    m_lastCharacter = QChar();

    QStringList lines = styleSheet.split(QRegExp("[\\n\\r]"), Qt::SkipEmptyParts);
    for (QString& line : lines)
    {
        parseLine(line);

        m_lineNumber++;
    }

    return m_result;
}

void MiniLessParser::parseLine(const QString& line)
{
    for (int i = 0; i < line.size(); i++)
    {
        QChar currentChar = line[i];
        switch (m_state)
        {
            case State::Default:
            {
                if (currentChar == '@')
                {
                    // parse for import
                    i += parseForImportStatement(line.mid(i));
                    currentChar = QChar();
                }
                else
                {
                    if ((m_lastCharacter == '/') && (currentChar == '*'))
                    {
                        m_state = State::Comment;
                    }

                    m_result += currentChar;
                }
            }
            break;

            case State::Comment:
            {
                if ((m_lastCharacter == '*') && (currentChar == '/'))
                {
                    m_state = State::Default;
                }
                m_result += currentChar;
            }
            break;
        }

        m_lastCharacter = currentChar;
    }

    m_result += "\n";

    // reset our state
    m_lastCharacter = QChar();
    m_state = State::Default;
}

int MiniLessParser::parseForImportStatement(const QString& line)
{
    QString importName;
    int ret = 0;
    int pos = m_importStatement.indexIn(line, 0);
    if (pos != -1)
    {
        importName = m_importStatement.cap(1);
        ret = m_importStatement.cap(0).size();

        if (!importName.isEmpty())
        {
            // attempt to import
            QString subStyleSheet = m_cache->loadStyleSheet(importName);
            if (!subStyleSheet.isEmpty())
            {
                // error?
            }

            m_result += subStyleSheet;
        }
    }

    return ret;
}

QString StyleSheetCache::preprocess(QString styleFileName, QString loadedStyleSheet)
{
    // Add in really basic support in here for less style @import statements
    // This allows us to split up css chunks into other files, to group things
    // in a much saner way

    // check for dumb recursion
    if (m_processingFiles.contains(styleFileName))
    {
        qDebug() << QString("Recursion found while processing styleSheets in the following order:");
        for (QString& file : m_processingStack)
        {
            qDebug() << file;
        }

        return loadedStyleSheet;
    }

    m_processingStack.push_back(styleFileName);
    m_processingFiles.insert(styleFileName);

    // take a guess at the size of the string needed with imports
    QString result;
    result.reserve(loadedStyleSheet.size() * 3);

    // run our mini less parser on the stylesheet, to process imports now
    MiniLessParser lessParser(this);
    result = lessParser.process(loadedStyleSheet);

    m_processingStack.pop_back();
    m_processingFiles.remove(styleFileName);

    return result;
}

QString StyleSheetCache::findStyleSheetPath(const QString& styleFileName)
{
    if (styleFileName.contains(':'))
    {
        // The file name is in a resource file (":/style.qss")
        // or already has a prefix ("prefix:style.qss")
        // or an absolute path on Windows
        return styleFileName;
    }

    if (QFile::exists(styleFileName))
    {
        return styleFileName;
    }

    const auto stackSize = m_processingStack.size();
    if (stackSize > 0)
    {
        // Resursively search ancestors of the file currently being processed
        const auto ancestorStyleFileName = m_processingStack.pop();
        const auto ancestorFilePath = findStyleSheetPath(ancestorStyleFileName);
        m_processingStack.push(ancestorStyleFileName);

        if (QFile::exists(ancestorFilePath))
        {
            auto prefix = ancestorFilePath.mid(0, ancestorFilePath.indexOf(':'));
            if (ancestorFilePath.contains(':') && prefix.length() != 1)
            {
                // QDir::isAbsolutePath returns true for paths with a valid search prefix. The
                // only way to distinguish between a prefix and absolute path on Windows is the
                // length of the prefix. Prefix length must be at least two to avoid conflicting
                // with Windows drive letters. However, files in Qt Resources files have a prefix
                // length of zero.
                const auto result = QString("%1:%2").arg(prefix, styleFileName);
                if (QFile::exists(result))
                {
                    return result;
                }
            }
            else
            {
                // Assume the styleFileName is relative to the ancestorFilePath
                const QFileInfo ancestorFileInfo(ancestorFilePath);
                const auto result = ancestorFileInfo.absoluteDir().absoluteFilePath(styleFileName);
                if (QFile::exists(result))
                {
                    return result;
                }
            }
        }
    }

    // If we didn't find it in the processing stack, search all known prefixes
    for (const auto& prefix : m_prefixes)
    {
        const auto result = QString("%1:%2").arg(prefix, styleFileName);
        if (QFile::exists(result))
        {
            return result;
        }
    }

    // Finally, fall back to m_fallbackPrefix
    return m_fallbackPrefix.isEmpty() ? QString() : QString("%1:%2").arg(m_fallbackPrefix, styleFileName);
}

} // namespace AzQtComponents

#include "Components/moc_StyleSheetCache.cpp"
