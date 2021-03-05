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

#include <AzCore/Debug/Trace.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        template <typename ValueType, typename ElementArrayType>
        void FbxLayerElementUtilities::GetGeometryElement( ValueType& value, const ElementArrayType* elementArray,
            int polygonIndex, int polygonVertexIndex, int controlPointIndex)
        {
            if (!elementArray)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Invalid ElementArrayType input");
                return;
            }

            int fbxElementIndex;
            switch (elementArray->GetMappingMode())
            {
            case FbxGeometryElement::eByControlPoint:
                // One mapping coordinate for each surface control point/vertex.
                fbxElementIndex = controlPointIndex;
                break;
            case FbxGeometryElement::eByPolygonVertex:
                // One mapping coordinate for each vertex, for every polygon of which it is a part.
                // This means that a vertex will have as many mapping coordinates as polygons of which it is a part.
                fbxElementIndex = polygonVertexIndex;
                break;
            case FbxGeometryElement::eByPolygon:
                // One mapping coordinate for the whole polygon.
                fbxElementIndex = polygonIndex;
                break;
            default:
                AZ_TraceContext("Unknown value", elementArray->GetMappingMode());
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Invalid ElementArrayType mapping mode");
                return;
            }

            if (elementArray->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
            {
                // Convert index from "index of value's index" to "index of value".
                const FbxLayerElementArrayTemplate<int>& indices = elementArray->GetIndexArray();
                AZ_Assert(fbxElementIndex < indices.GetCount(), "Invalid element index %d", fbxElementIndex);
                if (fbxElementIndex >= indices.GetCount())
                {
                    return;
                }
                fbxElementIndex = indices.GetAt(fbxElementIndex);
            }

            const FbxLayerElementArrayTemplate<ValueType>& elements = elementArray->GetDirectArray();
            AZ_Assert(fbxElementIndex < elements.GetCount(), "Invalid element index %d", fbxElementIndex);
            if (fbxElementIndex >= elements.GetCount())
            {
                return;
            }
            value = elements.GetAt(fbxElementIndex);
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
