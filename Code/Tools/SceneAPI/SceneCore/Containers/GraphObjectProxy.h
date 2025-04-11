/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    struct BehaviorParameter;
    struct BehaviorArgument;
    class BehaviorClass;
    class BehaviorMethod;
    class BehaviorProperty;
    class ReflectContext;

    namespace Python
    {
        class PythonBehaviorInfo final
        {
        public:
            AZ_RTTI(PythonBehaviorInfo, "{8055BD03-5B3B-490D-AEC5-1B1E2616D529}");
            AZ_CLASS_ALLOCATOR(PythonBehaviorInfo, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            explicit PythonBehaviorInfo(const AZ::BehaviorClass* behaviorClass);
            PythonBehaviorInfo() = delete;

        protected:
            bool IsMemberLike(const AZ::BehaviorMethod& method, const AZ::TypeId& typeId) const;
            AZStd::string FetchPythonType(const AZ::BehaviorParameter& param) const;
            void PrepareMethod(AZStd::string_view methodName, const AZ::BehaviorMethod& behaviorMethod);
            void PrepareProperty(AZStd::string_view propertyName, const AZ::BehaviorProperty& behaviorProperty);

        private:
            const AZ::BehaviorClass* m_behaviorClass = nullptr;
            AZStd::vector<AZStd::string> m_methodList;
            AZStd::vector<AZStd::string> m_propertyList;
        };
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
                AZ_CLASS_ALLOCATOR(GraphObjectProxy, AZ::SystemAllocator);

                static void Reflect(AZ::ReflectContext* context);

                GraphObjectProxy(AZStd::shared_ptr<const DataTypes::IGraphObject> graphObject);
                GraphObjectProxy() = default;
                GraphObjectProxy(const GraphObjectProxy&);
                ~GraphObjectProxy();

                bool CastWithTypeName(const AZStd::string& classTypeName);

            protected:
                bool Convert(AZStd::any& input, const AZ::BehaviorParameter* argBehaviorInfo, AZ::BehaviorArgument& behaviorParam);

                AZStd::any Invoke(AZStd::string_view method, AZStd::vector<AZStd::any> argList);
                AZStd::any Fetch(AZStd::string_view property);
                AZStd::any InvokeBehaviorMethod(AZ::BehaviorMethod* behaviorMethod, AZStd::vector<AZStd::any> argList);

            private:
                AZStd::shared_ptr<const DataTypes::IGraphObject> m_graphObject;
                const AZ::BehaviorClass* m_behaviorClass = nullptr;
                AZStd::shared_ptr<Python::PythonBehaviorInfo> m_pythonBehaviorInfo;
            };

        } // Containers
    } // SceneAPI
} // AZ
