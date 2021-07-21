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
        class BinaryOperator
            : public Node
        {
        public:
            AZ_COMPONENT(BinaryOperator, "{5BD0E8C7-9B0A-42F5-9EB0-199E6EC8FA99}", Node);

            static void Reflect(AZ::ReflectContext* reflection);

            static const char* k_lhsName;
            static const char* k_rhsName;
            static const char* k_resultName;

            static const char* k_evaluateName;
            static const char* k_outName;
            static const char* k_onTrue;
            static const char* k_onFalse;

        protected:
            ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& /*executionSlot*/, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override
            {
                return AZ::Success(GetSlotsByType(targetSlotType));
            }

            // Indices into m_inputData
            static const int k_datumIndexLHS = 0;
            static const int k_datumIndexRHS = 1;

            void OnInit() override;

            SlotId GetOutputSlotId() const;
        };

        class ArithmeticExpression
            : public BinaryOperator
        {
        public:
            AZ_COMPONENT(ArithmeticExpression, "{B13F8DE1-E017-484D-9910-BABFB355D72E}", BinaryOperator);

            static void Reflect(AZ::ReflectContext* reflection);

            bool IsDeprecated() const override { return true; }

            void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

        protected:
            // adds Number inputs, adds Number output type
            void OnInit() override;
        };

        class BooleanExpression
            : public BinaryOperator
        {
        public:
            AZ_COMPONENT(BooleanExpression, "{36C69825-CFF8-4F70-8F3B-1A9227E8BEEA}", BinaryOperator);

            static void Reflect(AZ::ReflectContext* reflection);

            //////////////////////////////////////////////////////////////////////////
            // Translation
            AZ::Outcome<DependencyReport, void> GetDependencies() const override
            {
                return AZ::Success(DependencyReport{});
            }

            bool IsIfBranch() const override
            {
                return true;
            }

            bool IsIfBranchPrefacedWithBooleanExpression() const override
            {
                return true;
            }
            // Translation
            //////////////////////////////////////////////////////////////////////////

        protected:
            // initialize boolean expression, adds boolean output type calls
            void OnInit() override;
            virtual void InitializeBooleanExpression();
        };

        // accepts any type, checks for type equality, and then value equality or pointer equality
        class EqualityExpression
            : public BooleanExpression
        {
        public:
            AZ_COMPONENT(EqualityExpression, "{78D20EB6-BA07-4071-B646-7C2D68A0A4A6}", BooleanExpression);

            static void Reflect(AZ::ReflectContext* reflection);

        protected:
            // adds any required input types
            void InitializeBooleanExpression() override;

        private:
            SlotId                   m_firstSlotId;
            SlotId                   m_secondSlotId;
            ScriptCanvas::Data::Type m_displayType;
        };

        // accepts numbers only
        class ComparisonExpression
            : public EqualityExpression
        {
        public:
            AZ_COMPONENT(ComparisonExpression, "{82C50EAD-D3DD-45D2-BFCE-981D95771DC8}", EqualityExpression);

            static void Reflect(AZ::ReflectContext* reflection);

        protected:
            // adds number types
            void InitializeBooleanExpression() override;
        };
    }
}
