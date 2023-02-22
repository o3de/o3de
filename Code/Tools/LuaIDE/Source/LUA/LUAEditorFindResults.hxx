/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Asset/AssetCommon.h>

#include <QSyntaxHighlighter>
#include <QWidget>
#endif

#pragma once

namespace Ui
{
    class LUAEditorFindResults;
}

namespace LUAEditor
{
    class FindResultsHighlighter
        : public QSyntaxHighlighter
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(FindResultsHighlighter, AZ::SystemAllocator);

        FindResultsHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {}
        ~FindResultsHighlighter() {}

        void highlightBlock(const QString& text) override;
        void SetFont(QFont font) { m_font = font; }
        void SetSearchString(const QString& searchString, bool regEx, bool wholeWord, bool caseSensitive);

    private:
        QFont m_font{"OpenSans", 10};
        QString m_searchString;
        bool m_regEx; 
        bool m_wholeWord;
        bool m_caseSensitive;
    };

    class FindResultsBlockInfo : public QTextBlockUserData
    {
    public:
        AZ_CLASS_ALLOCATOR(FindResultsBlockInfo, AZ::SystemAllocator);
        typedef AZStd::function<void(const AZStd::string&, const AZStd::string&)> AssignAssetIdType;

        FindResultsBlockInfo(const AZStd::string& assetId, const char* assetName, int lineNumber, int firstMatchPosition, AssignAssetIdType assignAssetId) 
            : m_assetId(assetId)
            , m_assetName(assetName)
            , m_lineNumber(lineNumber)
            , m_firstMatchPosition(firstMatchPosition)
            , m_assignAssetId(assignAssetId)
        {}

        FindResultsBlockInfo(const FindResultsBlockInfo& rhs)
            : m_assetId(rhs.m_assetId)
            , m_assetName(rhs.m_assetName)
            , m_lineNumber(rhs.m_lineNumber)
            , m_firstMatchPosition(rhs.m_firstMatchPosition)
            , m_assignAssetId(rhs.m_assignAssetId)
        {
        }

        AZStd::string m_assetId;
        AZStd::string m_assetName;
        int m_lineNumber;
        int m_firstMatchPosition;
        AssignAssetIdType m_assignAssetId;
    };

    class FindResults : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(FindResults, AZ::SystemAllocator);

        FindResults(QWidget *parent = 0);
        ~FindResults();

        void Clear();
        QTextDocument* Document();
        void AppendPlainText(const QString& text);
        void AssignAssetId(const AZStd::string& assetName, const AZStd::string& assetId);
        void FinishedAddingText(const QString& searchString, bool regEx, bool wholeWord, bool caseSensitive);

    signals:
        void ResultSelected(const FindResultsBlockInfo& result);

    public slots:
        void OnBlockDoubleClicked(QMouseEvent* event, const QTextBlock& block);

    private:
        Ui::LUAEditorFindResults* m_gui;
        class FindResultsHighlighter* m_highlighter;
        QColor m_resultLineHighlightColor;
    };
}
