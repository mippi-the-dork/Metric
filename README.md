# Metric 0.11.0 Prototype

Metric is a viewport-first spatial reference tool for Unreal Engine 5.8 and the Vista Viewport Suite.

Metric 0.11.0 adds session-only Pinned Benchmarks for leaving lightweight comparison hulls in the current editor world while continuing to inspect other spaces.

## Current prototype capabilities

- **Backslash (`\\`)** toggles the Metric Lens while a perspective Level Editor viewport has keyboard focus.
- **Shift + Backslash** cycles through one unified list:
  1. Metric's built-in Human Reference, when enabled.
  2. Project-registered `UMetricPreset` assets.
  3. Unreal's existing `ULevelEditorViewportSettings::PreviewMeshes`, when enabled in Metric user settings.
- The two Metric commands are registered with Unreal's command system and can be rebound in **Editor Preferences > Keyboard Shortcuts > Metric**.
- Metric consumes the legacy Backslash chords in a focused Level Editor viewport so Unreal's native preview mesh does not render at the same time.
- The lens follows the mouse cursor using an editor-world line trace.
- Metric entries render a world-space hull, ground anchor, forward arrow, and label without changing editor modes.
- Native Unreal preview-mesh entries remain visual-only Static Mesh scale references. Metric does not invent gameplay data for them.
- The render host is transient, editor-only, hidden from the Scene Outliner, unselectable, collisionless, and destroyed with Metric/editor teardown.
- Native Preview Mesh settings are read-only. Metric never rewrites Unreal's existing preview mesh array.


## Pinned Benchmarks

- **Ctrl + Backslash** pins the current Metric hull at the live lens position.
- **Ctrl + Shift + Backslash** clears all pins in the current editor world.
- Pins are subsystem-owned visualization data, not Actors, so they never appear in the Scene Outliner and never become production level content.
- Pins are session/world scoped. Loading another map clears them intentionally.
- The oldest pin is removed automatically when the configurable maximum pin count is reached.
- Pinned hulls use the dedicated user-configurable yellow benchmark color and appear in the contextual legend.

## Clearance Probe

Full Metric preset entries now perform a forward shape sweep using the active hull. Native Unreal Preview Mesh entries remain visual-only and never run clearance queries.

Each preset exposes:

- **Show Clearance Probe**
- **Probe Distance**
- **Warning Margin**
- **Trace Channel**

Supported query shapes:

- **Capsule**: exact standard Unreal capsule sweep.
- **Oriented Box**: exact standard Unreal box sweep using the Metric forward direction.
- **Sphere**: exact standard Unreal sphere sweep.
- **Cylinder**: approximated with a capsule of matching width/height and labeled `EST` in the readout.

Viewport result language:

- **Green**: the full requested distance is clear.
- **Amber**: the requested distance is clear, but an obstacle lies inside the configured warning margin beyond it.
- **Red**: the configured hull blocks before reaching the requested distance.

The viewport shows a measurement line, a ghost hull at the resolved/requested end position, a contact marker when an obstacle is found, and a compact readout such as `CLEAR 300u+`, `CLEAR 300u | obstacle +14u`, or `BLOCKED 142u / 300u`.

This is a spatial preflight query, not a claim of exact gameplay movement parity. The result is only as representative as the preset hull and collision channel.


## Trajectory Arc

Metric presets now expose an optional **Trajectory** layer for jump-space and ballistic-path approximation. The trajectory uses the preset's current hull, a configurable horizontal speed, vertical launch speed, gravity scale, simulation time, sample interval, landing slope limit, and collision channel.

The current hull is swept between trajectory samples rather than tracing only a center line. This allows the arc to catch ceilings, walls, and other geometry that the entity volume itself would hit. Capsule, Oriented Box, and Sphere use Unreal's corresponding standard collision shapes. Cylinder remains a capsule approximation and is labeled `EST`.

Viewport states:

- **Orange**: the configured arc is still in flight when the simulation window ends, so no landing was found.
- **Green**: the descending hull contacted a surface at or below the configured maximum landing slope.
- **Red**: the hull hit obstructing geometry before reaching a valid landing.

The readout reports flight time and horizontal distance, for example `LANDING OK 1.1s / 660u`, `ARC BLOCKED 0.6s / 360u`, or `NO LANDING 2.0s / 1200u`.

Trajectory Arc is intentionally an approximation tool. It does not model CharacterMovement acceleration, air control, custom gravity, root motion, gameplay ability logic, animation, or project-specific jump code.


