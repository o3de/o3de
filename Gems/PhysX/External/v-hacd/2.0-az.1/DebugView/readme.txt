This is a sample Windows application for testing V-HACD with the DebugView remote debug visualization app

https://github.com/jratcliff63367/debugview

To build this sample app go to:

./DebugView/compiler/vc14win64 (or vc14win32, doesn't make any difference)

and load 'ConvexDecomposition.sln'

Build with Visual Studio.

After it builds, but before it runs, launch the DebugView application.

DebugView is a remote debug visualization application.  ConvexDecomposition is a simple
console application to test V-HACD.  This application has not graphics code, it simply
connects remotely to the DebugView application and sends commands.

To learn more about how to use the NvRenderDebug library look in the directory:

./DebugView/docs

Documentation for the NvRenderDebug API is in: NvRenderDebug.chm
Documentation for how to create GUI elements in DebugView load NvRenderDebugUserGuide.chm

This documentation is likely incomplete and may be out of date.
