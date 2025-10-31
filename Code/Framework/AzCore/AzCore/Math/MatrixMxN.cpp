/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MatrixMxN.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/MathScriptHelpers.h>

namespace AZ
{
    void MatrixMxN::Reflect([[maybe_unused]] ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<MatrixMxN>()
                ->Version(1)
                ->Field("RowCount", &MatrixMxN::m_rowCount)
                ->Field("ColCount", &MatrixMxN::m_colCount)
                ->Field("RowGroups", &MatrixMxN::m_numRowGroups)
                ->Field("ColGroups", &MatrixMxN::m_numColGroups)
                ->Field("Values", &MatrixMxN::m_values)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MatrixMxN>("MxN-Dimensional Matrix", "")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                    ->DataElement(Edit::UIHandlers::Default, &MatrixMxN::m_rowCount, "Total Rows", "The total number of rows in the matrix")
                        ->Attribute(Edit::Attributes::ChangeNotify, &MatrixMxN::OnSizeChanged)
                    ->DataElement(Edit::UIHandlers::Default, &MatrixMxN::m_colCount, "Total Columns", "The total number of columns in the matrix")
                        ->Attribute(Edit::Attributes::ChangeNotify, &MatrixMxN::OnSizeChanged)
                    ;
            }
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<MatrixMxN>()->
                Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)->
                Attribute(Script::Attributes::Module, "math")->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Constructor<AZStd::size_t, AZStd::size_t>()->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Method("CreateZero", &MatrixMxN::CreateZero)->
                Method("CreateRandom", &MatrixMxN::CreateRandom)->
                Method("GetElement", &MatrixMxN::GetElement)->
                Method("SetElement", &MatrixMxN::SetElement)->
                Method("GetTranspose", &MatrixMxN::GetTranspose)->
                Method("GetFloor", &MatrixMxN::GetFloor)->
                Method("GetCeil", &MatrixMxN::GetCeil)->
                Method("GetRound", &MatrixMxN::GetRound)->
                Method("GetMin", &MatrixMxN::GetMin)->
                Method("GetMax", &MatrixMxN::GetMax)->
                Method("GetClamp", &MatrixMxN::GetClamp)->
                Method("GetAbs", &MatrixMxN::GetAbs)->
                Method("GetSquare", &MatrixMxN::GetSquare)->
                Method("OuterProduct", &OuterProduct)->
                Method("VectorMatrixMultiply", &VectorMatrixMultiply)->
                Method("VectorMatrixMultiplyLeft", &VectorMatrixMultiplyLeft)->
                Method("MatrixMatrixMultiply", &MatrixMatrixMultiply)
                ;
        }
    }
}
