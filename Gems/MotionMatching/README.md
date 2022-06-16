# Motion Matching

Motion matching is a data-driven animation technique that synthesizes motions based on existing animation data and the current character and input contexts.

https://user-images.githubusercontent.com/43751992/151820094-a7f0df93-bd09-4ea2-a34a-3d583815ff6c.mp4

## Setup

1. Add the `MotionMatching` gem to your project using the [Project Manager](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/) or the [Command Line Interface (CLI)](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/#using-the-command-line-interface-cli). See the documentation on  [Adding and Removing Gems in a Project](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/).
1. Compile your project and run.

## Features

A feature is a property extracted from the animation data and is used by the motion matching algorithm to find the next best matching frame. Examples of features are the position of the feet joints, the linear or angular velocity of the knee joints, or the trajectory history and future trajectory of the root joint. We can also encode environment sensations like obstacle positions and height, the location of the sword of an enemy character, or a football's position and velocity.

Their purpose is to describe a frame of the animation by their key characteristics and sometimes enhance the actual keyframe data (pos/rot/scale per joint) by e.g. taking the time domain into account and calculating the velocity or acceleration, or a whole trajectory to describe where the given joint came from to reach the frame and the path it moves along in the near future.

| Position Feature | Velocity Feature | Trajectory Feature |
| :------------- |:-------------| :-----|
| Matches joint positions | Matches joint velocities | Matches the trajectory history and future trajectory |
| ![Position Feature](https://user-images.githubusercontent.com/43751992/151818913-8ea11c40-3287-4fcf-aa7b-7209940cb852.png) | ![Velocity Feature](https://user-images.githubusercontent.com/43751992/151818945-546450ad-f970-4251-95d4-1d515e149d9b.png) | ![Trajectory Feature](https://user-images.githubusercontent.com/43751992/151819095-3cdb1524-957a-411e-9c0f-d2baa5a270c1.png) |

Features are responsible for each of the following:

1. Extract the feature values for a given frame in the motion database and store them in the feature matrix. For example, calculate the left foot joint linear velocity, convert it to relative to the root joint model space for frame 134 and place the XYZ components in the feature matrix starting at column 9.

1. Extract the feature from the current input context/pose and fill the query vector with it. For example, calculate the linear velocity of the left foot joint of the current character pose in relative-to the root joint model space and place the XYZ components in the feature query vector starting at position 9.

1. Calculate the cost of the feature so that the motion matching algorithm can weigh it in to search for the next best matching frame. An example would be calculating the squared distance between a frame in the motion matching database and the current character pose for the left foot joint.

> Features are extracted and stored relative to a given joint, in most cases the motion extraction or root joint, and thus are in model-space. This makes the search algorithm invariant to the character location and orientation and the extracted features, like e.g. a joint position or velocity, translate and rotate along with the character.

<table>
<tr>
<th>User-Interface</th>
<th>Property Descriptions</th>
</tr>
<tr>
    <td>
            <img src="https://user-images.githubusercontent.com/43751992/151819182-46d5d5c5-d3e0-47a2-a318-dc6181bdf188.png" alt="Shared Feature RPE" width="770">
    </td>
    <td>
        <b>Name:</b> Display name used for feature identification and debug visualizations.<br>
        <b>Joint:</b> Joint name to extract the data from.<br>
        <b>Relative To Joint:</b> When extracting feature data, convert it to relative-space to the given joint.<br>
        <b>Debug Draw:</b> Are debug visualizations enabled for this feature?<br>
        <b>Debug Draw Color:</b> Color used for debug visualizations to identify the feature.<br>
        <b>Cost Factor:</b> The cost factor for the feature is multiplied with the actual and can be used to change a feature's influence in the motion matching search.<br>
        <b>Residual:</b> Use 'Squared' in case minimal differences should be ignored and larger differences should overweight others. Use 'Absolute' for linear differences and don't want the mentioned effect.<br>
    </td>
</tr>
<tr>
    <td>
            <img src="https://user-images.githubusercontent.com/43751992/151819232-a441149a-8a77-4408-9ba0-c676414b16af.png" alt="Trajectory Feature RPE" width="770">
    </td>
    <td>
        <b>Past Time Range:</b> The time window the samples are distributed along for the trajectory history. [Default = 0.7 seconds]<br>
        <b>Past Samples:</b> The number of samples stored per frame for the past trajectory. [Default = 4 samples to represent the trajectory history]<br>
        <b>Past Cost Factor:</b> The cost factor is multiplied with the cost from the trajectory history and can be used to change the influence of the trajectory history match in the motion matching search.<br>
        <b>Future Time Range:</b> The time window the samples are distributed along for the future trajectory. [Default = 1.2 seconds]<br>
        <b>Future Samples:</b> The number of samples stored per frame for the future trajectory. [Default = 6 samples to represent the future trajectory]<br>
        <b>Future Cost Factor:</b> The cost factor is multiplied with the cost from the future trajectory and can be used to change the influence of the future trajectory match in the motion matching search.<br>
        <b>Facing Axis:</b> The facing direction of the character. Which axis of the joint transform is facing forward? [Default = Looking into Y-axis direction]<br>
    </td>
</tr>
</table>

## Feature schema

The feature schema is a set of features that define the criteria used in the motion matching algorithm and influences the runtime speed, memory used, and the results of the synthesized motion. It is the most influential, user-defined input to the system.

The schema defines which features are extracted from the motion database while the actual extracted data is stored in the feature matrix. Along with the feature type, settings like the joint to extract the data from, a debug visualization color, how the residual is calculated, or a custom feature is specified.

The more features are selected by the user, the bigger the chances are that the searched and matched pose hits the expected result but the slower the algorithm will be and the more memory will be used. The key is to use crucial and independent elements that define a pose and its movement without being too strict on the wrong end. The root trajectory along with the left and right foot positions and velocities have been proven to be a good start here.

![Feature Schema](https://user-images.githubusercontent.com/43751992/151819276-7b5dedc0-475b-4eb4-bc27-f29d799646d0.png)

## Feature matrix

The feature matrix is a NxM matrix that stores the extracted feature values for all frames in our motion database based upon a given feature schema. The feature schema defines the order of the columns and values and is used to identify values and find their location inside the matrix.

A 3D position feature storing XYZ values e.g. will use three columns in the feature matrix. Every component of a feature is linked to a column index, so e.g. the left foot position Y value might be at column index 6. The group of values or columns that belong to a given feature is what we call a feature block. The accumulated number of dimensions for all features in the schema, while the number of dimensions might vary per feature, form the number of columns of the feature matrix.

Each row represents the features of a single frame of the motion database. The number of rows of the feature matrix is defined by the number.

> Memory usage: A motion capture database holding 1 hour of animation data together with a sample rate of 30 Hz to extract features, resulting in 108,000 frames, using the default feature schema having 59 features, will result in a feature matrix holding ~6.4 million values and use ~24.3 MB of memory.

## Frame database (Motion database)

A set of frames from your animations sampled at a given sample rate is stored in the frame database. A frame object knows about its index in the frame database, the animation it belongs to, and the sample time in seconds. It does not hold the sampled pose for memory reasons as the `EMotionFX::Motion` already stores the transform keyframes.

The sample rate of the animation might differ from the sample rate used for the frame database. For example, your animations might be recorded with 60 Hz while we only want to extract the features with a sample rate of 30 Hz. As the motion matching algorithm is blending between the frames in the motion database while playing the animation window between the jumps/blends, it can make sense to have animations with a higher sample rate than we use to extract the features.

A frame of the motion database can be used to sample a pose from which we can extract the features. It also provides functionality to sample a pose with a time offset to that frame. This can be handy to calculate joint velocities or trajectory samples.

When importing animations, frames that are within the range of a discard frame motion event are ignored and won't be added to the motion database. Discard motion events can be used to cut out sections of the imported animations that are unwanted like a stretching part between two dance cards.

## Trajectory history

The trajectory history stores world space position and facing direction data of the root joint (motion extraction joint) with each game tick. The maximum recording time is adjustable but needs to be at least as long as the past trajectory window from the trajectory feature as the trajectory history is used to build the query for the past trajectory feature.

![Trajectory Feature](https://user-images.githubusercontent.com/43751992/151819315-beb8d9a1-69ca-49cd-bec0-ba2bae2dc469.png)

## Trajectory prediction

The user controls the character by its future trajectory. The future trajectory contains the path the character is expected to move along, if it should accelerate, move faster, or come to a stop, and if it should be walking forward doing a turn, or strafe sideways. Based on a joystick position, we need to predict the future trajectory and build the path and the facing direction vectors across the control points. The trajectory feature defines the time window of the prediction and the number of samples to be generated. We generate an exponential curve that starts in the direction of the character and then bends towards the given target.

https://user-images.githubusercontent.com/43751992/156741698-d2306bac-cdf5-4a25-96bd-0fc4422b598b.mp4

## Motion Matching data

Data based on a given skeleton but independent of the instance like the motion capture database, the feature schema or feature matrix is stored here. It is just a wrapper to group the sharable data.

## Motion Matching instance

The instance is where everything comes together. It stores the trajectory history, the trajectory query along with the query vector, knows about the last lowest cost frame index, and stores the time of the animation that the instance is currently playing. It is responsible for motion extraction, blending towards a new frame in the motion capture database in case the algorithm found a better matching frame and executes the actual search.

## Architecture
![Class Diagram](https://user-images.githubusercontent.com/43751992/151819361-878edcb5-2b1f-4867-bb7f-8ed5c09a075a.png)

## The motion matching algorithm

Motion matching plays small clips of a motion database, while jumping and smoothly transitioning back and forth, to synthesize a new animation from that data.

### Update loop

In the majority of the game ticks, the current motion gets advanced. A few times per second, the actual motion matching search is triggered to not drift away too far from the expected user input (as we would just play the recorded animation otherwise).

When a search for a better next matching frame is triggered, the current pose, including its joint velocities, gets evaluated. This pose (which we'll call input or query pose) is used to fill the query vector. The query vector contains feature values and is compared against other frames in the feature matrix. The query vector has the same size as there are columns in the feature matrix and is similar to any other row but represents the query pose.

Using the query vector, we can find the next best matching frame in the motion database and start transitioning towards that.

In case the new best matching frame candidate is close to the time in the animation that we are already playing, we don't do anything as we seem to be at the sweet spot in the motion database already.

Pseudo-code:
```
// Keep playing the current animation.
currentMotion.Update(timeDelta);

if (Is it time to search for a new best matching frame?) // We might e.g. do this 5x a second
{
    // Evaluate the current pose including joint velocities.
    queryPose = SamplePose(newMotionTime);

    // Update the input query vector (Calculate features for the query pose)
    queryValues = CalculateFeaturesFromPose(queryPose);

    // Find the frame with the lowest cost based on the query vector.
    bestMatchingFrame = FindBestMatchingFrame(queryValues);

    // Start transitioning towards the new best matching frame in case it is not
    // really close to the frame we are already playing.
    if (IsClose(bestMatchingFrame, currentMotion.GetFrame()) == false)
    {
        StartTransition(bestMatchingFrame);
    }
}
```

### Cost function

The core question in the algorithm is: Where do we jump and transition to? The algorithm tries to find the best time in the motion database that matches the current character pose including its movements and the user input. To compare the frame candidates with each other, we use a cost function.

The feature schema defines the cost function. Every feature added to the feature schema adds up to the cost. The bigger the discrepancy between e.g. the current velocity and the one from the frame candidate, the higher the penalty to the cost and the less likely the candidate is a good one to take.

This makes motion matching an optimization problem where the frame with the minimum cost is the most preferred candidate to transition to.

### Searching next best matching frame

The actual search happens in two phases, a broad phase to eliminate most of the candidates followed by a narrow phase to find the actual best candidate.

#### 1. Broad-phase (KD-tree)

A KD-tree is used to find the nearest neighbors (frames in the motion database) to the query vector (given input). The result is a set of pre-selected frames for the next best matching frame that is passed on to the narrow-phase. By adjusting the maximum tree depth or the minimum number of frames for the leaf nodes, the resulting number of frames can be adjusted. The bigger the set of frames the broad-phase returns, the more candidates the narrow-phase can choose from, the better the visual quality of the animation but the slower the algorithm.

#### 2. Narrow-phase

Inside the narrow-phase, we iterate through the returned set of frames from the KD-tree, and evaluate and compare their cost against each other. The frame with the minimal cost is the best match that we transition to.

Pseudo-code:
```
minCost = MAX;
for_all (nearest frames found in the broad-phase)
{
    frameCost = 0.0
    for_all (features)
    {
        frameCost += CalculateCost(feature);
    }

    if (frameCost < minCost)
    {
        // We found a better next matching frame
        minCost = frameCost;
        newBestMatchingFrame = currentFrame;
    }
}

StartTransition(newBestMatchingFrame);
```

## Demo Levels & Prefabs

There are two demo levels available in the `Gems/MotionMatching/Assets/Levels/`. Copy & paste the content of the `/Levels/` folder to your project levels folder to give them a test run.

There are also two prefabs available that you can instantiate into any of your existing levels or an empty new one for a quick try:

- `/Assets/AutomaticDemo/MotionMatching_AutoDemoCharacter.prefab`: Motion matching demo where the character will move around and follow a path in the level automatically.
- `/Assets/AutomaticDemo/MotionMatching_ControllableCharacter.prefab`: Motion matching demo where you can use a gamepad to move the character around.

https://user-images.githubusercontent.com/43751992/158962835-22a769a4-183b-438d-b7a9-f782e086b600.mp4

## Jupyter notebook

### Feature histograms

In the image below you can see histograms per feature component showing their value distributions across the motion database. They can provide interesting insights, like e.g. if the motion database is holding more moving forward animations than it has strafing or backward moving animations, or how many fast vs slow turning animations are in the database. This information can be used to see if there is still a need to record some animations or if some type of animation is overrepresented and will lead to ambiguity and decrease the quality of the resulting synthesized animation.

![Feature Histograms](https://user-images.githubusercontent.com/43751992/151819418-63580dc0-4358-4034-b60d-e8f1cbe138e2.png)

### Scatterplot using PCA

The image below shows our high-dimensional feature matrix data projected down to two dimensions using principal component analysis. The density of the clusters and the distribution of the samples overall indicate how hard it is for the search algorithm to find a good matching frame candidate.

> Clusters in the image after multiple projections might still be separatable over one of the diminished dimensions.

![Feature Scatterplot PCA](https://user-images.githubusercontent.com/43751992/151819455-b1272ce8-423b-4037-9a7c-f4cd0044c25b.png)