Alphabet Letter P asset test cases:

Freeze transform vs Non Freeze Transform
Description: The non freeze transform version has an arbitrary transform on the root joint, while the freeze transform version has identical transform on the root joint.
Expectation: A straight up letter P that animated with slight mesh deformation. Both asset should be identical to each other, including skeleton, mesh, skinning and animation.

Dot name vs non dot name
Description: The dot name version has a dot character in the joint name. SceneAPI will recongnize the dot character as an invalid character and translate it to a valid character. (e.g xxx.xx to xxx_xx)
Expectation: The dot named version looks identical to the non dot named version. No error in AP is detected, but a warning in the AP for the dot named version suggesting that invalid name has been renamed to valid name.