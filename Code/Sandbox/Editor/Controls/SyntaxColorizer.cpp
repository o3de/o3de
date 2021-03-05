/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
// SyntaxColorizer.cpp: implementation of the CSyntaxColorizer class.
//
// Version: 1.0.0
// Author:  Jeff Schering jeffschering@hotmail.com
// Date:    Jan 2001
// Copyright 2001 by Jeff Schering
//
//////////////////////////////////////////////////////////////////////

#include "EditorDefs.h"

#include "SyntaxColorizer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSyntaxColorizer::CSyntaxColorizer(QTextDocument *pParent)
    : QSyntaxHighlighter(pParent)
{
    createDefaultCharFormat();
    SetCommentColor(CLR_COMMENT);
    SetStringColor(CLR_STRING);
    createTables();
    m_pskKeyword = NULL;
    createDefaultKeywordList();
}

CSyntaxColorizer::~CSyntaxColorizer()
{
    ClearKeywordList(); 
    deleteTables(); 
}

//////////////////////////////////////////////////////////////////////
// Member Functions
//////////////////////////////////////////////////////////////////////
void CSyntaxColorizer::createDefaultCharFormat()
{
    m_cfComment = m_cfDefault;
    m_cfString = m_cfDefault;
}

void CSyntaxColorizer::createDefaultKeywordList()
{
    const char *sKeywords = "__asm,else,main,struct,__assume,enum,"
        "__multiple_inheritance,switch,auto,__except,__single_inheritance,"
        "template,__based,explicit,__virtual_inheritance,this,bool,extern,"
        "mutable,thread,break,false,naked,throw,case,__fastcall,namespace,"
        "true,catch,__finally,new,try,__cdecl,float,noreturn,__try,char,for,"
        "operator,typedef,class,friend,private,typeid,const,goto,protected,"
        "typename,const_cast,if,public,union,continue,inline,register,"
        "unsigned,__declspec,__inline,reinterpret_cast,using,declaration,"
        "directive,default,int,return,uuid,delete,__int8,short,"
        "__uuidof,dllexport,__int16,signed,virtual,dllimport,__int32,sizeof,"
        "void,do,__int64,static,volatile,double,__leave,static_cast,wmain,"
        "dynamic_cast,long,__stdcall,while";
    const char *sDirectives = "#define,#elif,#else,#endif,#error,#ifdef,"
        "#ifndef,#import,#include,#line,#pragma,#undef";
    const char *sPragmas = "alloc_text,comment,init_seg1,optimize,auto_inline,"
        "component,inline_depth,pack,bss_seg,data_seg,"
        "inline_recursion,pointers_to_members1,check_stack,"
        "function,intrinsic,setlocale,code_seg,hdrstop,message,"
        "vtordisp1,const_seg,include_alias,once,warning"; 

    AddKeyword(sKeywords,CLR_KEYWORD,GRP_KEYWORD);
    AddKeyword(sDirectives,CLR_KEYWORD,GRP_KEYWORD);
    AddKeyword(sPragmas,CLR_KEYWORD,GRP_KEYWORD);
}

void CSyntaxColorizer::createTables()
{
    m_pTableZero = new unsigned char[256]; m_pTableOne   = new unsigned char[256];
    m_pTableTwo  = new unsigned char[256]; m_pTableThree = new unsigned char[256];
    m_pTableFour = new unsigned char[256]; m_pAllowable  = new unsigned char[256];

    memset(m_pTableZero,SKIP,256); memset(m_pTableOne,SKIP,256);
    memset(m_pTableTwo,SKIP,256);  memset(m_pTableThree,SKIP,256);
    memset(m_pTableFour,SKIP,256); memset(m_pAllowable,false,256);

    *(m_pTableZero + int('"')) = DQSTART; *(m_pTableZero + int('\''))  = SQSTART;
    *(m_pTableZero + int('/')) = CMSTART; *(m_pTableOne + int('"'))    = DQEND;
    *(m_pTableTwo + int('\'')) = SQEND;
    *(m_pTableFour + int('*')) = MLEND;

    *(m_pAllowable + int('\n')) = true; *(m_pAllowable + int('\r')) = true;
    *(m_pAllowable + int('\t')) = true; *(m_pAllowable + int('\0')) = true;
    *(m_pAllowable + int(' '))  = true; *(m_pAllowable + int(';'))  = true;
    *(m_pAllowable + int('('))  = true; *(m_pAllowable + int(')'))  = true;
    *(m_pAllowable + int('{'))  = true; *(m_pAllowable + int('}'))  = true;
    *(m_pAllowable + int('['))  = true; *(m_pAllowable + int(']'))  = true;
    *(m_pAllowable + int('*'))  = true;
}

