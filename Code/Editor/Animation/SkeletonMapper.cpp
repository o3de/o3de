/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "SkeletonMapper.h"

using namespace Skeleton;

/*

  CMapper

*/

CMapper::CMapper()
{
}

CMapper::~CMapper()
{
}

//

void CMapper::CreateFromHierarchy()
{
    m_nodes.clear();

    uint32 nodeCount = m_hierarchy.GetNodeCount();
    m_nodes.resize(nodeCount);
}

//

uint32 CMapper::CreateLocation(const char* name)
{
    int32 index = FindLocation(name);
    if (index < 1)
    {
        CMapperLocation* pLocation = new CMapperLocation();
        pLocation->SetName(name);
        m_locations.push_back(pLocation);
    }
    return uint32(m_locations.size() - 1);
}

void CMapper::ClearLocations()
{
    uint32 count = uint32(m_nodes.size());
    for (uint32 i = 0; i < count; ++i)
    {
        m_nodes[i].position = NULL;
        m_nodes[i].orientation = NULL;
    }

    m_locations.clear();
}

int32 CMapper::FindLocation(const char* name) const
{
    uint32 count = uint32(m_locations.size());
    for (uint32 i = 0; i < count; ++i)
    {
        if (::_stricmp(m_locations[i]->GetName(), name))
        {
            continue;
        }

        return int32(i);
    }

    return -1;
}

void CMapper::SetLocation(CMapperLocation& location)
{
    int32 index = FindLocation(location.GetName());
    if (index < 0)
    {
        m_locations.push_back(&location);
        return;
    }

    m_locations[index] = &location;
}

//

bool CMapper::CreateLocationsHierarchy(uint32 index, CHierarchy& hierarchy, int32 hierarchyParent)
{
    if (NodeHasLocation(index))
    {
        const CHierarchy::SNode* pNode = m_hierarchy.GetNode(index);
        uint32 nodeIndex = hierarchy.AddNode(pNode->name, pNode->pose, hierarchyParent);
        hierarchyParent = uint32(nodeIndex);
    }

    std::vector<uint32> children;
    GetChildrenIndices(index, children);
    uint32 childCount = uint32(children.size());
    for (uint32 i = 0; i < childCount; ++i)
    {
        CreateLocationsHierarchy(children[i], hierarchy, hierarchyParent);
    }

    return hierarchy.GetNodeCount() != 0;
}

bool CMapper::CreateLocationsHierarchy(CHierarchy& hierarchy)
{
    hierarchy.ClearNodes();
    if (!CreateLocationsHierarchy(0, hierarchy, -1))
    {
        return false;
    }

    hierarchy.ValidateReferences();
    return true;
}

void CMapper::Map(QuatT* pResult)
{
    uint32 outputCount = m_hierarchy.GetNodeCount();
    std::vector<Quat> absolutes(outputCount);
    for (uint32 i = 0; i < outputCount; ++i)
    {
        pResult[i].SetIdentity();
        absolutes[i].SetIdentity();

        CHierarchy::SNode* pNode = m_hierarchy.GetNode(i);
        if (!pNode)
        {
            continue;
        }

        CHierarchy::SNode* pParent = pNode->parent < 0 ?
            NULL : m_hierarchy.GetNode(pNode->parent);
        if (pParent)
        {
            pResult[i].t =
                (pNode->pose.t - pParent->pose.t) * pParent->pose.q;
        }

        if (m_nodes[i].position)
        {
            pResult[i].t = m_nodes[i].position->Compute().t;
        }

        if (m_nodes[i].orientation)
        {
            absolutes[i] = m_nodes[i].orientation->Compute().q;
        }
        else if (pParent)
        {
            Quat relative = pParent->pose.q.GetInverted() * pNode->pose.q;
            absolutes[i] = absolutes[pNode->parent] * relative;
        }
    }

    for (uint32 i = 0; i < outputCount; ++i)
    {
        CHierarchy::SNode* pNode = m_hierarchy.GetNode(i);
        if (!pNode)
        {
            continue;
        }

        CHierarchy::SNode* pParent = pNode->parent < 0 ?
            NULL : m_hierarchy.GetNode(pNode->parent);
        if (!pParent)
        {
            pResult[i].q = absolutes[i];
            continue;
        }

        pResult[i].q = absolutes[i];
        if (!m_nodes[i].position)
        {
            pResult[i].t = pResult[pNode->parent].t +
                pResult[i].t * absolutes[pNode->parent].GetInverted();
        }
    }
}

