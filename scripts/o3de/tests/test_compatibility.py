#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest
import pathlib

from o3de import compatibility 

@pytest.mark.parametrize(
    "gem_json_data, all_gem_json_data, expected_number_incompatible", [
        # when no dependencies or versions exist expect compatible
        pytest.param({'gem_name':'gemA'}, {'gemA':[{'gem_name':'gemA'}]}, 0),
        # when dependency exists with no versions specifier expect compatible
        pytest.param({'gem_name':'gemA','dependencies':['gemB']}, 
            {
                'gemA':[{'gem_name':'gemA', 'dependencies':'gemB'}],
                'gemB':[{'gem_name':'gemB'}]
            },
            0),
        # when dependency is missing expect incompatible
        pytest.param({'gem_name':'gemA','dependencies':['gemB']}, 
            {
                'gemA':[{'gem_name':'gemA', 'dependencies':'gemB'}]
            },
            1),
        # when dependency exists with version specifier expect compatible
        pytest.param({'gem_name':'gemA','dependencies':['gemB==1.0.0']}, 
            {
                'gemA':[{'gem_name':'gemA', 'dependencies':'gemB==1.0.0'}],
                'gemB':[
                    {'gem_name':'gemB','version':'1.0.0'},
                    {'gem_name':'gemB','version':'2.0.0'}
                    ]
            },
            0),
        # when multiple compatible gems exist with versions expect compatible
        pytest.param({'gem_name':'gemA','dependencies':['gemB>1.0.0']}, 
            {
                'gemA':[{'gem_name':'gemA', 'dependencies':'gemB>1.0.0'}],
                'gemB':[
                    {'gem_name':'gemB','version':'2.0.0'}, # compatible
                    {'gem_name':'gemB','version':'3.0.0'}  # also compatible
                    ]
            },
            0),
        # when dependency with expected version missing expect incompatible
        pytest.param({'gem_name':'gemA','dependencies':['gemB==1.0.0']}, 
            {
                'gemA':[{'gem_name':'gemA', 'dependencies':'gemB==1.0.0'}],
                'gemB':[{'gem_name':'gemB','version':'2.0.0'}] # not compatible
            },
            1),
        # when nested dependency exists with version specifier expect compatible
        pytest.param({'gem_name':'gemA','dependencies':['gemB==1.0.0']}, 
            {
                'gemA':[{'gem_name':'gemA', 'dependencies':'gemB==1.0.0'}],
                'gemB':[
                    {'gem_name':'gemB','version':'1.0.0', 'dependencies':['gemC==1.0.0']},
                    {'gem_name':'gemB','version':'2.0.0', 'dependencies':['gemC==2.0.0']}
                    ],
                'gemC':[
                    {'gem_name':'gemC','version':'1.0.0'}, # compatible
                    {'gem_name':'gemC','version':'2.0.0'}
                    ]
            },
            0),
        # when nested dependency with requested version not found expect incompatible
        pytest.param({'gem_name':'gemA','dependencies':['gemB==1.0.0']}, 
            {
                'gemA':[{'gem_name':'gemA', 'dependencies':'gemB==1.0.0'}],
                'gemB':[
                    {'gem_name':'gemB','version':'1.0.0', 'dependencies':['gemC==1.0.0']},
                    {'gem_name':'gemB','version':'2.0.0', 'dependencies':['gemC==2.0.0']}
                    ],
                'gemC':[
                    {'gem_name':'gemC','version':'3.0.0'}, # not compatible 
                    {'gem_name':'gemC','version':'4.0.0'} # not compatible 
                    ]
            },
            2),
        # when multiple compatible options exist expect compatible 
        pytest.param({'gem_name':'gemA','dependencies':['gemB']}, 
            {
                'gemA':[{'gem_name':'gemA', 'dependencies':'gemB'}],
                'gemB':[
                    {'gem_name':'gemB','version':'1.0.0', 'dependencies':['gemC']},
                    {'gem_name':'gemB','version':'2.0.0', 'dependencies':['gemC==2.0.0']}
                    ],
                'gemC':[
                    {'gem_name':'gemC','version':'1.0.0'}, # compatible
                    {'gem_name':'gemC','version':'2.0.0'}  # also compatible
                    ]
            },
            0),
    ],
)
def test_get_incompatible_gem_version_specifiers(gem_json_data, all_gem_json_data, expected_number_incompatible):
    result = compatibility.get_incompatible_gem_version_specifiers(gem_json_data, all_gem_json_data, set())
    # Test number of incompatible version specifiers, because we don't want these 
    # tests to fail if the error message changes
    assert len(result) == expected_number_incompatible