void CSyntaxColorizer::deleteTables()
{
    delete m_pTableZero;  delete m_pTableOne;  delete m_pTableTwo;
    delete m_pTableThree; delete m_pTableFour; delete m_pAllowable;
}

void CSyntaxColorizer::AddKeyword(const char *Keyword, const QBrush &cr, int grp)
{
    QTextCharFormat cf = m_cfDefault;
    cf.setForeground(cr);

    for (auto token : QString(Keyword).split(","))
    {
        addKey(token,cf,grp);
    }
}

void CSyntaxColorizer::AddKeyword(const char *Keyword, const QTextCharFormat& cf, int grp)
{
    for (auto token : QString(Keyword).split(","))
    {
        addKey(token,cf,grp);
    }
}

void CSyntaxColorizer::addKey(const QString& Keyword, const QTextCharFormat& cf, int grp) //add in ascending order
{
    SKeyword* pskNewKey = new SKeyword;
    SKeyword* prev,*curr;

    pskNewKey->keyword = Keyword;

    m_keywords[pskNewKey->keyword] = pskNewKey;

    pskNewKey->cf = cf;
    pskNewKey->group = grp;
    pskNewKey->pNext = NULL;
    *(m_pTableZero + pskNewKey->keyword[0].toLatin1()) = KEYWORD;

    //if list is empty, add first node
    if(m_pskKeyword == NULL)
        m_pskKeyword = pskNewKey; 
    else
    {
        //check to see if new node goes before first node
        if(Keyword.compare(m_pskKeyword->keyword) < 0)
        {
            pskNewKey->pNext = m_pskKeyword;
            m_pskKeyword = pskNewKey;
        }
        //check to see if new keyword already exists at the first node
        else if(Keyword == m_pskKeyword->keyword)
        {
            //the keyword exists, so replace the existing with the new
            pskNewKey->pNext = m_pskKeyword->pNext;
            delete m_pskKeyword;
            m_pskKeyword = pskNewKey;
        }
        else
        {
            prev = m_pskKeyword;
            curr = m_pskKeyword->pNext;
            while(curr != NULL && curr->keyword.compare(Keyword) < 0)
            {
                prev = curr;
                curr = curr->pNext;
            }
            if(curr != NULL && curr->keyword == Keyword)
            {
                //the keyword exists, so replace the existing with the new
                prev->pNext = pskNewKey;
                pskNewKey->pNext = curr->pNext;
                delete curr;
            }
            else
            {
                pskNewKey->pNext = curr;
                prev->pNext = pskNewKey;
            }
        }
    }
}

void CSyntaxColorizer::ClearKeywordList()
{
    SKeyword* pTemp = m_pskKeyword;

    m_keywords.clear();

    while(m_pskKeyword != NULL)
    {
        *(m_pTableZero + m_pskKeyword->keyword[0].toLatin1()) = SKIP;
        pTemp = m_pskKeyword->pNext;
        delete m_pskKeyword;
        m_pskKeyword = pTemp;
    }
}

QString CSyntaxColorizer::GetKeywordList()
{
    QString sList;

    SKeyword* pTemp = m_pskKeyword;

    while(pTemp != NULL)
    {
        if(!sList.isEmpty())
            sList += QLatin1Char(',');
        sList += pTemp->keyword;
        pTemp = pTemp->pNext;
    }

    return sList;
}

QString CSyntaxColorizer::GetKeywordList(int grp)
{
    QString sList;

    SKeyword* pTemp = m_pskKeyword;

    while(pTemp != NULL)
    {
        if(pTemp->group == grp)
        {
            if(!sList.isEmpty())
                sList += QLatin1Char(',');
            sList += pTemp->keyword;
        }
        pTemp = pTemp->pNext;
    }

    return sList;
}

void CSyntaxColorizer::SetCommentColor(const QBrush& cr)
{
    QTextCharFormat cf = m_cfComment;
    cf.setForeground(cr);

    SetCommentStyle(cf);
}

void CSyntaxColorizer::SetStringColor(const QBrush& cr)
{
    QTextCharFormat cf = m_cfString;

    cf.setForeground(cr);

    SetStringStyle(cf);
}

void CSyntaxColorizer::SetGroupStyle(int grp, const QTextCharFormat& cf)
{
    SKeyword* pTemp = m_pskKeyword;

    while(pTemp != NULL)
    {
        if(pTemp->group == grp)
        {
            pTemp->cf = cf;
        }
        pTemp = pTemp->pNext;
    }
}

void CSyntaxColorizer::GetGroupStyle(int grp, QTextCharFormat& cf)
{
    SKeyword* pTemp = m_pskKeyword;

    while(pTemp != NULL)
    {
        if(pTemp->group == grp)
        {
            cf = pTemp->cf;
            pTemp = NULL;
        }
        else
        {
            pTemp = pTemp->pNext;
            //if grp is not found, return default style
            if(pTemp == NULL) cf = m_cfDefault;
        }
    }
}

