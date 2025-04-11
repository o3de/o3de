/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/string/string.h>
#include <QColor>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
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

        AZ_CLASS_ALLOCATOR(LUASyntaxHighlighter, AZ::SystemAllocator);
        LUASyntaxHighlighter(QWidget* parent);
        LUASyntaxHighlighter(QTextDocument* parent);
        ~LUASyntaxHighlighter();

        void highlightBlock(const QString& text) override;
        //set to -1 to disable bracket highlighting
        void SetBracketHighlighting(int openBracketPos, int closeBracketPos);

        //this is done using QT extraSelections not regular highlighting
        QList<QTextEdit::ExtraSelection> HighlightMatchingNames(const QTextCursor& cursor, const QString& text) const;

        //returns empty string if cursor is not currently on a lua name i.e. in a string literal or comment.
        QString GetLUAName(const QTextCursor& cursor);

    signals:
        void LUANamesInScopeChanged(const QStringList& luaNames) const;

    private:
        void BuildRegExes();
        void AddBlockKeywords();

        StateMachine* m_machine;
        AZStd::unordered_set<AZStd::string> m_LUAStartBlockKeywords;
        AZStd::unordered_set<AZStd::string> m_LUAEndBlockKeywords;

        struct HighlightingRule
        {
            bool stopProcessingMoreRulesAfterThis = false;
            QRegularExpression pattern;
            AZStd::function<QColor()> colorCB;
        };
        AZStd::vector<HighlightingRule> m_highlightingRules;

        int m_openBracketPos{-1};
        int m_closeBracketPos{-1};
        mutable int m_currentScopeBlock{-1};
    };
}
