## How to use this template

Use this template to produce a build target that may hold a library of Script Canvas nodes. 


This guide will assume that you have created a gem called `YourGem` using the command: 

```
<O3DE_PATH>\scripts\o3de create-gem -gn YourGem -gp <PATH\TO\YourGem>
```


## Instantiate the template
From your gems folder call this command:

```
<O3DE_PATH>\scripts\o3de create-from-template -tn ScriptCanvasNode -dp Code\Source\MyScriptCanvasNodes
```


## Steps after using this template

After running this template, there are still some manual steps needed in order to make it compile.

### Step 1.
From the gems folder call the following command to add the newly created Script Canvas nodes folder to the Gems gem.json:

```
<O3DE_PATH>\scripts\o3de register -esgp . -es .\Code\Source\MyScriptCanvasNodes
```

### Step 2.

Configure & Generate your projects

### Step 3. 

Build & Run
