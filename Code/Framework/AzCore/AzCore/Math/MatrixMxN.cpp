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
                editContext->Class<MatrixMxN>("N-Dimensional Vector", "")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        Edit::UIHandlers::Default, &MatrixMxN::m_rowCount, "Total Rows", "The total number of rows in the matrix")
                    ->DataElement(
                        Edit::UIHandlers::Default, &MatrixMxN::m_colCount, "Total Columns", "The total number of columns in the matrix")
                    ->DataElement(
                        Edit::UIHandlers::Default, &MatrixMxN::m_values, "Array of values", "The array of values in the matrix")
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
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)
                ;
        }
    }
}
