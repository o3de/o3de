#ifndef NV_PHYSX_FRAMEWORK_H
#define NV_PHYSX_FRAMEWORK_H

// The PhysX framework is a DLL which provides a very simple interface to perform
// physics simulations using the PhysX SDK.  It also contains bindings to do debug
// visualizations of the current simulation using the NvRenderDebug remote visualization
// API and/or PVD (the PhysX Visual Debugger)
// This is not intended to be a full function physics API but rather a minimal simple
// system for getting basic demos to work.  The intial version was written specifically 
// to improve and test the V-HACD (voxelized hierarchical convex decomposition library) results.
// @see: https://github.com/kmammou/v-hacd

#include <stdint.h>

namespace RENDER_DEBUG
{
    class RenderDebug;
}

namespace NV_PHYSX_FRAMEWORK
{

enum ConstraintType
{
    CT_FIXED,
    CT_SPHERICAL,
    CT_HINGE,
    CT_BALL_AND_SOCKET,
    CT_REVOLUTE,
};

#define PHYSX_FRAMEWORK_VERSION_NUMBER 1

// Instantiate the PhysX SDK, create a scene, and a ground plane
class PhysXFramework
{
public:
    class CommandCallback
    {
    public:
        /**
        *\brief Optional callback to the application to process an arbitrary console command.

        This allows the application to process an incoming command from the server.  If the application consumes the command, then it will not be passed on
        to the rest of the default processing.  Return true to indicate that you consumed the command, false if you did not.

        \return Return true if your application consumed the command, return false if it did not.
        */
        virtual bool processDebugCommand(uint32_t argc, const char **argv) = 0;
    };

    // A convex mesh
    class ConvexMesh
    {
    public:
        virtual void release(void) = 0;
    };

    // A compound actor comprised of an array of convex meshes
    class CompoundActor
    {
    public:
        virtual void addConvexMesh(ConvexMesh *cmesh,
            float meshPosition[3],
            float meshScale[3]) = 0;

        // Create a simulated actor based on the collection of convex meshes
        virtual void createActor(const float centerOfMass[3],float mass,bool asRagdoll) = 0;

        // Creates a fixed constraint between these two bodies
        virtual bool createConstraint(uint32_t bodyA,    // Index of first body
            uint32_t bodyB,                                // Index of second body
            const float worldPos[3],                    // World position of the constraint location
            const float worldOrientation[4],
            ConstraintType type,            // Type of constraint to use
            float    limitDistance,            // If a revolute joint, the distance limit
            uint32_t twistLimit,            // Twist limit in degrees (if used)
            uint32_t swing1Limit,            // Swing 1 limit in degrees (if used)
            uint32_t swing2Limit) = 0;        // Swing 2 limit in degrees (if used)

        virtual bool getXform(float xform[16],uint32_t index) = 0;
        virtual bool getConstraintXform(float xform[16], uint32_t constraint) = 0;

        // If we are mouse dragging and the currently selected object is an actor in this compound
        // system, then return true and assign 'bodyIndex' to the index number of the body selected.
        virtual bool getSelectedBody(uint32_t &bodyIndex) = 0;

        // Sets the collision filter pairs.
        virtual void setCollisionFilterPairs(uint32_t pairCount, const uint32_t *collisionPairs) = 0;

        virtual void release(void) = 0;
    };

    // Create a convex mesh using the provided raw triangles describing a convex hull.
    virtual ConvexMesh *createConvexMesh(uint32_t vcount,
        const float *vertices,
        uint32_t tcount,
        const uint32_t *indices) = 0;

    // Create a physically simulated compound actor comprised of a collection of convex meshes
    virtual CompoundActor *createCompoundActor(void) = 0;

    // Returns delta time since last simulation step
    virtual float simulate(bool showPhysics) = 0;

    // Create a default series of stacked boxes for testing purposes
    virtual void createDefaultStacks(void) = 0;

    virtual void setCommandCallback(CommandCallback *cc) = 0;

    // create a box in the simulated scene
    virtual void createBox(const float boxSize[3], const float boxPosition[3]) = 0;

    // Return the render debug interface if available
    virtual RENDER_DEBUG::RenderDebug *getRenderDebug(void) = 0;

    // Release the PhysXFramework interface
    virtual void release(void) = 0;

protected:
    virtual ~PhysXFramework(void)
    {
    }
};

PhysXFramework *createPhysXFramework(uint32_t versionNumber,const char *dllName);


};

#endif
