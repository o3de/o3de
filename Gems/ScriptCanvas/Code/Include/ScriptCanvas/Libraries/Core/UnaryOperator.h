/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <ScriptCanvas/Core/Node.h>
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
            ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot*) const override
            {
                return AZ::Success(GetSlotsByType(targetSlotType));
            }

            // Indices into m_inputData
            static const int k_datumIndex = 0;

            void ConfigureSlots() override;

            SlotId GetOutputSlotId() const;
        };

        class UnaryExpression : public UnaryOperator
        {
        public:
            AZ_COMPONENT(UnaryExpression, "{70FF2162-3D01-41F1-B009-7DC071A38471}", UnaryOperator);

            static void Reflect(AZ::ReflectContext* reflection);

        protected:

            void ConfigureSlots() override;

            virtual void InitializeUnaryExpression();
        };
    }
}