## Camera Probe

Metric presets now expose an optional **Camera Probe** layer for third-person camera-space validation. It approximates a spring-arm-style camera collision query using a sphere sweep from a configurable pivot to a desired camera position behind the Metric reference.

Each preset exposes:

- **Show Camera Probe**
- **Pivot Z Offset**
- **Target Distance**
- **Vertical Offset**
- **Shoulder Offset**
- **Probe Radius**
- **Min Distance**
- **Trace Channel**

Viewport states:

- **Magenta / CAM CLEAR**: the full requested camera position is unobstructed.
- **Compressed / CAM COMPRESSED**: geometry pushes the camera closer than the requested position.
- **Red / CAM TOO CLOSE**: the resolved distance falls below the preset's configured minimum usable camera distance.

The requested camera endpoint remains visible while compressed, so the designer can compare desired and resolved placement directly. Metric also draws the sphere at the resolved camera position and a contact normal when blocked.

Camera Probe is an approximation layer. It does not run a project's actual Camera Manager, spring arm component, camera modifiers, lag, collision callbacks, or gameplay camera state.



## Camera Sightline / Framing Validation

Metric presets now expose an optional **Camera Sightline** layer. The layer evaluates from the resolved Camera Probe position when Camera Probe is active, so compression affects the same viewpoint the designer is validating. It checks two things: whether geometry occludes the reference subject point, and whether that point stays inside a configurable framing half-angle around the camera's approximate aim toward the camera pivot.

Each preset exposes:

- **Show Camera Sightline**
- **Subject Z Offset**
- **Framing Half Angle**
- **Trace Radius**
- **Trace Channel**

Viewport states include `VIEW OK`, `VIEW OCCLUDED`, `FRAMING FAIL`, and `VIEW BLOCKED + FRAMING FAIL`. The lilac view line and framing wedge are reference geometry only. This does not run the project's real FOV, camera manager, look-at logic, camera modifiers, animation, or target-framing system.

## Ground Validation

Metric presets now expose an optional **Ground Validation** layer. It samples a forward strip across the preset width and compares the ground beneath those samples against the preset's traversal limits.

Each preset exposes:

- **Show Ground Validation**
- **Forward Distance**
- **Sample Spacing**
- **Lateral Coverage**
- **Max Walkable Slope**
- **Max Step Up**
- **Max Step Down**
- **Trace Height**
- **Trace Depth**
- **Trace Channel**

Metric uses three lanes when Lateral Coverage is greater than zero: left, center, and right. Samples are traced vertically onto the world. The result flags excessive surface slope, excessive rise/fall compared with the previous sample in that lane, and missing ground within the trace depth.

Viewport language:

- **Green**: sampled ground is within the configured traversal limits.
- **Amber**: step-up or step-down exceeds the configured limit.
- **Red**: the sampled surface exceeds the slope limit or no ground is found within the configured trace depth.

The label summarizes the field as `GROUND OK` or `GROUND FAIL` and reports the worst observed slope and step values. This is a geometric approximation only. It does not run CharacterMovement floor finding, navmesh generation, step-up code, movement modes, or project-specific traversal logic.

## Fit Analysis

Metric presets now expose an optional **Fit Analysis** layer for corridor, doorway, tunnel, and ceiling evaluation. Unlike Clearance Probe, which asks whether the current hull can sweep forward, Fit Analysis samples cross-sections along a configurable forward distance and measures lateral room plus floor-to-ceiling room.

Each preset exposes:

- **Show Fit Analysis**
- **Forward Distance**
- **Sample Spacing**
- **Max Side Probe Distance**
- **Max Ceiling Probe Height**
- **Warning Margin**
- **Floor Trace Height**
- **Floor Trace Depth**
- **Trace Channel**

Viewport states:

- **Green / FIT OK**: the sampled cross-sections meet or exceed the current hull width and height.
- **Amber / FIT TIGHT**: the hull fits, but one or more measurements fall inside the configured warning margin.
- **Red / FIT FAIL**: at least one sample is too narrow or too low. The readout reports the largest measured width and height deficits directly.
- **FIT NO FLOOR**: a sample could not establish a floor within the configured vertical trace range, so Metric does not pretend that width/height measurements there are trustworthy.

Fit Analysis measures against the preset hull dimensions at the current Metric centerline. It is a geometric blockout instrument, not navmesh, movement-component, crouch, avoidance, or path-planning simulation. A space may be globally wide enough but still fail at the current centerline if the hull is positioned too close to one side.

