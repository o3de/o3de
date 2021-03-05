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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERXML_XMLCONVERTER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERXML_XMLCONVERTER_H
#pragma once


#include "IConvertor.h"
#include "ICryXML.h"
#include "NameConvertor.h"
#include "XMLBinaryHeaders.h"

struct XMLFilterElement
{
    XMLBinary::IFilter::EType type;
    bool bAccept;
    string wildcards;
};

class XMLCompiler
    : public ICompiler
{
public:
    XMLCompiler(
        ICryXML* pCryXML,
        const std::vector<XMLFilterElement>& filter,
        const std::vector<string>& tableFilemasks,
        const NameConvertor& nameConverter);

public:
    virtual void Release();

    virtual void BeginProcessing([[maybe_unused]] const IConfig* config) { }
    virtual void EndProcessing() { }
    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();

private:
    string GetOutputFileNameOnly() const;
    string GetOutputPath() const;

private:
    int m_refCount;
    ICryXML* m_pCryXML;
    const std::vector<XMLFilterElement>* m_pFilter;
    const std::vector<string>* m_pTableFilemasks;
    const NameConvertor* m_pNameConverter;
    ConvertContext m_CC;
};

class XMLConverter
    : public IConvertor
{
public:
    XMLConverter(ICryXML* pCryXML);
    ~XMLConverter();

    virtual void Release();

    virtual void Init(const ConvertorInitContext& context);
    virtual ICompiler* CreateCompiler();
    virtual const char* GetExt(int index) const;

private:
    int m_refCount;
    ICryXML* m_pCryXML;
    std::vector<XMLFilterElement> m_filter;
    std::vector<string> m_tableFilemasks;
    NameConvertor m_nameConvertor;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERXML_XMLCONVERTER_H
