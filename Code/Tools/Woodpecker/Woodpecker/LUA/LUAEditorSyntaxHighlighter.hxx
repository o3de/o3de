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

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <QSyntaxHighlighter>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <QTextEdit>
#endif

#pragma once

namespace LUAEditor
{
    class LUASyntaxHighlighter
        : public QSyntaxHighlighter
    {
        Q_OBJECT
    public:
        class StateMachine;

        AZ_CLASS_ALLOCATOR(LUASyntaxHighlighter, AZ::SystemAllocator, 0);
        LUASyntaxHighlighter(QWidget* parent);
        LUASyntaxHighlighter(QTextDocument* parent);
        ~LUASyntaxHighlighter();

        void highlightBlock(const QString& text) override;
        //set to -1 to disable bracket highlighting
        void SetBracketHighlighting(int openBracketPos, int closeBracketPos);
        void SetFont(QFont font) { m_font = font; }

        //this is done using QT extraSelections not regular highlighting
        QList<QTextEdit::ExtraSelection> HighlightMatchingNames(const QTextCursor& cursor, const QString& text) const;

        //returns empty string if cursor is not currently on a lua name i.e. in a string literal or comment.
        QString GetLUAName(const QTextCursor& cursor);

    signals:
        void LUANamesInScopeChanged(const QStringList& luaNames) const;

    private:
        void AddBlockKeywords();

        StateMachine* m_machine;
        AZStd::unordered_set<AZStd::string> m_LUAStartBlockKeywords;
        AZStd::unordered_set<AZStd::string> m_LUAEndBlockKeywords;
        int m_openBracketPos{-1};
        int m_closeBracketPos{-1};
        QFont m_font{"OpenSans", 10};
        mutable int m_currentScopeBlock{-1};
    };
}
