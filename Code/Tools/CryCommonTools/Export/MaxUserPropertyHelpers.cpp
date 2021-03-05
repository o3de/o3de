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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "MaxUserPropertyHelpers.h"
#include "StringHelpers.h"
#include "MaxHelpers.h"


std::string MaxUserPropertyHelpers::GetNodeProperties(INode* node)
{
    if (node == 0)
    {
        return std::string();
    }

    MSTR buf;
    node->GetUserPropBuffer(buf);

    return MaxHelpers::CreateAsciiString(buf);
}


std::string MaxUserPropertyHelpers::GetStringNodeProperty(INode* node, const char* name, const char* defaultValue)
{
    if (node == 0)
    {
        return defaultValue;
    }

    MSTR val;
    if (!node->GetUserPropString(MaxHelpers::CreateMaxStringFromAscii(name), val))
    {
        return defaultValue;
    }

    return MaxHelpers::CreateAsciiString(val);
}


float MaxUserPropertyHelpers::GetFloatNodeProperty(INode* node, const char* name, float defaultValue)
{
    if (node == 0)
    {
        return defaultValue;
    }

    float val;
    if (!node->GetUserPropFloat(MaxHelpers::CreateMaxStringFromAscii(name), val))
    {
        return defaultValue;
    }

    return val;
}


int MaxUserPropertyHelpers::GetIntNodeProperty(INode* node, const char* name, int defaultValue)
{
    if (node == 0)
    {
        return defaultValue;
    }

    int val;
    if (!node->GetUserPropInt(MaxHelpers::CreateMaxStringFromAscii(name), val))
    {
        return defaultValue;
    }

    return val;
}


bool MaxUserPropertyHelpers::GetBoolNodeProperty(INode* node, const char* name, bool defaultValue)
{
    if (node == 0)
    {
        return defaultValue;
    }

    BOOL val;
    if (!node->GetUserPropBool(MaxHelpers::CreateMaxStringFromAscii(name), val))
    {
        return defaultValue;
    }

    return (val != 0);
}
