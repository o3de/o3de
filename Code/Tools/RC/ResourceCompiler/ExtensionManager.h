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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_EXTENSIONMANAGER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_EXTENSIONMANAGER_H
#pragma once

// forward declarations.
struct IConvertor;

/** Manages mapping between file extensions and convertors.
*/
class ExtensionManager
{
public:
    ExtensionManager();
    ~ExtensionManager();

    //! Register new convertor with extension manager.
    //!  \param conv must not be 0
    //!  \param rc must not be 0
    void RegisterConvertor(const char* name, IConvertor* conv, IResourceCompiler* rc);
    //! Unregister all convertors.
    void UnregisterAll();

    //! Find convertor that matches given platform and extension.
    IConvertor* FindConvertor(const char* filename) const;

private:
    // Links extensions and convertors.
    typedef std::vector<std::pair<string, IConvertor*> > ExtVector;
    ExtVector m_extVector;

    std::vector<IConvertor*> m_convertors;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_EXTENSIONMANAGER_H
