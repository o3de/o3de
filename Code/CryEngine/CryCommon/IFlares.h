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

#ifndef CRYINCLUDE_CRYCOMMON_IFLARES_H
#define CRYINCLUDE_CRYCOMMON_IFLARES_H
#pragma once

#include <IFuncVariable.h> // <> required for Interfuscator
#include <IXml.h> // <> required for Interfuscator
#include "smartptr.h"

struct IShader;
class CCamera;

class __MFPA
{
};
class __MFPB
{
};
#define MFP_SIZE_ENFORCE : public __MFPA, public __MFPB

enum EFlareType
{
    eFT__Base__,
    eFT_Root,
    eFT_Group,
    eFT_Ghost,
    eFT_MultiGhosts,
    eFT_Glow,
    eFT_ChromaticRing,
    eFT_IrisShafts,
    eFT_CameraOrbs,
    eFT_ImageSpaceShafts,
    eFT_Streaks,
    eFT_Reference,
    eFT_Proxy,
    eFT_Max
};

#define FLARE_LIBS_PATH "libs/flares/"
#define FLARE_EXPORT_FILE "LensFlareList.xml"
#define FLARE_EXPORT_FILE_VERSION "1"

struct FlareInfo
{
    EFlareType type;
    const char* name;
#if defined(FLARES_SUPPORT_EDITING)
    const char* imagename;
#endif
};

#if defined(FLARES_SUPPORT_EDITING)
#   define ADD_FLARE_INFO(type, name, imagename) {type, name, imagename}
#else
#   define ADD_FLARE_INFO(type, name, imagename) {type, name}
#endif

class FlareInfoArray
{
public:
    struct Props
    {
        const FlareInfo* p;
        size_t size;
    };

    static const Props Get()
    {
        static const FlareInfo flareInfoArray[] =
        {
            ADD_FLARE_INFO(eFT__Base__, "__Base__", NULL),
            ADD_FLARE_INFO(eFT_Root, "Root", NULL),
            ADD_FLARE_INFO(eFT_Group, "Group", NULL),
            ADD_FLARE_INFO(eFT_Ghost, "Ghost", "EngineAssets/Textures/flares/icons/ghost.dds"),
            ADD_FLARE_INFO(eFT_MultiGhosts, "Multi Ghost", "EngineAssets/Textures/flares/icons/multi_ghost.dds"),
            ADD_FLARE_INFO(eFT_Glow, "Glow", "EngineAssets/Textures/flares/icons/glow.dds"),
            ADD_FLARE_INFO(eFT_ChromaticRing, "ChromaticRing", "EngineAssets/Textures/flares/icons/ring.dds"),
            ADD_FLARE_INFO(eFT_IrisShafts, "IrisShafts", "EngineAssets/Textures/flares/icons/iris_shafts.dds"),
            ADD_FLARE_INFO(eFT_CameraOrbs, "CameraOrbs", "EngineAssets/Textures/flares/icons/orbs.dds"),
            ADD_FLARE_INFO(eFT_ImageSpaceShafts, "Vol Shafts", "EngineAssets/Textures/flares/icons/vol_shafts.dds"),
            ADD_FLARE_INFO(eFT_Streaks, "Streaks", "EngineAssets/Textures/flares/icons/iris_shafts.dds")
        };

        Props ret;
        ret.p = flareInfoArray;
        ret.size = sizeof(flareInfoArray) / sizeof(flareInfoArray[0]);
        return ret;
    }

private:
    FlareInfoArray();
    ~FlareInfoArray();
};

struct SLensFlareRenderParam
{
    SLensFlareRenderParam()
        : pCamera(NULL)
        , pShader(NULL)
    {
    }
    ~SLensFlareRenderParam(){}
    bool IsValid() const
    {
        return pCamera && pShader;
    }
    CCamera* pCamera;
    IShader* pShader;
};

class ISoftOcclusionQuery
{
public:
    // <interfuscator:shuffle>
    virtual ~ISoftOcclusionQuery() {}

    virtual void AddRef() = 0;
    virtual void Release() = 0;
    // </interfuscator:shuffle>
};

