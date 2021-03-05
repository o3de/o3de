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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_PROPERTYVARS_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_PROPERTYVARS_H
#pragma once

struct IResourceCompiler;

class CPropertyVars
{
public:
    CPropertyVars(IResourceCompiler* pRC);

    void SetProperty(const string& name, const string& value);
    void RemoveProperty(const string& key);
    void ClearProperties();

    // Return property.
    bool GetProperty(const string& name, string& value) const;
    bool HasProperty(const string& name) const;

    // Expand variables as ${propertyName} with the value of propertyName variable
    void ExpandProperties(string& str) const;

    void PrintProperties() const;

    // Enumerate properties
    // Functor takes two arguments: (name, value)
    template<typename Functor>
    void Enumerate(const Functor& callback) const
    {
        for (PropertyMap::const_iterator it = m_properties.begin(); it != m_properties.end(); ++it)
        {
            callback(it->first, it->second);
        }
    }

private:
    typedef std::map<string, string> PropertyMap;
    PropertyMap m_properties;

    IResourceCompiler* m_pRC;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_PROPERTYVARS_H
