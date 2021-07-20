/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SharedDataSlotExample.h"

namespace ScriptCanvasTesting
{
    namespace Nodeables
    {
        AZStd::string InputMethodSharedDataSlotExample::AppendHello(const ScriptCanvas::Data::StringType& arg)
        {
            return AZStd::string::format("%sHello", arg.c_str());
        }

        AZStd::string InputMethodSharedDataSlotExample::ConcatenateTwo(const ScriptCanvas::Data::StringType& arg1, const ScriptCanvas::Data::StringType& arg2)
        {
            return AZStd::string::format("%s%s", arg1.c_str(), arg2.c_str());
        }

        AZStd::string InputMethodSharedDataSlotExample::ConcatenateThree(const ScriptCanvas::Data::StringType& arg1, const ScriptCanvas::Data::StringType& arg2, const ScriptCanvas::Data::StringType& arg3)
        {
            return AZStd::string::format("%s%s%s", arg1.c_str(), arg2.c_str(), arg3.c_str());
        }

        void BranchMethodSharedDataSlotExample::StringMagicbox(int arg)
        {
            switch (arg)
            {
            case 1:
                CallOneString("Hello1");
                break;
            case 2:
                CallTwoStrings("Bob", "Hello2");
                break;
            case 3:
                CallThreeStrings("Square", "Pants", "Hello3");
                break;
            }
        }
    }
}
