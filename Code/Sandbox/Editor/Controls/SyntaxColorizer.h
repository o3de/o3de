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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : interface for the CSyntaxColorizer class.


#ifndef CRYINCLUDE_EDITOR_CONTROLS_SYNTAXCOLORIZER_H
#define CRYINCLUDE_EDITOR_CONTROLS_SYNTAXCOLORIZER_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QSyntaxHighlighter>
#endif

#define CLR_STRING  QColor(55, 0, 200)
#define CLR_PLAIN   QColor(0, 0, 0)
#define CLR_COMMENT QColor(0, 170, 0)
#define CLR_KEYWORD QColor(0, 0, 255)
#define GRP_KEYWORD     0

class CSyntaxColorizer
    : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    CSyntaxColorizer(QTextDocument* pParent = nullptr);
    virtual ~CSyntaxColorizer();

    //protected vars
protected:
    unsigned char* m_pTableZero, * m_pTableOne;
    unsigned char* m_pTableTwo,  * m_pTableThree;
    unsigned char* m_pTableFour, * m_pAllowable;

    enum Types
    {
        SKIP,
        DQSTART,    //Double Quotes start
        DQEND,      //Double Quotes end
        SQSTART,    //Single Quotes start
        SQEND,      //Single Quotes end
        CMSTART,    //Comment start (both single and multi line)
        MLEND,      //Multi line comment end
        KEYWORD     //Keyword start
    } m_type;

    struct SKeyword
    {
        QString keyword;
        int  keylen;
        QTextCharFormat cf;
        int group;
        SKeyword* pNext;
        SKeyword() { pNext = NULL; }
    };

    SKeyword*   m_pskKeyword;
    QTextCharFormat m_cfComment;
    QTextCharFormat m_cfString;
    QTextCharFormat m_cfDefault;

    //typedef std::map<LPCSTR,SKeyword*,> Keywords;
    //typedef std::hash_map<const char*,SKeyword*,stl::hash_strcmp<const char*> > Keywords;

    //typedef std::map<const char*,SKeyword*,stl::hash_strcmp<const char*> > Keywords;
    typedef std::map<QString, SKeyword*> Keywords;

    Keywords m_keywords;

    //protected member functions
protected:
    void addKey(const QString& Keyword, const QTextCharFormat& cf, int grp);
    void createTables();
    void deleteTables();
    void createDefaultKeywordList();
    void createDefaultCharFormat();

    //public member functions
public:
    void highlightBlock(const QString& text) override;

    void GetCommentStyle(QTextCharFormat& cf) { cf = m_cfComment; };
    void GetStringStyle(QTextCharFormat& cf) { cf = m_cfString; };
    void GetGroupStyle(int grp, QTextCharFormat& cf);
    void GetDefaultStyle(QTextCharFormat& cf) { cf = m_cfDefault; };

    void SetCommentStyle(const QTextCharFormat& cf) { m_cfComment = cf; };
    void SetCommentColor(const QBrush& cr);
    void SetStringStyle(const QTextCharFormat& cf) { m_cfString = cf; };
    void SetStringColor(const QBrush& cr);
    void SetGroupStyle(int grp, const QTextCharFormat& cf);
    void SetGroupColor(int grp, const QBrush& cr);
    void SetDefaultStyle(const QTextCharFormat& cf) { m_cfDefault = cf; };

    void AddKeyword(const char* Keyword, const QTextCharFormat& cf, int grp = 0);
    void AddKeyword(const char* Keyword, const QBrush& cr, int grp = 0);
    void ClearKeywordList();
    QString GetKeywordList();
    QString GetKeywordList(int grp);
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_SYNTAXCOLORIZER_H