class IOpticsElementBase MFP_SIZE_ENFORCE
{
public:

    IOpticsElementBase()
        : m_nRefCount(0)
    {
    }
    void AddRef()
    {
        CryInterlockedIncrement(&m_nRefCount);
    }
    void Release()
    {
        if (CryInterlockedDecrement(&m_nRefCount) <= 0)
        {
            delete this;
        }
    }

    // <interfuscator:shuffle>
    virtual EFlareType GetType() = 0;
    virtual bool IsGroup() const = 0;
    virtual string GetName() const = 0;
    virtual void SetName(const char* ch_name) = 0;
    virtual void Load(IXmlNode* pNode) = 0;

    virtual IOpticsElementBase* GetParent() const = 0;
    virtual ~IOpticsElementBase() {
    }

    virtual bool IsEnabled() const = 0;

    virtual void AddElement(IOpticsElementBase* pElement) = 0;
    virtual void InsertElement(int nPos, IOpticsElementBase* pElement) = 0;
    virtual void Remove(int i) = 0;
    virtual void RemoveAll() = 0;
    virtual int GetElementCount() const = 0;
    virtual IOpticsElementBase* GetElementAt(int  i) const = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    virtual void Invalidate() = 0;

    virtual void Render(SLensFlareRenderParam* pParam, const Vec3& vPos) = 0;

    virtual void SetOpticsReference([[maybe_unused]] IOpticsElementBase* pReference) {}
    virtual IOpticsElementBase* GetOpticsReference() const { return NULL; }
    // </interfuscator:shuffle>

#if defined(FLARES_SUPPORT_EDITING)
    virtual AZStd::vector<FuncVariableGroup> GetEditorParamGroups() = 0;
#endif

    ///Basic Setters///////////////////////////////////////////////////////////////
    virtual void SetEnabled(bool enabled) { (void)enabled; }
    virtual void SetSize(float size) { (void)size; }
    virtual void SetPerspectiveFactor(float perspectiveFactor) { (void)perspectiveFactor; }
    virtual void SetDistanceFadingFactor(float distanceFadingFactor) { (void)distanceFadingFactor; }
    virtual void SetBrightness(float brightness) { (void)brightness; }
    virtual void SetColor(ColorF color) { (void)color; }
    virtual void SetMovement(Vec2 movement) { (void)movement; }
    virtual void SetTransform(const Matrix33& xform) { (void)xform; }
    virtual void SetOccBokehEnabled(bool occBokehEnabled) { (void)occBokehEnabled; }
    virtual void SetOrbitAngle(float orbitAngle) { (void)orbitAngle; }
    virtual void SetSensorSizeFactor(float sizeFactor) { (void)sizeFactor; }
    virtual void SetSensorBrightnessFactor(float brightnessFactor) { (void)brightnessFactor; }
    virtual void SetAutoRotation(bool autoRotation) { (void)autoRotation; }
    virtual void SetAspectRatioCorrection(bool aspectRatioCorrection) { (void)aspectRatioCorrection; }
    ////////////////////////////////////////////////////////////////////////////////

private:

    volatile int m_nRefCount;
};

class IOpticsManager
{
public:
    // <interfuscator:shuffle>
    virtual ~IOpticsManager(){}
    virtual void Reset() = 0;
    virtual IOpticsElementBase* Create(EFlareType type) const = 0;
    virtual bool Load(const char* fullFlareName, int& nOutIndex, bool forceReload = false) = 0;
    virtual bool Load(XmlNodeRef& rootNode, int& nOutIndex) = 0;
    virtual IOpticsElementBase* GetOptics(int nIndex) = 0;
    virtual bool AddOptics(IOpticsElementBase* pOptics, const char* name, int& nOutNewIndex, bool allowReplace = false) = 0;
    virtual bool Rename(const char* fullFlareName, const char* newFullFlareName) = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    virtual void Invalidate() = 0;
    // </interfuscator:shuffle>
};

typedef _smart_ptr<IOpticsElementBase> IOpticsElementBasePtr;

#endif // CRYINCLUDE_CRYCOMMON_IFLARES_H
