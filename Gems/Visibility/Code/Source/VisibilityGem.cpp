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
#include "Visibility_precompiled.h"

#include "VisibilityGem.h"

#include "OccluderAreaComponent.h"
#include "PortalComponent.h"
#include "VisAreaComponent.h"

#ifdef VISIBILITY_EDITOR
#include "EditorOccluderAreaComponent.h"
#include "EditorPortalComponent.h"
#include "EditorVisAreaComponent.h"
#endif //VISIBILITY_EDITOR

VisibilityGem::VisibilityGem()
{
    m_descriptors.push_back(Visibility::OccluderAreaComponent::CreateDescriptor());
    m_descriptors.push_back(Visibility::PortalComponent::CreateDescriptor());
    m_descriptors.push_back(Visibility::VisAreaComponent::CreateDescriptor());

#ifdef VISIBILITY_EDITOR
    m_descriptors.push_back(Visibility::EditorOccluderAreaComponent::CreateDescriptor());
    m_descriptors.push_back(Visibility::EditorPortalComponent::CreateDescriptor());
    m_descriptors.push_back(Visibility::EditorVisAreaComponent::CreateDescriptor());
#endif //VISIBILITY_EDITOR
}

AZ_DECLARE_MODULE_CLASS(Gem_Visibility, VisibilityGem)
