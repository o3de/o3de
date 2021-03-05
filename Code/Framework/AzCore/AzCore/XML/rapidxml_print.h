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

#ifndef AZCORE_RAPIDXML_PRINT_H_INCLUDED
#define AZCORE_RAPIDXML_PRINT_H_INCLUDED

#define RAPIDXML_SKIP_AZCORE_ERROR

// the intention is that you only include the customized version of rapidXML through this header, so that
// you can override behavior here.
#include <rapidxml/rapidxml_print.h>

#undef INCLUDING_RAPIDXML_VIA_AZCORE

#endif // AZCORE_RAPIDXML_PRINT_H_INCLUDED
