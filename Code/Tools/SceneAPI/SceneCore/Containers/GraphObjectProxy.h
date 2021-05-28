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

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace Python
    {
        class PythonBehaviorInfo;
    }

    namespace SceneAPI
    {
        namespace Containers
        {
            //
            // GraphObjectProxy wraps the smart pointer to IGraphObject privately
            // so that scripts can access the "graph node content" inside the SceneGraph
            //
            class GraphObjectProxy final
            {
            public:
                AZ_RTTI(GraphObjectProxy, "{3EF0DDEC-C734-4804-BE99-82058FEBDA71}");
                AZ_CLASS_ALLOCATOR(GraphObjectProxy, AZ::SystemAllocator, 0);

                static void Reflect(AZ::ReflectContext* context);

                GraphObjectProxy(AZStd::shared_ptr<const DataTypes::IGraphObject> graphObject);
                ~GraphObjectProxy();

                bool CastWithTypeName(const AZStd::string& classTypeName);
                AZStd::any Invoke(AZStd::string_view method, AZStd::vector<AZStd::any> argList);

            protected:
                bool Convert(AZStd::any& input, const AZ::BehaviorParameter* argBehaviorInfo, AZ::BehaviorValueParameter& behaviorParam);

            private:
                AZStd::shared_ptr<const DataTypes::IGraphObject> m_graphObject;
                const AZ::BehaviorClass* m_behaviorClass = nullptr;
                AZStd::shared_ptr<Python::PythonBehaviorInfo> m_pythonBehaviorInfo;
            };

        } // Containers
    } // SceneAPI
} // AZ