void CSyntaxColorizer::SetGroupColor(int grp, const QBrush& cr)
{
    QTextCharFormat cf;
    GetGroupStyle(grp,cf);

    cf.setForeground(cr);

    SetGroupStyle(grp,cf);
}

void CSyntaxColorizer::highlightBlock(const QString& text)
{
    //setup some vars
    char sWord[4096];
    QTextCharFormat cf;
    const char *lpszTemp;
    long iStart = 0;
    long x = 0;
    SKeyword* pskTemp = m_pskKeyword;

    QByteArray latin1Text = text.toLatin1();
    const char *lpszBuf = latin1Text.constData();

    const unsigned char *ppTables[] = { m_pTableZero, m_pTableOne, m_pTableTwo, m_pTableThree, m_pTableFour };
    int iState = previousBlockState();
    if (iState == -1)
        iState = 0;

    //do the work
    while(lpszBuf[x])
    {
        switch(ppTables[iState][int(lpszBuf[x])])
        {
        case DQSTART:
            iState = 1;
            iStart = x;
            break;
        case SQSTART:
            iState = 2;
            iStart = x;
            break;
        case CMSTART:
            if(lpszBuf[x+1] == '/')
            {
                iState = 3;
                iStart = x;
                x++;
            }
            else if(lpszBuf[x+1] == '*')
            {
                iState = 4;
                iStart = x;
                x++;
            }
            else if(lpszBuf[x] == '\'')
            {
                iState = 3;
                iStart = x;
                x++;
            }

            break;
        case MLEND:
            if(lpszBuf[x+1] == '/')
            {
                x++;
                iState = 0;
                setFormat(iStart, x + 1 - iStart, m_cfComment);
            }
            break;
        case DQEND:
            iState = 0;
            setFormat(iStart, x + 1 - iStart, m_cfString);
            break;
        case SQEND:
            if(lpszBuf[x-1] == '\\' && lpszBuf[x+1] == '\'')
                break;
            iState = 0;
            setFormat(iStart, x + 1 - iStart, m_cfString);
            break;

        case KEYWORD:
            // Extract whole word.
            lpszTemp = lpszBuf+x;
            {
                if (x > 0 && !m_pAllowable[int(lpszBuf[x-1])])
                    break;
                
                int i = 0;
                while (!m_pAllowable[int(lpszTemp[i])])
                {
                    sWord[i] = lpszTemp[i];
                    i++;
                }
                if (i > 0)
                {
                    sWord[i] = 0;
                    // Word extracted.
                    // Find keyword.
                    Keywords::iterator it = m_keywords.find( QString(sWord) );
                    if (it != m_keywords.end())
                    {
                        int iStart2 = x;
                        pskTemp = it->second;
                        x += pskTemp->keyword.size();
                        setFormat(iStart2, x - iStart2, pskTemp->cf);
                    }
                }
            }
            /*
            lpszTemp = lpszBuf+x;
            while(pskTemp != NULL)
            {
                if(pskTemp->keyword[0] == lpszTemp[0])
                {
                    int x1=0,y1=0;iStart = iOffset + x;
                    while(pskTemp->keyword[x1])
                    {
                        y1 += lpszTemp[x1] ^ pskTemp->keyword[x1];
                        x1++;
                    }
                    if(y1 == 0 && (*(m_pAllowable + (lpszBuf[x-1])) && 
                            *(m_pAllowable + (lpszBuf[x+pskTemp->keylen]))))
                    {
                        if(_stricmp(pskTemp->keyword,"rem") == 0)
                        {
                            pTable = m_pTableThree;
                        }
                        else 
                        {
                            x += pskTemp->keylen;
                            pCtrl->SetSel(iStart,iOffset + x);
                            pCtrl->SetSelectionCharFormat(pskTemp->cf);
                        }
                    }
                }
                pskTemp = pskTemp->pNext;
            }
            */
            pskTemp = m_pskKeyword;
            break;
        case SKIP:;
        }
        x++;
    }
    //sometimes we get to the end of the file before the end of the string
    //or comment, so we deal with that situation here
    if(iState == 1)
    {
        setFormat(iStart, x + 1 - iStart, m_cfString);
    }
    else if(iState == 2)
    {
        setFormat(iStart, x + 1 - iStart, m_cfString);
    }
    else if(iState == 3)
    {
        setFormat(iStart, x + 1 - iStart, m_cfComment);
        if(lpszBuf[x-2] != '\\') // line continuation character
            iState = 0;
    }
    else if(iState == 4)
    {
        setFormat(iStart, x + 1 - iStart, m_cfComment);
    }

    setCurrentBlockState(iState);
}

#include <Controls/moc_SyntaxColorizer.cpp>
