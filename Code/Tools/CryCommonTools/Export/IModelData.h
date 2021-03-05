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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IMODELDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IMODELDATA_H
#pragma once


#include "HelperData.h"
#include <string>

class IModelData
{
public:
    virtual int AddModel(const void* handle, const char* modelName, int parentModelIndex, bool geometry, const SHelperData& helperData, const std::string& propertiesString) = 0;
    virtual int GetModelCount() const = 0;
    virtual const void* GetModelHandle(int modelIndex) const = 0;
    virtual const char* GetModelName(int modelIndex) const = 0;
    virtual void SetTranslationRotationScale(int modelIndex, const float* translation, const float* rotation, const float* scale) = 0;
    virtual void GetTranslationRotationScale(int modelIndex, float* translation, float* rotation, float* scale) const = 0;
    virtual const SHelperData& GetHelperData(int modelIndex) const = 0;
    virtual const std::string& GetProperties(int modelIndex) const = 0;
    virtual bool IsRoot(int modelIndex) const = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IMODELDATA_H