## Built-in Human Reference

The fallback reference is intentionally simple so Metric works immediately in a new project:

- Capsule
- Height: 180u
- Width: 88u
- Length: 88u

These are prototype values, not a claim about any project's player dimensions.

## Creating a Metric Preset

1. In the Content Browser, create a **Data Asset**.
2. Choose `MetricPreset` as the Data Asset class.
3. Set its display name and hull dimensions.
4. Optionally enable a Static Mesh representation and assign its relative transform.
5. Open **Project Settings > Plugins > Metric**.
6. Add the preset to **Registered Presets**.
7. Toggle Metric off/on or use Shift + Backslash to refresh the runtime cycle list.

## Settings

### Project Settings > Plugins > Metric

- Include Built-In Human Reference
- Registered Presets
- Placement Trace Channel
- Placement Trace Distance

### Project Settings > Plugins > Metric - User

- Include Unreal Preview Meshes
- Hull Color
- Hull Line Thickness
- Forward Color
- Anchor Lift
- Anchor Radius
- Anchor Line Thickness
- Label World Size
- Label Height Padding
- Trajectory Color
- Trajectory Landing Color
- Trajectory Blocked Color
- Camera Probe Color
- Camera Compressed Color
- Camera Sightline Color
- Show Legend and legend display controls

`Include Unreal Preview Meshes` defaults to **On**.

## Native Preview Mesh Compatibility

Metric reads Unreal Engine 5.8's existing `ULevelEditorViewportSettings::PreviewMeshes` array directly. Those entries are appended to Metric's cycle list as visual-only Static Mesh references.

Metric does not migrate, modify, delete, or reorder the engine array.

If **Include Unreal Preview Meshes** is disabled, Shift + Backslash cycles Metric entries only.

## Scope of this prototype

0.10.0 is still an early prototype. It does **not** yet include:

- Shader-filled swept volumes
- Pins / benchmark actors
- Skeletal representations
- Shader-backed translucent hull fills
- Toolbar controls

Those systems should be layered onto this foundation only after the core viewport interaction is proven stable and comfortable.

## Compatibility

- Unreal Engine 5.8.x
- Designed against the UE 5.8.3 editor API
- Windows 64-bit target
- Editor Only
- No engine source modifications

## Installation

Copy the `Metric` folder into your project's `Plugins` folder, regenerate project files if necessary, then build the Editor target.

Example:

```text
YourProject/
  Plugins/
    Metric/
      Metric.uplugin
      Source/
```

## First-test priority

The most important question for this build is not feature count. It is whether Metric feels invisible until summoned and immediately useful when visible.

## Starter Preset Assets

On the first editor launch after installing Metric, Metric creates two editable `UMetricPreset` Data Assets in the project when **Create Starter Preset Assets If Missing** is enabled:

- `/Game/Metric/Presets/DA_Metric_Human` - Capsule, 180 cm high, 88 cm wide.
- `/Game/Metric/Presets/DA_Metric_Heavy` - Oriented Box, 300 cm high, 180 cm wide, 140 cm long.

