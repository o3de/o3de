## How to use this template

Use this template to produce a Script Canvas project that may hold Script Canvas nodes. 

This guide will assume that you have created a gem called: `YourGem`

```
scripts\o3de create-from-template -tp C:\dev\o3de\Templates\ScriptCanvasNode -dp C:\dev\o3de\Gems\YourGem\Code\Source\MyScriptCanvasNodes
```

## Steps after using this template

After running this template, there are still some manual steps needed in order to make it compile.

### Step 1.

Add the following into Gems/`YourGem`/Code/CMakeLists.txt

```
add_subdirectory(Source/${Name})
```

### Step 2.

Locate the following configurations:

```
NAME YourGem.Editor GEM_MODULE
NAME ScriptCanvasO3DCon.Private.Object STATIC
NAME YourGem ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}```
```

And add the following under their respective BUILD_DEPENDENCIES

```
ScriptCanvasNodes::${Name}.Static
```

Then locate the file: YourGemSystemComponent.cpp

and add the following code:

```
#include <${Name}_Nodeables.generated.h>

REGISTER_SCRIPTCANVAS_AUTOGEN_NODEABLE(${Name}Static);
```

### Step 3. 

Configure & Generate your projects

### Step 4. 

Build