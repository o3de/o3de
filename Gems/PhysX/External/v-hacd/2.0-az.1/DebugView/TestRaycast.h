#ifndef TEST_RAYCAST_H
#define TEST_RAYCAST_H

#include <stdint.h>

namespace RENDER_DEBUG
{
    class RenderDebug;
}


// Little helper class to test the raycast mesh code and display the results
class TestRaycast
{
public:
    static TestRaycast *create(void);

    virtual void testRaycast(uint32_t vcount,    // number of vertices
        uint32_t tcount,    // number of triangles
        const double *vertices,        // Vertices in the mesh
        const uint32_t *indices,    // triangle indices
        RENDER_DEBUG::RenderDebug *renderDebug) = 0; // rendering interface 

    virtual void release(void) = 0;

protected:
    virtual ~TestRaycast(void)
    {
    }
};

#endif
