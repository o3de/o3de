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

#pragma once

#include "PathHelpers.h"
#include "string.h"
#include "IMultiplatformConfig.h"
#include "IResCompiler.h"

class IConfig;

// IConvertContext is a description of what and how should be processed by compiler
struct IConvertContext
{
    virtual void SetConvertorExtension(const char* convertorExtension) = 0;

    virtual void SetSourceFolder(const char* sourceFolder) = 0;
    virtual void SetSourceFileNameOnly(const char* sourceFileNameOnly) = 0;
    virtual void SetOutputFolder(const char* sOutputFolder) = 0;

    virtual void SetRC(IResourceCompiler* pRC) = 0;
    virtual void SetMultiplatformConfig(IMultiplatformConfig* pMultiConfig) = 0;
    virtual void SetPlatformIndex(int platformIndex) = 0;
    virtual void SetForceRecompiling(bool bForceRecompiling) = 0;

    virtual void CopyTo(IConvertContext* context) = 0;

};

struct ConvertContext
    : public IConvertContext
{
    //////////////////////////////////////////////////////////////////////////
    // Interface IConvertContext

    virtual void SetConvertorExtension(const char* convertorExtension)
    {
        this->m_convertorExtension = convertorExtension;
    }

    virtual void SetSourceFolder(const char* sourceFolder)
    {
        this->m_sourceFolder = sourceFolder;
    }
    virtual void SetSourceFileNameOnly(const char* sourceFileNameOnly)
    {
        this->m_sourceFileNameOnly = sourceFileNameOnly;
    }
    virtual void SetOutputFolder(const char* sOutputFolder)
    {
        this->m_outputFolder = sOutputFolder;
    }

    virtual void SetRC(IResourceCompiler* pRC)
    {
        this->m_pRC = pRC;
    }
    virtual void SetMultiplatformConfig(IMultiplatformConfig* pMultiConfig)
    {
        this->m_multiConfig = pMultiConfig;
        this->m_config = &pMultiConfig->getConfig();
        this->m_platform = pMultiConfig->getActivePlatform();
    }
    virtual void SetPlatformIndex(int platformIndex)
    {        
        this->m_platform = platformIndex;
        m_multiConfig->setActivePlatform(platformIndex);
    }
    virtual void SetForceRecompiling(bool bForceRecompiling)
    {
        this->m_bForceRecompiling = bForceRecompiling;
    }

    virtual void CopyTo(IConvertContext* context)
    {
        context->SetConvertorExtension(m_convertorExtension);
        context->SetSourceFolder(m_sourceFolder);
        context->SetSourceFileNameOnly(m_sourceFileNameOnly);
        context->SetOutputFolder(m_outputFolder);
        context->SetRC(m_pRC);
        context->SetMultiplatformConfig(m_multiConfig);
        context->SetForceRecompiling(m_bForceRecompiling);
    }
    //////////////////////////////////////////////////////////////////////////

    const string GetSourcePath() const
    {
        return PathHelpers::Join(m_sourceFolder, m_sourceFileNameOnly).c_str();
    }

    const string& GetOutputFolder() const
    {
        return m_outputFolder;
    }

public:
    // Convertor will assume that the source file has content matching this extension
    // (the sourceFileNameOnly can have a different extension, say 'tmp').
    string m_convertorExtension;

    // Source file's folder.
    string m_sourceFolder;
    // Source file that needs to be converted, for example "test.tif".
    // Contains filename only, the folder is stored in sourceFolder.
    string m_sourceFileNameOnly;

    // Pointer to resource compiler interface.
    IResourceCompiler* m_pRC;

    // Configuration settings.
    IMultiplatformConfig* m_multiConfig;
    // Platform to which file must be processed.
    int m_platform;
    // Platform's config.
    const IConfig* m_config;

    // true if compiler is requested to skip up-to-date checks
    bool m_bForceRecompiling;

private:
    // Output folder.
    string m_outputFolder;
};

