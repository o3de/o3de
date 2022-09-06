## How to use this template

Use this template to produce a Script Canvas project that may hold Script Canvas nodes. 

This guide will assume that you have created a gem called: `YourGem`

```
scripts\o3de create-from-template -tp C:\dev\o3de\Templates\ScriptCanvasNode -dp C:\dev\o3de\Gems\YourGem\Code\Source\MyScriptCanvasNodes -r gem_name YourGem
```

## Steps after using this template

After running this template, there are still some manual steps needed in order to make it compile.

### Step 1.

Add the following AT THE END of Gems/`YourGem`/Code/CMakeLists.txt

```
add_subdirectory(Source/${Name})
```

### Step 2.

Configure & Generate your projects

### Step 3. 

Build