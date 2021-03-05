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

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <QHash>
#endif

class MaterialBrowserFilterModel;

//! Filter that checks the name of each sub-material in a material to see if it contains the filter string
class SubMaterialSearchFilter
    : public AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter
{
    Q_OBJECT
public:
    SubMaterialSearchFilter(const MaterialBrowserFilterModel* filterModel);
    ~SubMaterialSearchFilter() override = default;

    void SetFilterString(const QString& filterString);

protected:
    QString GetNameInternal() const override;
    bool MatchInternal(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;

private:
    bool TextMatchesFilter(const QString &text) const;
    QString m_filterString = "";
    const MaterialBrowserFilterModel* m_filterModel = nullptr;
};

class LevelMaterialSearchFilter
    : public AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter
{
    Q_OBJECT
public:
    LevelMaterialSearchFilter(const MaterialBrowserFilterModel* filterModel);
    ~LevelMaterialSearchFilter() override = default;

    void ShowOnlyLevelMaterials(bool onlyLevel) { m_onlyShowLevelMaterials = onlyLevel; }

    void CacheLoadedMaterials();

protected:
    QString GetNameInternal() const override;
    bool MatchInternal(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;

private:
    bool m_onlyShowLevelMaterials = false;
    const MaterialBrowserFilterModel* m_filterModel = nullptr;
    using MaterialMap = QHash<QString, _smart_ptr<IMaterial> >;
    MaterialMap m_localMap;
};
