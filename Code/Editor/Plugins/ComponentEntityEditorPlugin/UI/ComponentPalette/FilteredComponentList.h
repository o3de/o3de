/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QTableView>
#include <QWidget>

#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>
#include <AzToolsFramework/UI/SearchWidget/SearchWidgetTypes.hxx>
#include <AzCore/Serialization/SerializeContext.h>
#include "ComponentDataModel.h"
#endif

namespace AZ
{
    class SerializeContext;
    class ClassData;
}

class ComponentDataModel;

//! FilteredComponentList
//! Provides a list of components that can be filtered according to search criteria provided and/or from
//! a category selection control.
class FilteredComponentList 
    : public QTableView
{
    Q_OBJECT

public:

    explicit FilteredComponentList(QWidget* parent = nullptr);

    ~FilteredComponentList() override;

    virtual void Init();

    void SearchCriteriaChanged(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator);

    void SetCategory(const char* category);

protected:

    // Filtering support
    void BuildFilter(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator);
    void AppendFilter(QString& filter, const QString& text, AzToolsFramework::FilterOperatorType filterOperator);
    void SetFilterRegExp(const AZStd::string& filterType, const QRegExp& regExp);
    void ClearFilterRegExp(const AZStd::string& filterType = AZStd::string());

    // Context menu handlers 
    void ShowContextMenu(const QPoint&);
    void ContextMenu_NewEntity();
    void ContextMenu_AddToFavorites();
    void ContextMenu_AddToSelectedEntities();

    void modelReset();

    AzToolsFramework::FilterByCategoryMap m_filtersRegExp;
    ComponentDataModel* m_componentDataModel;

};
