## How to use this template

Use this template to produce a Multiplayer dependent Gem that can implement different behavior on the GameLauncher, ServerLauncher and UnifiedLauncher.

## Instanciate the template
From your gems folder call this command:

```
<O3DE_PATH>\scripts\o3de create-from-template -tn UnifiedMultiplayerGem -gp <PATH_TO_GEM>
```

## Authoring separated Client and Server logic

The Client, Server and Unified targets can all make use of their own sets of dependencies and cmake files.

They can separate logic within files using the macros AZ_TRAIT_CLIENT and AZ_TRAIT_SERVER.

For more information, see O3DE's documentation: https://development--o3deorg.netlify.app/docs/user-guide/networking/multiplayer/code_separation/
