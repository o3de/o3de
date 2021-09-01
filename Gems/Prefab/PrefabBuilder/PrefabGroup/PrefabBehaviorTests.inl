/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace UnitTest
{
    namespace Data
    {
        const char* jsonPrefab = R"JSON(
        {
            "ContainerEntity": {
                "Id": "ContainerEntity",
                "Name": "test_template_1",
                "Components": {
                    "Component_[12122553907433030840]": {
                        "$type": "EditorVisibilityComponent",
                        "Id": 12122553907433030840
                    },
                    "Component_[1498016384077983031]": {
                        "$type": "EditorInspectorComponent",
                        "Id": 1498016384077983031
                    },
                    "Component_[15487514740313433733]": {
                        "$type": "SelectionComponent",
                        "Id": 15487514740313433733
                    },
                    "Component_[16610890794137419503]": {
                        "$type": "EditorLockComponent",
                        "Id": 16610890794137419503
                    },
                    "Component_[5332865223082427447]": {
                        "$type": "EditorEntityIconComponent",
                        "Id": 5332865223082427447
                    },
                    "Component_[5666150279650800686]": {
                        "$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent",
                        "Id": 5666150279650800686,
                        "Parent Entity": ""
                    },
                    "Component_[765328059189592397]": {
                        "$type": "EditorPendingCompositionComponent",
                        "Id": 765328059189592397
                    },
                    "Component_[8498001522604685238]": {
                        "$type": "EditorDisabledCompositionComponent",
                        "Id": 8498001522604685238
                    },
                    "Component_[8790726658974076423]": {
                        "$type": "EditorOnlyEntityComponent",
                        "Id": 8790726658974076423
                    },
                    "Component_[9667185869154382250]": {
                        "$type": "EditorEntitySortComponent",
                        "Id": 9667185869154382250
                    }
                }
            },
            "Entities": {
                "Entity_[1588652751483]": {
                    "Id": "Entity_[1588652751483]",
                    "Name": "root",
                    "Components": {
                        "Component_[11872748096995986607]": {
                            "$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent",
                            "Id": 11872748096995986607,
                            "Parent Entity": "ContainerEntity",
                            "Transform Data": {
                                "Rotate": [
                                    0.0,
                                    0.10000000149011612,
                                    180.0
                                ]
                            }
                        },
                        "Component_[12138841758570858610]": {
                            "$type": "EditorVisibilityComponent",
                            "Id": 12138841758570858610
                        },
                        "Component_[14625581157711016218]": {
                            "$type": "EditorInspectorComponent",
                            "Id": 14625581157711016218
                        },
                        "Component_[14919101303050366796]": {
                            "$type": "EditorEntitySortComponent",
                            "Id": 14919101303050366796
                        },
                        "Component_[15515181886458915298]": {
                            "$type": "EditorLockComponent",
                            "Id": 15515181886458915298
                        },
                        "Component_[15735658354806796004]": {
                            "$type": "EditorOnlyEntityComponent",
                            "Id": 15735658354806796004
                        },
                        "Component_[17953564020971650687]": {
                            "$type": "EditorEntityIconComponent",
                            "Id": 17953564020971650687
                        },
                        "Component_[3285888823740010744]": {
                            "$type": "EditorPendingCompositionComponent",
                            "Id": 3285888823740010744
                        },
                        "Component_[3904247990633064545]": {
                            "$type": "SelectionComponent",
                            "Id": 3904247990633064545
                        },
                        "Component_[4060055610235425829]": {
                            "$type": "EditorDisabledCompositionComponent",
                            "Id": 4060055610235425829
                        }
                    }
                },
                "Entity_[1592947718779]": {
                    "Id": "Entity_[1592947718779]",
                    "Name": "cube",
                    "Components": {
                        "Component_[11028432302188930225]": {
                            "$type": "EditorPendingCompositionComponent",
                            "Id": 11028432302188930225
                        },
                        "Component_[11897961090905791223]": {
                            "$type": "EditorDisabledCompositionComponent",
                            "Id": 11897961090905791223
                        },
                        "Component_[12905395079655711814]": {
                            "$type": "EditorInspectorComponent",
                            "Id": 12905395079655711814
                        },
                        "Component_[13209575004398276516]": {
                            "$type": "EditorEntitySortComponent",
                            "Id": 13209575004398276516
                        },
                        "Component_[16841236039772333844]": {
                            "$type": "EditorEntityIconComponent",
                            "Id": 16841236039772333844
                        },
                        "Component_[2505301170249328189]": {
                            "$type": "EditorOnlyEntityComponent",
                            "Id": 2505301170249328189
                        },
                        "Component_[3716170894544198343]": {
                            "$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent",
                            "Id": 3716170894544198343,
                            "Parent Entity": "Entity_[1588652751483]"
                        },
                        "Component_[5862175558847453681]": {
                            "$type": "EditorVisibilityComponent",
                            "Id": 5862175558847453681
                        },
                        "Component_[7605739423345701660]": {
                            "$type": "SelectionComponent",
                            "Id": 7605739423345701660
                        },
                        "Component_[867597314797543071]": {
                            "$type": "EditorLockComponent",
                            "Id": 867597314797543071
                        }
                    }
                },
                "Entity_[1597242686075]": {
                    "Id": "Entity_[1597242686075]",
                    "Name": "cubeKid",
                    "Components": {
                        "Component_[10128771992421174485]": {
                            "$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent",
                            "Id": 10128771992421174485,
                            "Parent Entity": "Entity_[1592947718779]"
                        },
                        "Component_[13679695319191333980]": {
                            "$type": "EditorEntityIconComponent",
                            "Id": 13679695319191333980
                        },
                        "Component_[14739513975517725784]": {
                            "$type": "EditorPendingCompositionComponent",
                            "Id": 14739513975517725784
                        },
                        "Component_[14936165953779771344]": {
                            "$type": "EditorVisibilityComponent",
                            "Id": 14936165953779771344
                        },
                        "Component_[15636711858861940141]": {
                            "$type": "EditorEntitySortComponent",
                            "Id": 15636711858861940141
                        },
                        "Component_[16486885492489102758]": {
                            "$type": "EditorInspectorComponent",
                            "Id": 16486885492489102758
                        },
                        "Component_[2598752278756923527]": {
                            "$type": "EditorLockComponent",
                            "Id": 2598752278756923527
                        },
                        "Component_[3275374693902181278]": {
                            "$type": "EditorDisabledCompositionComponent",
                            "Id": 3275374693902181278
                        },
                        "Component_[403416213715997356]": {
                            "$type": "EditorOnlyEntityComponent",
                            "Id": 403416213715997356
                        },
                        "Component_[4665465936340642134]": {
                            "$type": "SelectionComponent",
                            "Id": 4665465936340642134
                        }
                    }
                }
            }
        }
        )JSON";

    }
}
