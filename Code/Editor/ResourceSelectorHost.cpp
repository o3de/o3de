/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ResourceSelectorHost.h"

// Qt
#include <QMessageBox>
#include <QString>

// AzToolsFramework
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>


class CResourceSelectorHost
    : public IResourceSelectorHost
{
public:
    CResourceSelectorHost()
    {
        RegisterModuleResourceSelectors(this);
    }

    QString SelectResource(const SResourceSelectorContext& context, const QString& previousValue) override
    {
        if (!context.typeName)
        {
            assert(false && "SResourceSelectorContext::typeName is not specified");
            return QString();
        }

        TTypeMap::iterator it = m_typeMap.find(context.typeName);
        if (it == m_typeMap.end())
        {
            QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("No Resource Selector is registered for resource type \"%1\"").arg(context.typeName));
            return previousValue;
        }

        QString result = previousValue;
        if (it->second->function)
        {
            result = it->second->function(context, previousValue);
        }
        else if (it->second->functionWithContext)
        {
            result = it->second->functionWithContext(context, previousValue, context.contextObject);
        }

        return result;
    }

    const char* ResourceIconPath(const char* typeName) const override
    {
        TTypeMap::const_iterator it = m_typeMap.find(typeName);
        if (it != m_typeMap.end())
        {
            return it->second->iconPath;
        }
        return "";
    }

    void RegisterResourceSelector(const SStaticResourceSelectorEntry* entry) override
    {
        m_typeMap[entry->typeName] = entry;
    }

    void SetGlobalSelection(const char* resourceType, const char* value) override
    {
        if (!resourceType)
        {
            return;
        }
        if (!value)
        {
            return;
        }
        m_globallySelectedResources[resourceType] = value;
    }

    const char* GetGlobalSelection(const char* resourceType) const override
    {
        if (!resourceType)
        {
            return "";
        }
        auto it = m_globallySelectedResources.find(resourceType);
        if (it != m_globallySelectedResources.end())
        {
            return it->second.c_str();
        }
        return "";
    }

private:
    typedef std::map<string, const SStaticResourceSelectorEntry*, stl::less_stricmp<string> > TTypeMap;
    TTypeMap m_typeMap;

    std::map<string, string> m_globallySelectedResources;
};

// ---------------------------------------------------------------------------

IResourceSelectorHost* CreateResourceSelectorHost()
{
    return new CResourceSelectorHost();
}

// ---------------------------------------------------------------------------

QString SoundFileSelector([[maybe_unused]] const SResourceSelectorContext& x, const QString& previousValue)
{
    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Audio");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (selection.IsValid())
    {
        return Path::FullPathToGamePath(QString(selection.GetResult()->GetFullPath().c_str()));
    }
    else
    {
        return Path::FullPathToGamePath(previousValue);
    }
}
REGISTER_RESOURCE_SELECTOR("Sound", SoundFileSelector, "")

// ---------------------------------------------------------------------------
QString ModelFileSelector([[maybe_unused]] const SResourceSelectorContext& x, const QString& previousValue)
{
    AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Geometry");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (selection.IsValid())
    {
        return Path::FullPathToGamePath(QString(selection.GetResult()->GetFullPath().c_str()));
    }
    else
    {
        return Path::FullPathToGamePath(previousValue);
    }
}
REGISTER_RESOURCE_SELECTOR("Model", ModelFileSelector, "")

// ---------------------------------------------------------------------------
QString GeomCacheFileSelector([[maybe_unused]] const SResourceSelectorContext& x, const QString& previousValue)
{
    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Geom Cache");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (selection.IsValid())
    {
        return Path::FullPathToGamePath(QString(selection.GetResult()->GetFullPath().c_str()));
    }
    else
    {
        return Path::FullPathToGamePath(previousValue);
    }
}

REGISTER_RESOURCE_SELECTOR("GeomCache", GeomCacheFileSelector, "")


