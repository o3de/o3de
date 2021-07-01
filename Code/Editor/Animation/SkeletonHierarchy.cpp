/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"
#include "SkeletonHierarchy.h"

using namespace Skeleton;

/*

  CHierarchy

*/

CHierarchy::CHierarchy()
{
}

CHierarchy::~CHierarchy()
{
}

//

uint32 CHierarchy::AddNode(const char* name, const QuatT& pose, int32 parent)
{
    int32 index = FindNodeIndexByName(name);

    if (index < 0)
    {
        m_nodes.push_back(SNode());
        index = int32(m_nodes.size() - 1);
    }

    m_nodes[index].name = name;
    m_nodes[index].pose = pose;
    m_nodes[index].parent = parent;
    return uint32(index);
}

int32 CHierarchy::FindNodeIndexByName(const char* name) const
{
    uint32 count = uint32(m_nodes.size());
    for (uint32 i = 0; i < count; ++i)
    {
        if (::_stricmp(m_nodes[i].name, name))
        {
            continue;
        }

        return i;
    }

    return -1;
}

const CHierarchy::SNode* CHierarchy::FindNode(const char* name) const
{
    int32 index = FindNodeIndexByName(name);
    return index < 0 ? NULL : &m_nodes[index];
}

void CHierarchy::CreateFrom(IDefaultSkeleton* pIDefaultSkeleton)
{
    const uint32 jointCount = pIDefaultSkeleton->GetJointCount();

    m_nodes.clear();
    m_nodes.reserve(jointCount);
    for (uint32 i = 0; i < jointCount; ++i)
    {
        m_nodes.push_back(SNode());

        m_nodes.back().name = pIDefaultSkeleton->GetJointNameByID(int32(i));
        m_nodes.back().pose = pIDefaultSkeleton->GetDefaultAbsJointByID(int32(i));

        m_nodes.back().parent = pIDefaultSkeleton->GetJointParentIDByID(int32(i));
    }

    ValidateReferences();
}

void CHierarchy::ValidateReferences()
{
    uint32 nodeCount = m_nodes.size();
    if (!nodeCount)
    {
        return;
    }

    for (uint32 i = 0; i < nodeCount; ++i)
    {
        if (m_nodes[i].parent < nodeCount)
        {
            continue;
        }

        m_nodes[i].parent = -1;
    }
}

void CHierarchy::AbsoluteToRelative(const QuatT* pSource, QuatT* pDestination)
{
    uint32 count = uint32(m_nodes.size());
    std::vector<QuatT> absolutes(count);
    for (uint32 i = 0; i < count; ++i)
    {
        absolutes[i] = pSource[i];
    }

    for (uint32 i = 0; i < count; ++i)
    {
        int32 parent = m_nodes[i].parent;
        if (parent < 0)
        {
            pDestination[i] = absolutes[i];
            continue;
        }

        pDestination[i].t = (absolutes[i].t - absolutes[parent].t) * absolutes[parent].q;
        pDestination[i].q = absolutes[parent].q.GetInverted() * absolutes[i].q;
    }
}

bool CHierarchy::SerializeTo(XmlNodeRef& node)
{
    XmlNodeRef hierarchy = node->newChild("Hierarchy");

    uint32 nodeCount = uint32(m_nodes.size());
    std::vector<IXmlNode*> nodes(nodeCount);
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        XmlNodeRef parent = hierarchy;
        if (m_nodes[i].parent > -1)
        {
            parent = nodes[m_nodes[i].parent];
        }

        nodes[i] = parent->newChild("Node");
        nodes[i]->setAttr("name", m_nodes[i].name);
    }

    return true;
}
