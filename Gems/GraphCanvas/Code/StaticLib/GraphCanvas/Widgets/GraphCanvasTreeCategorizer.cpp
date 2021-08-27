/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeCategorizer.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // GraphCanvasTreeCategorizer
    ///////////////////////////////

    GraphCanvasTreeCategorizer::GraphCanvasTreeCategorizer(const CategorizerInterface& categorizerInterface)
        : m_categorizerInterface(categorizerInterface)
        , m_debugEnabled(false)
    {
    }

    void GraphCanvasTreeCategorizer::RegisterCategoryNode(GraphCanvas::GraphCanvasTreeItem* treeItem, const char* subCategory, GraphCanvas::GraphCanvasTreeItem* parentRoot)
    {
        CategoryKey key(parentRoot, subCategory);
        auto rootIter = m_rootMaps.find(key);

        if (rootIter == m_rootMaps.end())
        {
            m_rootMaps[key] = treeItem;
        }        
    }

    GraphCanvas::GraphCanvasTreeItem* GraphCanvasTreeCategorizer::GetCategoryNode(const char* categoryPath, GraphCanvas::GraphCanvasTreeItem* parentRoot)
    {
        AZ_Warning("GraphCanvas", parentRoot, "Null parent root passed into Categorizer.");

        if (!categoryPath)
        {
            return parentRoot;
        }

        GraphCanvas::GraphCanvasTreeItem* intermediateRoot = parentRoot;

        size_t separatorIndex = 0;
        AZStd::string_view rawView(categoryPath);

        while (separatorIndex != AZStd::string_view::npos)
        {
            size_t nextIndex = rawView.find_first_of('/', separatorIndex);

            AZStd::string_view categoryAggregate = rawView.substr(0, nextIndex);
            AZStd::string_view categoryName = categoryAggregate.substr(separatorIndex);

            if (nextIndex == AZStd::string_view::npos)
            {
                separatorIndex = nextIndex;
            }
            else
            {
                separatorIndex = nextIndex + 1;
            }

            if (categoryName.empty())
            {
                continue;
            }

            CategoryKey key(parentRoot, categoryAggregate);

            if (m_rootMaps.find(key) == m_rootMaps.end())
            {
                if (m_debugEnabled)
                {
                    AZ_TracePrintf("GraphCanvas", "KeyName==%s", key.second.c_str());

                    AZStd::string catName = categoryName;
                    AZ_TracePrintf("GraphCanvas", "CatName==%s", catName.c_str());
                }

                GraphCanvas::GraphCanvasTreeItem* treeItem = m_categorizerInterface.CreateCategoryNode(categoryAggregate, categoryName, intermediateRoot);
                m_rootMaps[key] = treeItem;
            }

            intermediateRoot = m_rootMaps[key];
        }

        return intermediateRoot;
    }

    void GraphCanvasTreeCategorizer::PruneEmptyNodes()
    {
        AZStd::vector< CategoryKey > deletedKeys;
        AZStd::unordered_set< GraphCanvas::GraphCanvasTreeItem* > potentialCategories;

        for (auto& mapPair : m_rootMaps)
        {
            if (mapPair.second)
            {
                if (mapPair.second->GetChildCount() == 0 && mapPair.second->AllowPruneOnEmpty())
                {
                    if (GraphCanvas::GraphCanvasTreeItem* parentItem = mapPair.second->GetParent())
                    {
                        if (parentItem->GetChildCount() == 0 && mapPair.second->AllowPruneOnEmpty())
                        {
                            potentialCategories.insert(parentItem);
                        }
                    }

                    mapPair.second->DetachItem();
                    potentialCategories.erase(mapPair.second);
                    deletedKeys.push_back(mapPair.first);
                }
            }
            
        }

        for (const CategoryKey& key : deletedKeys)
        {
            m_rootMaps.erase(key);
        }

        PruneNodes(potentialCategories);
    }

    void GraphCanvasTreeCategorizer::PruneNode(GraphCanvasTreeItem* treeItem)
    {
        GraphCanvas::GraphCanvasTreeItem* parentItem = treeItem->GetParent();
        treeItem->DetachItem();
        delete treeItem;

        AZStd::unordered_set< GraphCanvas::GraphCanvasTreeItem* > potentialCategories;
        potentialCategories.insert(parentItem);

        PruneNodes(potentialCategories);
    }

    void GraphCanvasTreeCategorizer::PruneNodes(AZStd::unordered_set< GraphCanvas::GraphCanvasTreeItem*> potentialPruners)
    {
        AZStd::unordered_set< GraphCanvas::GraphCanvasTreeItem* > deletedRoots;
        while (!potentialPruners.empty())
        {
            GraphCanvas::GraphCanvasTreeItem* treeItem = (*potentialPruners.begin());
            potentialPruners.erase(potentialPruners.begin());
            if (treeItem && treeItem->GetChildCount() == 0 && treeItem->AllowPruneOnEmpty())
            {
                GraphCanvas::GraphCanvasTreeItem* parentItem = static_cast<GraphCanvas::GraphCanvasTreeItem*>(treeItem->GetParent());
                treeItem->DetachItem();                
                potentialPruners.insert(parentItem);
                deletedRoots.insert(treeItem);
                delete treeItem;
            }
        }
        auto mapIter = m_rootMaps.begin();
        while (mapIter != m_rootMaps.end() && !deletedRoots.empty())
        {
            size_t erasedCount = deletedRoots.erase(mapIter->second);
            if (erasedCount > 0)
            {
                mapIter = m_rootMaps.erase(mapIter);
            }
            else
            {
                ++mapIter;
            }
        }
    }
}
