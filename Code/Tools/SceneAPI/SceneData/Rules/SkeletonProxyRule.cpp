/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/Rules/SkeletonProxyRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            void SkeletonProxy::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SkeletonProxy>()->Version(1)
                    ->Field("jointName", &SkeletonProxy::m_jointName)
                    ->Field("proxyName", &SkeletonProxy::m_proxyName);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SkeletonProxy>("Skeleton proxy", "Select the physics mesh for ragdoll or for hit detection.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(Edit::UIHandlers::Default, &SkeletonProxy::m_jointName, "Joint name", "Select the skeleton joint for this proxy.")
                        ->DataElement(Edit::UIHandlers::Default, &SkeletonProxy::m_proxyName, "Proxy name", "Pick the physics mesh.");
                }
            }

            AZ_CLASS_ALLOCATOR_IMPL(SkeletonProxyGroup, SystemAllocator);

            void SkeletonProxyGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SkeletonProxyGroup>()->Version(1)
                    ->Field("proxyMaterial", &SkeletonProxyGroup::m_proxyMaterialName)
                    ->Field("skeletonProxies", &SkeletonProxyGroup::m_skeletonProxies);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SkeletonProxyGroup>("Skeleton proxy group", "Related group of skeleton physics proxies.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(Edit::UIHandlers::Default, &SkeletonProxyGroup::m_proxyMaterialName, "Proxy material name", "Name the material for the physics mesh.")
                        ->DataElement(Edit::UIHandlers::Default, &SkeletonProxyGroup::m_skeletonProxies, "Skeleton proxies", "Select the physics mesh for ragdoll or for hit detection.");
                }
            }

            AZ_CLASS_ALLOCATOR_IMPL(SkeletonProxyRule, SystemAllocator);

            size_t SkeletonProxyRule::GetProxyGroupCount() const
            {
                return m_proxyGroups.size();
            }

            void SkeletonProxyRule::Reflect(ReflectContext* context)
            {
                SkeletonProxyGroup::Reflect(context);
                SkeletonProxy::Reflect(context);
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SkeletonProxyRule, DataTypes::ISkeletonProxyRule>()->Version(1)
                    ->Field("skeletonProxyGroups", &SkeletonProxyRule::m_proxyGroups);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SkeletonProxyRule>("Skeleton proxies", "Select the physics mesh for ragdoll or for hit detection.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(Edit::UIHandlers::Default, &SkeletonProxyRule::m_proxyGroups, "Proxy groups", "Proxy groups");
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
