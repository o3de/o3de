/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    struct BehaviorParameter;
    struct BehaviorValueParameter;
    class BehaviorClass;

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
