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

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/PureData.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Datum.h>

namespace AZ
{
    class BehaviorClass;
    struct BehaviorValueParameter;
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class BehaviorContextObjectNode
                : public PureData
            {
            public:
                //\todo move to behavior context
                enum ParameterCount
                {
                    Getter = 1,
                    Setter = 2,
                };

                AZ_COMPONENT(BehaviorContextObjectNode, "{4344869D-2543-4FA3-BCD0-B6DB1E815928}", PureData);

                static void Reflect(AZ::ReflectContext* reflectContext);

                BehaviorContextObjectNode() = default;

                ~BehaviorContextObjectNode() override = default;

                AZStd::string GetDebugName() const override;            
                
                void InitializeObject(const AZ::Uuid& azType);

                void InitializeObject(const AZStd::string& classNameString);
                
                void InitializeObject(const Data::Type& type);

                void OnWriteEnd();

            protected:
                void InitializeObject(const AZ::BehaviorClass& behaviorClass);
                void OnInit() override;

                virtual void ConfigureSetters(const AZ::BehaviorClass& behaviorClass);
                virtual void ConfigureGetters(const AZ::BehaviorClass& behaviorClass);
                void ConfigureProperties(const AZ::BehaviorClass& behaviorClass);
                
            private:                
                AZStd::recursive_mutex m_mutex; // post-serialization
                AZStd::string m_className;

                BehaviorContextObjectNode(const BehaviorContextObjectNode&) = delete;
            };
        }
    }
}