The assets are automatically added to **Project Settings > Plugins > Metric > Registered Presets** and immediately participate in `Shift + \` cycling. They are ordinary project Data Assets after creation, so you can open and edit them normally.

Metric only performs this bootstrap once. If you later remove or delete either starter preset intentionally, Metric will not force it back into your project. The code-only Human Reference remains a fallback when no Metric presets are registered.

The package also includes `Doc/Starter-Preset-Values.md`, which lists every current Human and Heavy Data Asset value. Existing starter assets are not silently overwritten when Metric adds new fields.


## 0.2.0 Startup Bootstrap Change

Starter preset creation is now deferred briefly after subsystem initialization instead of saving assets immediately during editor startup. This is intended to avoid the UE 5.8 Data Validation warning about `UpdateValidators` being requested before Blueprint validators finish registering.


## 0.3.0 Viewport Readability Pass

- The white anchor ring and forward arrow now sit above the sampled surface instead of being nearly coplanar with it.
- The anchor ring is oriented to the hit surface normal, which keeps it readable on slopes and uneven geometry.
- **Anchor Lift**, **Anchor Radius**, and **Anchor Line Thickness** are exposed in Metric user settings.
- Metric's world label now uses a transient unlit copy of Unreal's stock TextRender material. The copy preserves the stock font mask but routes its color through Emissive so viewport lighting no longer dims or tints the readout. Nothing is saved into the project.
- **Label World Size** and **Label Height Padding** are exposed in Metric user settings.
- Clearance Probe now draws quiet side rails and an end cap at the hull width, making the requested passage envelope easier to read without adding a solid debug volume yet.


## 0.4.0 Sensors and Engagement Ranges

Metric presets now include two additional viewport-only measurement groups.

### Sensor Heights

- **Show Sensor Heights**
- **Primary Sensor Z Offset**
- **Low Profile Sensor Z Offset**
- **Marker Radius**

The primary sensor is drawn as a continuous blue ring/cross at the configured height. The low-profile sensor uses a segmented ring so the two stances remain distinguishable even without relying only on color. These are informational reference heights and do not perform visibility or AI perception queries.

### Engagement Ranges

- **Show Engagement Ranges**
- **Short Range**
- **Medium Range**
- **Long Range**

When enabled, Metric draws three horizontal distance rings around the anchor. Short is continuous and strongest, Medium is segmented, and Long is the quietest/sparsest. The rings are pure spatial references, not navmesh travel-time or line-of-sight claims.

Engagement Ranges default to **Off** to avoid adding large rings to every existing preset automatically. Sensor Heights default to **On** because they remain local to the rig and are visually quiet.

User settings now also expose **Sensor Color**, **Engagement Range Color**, and **Secondary Line Thickness**. Native Unreal Preview Mesh entries remain visual-only and do not render either Metric-specific layer.


## 0.4.1 Editor World Lifecycle Fix

- Fixed a fatal editor crash when loading or switching maps while Metric had created its transient render host.
- `UMetricSubsystem` no longer owns the world-scoped render host through a strong `UPROPERTY` reference. The host is now tracked weakly and remains owned by its editor World/Level.
- Metric releases its visualization before `LoadMap` begins and also clears the handle when a World starts tearing down.
- The render host is recreated lazily in the next valid editor World, so Metric can remain enabled across a map switch without retaining the old World.
- Subsystem shutdown removes all lifecycle delegates and destroys any surviving host safely.


## 0.5.0 Contextual Legend and Locomotion Time Bands

### Contextual Legend

Metric now displays an optional compact legend above the main readout while the Lens is active. The legend is generated from the current cycle entry instead of showing every possible Metric feature.

For a full Metric preset it can identify active layers such as:

- **HULL**
- **CLEARANCE**
- **SENSORS**
- **ENGAGEMENT**
- **MOVE**

It also summarizes the visual language currently in use, such as cyan hull geometry, green/amber/red clearance states, blue informational layers, and purple locomotion bands. When Locomotion Bands are active, the legend includes the configured time bands and movement speed.

Inherited Unreal Preview Mesh entries remain explicitly visual-only and the legend reports **VISUAL SCALE REFERENCE ONLY** instead of implying Metric gameplay analysis.

User settings under **Metric - User > Legend** include:

- **Show Legend**
- **Legend Text Color**
- **Legend World Size**
- **Legend Height Offset**

The legend uses the same transient unlit text material as Metric's primary label, so scene lighting should not dim or tint it.

### Locomotion Time Bands

Metric presets now include a **Locomotion Bands** group:

- **Show Locomotion Bands**
- **Speed**
- **Near Time**
- **Mid Time**
- **Far Time**

The radius of each band is simply `Speed x Time`. For example, a 600u/s preset with 1s, 3s, and 5s bands renders distance references at 600u, 1800u, and 3000u.

These rings are intentionally described as **distance/time references**, not pathfinding or guaranteed gameplay travel times. They do not account for acceleration, turns, obstacles, navigation, slopes, or project-specific movement code.

Locomotion uses a purple visual family and distinct segmented line patterns so it remains separate from blue Engagement Ranges. **Locomotion Band Color** is exposed in Metric user settings.


## 0.7.0 Camera Probe and Starter Preset Baseline

- Added the preset-driven **Camera Probe** layer using a sphere sweep from the configured pivot to the requested camera position.
- Added clear, compressed, and below-minimum result states with direct distance/compression readouts.
- Added magenta camera visualization and a separate compressed-state color in Metric user settings.
- The contextual legend now includes **CAMERA** only while the Camera Probe is active.
- Fresh Human and Heavy starter assets now receive explicit values for every current Metric group, including Camera Probe.
- Added `Doc/Starter-Preset-Values.md` as the canonical manual update sheet for existing Human and Heavy starter Data Assets.
- Existing Data Assets are never auto-overwritten by version updates.
