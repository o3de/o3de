/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzQtComponents/Components/StyleManagerInterface.h>

#include <QObject>
#include <QRegularExpression>

namespace
{
    const char* cStylesheetVariablesKey = "theme_properties";
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

    void StylesheetPreprocessor::Initialize()
    {
        m_styleManagerInterface = AZ::Interface<StyleManagerInterface>::Get();
        AZ_Assert(m_styleManagerInterface, "StylesheetPreprocessor - could not get StyleManagerInterface on StylesheetPreprocessor initialization.");
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
                case '$':
                    i++;
                    state = ParseState::Variable;
                    break;
                default:
                    out.append(*i);
                    i++;
                }
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
                    out.append(QString(m_styleManagerInterface->GetStylePropertyAsString(varName.toStdString().c_str())));
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

} // namespace AzQtComponents

#include "Components/moc_StylesheetPreprocessor.cpp"
