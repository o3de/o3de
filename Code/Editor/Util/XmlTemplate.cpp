/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CXmlTemplate implementation.

#include "EditorDefs.h"

#include "XmlTemplate.h"

//////////////////////////////////////////////////////////////////////////
// CXmlTemplate implementation
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::GetValues(XmlNodeRef& node, const XmlNodeRef& fromNode)
{
    assert(node != 0 && fromNode != 0);

    if (!node)
    {
        gEnv->pLog->LogError("CXmlTemplate::GetValues invalid node. Possible problems with Editor folder.");
        return;
    }

    for (int i = 0; i < node->getChildCount(); i++)
    {
        XmlNodeRef prop = node->getChild(i);

        if (prop->getChildCount() == 0)
        {
            QString value;
            if (fromNode->getAttr(prop->getTag(), value))
            {
                prop->setAttr("Value", value.toUtf8().data());
            }
        }
        else
        {
            // Have childs.
            XmlNodeRef fromNodeChild = fromNode->findChild(prop->getTag());
            if (fromNodeChild)
            {
                CXmlTemplate::GetValues(prop, fromNodeChild);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::SetValues(const XmlNodeRef& node, XmlNodeRef& toNode)
{
    assert(node != 0 && toNode != 0);

    toNode->removeAllAttributes();
    toNode->removeAllChilds();

    assert(node);
    if (!node)
    {
        gEnv->pLog->LogError("CXmlTemplate::SetValues invalid node. Possible problems with Editor folder.");
        return;
    }

    for (int i = 0; i < node->getChildCount(); i++)
    {
        XmlNodeRef prop = node->getChild(i);
        if (prop)
        {
            if (prop->getChildCount() > 0)
            {
                XmlNodeRef childToNode = toNode->newChild(prop->getTag());
                if (childToNode)
                {
                    CXmlTemplate::SetValues(prop, childToNode);
                }
            }
            else
            {
                QString value;
                prop->getAttr("Value", value);
                toNode->setAttr(prop->getTag(), value.toUtf8().data());
            }
        }
        else
        {
            assert(!"nullptr returned from node->GetChild()");
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CXmlTemplate::SetValues(const XmlNodeRef& node, XmlNodeRef& toNode, const XmlNodeRef& modifiedNode)
{
    assert(node != 0 && toNode != 0 && modifiedNode != 0);

    for (int i = 0; i < node->getChildCount(); i++)
    {
        XmlNodeRef prop = node->getChild(i);
        if (prop)
        {
            if (prop->getChildCount() > 0)
            {
                XmlNodeRef childToNode = toNode->findChild(prop->getTag());
                if (childToNode)
                {
                    if (CXmlTemplate::SetValues(prop, childToNode, modifiedNode))
                    {
                        return true;
                    }
                }
            }
            else if (prop == modifiedNode)
            {
                QString value;
                prop->getAttr("Value", value);
                toNode->setAttr(prop->getTag(), value.toUtf8().data());
                return true;
            }
        }
        else
        {
            assert(!"nullptr returned from node->GetChild()");
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam(XmlNodeRef& templ, const char* sName, bool value)
{
    XmlNodeRef param = templ->newChild(sName);
    param->setAttr("type", "Bool");
    param->setAttr("value", value);
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam(XmlNodeRef& templ, const char* sName, int value, int min, int max)
{
    XmlNodeRef param = templ->newChild(sName);
    param->setAttr("type", "Int");
    param->setAttr("value", value);
    param->setAttr("min", min);
    param->setAttr("max", max);
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam(XmlNodeRef& templ, const char* sName, float value, float min, float max)
{
    XmlNodeRef param = templ->newChild(sName);
    param->setAttr("type", "Float");
    param->setAttr("value", value);
    param->setAttr("min", min);
    param->setAttr("max", max);
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam(XmlNodeRef& templ, const char* sName, const char* sValue)
{
    XmlNodeRef param = templ->newChild(sName);
    param->setAttr("type", "String");
    param->setAttr("value", sValue);
}


//////////////////////////////////////////////////////////////////////////
//
// CXmlTemplateRegistry implementation
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CXmlTemplateRegistry::CXmlTemplateRegistry()
{}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplateRegistry::LoadTemplates(const QString& path)
{
    m_templates.Clear();

    QString dir = Path::AddPathSlash(path);

    IFileUtil::FileArray files;
    CFileUtil::ScanDirectory(dir, "*.xml", files, false);

    for (int k = 0; k < files.size(); k++)
    {
        XmlNodeRef child;
        // Construct the full filepath of the current file
        XmlNodeRef node = XmlHelpers::LoadXmlFromFile((dir + files[k].filename).toUtf8().data());
        if (node != nullptr && node->isTag("Templates"))
        {
            QString name;
            for (int i = 0; i < node->getChildCount(); i++)
            {
                child = node->getChild(i);
                AddTemplate(child->getTag(), child);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplateRegistry::AddTemplate(const QString& name, XmlNodeRef& tmpl)
{
    m_templates[name] = tmpl;
}


//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlTemplateRegistry::FindTemplate(const QString& name)
{
    XmlNodeRef node;
    if (m_templates.Find(name, node))
    {
        return node;
    }
    return nullptr;
}
