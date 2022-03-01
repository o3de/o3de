Purpose: 
--------
MeshExampleComponent will generate a secondary meshlet model that will be displayed
along side the original one with UVs grouped by meshlet and can be associated with 
color - this is done by using the material DebugShaderMaterial_01 based on the 
shader DebugShaderPBR_ForwardPass - all copied to this folder.
 
How to Build and Run:
1. Copy the files in this folder over the original ASV files
2. Run cmake again
3. Compile meshoptimizer.lib as static library and add it to the Meshlets gem
3. Compile in VS
4. Run the standalone ASV and choose the MeshExampleComponent
5. When selecting a large enough model, a secondary meshlet model will be generated
and displayed along side.