#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest

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
        # when dependency is missing expect expect incompatible
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
        # when multiple compatible gems exists with version specifier expect compatible
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