@pytest.mark.parametrize(
    "gem_names, all_gem_json_data, expected_result", [
        # when the gem exists and no version specifiers are provided expect found
        pytest.param(['gemA'], {'gemA':[{'gem_name':'gemA','gem_path':pathlib.PurePath()}]}, ['gemA==0.0.0']),
        # when the gem dependency doesn't exist expect failure
        pytest.param(['gemA'], {}, []),
        # when the gem dependency with correct version doesn't exist expect failure
        pytest.param(['gemA~=1.2.0'], {'gemA':[{'gem_name':'gemA','version':'2.4.0'}]}, []),
        # when two gems exist with different versions expect higher version is selected 
        pytest.param(['gemA'], {
            'gemA':[
                {'gem_name':'gemA','version':'3.2.3'},
                {'gem_name':'gemA','version':'20.3.4'},
                {'gem_name':'gemA','version':'0.1.2'}
            ]}, ['gemA==20.3.4']),
        # when the gem sub dependency exists and no version specifiers are provided expect found
        pytest.param(['gemA'], {
            'gemA':[{'gem_name':'gemA','dependencies':['gemB']}],
            'gemB':[{'gem_name':'gemB'}]
            }, ['gemA==0.0.0','gemB==0.0.0']),
        # when the gem sub dependency doesn't exist expect failure 
        pytest.param(['gemA'], {
            'gemA':[{'gem_name':'gemA','dependencies':['gemB']}]
            }, []),
        # when the gem is compatible with the engine expect it is found 
        pytest.param(['gemA'], {
            'gemA':[{'gem_name':'gemA','compatible_engines':['o3de==1.0.0']}]
            }, ['gemA==0.0.0']),
        # when the gem is not compatible with the engine expect failure 
        pytest.param(['gemA'], {
            'gemA':[{'gem_name':'gemA','compatible_engines':['o3de==2.0.0']}]
            }, []),
        # when a gem that is compatible with the engine exists expect it is found 
        pytest.param(['gemA'], {
            'gemA':[
                {'gem_name':'gemA',"version":"1.0.0",'compatible_engines':['o3de==1.0.0']},
                {'gem_name':'gemA',"version":"2.0.0",'compatible_engines':['o3de==2.0.0']}
                ]
            }, ['gemA==1.0.0']),
        # when the circular dependency exists expect still succeeds 
        pytest.param(['gemA'], {
            'gemA':[{'gem_name':'gemA','dependencies':['gemB']}],
            'gemB':[{'gem_name':'gemB','dependencies':['gemA']}]
            }, ['gemA==0.0.0','gemB==0.0.0']),
        # when the two versions of a gem exist, but only one solution expect version gemC==1.0.0 used
        pytest.param(['gemA','gemB'], { 
            'gemA':[
                # gemA can use gemC 1.0.0 or 2.0.0
                {'gem_name':'gemA','version':'1.0.0','dependencies':['gemC>=1.0.0']}
                ],
            'gemB':[
                # gemB can only use gemC 1.0.0
                {'gem_name':'gemB','version':'4.3.2','dependencies':['gemC==1.0.0']}
                ],
            'gemC':[
                {'gem_name':'gemC','version':'1.0.0'}, # <-- should be selected
                {'gem_name':'gemC','version':'2.0.0'}
                ]
            }, ['gemA==1.0.0','gemB==4.3.2','gemC==1.0.0'])
    ]
)
def test_resolve_gem_dependencies(gem_names, all_gem_json_data, expected_result):
    engine_json_data = {
        'engine_name':'o3de',
        'version':'1.0.0'
    }
    result, errors = compatibility.resolve_gem_dependencies(gem_names, all_gem_json_data, engine_json_data)
    if result:
        result_set = set()
        for _, candidate in result.items():
            result_set.add(f'{candidate.name}=={candidate.version}')
        assert result_set == set(expected_result)
    else:
        assert not expected_result
        assert errors

@pytest.mark.parametrize(
    "num_root_gems, num_dependencies_per_gem, num_optional_gems, tree_depth", [
        # Given:
        # R num_root_gems
        # N num_dependencies_per_gem
        # D tree_depth
        # Total gems = R * (pow(N, D) - 1) / (N - 1)

        # Single gem with no dependencies, 1 gem
        pytest.param(1, 0, 0, 1),
        # 100 root gems with no dependencies, 100 gems
        pytest.param(100, 0, 0, 1),
        # 1 root gem with 3 dependencies, 2 levels, 4 gems
        pytest.param(1, 3, 0, 2),
        # 10 root gems with 3 dependencies, 2 levels, 40 gems
        pytest.param(10, 3, 0, 2),
        # 1 root gem with 3 dependencies, 3 levels, 13 gems
        pytest.param(1, 3, 0, 3),
        # 10 root gems with 2 dependencies, 5 levels, 310 gems, 4 optional
        pytest.param(10, 2, 4, 5),
        # 1 root gem with 3 dependencies, 3 levels, 13 gems, 3 optional
        pytest.param(1, 3, 3, 3),
    ]
)
def test_resolve_gem_dependencies_depth(num_root_gems, num_dependencies_per_gem, num_optional_gems, tree_depth):
    engine_json_data = {
        'engine_name':'o3de',
        'version':'1.0.0'
    }
    # require the root gems
    gem_names = [f"Gem_0_{index}" for index in range(num_root_gems)]

    # build the dependency tree to the requested depth
    all_gem_json_data = {} 
    for depth in range(tree_depth):
        for index in range(num_root_gems * pow(num_dependencies_per_gem, depth)):
            gem_name = f'Gem_{depth}_{index}'
            if depth > 0 and num_optional_gems > 0:
                gem_json_data = {'gem_name':{'name':gem_name, 'optional':True}}
                num_optional_gems = num_optional_gems - 1
            else:
                gem_json_data = {'gem_name':gem_name}
            if num_dependencies_per_gem and depth < tree_depth - 1:
                gem_json_data['dependencies'] = [
                    f'Gem_{depth + 1}_{index * num_dependencies_per_gem + dep_index}' 
                    for dep_index in range(num_dependencies_per_gem)]
            
            all_gem_json_data[gem_name] = [gem_json_data]

    result, errors = compatibility.resolve_gem_dependencies(gem_names, all_gem_json_data, engine_json_data)
    assert result and not errors