//

bool CMapper::NodeHasLocation(uint32 index)
{
    if (CMapperOperator* pOperator = m_nodes[index].position)
    {
        if (pOperator->IsOfClass("Location"))
        {
            return true;
        }
        if (pOperator->HasLinksOfClass("Location"))
        {
            return true;
        }
    }

    if (CMapperOperator* pOperator = m_nodes[index].orientation)
    {
        if (pOperator->IsOfClass("Location"))
        {
            return true;
        }
        if (pOperator->HasLinksOfClass("Location"))
        {
            return true;
        }
    }

    return false;
}

void CMapper::GetChildrenIndices(uint32 parent, std::vector<uint32>& children)
{
    uint32 nodeCount = m_hierarchy.GetNodeCount();
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        if (m_hierarchy.GetNode(i)->parent != parent)
        {
            continue;
        }

        children.push_back(i);
    }
}

bool CMapper::ChildrenHaveLocation(uint32 index)
{
    std::vector<uint32> children;
    GetChildrenIndices(index, children);

    uint32 childrenCount = uint32(children.size());
    for (uint32 i = 0; i < childrenCount; ++i)
    {
        if (ChildrenHaveLocation(children[i]))
        {
            return true;
        }
    }

    return false;
}

bool CMapper::NodeOrChildrenHaveLocation(uint32 index)
{
    if (NodeHasLocation(index))
    {
        return true;
    }

    std::vector<uint32> children;
    GetChildrenIndices(index, children);

    uint32 childrenCount = uint32(children.size());
    for (uint32 i = 0; i < childrenCount; ++i)
    {
        if (NodeOrChildrenHaveLocation(children[i]))
        {
            return true;
        }
    }

    return false;
}

bool CMapper::SerializeTo(XmlNodeRef& node)
{
    XmlNodeRef hierarchy = node->newChild("Hierarchy");

    uint32 nodeCount = GetNodeCount();
    std::vector<IXmlNode*> nodes(nodeCount);
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        if (!NodeOrChildrenHaveLocation(i))
        {
            continue;
        }

        CHierarchy::SNode* pNode = m_hierarchy.GetNode(i);
        if (!pNode)
        {
            return false;
        }

        XmlNodeRef xmlParent = hierarchy;
        int32 parent = pNode->parent;
        if (parent > -1)
        {
            xmlParent = nodes[parent];
        }

        nodes[i] = xmlParent->newChild("Node");
        nodes[i]->setAttr("name", pNode->name);

        if (CMapperOperator* pOperator = m_nodes[i].position)
        {
            XmlNodeRef position = nodes[i]->newChild("Position");
            XmlNodeRef child = position->newChild("Operator");
            if (!pOperator->SerializeWithLinksTo(child))
            {
                return false;
            }
        }

        if (CMapperOperator* pOperator = m_nodes[i].orientation)
        {
            XmlNodeRef orientation = nodes[i]->newChild("Orientation");
            XmlNodeRef child = orientation->newChild("Operator");
            if (!pOperator->SerializeWithLinksTo(child))
            {
                return false;
            }
        }
    }

    return true;
}

bool CMapper::SerializeFrom(XmlNodeRef& node, int32 parent)
{
    int childCount = uint32(node->getChildCount());
    for (int i = 0; i < childCount; ++i)
    {
        XmlNodeRef child = node->getChild(i);
        if (::_stricmp(child->getTag(), "Node"))
        {
            continue;
        }

        uint32 index = m_hierarchy.AddNode(child->getAttr("name"), QuatT(IDENTITY), parent);
        if (!SerializeFrom(child, int32(index)))
        {
            return false;
        }
    }

    return true;
}

bool CMapper::SerializeFrom(XmlNodeRef& node)
{
    XmlNodeRef hierarchy = node->findChild("Hierarchy");
    if (!hierarchy)
    {
        return false;
    }

    m_hierarchy.ClearNodes();

    if (!SerializeFrom(hierarchy, -1))
    {
        return false;
    }

    m_nodes.resize(m_hierarchy.GetNodeCount());

    return true;
}
