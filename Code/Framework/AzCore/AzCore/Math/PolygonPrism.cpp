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

#include <AzCore/Math/PolygonPrism.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    void PolygonPrismReflect(ReflectContext* context)
    {
        PolygonPrism::Reflect(context);
    }

    void PolygonPrism::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<PolygonPrism>()
                ->Version(1)
                ->Field("Height", &PolygonPrism::m_height)
                ->Field("VertexContainer", &PolygonPrism::m_vertexContainer)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PolygonPrism>("PolygonPrism", "Polygon prism shape")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                    ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(Edit::UIHandlers::Default, &PolygonPrism::m_height, "Height", "Shape Height")
                        ->Attribute(Edit::Attributes::Suffix, " m")
                        ->Attribute(Edit::Attributes::Step, 0.05f)
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, &PolygonPrism::OnChangeHeight)
                    ->DataElement(Edit::UIHandlers::Default, &PolygonPrism::m_vertexContainer, "Vertices", "Data representing the polygon, in the entity's local coordinate space")
                        ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<PolygonPrism>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::RuntimeOwn)
                ->Property("height", BehaviorValueGetter(&PolygonPrism::m_height), nullptr)
                ->Property("vertexContainer", BehaviorValueGetter(&PolygonPrism::m_vertexContainer), nullptr)
                ;
        }
    }

    void PolygonPrism::SetHeight(const float height)
    {
        if (!IsCloseMag(height, m_height))
        {
            m_height = height;
            OnChangeHeight();
        }
    }

    void PolygonPrism::OnChangeHeight() const
    {
        if (m_onChangeHeightCallback)
        {
            m_onChangeHeightCallback();
        }
    }

    void PolygonPrism::SetCallbacks(
        const VoidFunction& onChangeElement,
        const VoidFunction& onChangeContainer,
        const VoidFunction& onChangeHeight)
    {
        m_vertexContainer.SetCallbacks(
            [onChangeContainer](size_t) { onChangeContainer(); },
            [onChangeContainer](size_t) { onChangeContainer(); },
            [onChangeElement](size_t) { onChangeElement(); },
            onChangeContainer,
            onChangeContainer);

        m_onChangeHeightCallback = onChangeHeight;
    }

    void PolygonPrism::SetCallbacks(
        const IndexFunction& onAddVertex, const IndexFunction& onRemoveVertex,
        const IndexFunction& onUpdateVertex, const VoidFunction& onSetVertices,
        const VoidFunction& onClearVertices, const VoidFunction& onChangeHeight)
    {
        m_vertexContainer.SetCallbacks(
            onAddVertex,
            onRemoveVertex,
            onUpdateVertex,
            onSetVertices,
            onClearVertices);

        m_onChangeHeightCallback = onChangeHeight;
    }

    AZ_CLASS_ALLOCATOR_IMPL(PolygonPrism, SystemAllocator, 0)
}
