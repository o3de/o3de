# Motion Matching

Motion matching is a data-driven animation technique that synthesizes motions based on existing animation data and the current character and input contexts.

# Features

A feature is a property extracted from the animation data and is used by the motion matching algorithm to find the next best matching frame. Examples of features are the position of the feet joints, the linear or angular velocity of the knee joints or the trajectory history and future trajectory of the root joint. We can also encode environment sensations like obstacle positions and height, the location of the sword of an enemy character or a football's position and velocity.

Their purpose is to describe a frame of the animation by their key characteristics and sometimes enhance the actual keyframe data (pos/rot/scale per joint) by e.g. taking the time domain into account and calculate the velocity or acceleration, or a whole trajectory to describe where the given joint came from to reach the frame and the path it moves along in the near future.

Features are responsible for each of the following:

1. Extract the feature values for a given frame in the motion database and store them in the feature matrix. For example calculate the left foot joint linear velocity, convert it to relative-to the root joint model space for frame 134 and place the XYZ components in the feature matrix starting at column 9.
1. Extract the feature from the current input context/pose and fill the query vector with it. For example calculate the linear velocity of the left foot joint of the current character pose in relative-to the root joint model space and place the XYZ components in the feature query vector starting at position 9.
1. Calculate the cost of the feature so that the motion matching algorithm can weight it into search for the next best matching frame. An example would be calculating the squared distance between a frame in the motion matching database and the current character pose for the left foot joint.

# Feature schema

The feature schema is a set of features that define the criteria used in the motion matching algorithm and influences the runtime speed, memory used, and the results of the synthesized motion. It is the most influential, user-defined input to the system.

The schema defines which features are extracted from the motion database while the actual extracted data is stored in the feature matrix. Along with the feature type, settings like the joint to extract the data from, a debug visualization color, how the residual is calculated or a custom feature is specified.

The more features are selected by the user, the bigger the chances are that the searched and matched pose hits the expected result but the slower the algorithm will be and the more memory will be used. The key is to use crucial and independent elements that define a pose and its movement without being too strict on the wrong end. The root trajectory along with the left and right foot positions and velocities have been proven to be a good start here.

# Feature matrix

The feature matrix is an NxN matrix which stores the extracted feature values for all frames in our motion database based upon a given feature schema. The feature schema defines the order of the columns and values and is used to identify values and find their location inside the matrix.

A 3D position feature storing XYZ values e.g. will use three columns in the feature matrix. Every component of a feature is linked to a column index, so e.g. the left foot position Y value might be at column index 6. The group of values or columns that belong to a given feature is what we call a feature block. The accumulated number of dimensions for all features in the schema, while the number of dimensions might vary per feature, form the number of columns of the feature matrix.

Each row represents the features of a single frame of the motion database. The number of rows of the feature matrix is defined by the number.

![Feature Schema](Docs/Images/FeatureSchema.png)

# Trajectory History

The trajectory history stores world space position and facing direction data of the root joint (motion extraction joint) with each game tick. The maximum recording time is adjustable but needs to be at least as long as the past trajectory window from the trajectory feature as the trajectory history is used to build the query for the past trajectory feature.

# Motion Matching data

Data based on a given skeleton but independent of the instance like the motion capture database, the feature schema or feature matrix is stored in here. It is just a wrapper to group the sharable data.

# Motion Matching instance

The instance is where everything comes together. It stores the trajectory history, the trajectory query along with the query vector, knows about the last lowest cost frame frame index and stores the time of the animation that the instance is currently playing. It is responsible for motion extraction, blending towards a new frame in the motion capture database in case the algorithm found a better matching frame and executes the actual search.

# Architecture Diagram
![Class Diagram](Docs/Images/ArchitectureDiagram.png)