#include "TestRaycast.h"
#include "NvRenderDebug.h"
#include "vhacdRaycastMesh.h"
#include <float.h>
#include <math.h>

const double FM_PI = 3.1415926535897932384626433832795028841971693993751f;
const double FM_DEG_TO_RAD = ((2.0f * FM_PI) / 360.0f);
const double FM_RAD_TO_DEG = (360.0f / (2.0f * FM_PI));

static void fm_getAABB(uint32_t vcount, const double *p,double *bmin, double *bmax)
{
    bmin[0] = p[0];
    bmin[1] = p[1];
    bmin[2] = p[2];

    bmax[0] = p[0];
    bmax[1] = p[1];
    bmax[2] = p[2];

    p += 3;

    for (uint32_t i = 1; i < vcount; i++)
    {
        if (p[0] < bmin[0]) bmin[0] = p[0];
        if (p[1] < bmin[1]) bmin[1] = p[1];
        if (p[2] < bmin[2]) bmin[2] = p[2];

        if (p[0] > bmax[0]) bmax[0] = p[0];
        if (p[1] > bmax[1]) bmax[1] = p[1];
        if (p[2] > bmax[2]) bmax[2] = p[2];
        p += 3;

    }
}

// Little helper class to test the raycast mesh code and display the results
class TestRaycastImpl : public TestRaycast
{
public:
    TestRaycastImpl(void)
    {

    }
    virtual ~TestRaycastImpl(void)
    {

    }

    void doubleToFloatVert(const double *source, float *dest)
    {
        dest[0] = float(source[0]);
        dest[1] = float(source[1]);
        dest[2] = float(source[2]);
    }

    void floatToDoubleVert(const float *source, double *dest)
    {
        dest[0] = double(source[0]);
        dest[1] = double(source[1]);
        dest[2] = double(source[2]);
    }


    virtual void testRaycast(uint32_t vcount,    // number of vertices
        uint32_t tcount,    // number of triangles
        const double *vertices,        // Vertices in the mesh
        const uint32_t *indices,    // triangle indices
        RENDER_DEBUG::RenderDebug *renderDebug)  // rendering interface 
    {
        if (vcount == 0 || tcount == 0) return;

        double bmin[3];
        double bmax[3];
        float center[3];
        fm_getAABB(vcount, vertices, bmin, bmax);
        center[0] = float((bmin[0] + bmax[0])*0.5f);
        center[1] = float((bmin[1] + bmax[1])*0.5f);
        center[2] = float((bmin[2] + bmax[2])*0.5f);

        double dx = bmax[0] - bmin[0];
        double dy = bmax[1] - bmin[1];
        double dz = bmax[2] - bmin[2];

        double distance = sqrt(dx*dx + dy*dy + dz*dz)*0.5;

        renderDebug->pushRenderState();
        renderDebug->setCurrentDisplayTime(5.0f); // 5 seconds
        renderDebug->removeFromCurrentState(RENDER_DEBUG::DebugRenderState::SolidShaded);
        renderDebug->removeFromCurrentState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
        renderDebug->setCurrentArrowSize(float(distance*0.01));

        VHACD::RaycastMesh *rm = VHACD::RaycastMesh::createRaycastMesh(vcount, vertices, tcount, indices);

        float bmn[3];
        float bmx[3];
        doubleToFloatVert(bmin, bmn);
        doubleToFloatVert(bmax, bmx);

        renderDebug->setCurrentColor(0xFFFFFF);
        renderDebug->debugBound(bmn, bmx);

        uint32_t stepSize = 15;
        //
        for (uint32_t theta = 0; theta < 360; theta += stepSize)
        {
            for (uint32_t phi = 0; phi < 360; phi += stepSize)
            {
                double t = theta*FM_DEG_TO_RAD;
                double p = phi*FM_DEG_TO_RAD;
                float point[3];

                point[0] = float(cos(t)*sin(p)*distance) + center[0];
                point[1] = float(cos(p)*distance) + center[1];
                point[2] = float(sin(t)*sin(p)*distance) + center[2];


                {
                    double from[3];
                    double to[3];
                    floatToDoubleVert(center, from);
                    floatToDoubleVert(point, to);
                    double hitLocation[3];
                    double hitDistance;
                    if (rm->raycast(from,to,from,hitLocation,&hitDistance))
                    {
                        float hit[3];
                        doubleToFloatVert(hitLocation, hit);
                        renderDebug->setCurrentColor(0xFFFF00);
                        renderDebug->debugLine(center, hit);
                        renderDebug->debugSphere(hit, float(distance)*0.01f);
                    }
                    else
                    {
                        renderDebug->setCurrentColor(0xFFFFFF);
                        renderDebug->debugRay(center, point);
                    }
                }

            }
        }
        //

        rm->release();

        renderDebug->popRenderState();
    }

    virtual void release(void)
    {
        delete this;
    }

protected:
};

TestRaycast *TestRaycast::create(void)
{
    TestRaycastImpl *ret = new TestRaycastImpl;
    return static_cast<TestRaycast *>(ret);
}

