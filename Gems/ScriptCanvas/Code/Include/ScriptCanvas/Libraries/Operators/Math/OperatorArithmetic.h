/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/SerializeContext.h>

#include <ScriptCanvas/Data/Data.h>
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorArithmetic.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            //! Base class for arithmetic operation nodes
            class OperatorArithmetic : public Node
            {
            public:

                SCRIPTCANVAS_NODE(OperatorArithmetic);

                // Need to update code gen to deal with enums.
                // Will do that in a separate pass for now still want to track versions using enums
                enum Version
                {
                    InitialVersion = 0,
                    RemoveOperatorBase,

                    Current
                };

                typedef AZStd::vector<const Datum*> ArithmeticOperands;

                static bool OperatorArithmeticVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                OperatorArithmetic() = default;

                AZ::Crc32 GetArithmeticExtensionId() const { return AZ_CRC_CE("AddnewValueExtension"); }
                AZ::Crc32 GetArithmeticDynamicTypeGroup() const { return AZ_CRC_CE("ArithmeticGroup"); }
                AZStd::string GetArithmeticDisplayGroup() const { return "ArithmeticGroup"; }

                virtual AZStd::string_view OperatorFunction() const { return ""; }
                virtual AZStd::unordered_set< Data::Type > GetSupportedNativeDataTypes() const
                {
                    return {
                        Data::Type::Number(),
                        Data::Type::Vector2(),
                        Data::Type::Vector3(),
                        Data::Type::Vector4(),
                        Data::Type::VectorN(),
                        Data::Type::Color(),
                        Data::Type::Quaternion(),
                        Data::Type::Transform(),
                        Data::Type::Matrix3x3(),
                        Data::Type::Matrix4x4(),
                        Data::Type::MatrixMxN()
                    };
                }

                // Node
                void OnSlotDisplayTypeChanged(const SlotId& slotId, const Data::Type& dataType) override final;
                void OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) override final;

                void OnConfigured() override;
                void OnInit() override;
                void OnActivate() override;
                void ConfigureVisualExtensions() override;

                SlotId HandleExtension(AZ::Crc32 extensionId) override;

                bool CanDeleteSlot(const SlotId& slotId) const override;
                ////

                void Evaluate(const ArithmeticOperands&, Datum&);

                virtual void Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result);
                virtual void InitializeSlot(const SlotId& slotId, const Data::Type& dataType);

                //////////////////////////////////////////////////////////////////////////
                // Translation
                AZ::Outcome<DependencyReport, void> GetDependencies() const override;
                // Translation
                //////////////////////////////////////////////////////////////////////////

            protected:

                SlotId CreateSlot(AZStd::string_view name, AZStd::string_view toolTip, ConnectionType connectionType);

                void UpdateArithmeticSlotNames();
                void SetSourceNames(const AZStd::string& inputName, const AZStd::string& outputName);

                virtual bool IsValidArithmeticSlot(const SlotId& slotId) const;

                // Contains the list of slots that have, or have the potential, to have values which will impact
                // the specific arithmetic operation.
                //
                // Used at run time to try to avoid invoking extra operator calls for no-op operations
                bool                    m_scrapedInputs = false;
                AZStd::vector< const Datum* > m_applicableInputs;

                Datum m_result;

                Slot* m_resultSlot;
                SlotId m_outSlot;
            };

            //! Deprecated: kept here for version conversion
            class OperatorArithmeticUnary : public OperatorArithmetic
            {
            public:

                SCRIPTCANVAS_NODE(OperatorArithmeticUnary);

                OperatorArithmeticUnary()
                    : OperatorArithmetic()
                {
                }
            };

            inline namespace OperatorEvaluator
            {
                template<typename ResultType, typename OperatorFunctor>
                static void Evaluate(
                    OperatorFunctor&& operatorFunctor, const OperatorArithmetic::ArithmeticOperands& operands, Datum& result)
                {
                    // At this point we know that the operands have been checked and we have at least 2.
                    // So we can just convert it to the value directly.
                    OperatorArithmetic::ArithmeticOperands::const_iterator operandIter = operands.begin();

                    ResultType resultType = (*(*operandIter)->GetAs<ResultType>());

                    for (++operandIter; operandIter != operands.end(); ++operandIter)
                    {
                        resultType = AZStd::invoke(operatorFunctor, resultType, (*(*operandIter)));
                    }

                    result.Set<ResultType>(resultType);
                }
            } // namespace OperatorEvaluator
        }
    }
}
