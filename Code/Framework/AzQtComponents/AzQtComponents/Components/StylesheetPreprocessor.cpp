/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/Trace.h>
#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <QtCore/QObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QRegularExpression>

namespace
{
    const char* cStylesheetVariablesKey = "StylesheetVariables";
}

namespace AzQtComponents
{
    StylesheetPreprocessor::StylesheetPreprocessor(QObject* pParent)
        : QObject(pParent)
    {
    }

    StylesheetPreprocessor::~StylesheetPreprocessor()
    {
    }

    void StylesheetPreprocessor::ClearVariables()
    {
        m_namedVariables.clear();
        m_cachedColors.clear();
    }

    void StylesheetPreprocessor::ReadVariables(const QString& jsonString)
    {
        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject rootObject = doc.object();

        //load in the stylesheet variables
        if (rootObject.contains(cStylesheetVariablesKey))
        {
            QJsonObject variablesObject = rootObject.value(cStylesheetVariablesKey).toObject();
            for (const QString& key : variablesObject.keys())
            {
                m_namedVariables[key] = variablesObject[key].toString();

                // clear any cached colors of the same key, so that they get recached on next fetch
                m_cachedColors.remove(key);
            }
        }
    }

    QString StylesheetPreprocessor::ProcessStyleSheet(const QString& stylesheetData)
    {
        enum class ParseState
        {
            Normal, Variable, Done
        };

        ParseState state = ParseState::Normal;
        QString out;
        QString varName;

        auto i = stylesheetData.cbegin();
        while (state != ParseState::Done && i != stylesheetData.end())
        {
            while (state == ParseState::Normal && i != stylesheetData.end())
            {
                char c = i->toLatin1();
                switch (c)
                {
                case '@':
                    i++;
                    state = ParseState::Variable;
                    break;
                default:
                    out.append(*i);
                    i++;
                }
                ;
            }

            while (state == ParseState::Variable && i != stylesheetData.end())
            {
                char c = i->toLatin1();

                //All characters valid in identifier
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
                {
                    varName.append(*i);
                    i++;
                }
                else
                {
                    //We are finished with reading the current varName
                    out.append(GetValueByName(varName));
                    varName.clear();
                    out.append(*i);
                    i++;
                    state = ParseState::Normal;
                    break;
                }
            }
        }

        return out;
    }


    QString StylesheetPreprocessor::GetValueByName(const QString& name)
    {
        if (m_namedVariables.contains(name))
        {
            return m_namedVariables.value(name);
        }
        else
        {
            return QString("");
        }
    }

    const QColor& StylesheetPreprocessor::GetColorByName(const QString& name)
    {
        if (m_cachedColors.contains(name))
        {
            return m_cachedColors[name];
        }

        if (m_namedVariables.contains(name))
        {
            QColor color;
            QString colorName(m_namedVariables.value(name));

            bool colorSet = false;
            if (QColor::isValidColor(colorName))
            {
                color.setNamedColor(colorName);
                colorSet = true;
            }
            else if (colorName.startsWith("rgb"))
            {
                QRegularExpression expression("\\((.+)\\)");
                QRegularExpressionMatch matches(expression.match(colorName));

                if (matches.hasMatch())
                {
                    QStringList colorComponents = matches.captured(1).split(',', Qt::SkipEmptyParts);
                    if (colorComponents.count() <= 4)
                    {
                        if (colorComponents.count() == 3)
                        {
                            colorComponents.push_back("255");
                        }

                        color.setRgb(
                            colorComponents[0].trimmed().toInt(),
                            colorComponents[1].trimmed().toInt(),
                            colorComponents[2].trimmed().toInt(),
                            colorComponents[3].trimmed().toInt()
                        );

                        colorSet = true;
                    }
                }
            }

            AZ_Assert(colorSet, "Invalid color format specified for %s", name.toUtf8().data());
            m_cachedColors[name] = color;

            return m_cachedColors[name];
        }

        static QColor defaultColor;
        return defaultColor;
    }

} // namespace AzQtComponents

#include "Components/moc_StylesheetPreprocessor.cpp"
