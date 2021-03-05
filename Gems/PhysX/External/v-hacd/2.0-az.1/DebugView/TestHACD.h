#ifndef TEST_HACD_H
#define TEST_HACD_H

#include <stdint.h>
#include "VHACD.h"

namespace RENDER_DEBUG
{
    class RenderDebug;
}

namespace NV_PHYSX_FRAMEWORK
{
    class PhysXFramework;
}

class TestHACD
{
public:

    static TestHACD *create(RENDER_DEBUG::RenderDebug *renderDebug,NV_PHYSX_FRAMEWORK::PhysXFramework *physxFramework);

    virtual void toggleSimulation(void) = 0;

    virtual void decompose(
        const double* const points,
        const uint32_t countPoints,
        const uint32_t* const triangles,
        const uint32_t countTriangles,
        VHACD::IVHACD::Parameters &desc) = 0;

    virtual void render(float explodeViewScale,const float center[3],bool wireframe) = 0;

    virtual void getTransform(float xform[16]) = 0;

    virtual uint32_t getHullCount(void) const = 0;

    virtual void saveConvexDecomposition(const char *fname,const char *sourceMeshName) = 0;

    virtual void cancel(void) = 0;

    virtual void release(void) = 0;
protected:
    virtual ~TestHACD(void)
    {
    }
};

#endif
