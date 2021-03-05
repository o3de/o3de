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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MODELDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MODELDATA_H
#pragma once


#include "IModelData.h"

class ModelData
    : public IModelData
{
public:
    // IModelData
    virtual int AddModel(const void* handle, const char* name, int parentModelIndex, bool geometry, const SHelperData& helperData, const std::string& propertiesString);
    virtual int GetModelCount() const;
    virtual const void* GetModelHandle(int modelIndex) const;
    virtual const char* GetModelName(int modelIndex) const;
    virtual void SetTranslationRotationScale(int modelIndex, const float* translation, const float* rotation, const float* scale);
    virtual void GetTranslationRotationScale(int modelIndex, float* translation, float* rotation, float* scale) const;
    virtual const SHelperData& GetHelperData(int modelIndex) const;
    virtual const std::string& GetProperties(int modelIndex) const;
    virtual bool IsRoot(int modelIndex) const;

    int GetRootCount() const;
    int GetRootIndex(int rootIndex) const;
    int GetChildCount(int modelIndex) const;
    int GetChildIndex(int modelIndex, int childIndexIndex) const;
    bool HasGeometry(int modelIndex) const;

private:
    struct ModelEntry
    {
        ModelEntry(const void* handle, const std::string& name, int parentIndex, bool geometry, const SHelperData& helperData, const std::string& propertiesString);

        const void* handle;
        std::string name;
        int parentIndex;
        bool geometry;
        std::vector<int> children;
        float translation[3];
        float rotation[3];
        float scale[3];
        SHelperData helperData;
        std::string propertiesString;
    };

    std::vector<ModelEntry> m_models;
    std::vector<int> m_roots;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MODELDATA_H
