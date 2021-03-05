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

            // Indices into m_inputData
            static const int k_datumIndexLHS = 0;
            static const int k_datumIndexRHS = 1;

            void OnInit() override;

            // must be overridden with the binary operations
            virtual Datum Evaluate(const Datum& lhs, const Datum& rhs);

            // Triggered by the execution signal
            void OnInputSignal(const SlotId& slot) override;

            SlotId GetOutputSlotId() const;
        };
                
        class ArithmeticExpression
            : public BinaryOperator
        {
        public:
            AZ_COMPONENT(ArithmeticExpression, "{B13F8DE1-E017-484D-9910-BABFB355D72E}", BinaryOperator);

            static void Reflect(AZ::ReflectContext* reflection);

        protected:
            // adds Number inputs, adds Number output type
            void OnInit() override;

            void OnInputSignal(const SlotId& slot) override;

            
        };
        
        class BooleanExpression
            : public BinaryOperator
        {
        public:
            AZ_COMPONENT(BooleanExpression, "{36C69825-CFF8-4F70-8F3B-1A9227E8BEEA}", BinaryOperator);
            
            static void Reflect(AZ::ReflectContext* reflection);

        protected:
            // initialize boolean expression, adds boolean output type calls 
            void OnInit() override;
            virtual void InitializeBooleanExpression();
            void OnInputSignal(const SlotId& slot) override;
            
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
    } // namespace Nodes
} // namespace ScriptCanvas
