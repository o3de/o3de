/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "Plugin.h"

// Editor
#include "Include/IViewPane.h"

#include <QMessageBox>

CClassFactory* CClassFactory::s_pInstance = nullptr;
CAutoRegisterClassHelper* CAutoRegisterClassHelper::s_pFirst = nullptr;
CAutoRegisterClassHelper* CAutoRegisterClassHelper::s_pLast = nullptr;

CClassFactory::CClassFactory()
{
    m_classes.reserve(100);
    RegisterAutoTypes();
}

CClassFactory::~CClassFactory()
{
    for (int i = 0; i < m_classes.size(); i++)
    {
        m_classes[i]->Release();
    }
}

void CClassFactory::RegisterAutoTypes()
{
    CAutoRegisterClassHelper* pClass = CAutoRegisterClassHelper::s_pFirst;

    while (pClass)
    {
        RegisterClass(pClass->m_pClassDesc);
        pClass = pClass->m_pNext;
    }
}

CClassFactory* CClassFactory::Instance()
{
    if (!s_pInstance)
    {
        s_pInstance = new CClassFactory;
    }

    return s_pInstance;
}

void CClassFactory::RegisterClass(IClassDesc* pClassDesc)
{
    assert(pClassDesc);

    auto findByGuid = m_guidToClass.find(pClassDesc->ClassID());

    if (findByGuid != m_guidToClass.end())
    {
        // you may not re-use UUIDs!
        // this can happen when someone makes a new plugin or downloads one
        // where the author copied and pasted code.  This can also happen if two DLLs exist which are actually the same plugin
        // either way, give info if you can:
        char errorMessageBuffer[512] = { 0 };
        AZ::Uuid newUUID(pClassDesc->ClassID());
        char existingUUIDString[64] = { 0 };
        newUUID.ToString(existingUUIDString, 64, true, true);

        sprintf_s(errorMessageBuffer, 512,
            "Error registering class '%s' -  UUID of that class already exists (%s), and belongs to class named '%s.'\n"
            "You may not have duplicate class identifiers.\n"
            "Check for duplicate plugins or copy and pasted code for plugins that duplicates UUIDs returned.",
            pClassDesc->ClassName().toUtf8().data(),
            existingUUIDString,
            findByGuid->second->ClassName().toUtf8().data());

        QMessageBox::critical(nullptr,"Invalid class registration - Duplicate UUID",QString::fromLatin1(errorMessageBuffer));
        return;
    }

    auto findByName = m_nameToClass.find(pClassDesc->ClassName());
    if (findByName != m_nameToClass.end())
    {
        // you may not re-use names, either!
        char errorMessageBuffer[512] = { 0 };

        AZ::Uuid newUUID(pClassDesc->ClassID());
        char newUUIDString[64] = { 0 };
        newUUID.ToString(newUUIDString, 64, true, true);

        AZ::Uuid existingUUID(findByName->second->ClassID());
        char existingUUIDString[64] = { 0 };
        existingUUID.ToString(existingUUIDString, 64, true, true);

        sprintf_s(errorMessageBuffer, 512,
            "Error registering class '%s' - that name is already taken by a different class.\n"
            "New class's UUID is %s\n"
            "Existing class's UUID is %s\n"
            "You may not have duplicate class names.\n"
            "Check for duplicate plugins or copy and pasted code for plugins.",
            pClassDesc->ClassName().toUtf8().constData(),
            newUUIDString,
            existingUUIDString);

        QMessageBox::critical(nullptr, "Invalid class registration - Duplicate Class Name", QString::fromLatin1(errorMessageBuffer));
        return;
    }

#if defined(DEBUG_CLASS_NAME_REGISTRATION)
    m_debugClassNames.push_back(pClassDesc->ClassName());
#endif // DEBUG_CLASS_NAME_REGISTRATION

    m_classes.push_back(pClassDesc);
    m_guidToClass[pClassDesc->ClassID()] = pClassDesc;
    m_nameToClass[pClassDesc->ClassName()] = pClassDesc;
}

