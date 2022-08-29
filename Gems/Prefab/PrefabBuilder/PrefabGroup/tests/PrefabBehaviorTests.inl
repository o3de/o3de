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
                    "Component_[5666150279650800686]": {
                        "$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent",
                        "Id": 5666150279650800686,
                        "Parent Entity": ""
                    },
                    "Component_[8790726658974076423]": {
                        "$type": "EditorOnlyEntityComponent",
                        "Id": 8790726658974076423
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
                        "Component_[15735658354806796004]": {
                            "$type": "EditorOnlyEntityComponent",
                            "Id": 15735658354806796004
                        }
                    }
                },
                "Entity_[1592947718779]": {
                    "Id": "Entity_[1592947718779]",
                    "Name": "cube",
                    "Components": {
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
                        "Component_[14936165953779771344]": {
                            "$type": "EditorVisibilityComponent",
                            "Id": 14936165953779771344
                        },
                        "Component_[403416213715997356]": {
                            "$type": "EditorOnlyEntityComponent",
                            "Id": 403416213715997356
                        }
                    }
                }
            }
        }
        )JSON";

    }
}
