/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            template<typename T>
            bool DoesSceneGraphContainDataLike(const Containers::Scene& scene, bool checkVirtualTypes)
            {
                static_assert(AZStd::is_base_of<DataTypes::IGraphObject, T>::value, "Specified type T is not derived from IGraphObject.");

                const Containers::SceneGraph& graph = scene.GetGraph();
                if (checkVirtualTypes)
                {
                    auto contentStorage = graph.GetContentStorage();
                    auto view = Containers::Views::MakeFilterView(contentStorage, Containers::DerivedTypeFilter<T>());
                    for (auto it = view.begin(); it != view.end(); ++it)
                    {
                        Events::GraphMetaInfo::VirtualTypesSet types;
                        Events::GraphMetaInfoBus::Broadcast(
                            &Events::GraphMetaInfoBus::Events::GetVirtualTypes,
                            types,
                            scene,
                            graph.ConvertToNodeIndex(it.GetBaseIterator()));
                        // Check if the type is not a virtual type. If it is, this isn't a valid type for T.
                        if (types.empty())
                        {
                            return true;
                        }
                    }
                    return false;
                }
                else
                {
                    Containers::SceneGraph::ContentStorageConstData graphContent = graph.GetContentStorage();
                    auto data = AZStd::find_if(graphContent.begin(), graphContent.end(), Containers::DerivedTypeFilter<T>());
                    return data != graphContent.end();
                }
            }
        } // Utilities
    } // SceneAPI
} // AZ
