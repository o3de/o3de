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

#include "ValuePointerReferenceExample.h"

namespace ScriptCanvasTesting
{
    namespace Nodeables
    {
        AZStd::vector<ScriptCanvas::Data::NumberType> ReturnTypeExample::ReturnByValue()
        {
            return m_internalVector;
        }

        AZStd::vector<ScriptCanvas::Data::NumberType>* ReturnTypeExample::ReturnByPointer()
        {
            return &m_internalVector;
        }

        AZStd::vector<ScriptCanvas::Data::NumberType>& ReturnTypeExample::ReturnByReference()
        {
            return m_internalVector;
        }


        AZStd::vector<ScriptCanvas::Data::NumberType> BranchInputTypeExample::GetInternalVector()
        {
            return m_internalVector;
        }

        void BranchInputTypeExample::BranchesOnInputType(AZStd::string inputType)
        {
            if (inputType == "Value")
            {
                CallByValue(m_internalVector);
            }
            else if (inputType == "Pointer")
            {
                CallByPointer(&m_internalVector);
            }
        }

        void InputTypeExample::ClearByValue(AZStd::vector<ScriptCanvas::Data::NumberType> input)
        {
            input.clear();
        }

        void InputTypeExample::ClearByPointer(AZStd::vector<ScriptCanvas::Data::NumberType>* input)
        {
            if (input)
            {
                input->clear();
            }
        }

        void InputTypeExample::ClearByReference(AZStd::vector<ScriptCanvas::Data::NumberType>& input)
        {
            input.clear();
        }
    }
}
