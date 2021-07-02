/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        void PropertyExample::In()
        {
            for ([[maybe_unused]] auto& num : Numbers)
            {
                AZ_TracePrintf("ScriptCanvas", "%f", num);
            }

            AZ_TracePrintf("ScriptCanvas", "Slang: %s", Slang.c_str());
        }
    }
}
