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

#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/PureData.h>
#include <ScriptCanvas/Core/Datum.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        class UnaryOperator
            : public Node
        {
        public:
            AZ_COMPONENT(UnaryOperator, "{B0BF8615-D718-4115-B3D8-CAB554BC6863}", Node);

            static void Reflect(AZ::ReflectContext* reflection);

            static const char* k_valueName;
            static const char* k_resultName;

            static const char* k_evaluateName;
            static const char* k_onTrue;
            static const char* k_onFalse;

        protected:

            // Indices into m_inputData
            static const int k_datumIndex = 0;

            void ConfigureSlots() override;

            // must be overridden with the binary operations
            virtual Datum Evaluate(const Datum& value);

            // Triggered by the execution signal
            void OnInputSignal(const SlotId& slot) override;

            SlotId GetOutputSlotId() const;

            
        };

        class UnaryExpression : public UnaryOperator
        {
        public:
            AZ_COMPONENT(UnaryExpression, "{70FF2162-3D01-41F1-B009-7DC071A38471}", UnaryOperator);

            static void Reflect(AZ::ReflectContext* reflection);

        protected:

            void ConfigureSlots() override;

            void OnInputSignal(const SlotId& slot) override;

            virtual void InitializeUnaryExpression();

            
        };
    }
}