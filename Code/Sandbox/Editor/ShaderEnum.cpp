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

// Description : Enumerate Installed Shaders.


#include "EditorDefs.h"

#include "ShaderEnum.h"

//////////////////////////////////////////////////////////////////////////
CShaderEnum::CShaderEnum()
{
    m_bEnumerated = false;
}

CShaderEnum::~CShaderEnum()
{
}


inline bool ShaderLess(const CShaderEnum::ShaderDesc& s1, const CShaderEnum::ShaderDesc& s2)
{
    return QString::compare(s1.name, s2.name, Qt::CaseInsensitive) < 0;
}
/*
struct StringLess {
    bool operator()( const CString &s1,const CString &s2 )
    {
        return _stricmp( s1,s2 ) < 0;
    }
};
*/

//! Enum shaders.
int CShaderEnum::EnumShaders()
{
    IRenderer* renderer = GetIEditor()->GetSystem()->GetIRenderer();
    if (!renderer)
    {
        return 0;
    }

    m_bEnumerated = true;

    m_shaders.clear();
    m_shaders.reserve(100);
    //! Enumerate Shaders.
    int nNumShaders = 0;
    string* files = renderer->EF_GetShaderNames(nNumShaders);
    for (int i = 0; i < nNumShaders; i++)
    {
        ShaderDesc sd;
        sd.name = files[i].c_str();
        sd.file = files[i].c_str();
        if (!sd.name.isEmpty())
        {
            // Capitalize first character of the string.
            sd.name[0] = sd.name[0].toUpper();
        }
        m_shaders.push_back(sd);
    }

    XmlNodeRef root = GetISystem()->GetXmlUtils()->LoadXmlFromFile("Materials/ShaderList.xml");
    if (root)
    {
        for (int i = 0; i < root->getChildCount(); ++i)
        {
            XmlNodeRef ChildNode = root->getChild(i);
            const char* pTagName    =   ChildNode->getTag();
            if (!_stricmp(pTagName, "Shader"))
            {
                QString name;
                if (ChildNode->getAttr("name", name) && !name.isEmpty())
                {
                    // make sure there is no duplication
                    bool isUnique = true;
                    for (std::vector<ShaderDesc>::iterator pSD = m_shaders.begin(); pSD != m_shaders.end(); ++pSD)
                    {
                        if (!QString::compare((*pSD).file, name, Qt::CaseInsensitive))
                        {
                            isUnique = false;
                            break;
                        }
                    }
                    if (isUnique)
                    {
                        ShaderDesc sd;
                        sd.name = name;
                        sd.file = name.toLower();
                        m_shaders.push_back(sd);
                    }
                }
            }
        }
    }

    std::sort(m_shaders.begin(), m_shaders.end(), ShaderLess);
    return m_shaders.size();
}

int CShaderEnum::GetShaderCount() const
{
    return m_shaders.size();
}

QString CShaderEnum::GetShader(int i) const
{
    return m_shaders[i].name;
}

QString CShaderEnum::GetShaderFile(int i) const
{
    return m_shaders[i].file;
}