IClassDesc* CClassFactory::FindClass(const char* pClassName) const
{
    IClassDesc* pClassDesc = stl::find_in_map(m_nameToClass, pClassName, (IClassDesc*)nullptr);

    if (pClassDesc)
    {
        return pClassDesc;
    }

    const char* pSubClassName = strstr(pClassName, "::");

    if (!pSubClassName)
    {
        return nullptr;
    }

    QString name = QString(pClassName).left(static_cast<int>(pSubClassName - pClassName));

    return stl::find_in_map(m_nameToClass, name, (IClassDesc*)nullptr);
}

IClassDesc* CClassFactory::FindClass(const GUID& rClassID) const
{
    IClassDesc* pClassDesc = stl::find_in_map(m_guidToClass, rClassID, (IClassDesc*)nullptr);

    return pClassDesc;
}

IViewPaneClass* CClassFactory::FindViewPaneClassByTitle(const char* pPaneTitle) const
{
    for (size_t i = 0; i < m_classes.size(); i++)
    {
        IViewPaneClass* viewPane = nullptr;
        IClassDesc* desc = m_classes[i];
        if (SUCCEEDED(desc->QueryInterface(__uuidof(IViewPaneClass), (void**)&viewPane)))
        {
            if (QString::compare(viewPane->GetPaneTitle(), pPaneTitle) == 0)
            {
                return viewPane;
            }
        }
    }
    return nullptr;
}

void CClassFactory::UnregisterClass(const char* pClassName)
{
    IClassDesc* pClassDesc = FindClass(pClassName);

    if (pClassDesc == nullptr)
    {
        return;
    }

#if defined(DEBUG_CLASS_NAME_REGISTRATION)
    stl::find_and_erase(m_debugClassNames, pClassDesc->ClassName());
#endif // DEBUG_CLASS_NAME_REGISTRATION

    stl::find_and_erase(m_classes, pClassDesc);
    stl::member_find_and_erase(m_guidToClass, pClassDesc->ClassID());
    stl::member_find_and_erase(m_nameToClass, pClassDesc->ClassName());
}

void CClassFactory::UnregisterClass(const GUID& rClassID)
{
    IClassDesc* pClassDesc = FindClass(rClassID);

    if (!pClassDesc)
    {
        return;
    }

#if defined(DEBUG_CLASS_NAME_REGISTRATION)
    stl::find_and_erase(m_debugClassNames, pClassDesc->ClassName());
#endif // DEBUG_CLASS_NAME_REGISTRATION

    stl::find_and_erase(m_classes, pClassDesc);
    stl::member_find_and_erase(m_guidToClass, pClassDesc->ClassID());
    stl::member_find_and_erase(m_nameToClass, pClassDesc->ClassName());
}

bool ClassDescNameCompare(IClassDesc* pArg1, IClassDesc* pArg2)
{
    return (QString::compare(pArg1->ClassName(), pArg2->ClassName(), Qt::CaseInsensitive) < 0);
}

void CClassFactory::GetClassesBySystemID(ESystemClassID aSystemClassID, std::vector<IClassDesc*>& rOutClasses)
{
    rOutClasses.clear();

    for (size_t i = 0; i < m_classes.size(); ++i)
    {
        if (m_classes[i]->SystemClassID() == aSystemClassID)
        {
            rOutClasses.push_back(m_classes[i]);
        }
    }

    std::sort(rOutClasses.begin(), rOutClasses.end(), ClassDescNameCompare);
}

void CClassFactory::GetClassesByCategory(const char* pCategory, std::vector<IClassDesc*>& rOutClasses)
{
    rOutClasses.clear();

    for (size_t i = 0; i < m_classes.size(); ++i)
    {
        if (!QString::compare(pCategory, m_classes[i]->Category(), Qt::CaseInsensitive))
        {
            rOutClasses.push_back(m_classes[i]);
        }
    }

    std::sort(rOutClasses.begin(), rOutClasses.end(), ClassDescNameCompare);
}
