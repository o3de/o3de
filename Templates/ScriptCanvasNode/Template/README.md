## How to use this template

Use this template to produce a build target that may hold a library of Script Canvas nodes. 


This guide will assume that you have created a gem called `YourGem` using the command: 

```
scripts\o3de create-gem -gn YourGem -gp <PATH\TO\YourGem>
```

From the o3de folder call this command:

```
scripts\o3de create-from-template -tp Templates\ScriptCanvasNode -dp Gems\YourGem\Code\Source\MyScriptCanvasNodes -r ${GemName} YourGem
```

## Steps after using this template

After running this template, there are still some manual steps needed in order to make it compile.

### Step 1.

Run the following command to register the newly created Script Canvas nodes project:

```
scripts\o3de register -esgp Gems\${GemName} -es Gems\${GemName}\Code\Source\${Name}
```

### Step 2.

Configure & Generate your projects

### Step 3. 

Build & Run