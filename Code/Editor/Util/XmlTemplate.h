/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CXmlTemplate declaration.


#ifndef CRYINCLUDE_EDITOR_UTIL_XMLTEMPLATE_H
#define CRYINCLUDE_EDITOR_UTIL_XMLTEMPLATE_H
#pragma once


/*!
 *  CXmlTemplate is XML base template of parameters.
 *
 */
class CXmlTemplate
{
public:
    //! Scans properties of XML template,
    //! for each property try to find corresponding attribute in specified XML node, and copy
    //! value to Value attribute of template.
    static void GetValues(XmlNodeRef& templateNode, const XmlNodeRef& fromNode);

    //! Scans properties of XML template, fetch Value attribute of each and put as Attribute in
    //! specified XML node.
    static void SetValues(const XmlNodeRef& templateNode, XmlNodeRef& toNode);
    static bool SetValues(const XmlNodeRef& templateNode, XmlNodeRef& toNode, const XmlNodeRef& modifiedNode);

    //! Add parameter to template.
    static void AddParam(XmlNodeRef& templ, const char* paramName, bool value);
    static void AddParam(XmlNodeRef& templ, const char* paramName, int value, int min = 0, int max = 10000);
    static void AddParam(XmlNodeRef& templ, const char* paramName, float value, float min = -10000, float max = 10000);
    static void AddParam(XmlNodeRef& templ, const char* paramName, const char* sValue);
};

/*!
 *  CXmlTemplateRegistry is a collection of all registred templates.
 */
class CXmlTemplateRegistry
{
public:
    CXmlTemplateRegistry();

    void LoadTemplates(const QString& path);
    void AddTemplate(const QString& name, XmlNodeRef& tmpl);

    XmlNodeRef FindTemplate(const QString& name);

private:
    StdMap<QString, XmlNodeRef> m_templates;
};

#endif // CRYINCLUDE_EDITOR_UTIL_XMLTEMPLATE_H
