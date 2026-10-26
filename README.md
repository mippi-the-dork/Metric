# Metric 0.23.0 Prototype

Metric is a viewport-first spatial validation tool for Unreal Engine 5.8 and the Vista Viewport Suite. It gives level designers configurable, approximation-oriented measurements for scale, traversal, clearance, movement, camera behavior, combat space, and persistent benchmark comparison directly in the Level Editor viewport.










## 0.23.0 Level-Designer Feature Expansion + View Presets

- Adds **Cover / Exposure Analysis** for low, torso, and head protection against a configurable threat direction.
- Adds **Jump Takeoff / Landing Runway** so a valid trajectory can still flag inadequate approach or recovery ground.
- Adds **Interaction Reach** for approximate use/hand reach distance and height.
- Adds **Shoulder Camera Clearance** for independent left/right third-person shoulder viability.
- Adds **Combat Spacing Density** for approximate same-size combatant capacity inside a configurable encounter radius.
- Adds **Alternate Stance Fit** so one Metric Profile can compare standing clearance with an alternate/crouched height.
- Adds **Mantle / Vault Envelope** for generic obstacle height/depth suitability.
- Adds **Recovery / Dodge Space** for forward, backward, left, and right short-movement clearance.
- Adds **Crossfire Exposure** for multi-direction line-of-sight exposure sampling around a position.
- Adds **Metric View Preset**, a shareable Data Asset that stores only Visual/Results visibility masks and never changes Metric Profile measurement values.
- Adds built-in view presets: **Full Audit, Traversal, Camera, Combat, Space & Nav, Interaction, Minimal**.
- Project-authored Metric View Preset assets are discovered automatically from Content and appear in the HUD Display Options menu for both Live Metric and selected benchmarks.
- All nine analysis systems participate in live analysis, selected benchmark re-evaluation, persistent benchmark data, HUD results, Legend presentation, Visual/Results filtering, and status-driven world rendering.
- All new analysis layers default **Off** in Human and Heavy starter profiles to keep the default viewport readable and avoid unnecessary tracing.
- Jump Runway validates support across the full requested takeoff and landing distances rather than checking endpoints only, preventing narrow gaps from producing false passes.
- Mantle / Vault depth is sampled from the detected top surface with vertical probes, avoiding self-hit depth readings from a trace that begins on the obstacle.
- Recovery / Dodge Space now sweeps the active Metric hull shape: capsule, oriented box, sphere, or a documented capsule approximation for cylinder profiles.
- Adds a dedicated **Metric Profile Editor** for existing `UMetricPreset` assets. Opening a Metric Profile now presents task-grouped navigation, a focused settings panel, profile enable state, feature intent, and fidelity classification instead of one long generic Data Asset Details view.
- Live Display Visibility now separates **Profile** enablement from **Visual** and **Results** visibility. Profile-disabled rows are subdued and their display controls are disabled rather than appearing active.
- Adds **Edit Active Profile** directly to Live Display Visibility so a designer can jump from the viewport HUD to the active project Metric Profile.
- Renames the current live-analysis control in the HUD to **Live Analysis** and reports **Live Analysis Disabled** when cursor-driven analysis is paused. The serialized user-setting field remains compatible with previous 0.23 builds.


## Metric Profile Editor

Double-click a Metric Profile asset to open the dedicated Metric Profile Editor. Existing Human, Heavy, and custom `UMetricPreset` assets require no migration. Features are grouped by design task, and only the selected feature's settings are shown at once. See `Doc/Metric-Profile-Editor.md`.

The Profile Editor controls whether an analysis runs for that Metric Profile. Display Options and Metric View Presets remain presentation-only controls for enabled analyses.

## Metric View Presets

Use **Display Options** in the viewport HUD to apply one of Metric's built-in focus views or any project-authored **Metric View Preset** Data Asset. View Presets control only the per-layer **Visual** and **Results** masks. They never modify the active Metric Profile's measurement values.

To share a custom view with a team, create a Data Asset using the `MetricViewPreset` class, configure its visibility mask, and commit the asset to source control. Metric discovers project-authored View Presets automatically. See `Doc/View-Presets.md`.

## 0.22.0 Stopping Distance

- Adds **Stopping Distance**, a profile-driven approach/runway check for level-design pacing and maneuver space.
- Computes approximate required stopping runway as reaction distance plus braking distance using configured speed, deceleration, and reaction time.
- Sweeps the active hull through the required runway and reports PASS, TIGHT, or FAIL against current geometry.
- Shows reaction and required-stop markers in-world, plus an impact marker when the runway is obstructed.
- Fully participates in Live/Benchmark Visual and Results masks, benchmark persistence and re-evaluation, the HUD, legend, and status-driven line thickness.
- Fresh Human recommendation: 600 cm/s, 1800 cm/s^2 deceleration, 0.15 s reaction, 75 cm warning margin, disabled by default.
- Fresh Heavy recommendation: 350 cm/s, 900 cm/s^2 deceleration, 0.20 s reaction, 100 cm warning margin, disabled by default.
- Existing Human/Heavy Metric Profiles are not overwritten automatically.

## 0.21.2 Live Analysis Pause + Result Parity

- **Live Analysis Off** now truly pauses the cursor-driven analyzer: no cursor ray, placement trace, live sweeps, or live sampling are performed.
- A selected pinned benchmark remains a live stationary analyzer and continues to re-evaluate against current level geometry while Live Metric is Off.
- The LIVE METRIC HUD section shows **Live Analysis Disabled** while the cursor analyzer is paused, and Live vs Pin reports that Live Metric is disabled.
- Fixes the minimized Metric icon hover state by keeping the icon above the button hover layer.
- Trajectory HUD results now include apex height and headroom state so the text reflects the apex-clearance probe shown in-world.
- No Metric Profile / Human / Heavy data changes in this release.

## 0.21.0 Live Benchmark Results + Ledge Analysis

- Selected benchmark HUD results now re-evaluate against the current level geometry instead of remaining frozen at pin time.
- Persistent benchmark snapshots still preserve their location, profile settings, visibility mask, and historical captured-result fallback.
- Benchmark Visual and Results masks are independent during re-evaluation.
- The minimized/full HUD now uses a rasterized copy of the canonical Metric SVG for reliable Slate rendering.
- Adds preset-driven Ledge Analysis for edge distance, drop depth, and safe/warning/fail drop classification.

## 0.20.0 Stable Live Results + Overhead Clearance

- Eliminates the remaining live-results HUD blink by keeping one permanent Slate row per Metric layer and updating text, color, and visibility through bound attributes instead of replacing result widgets while the lens moves.
- Adds **Overhead Clearance**, a fast vertical headroom check at the current Metric position. It compares available floor-to-ceiling height against the active hull height and reports PASS, TIGHT, or FAIL with the actual deficit.
- Overhead Clearance participates in Live and Benchmark Visual/Results filtering, benchmark snapshots and persistence, the contextual Legend, status-driven line weight, and selected benchmark visualization.
- Fresh Human defaults: enabled, 320 cm max probe height, 30 cm warning margin.
- Fresh Heavy defaults: enabled, 450 cm max probe height, 40 cm warning margin.
- Existing Human/Heavy Metric Profiles are never overwritten automatically.

## 0.19.2 HUD / Legend Stability Hotfix

- Stops rebuilding the full viewport HUD and legend every time live Metric values change.
- Keeps the Slate panel structure stable while atomically refreshing only the live/pinned result content.
- Prevents transient overlapping text/layout frames while moving the live Metric lens.
- Preserves the 0.19.1 compile hotfix and all 0.19.0 functionality.

## 0.19.1 Compile Hotfix

- Fixed UE 5.8 checked-format-string compilation in the Turn Clearance HUD result text.
- No behavior or Metric Profile value changes.


## 0.19.0 Turn Clearance and Live Visualizer Fix

- Fixes **Show Live Visualizer = Off** so every live world-space element is suppressed, including the anchor, forward marker, analysis geometry, and native preview mesh, while benchmarks and live HUD results remain available.
- Gives the active Metric Profile its own full-width secondary header row so names like **Human Reference** and **Heavy Reference** are no longer clipped by header actions.
- Enlarges the Metric HUD icon and uses the canonical SVG directly for both the full HUD and minimized restore button.
- Adds **Turn Clearance**, a preset-driven swept maneuver test for left/right turns. It sweeps the active hull along a configurable arc and reports PASS or the first blocked angle/distance.
- Turn Clearance participates in Live and Benchmark Visual/Results filtering, pinned benchmark snapshots, the contextual legend, status-driven line thickness, and persistent benchmark storage.
- Fresh Human defaults: 250 cm radius, 90 deg turn, 7.5 deg samples, Both directions, disabled by default.
- Fresh Heavy defaults: 400 cm radius, 90 deg turn, 10 deg samples, Both directions, disabled by default.


## 0.18.0 Native HUD Results and Live Visualizer Control

- Replaces fallback section glyphs with Unreal chevron disclosure icons.
- Uses the Metric artwork reliably in the HUD through a packaged PNG render of the canonical SVG, while keeping the SVG for Metric Profile assets.
- Presents Live and Pinned Metric results as compact two-column inspector rows with subdued labels, right-aligned values, and status colors.
- Replaces the header Edit text with an icon-driven Display Options menu.
- Adds **Show Live Visualizer** to Metric user settings and an **On / Off** control inside LIVE METRIC. Turning it off hides only the live world-space visualization; benchmark visuals and live result rows remain available.
- Keeps the contextual Legend independent from HUD minimization and live-visualizer visibility.


## 0.17.2 Native HUD Polish and Selected Viewports Fix

- Leaves **Follow Last Interacted Viewport** unchanged from 0.17.0.
- **Selected Viewports** is now evaluated directly by each viewport overlay using its visible pane number, independent of active/focus history.
- Filters hidden/zero-size viewport widgets before assigning Viewport 1-4, keeping numbering tied to visible pane order.
- Reworks the HUD into compact Unreal-style collapsible sections for Live Metric, Benchmarks, Pinned Results, and Live vs Pin.
- Removes large fixed-height result regions so the panel only occupies the space its current content needs.
- Replaces benchmark Focus / Replace / Rename / Delete text actions with compact native-style icon buttons and tooltips.
- Tightens the header into a Metric icon + METRIC + active profile treatment.
- Adds Edit quick presets: **All**, **Visuals**, **Results**, **None**, and **Reset**, while preserving the per-layer Visual / Results matrix.
- No `UMetricPreset` fields or Human / Heavy canonical values changed.

## 0.16.10 Deterministic HUD Targeting Hotfix

- Keeps one lightweight Slate overlay alive per Level Editor viewport so hidden HUDs can still tick and become visible deterministically.
- HUD and Legend child panels now own visibility instead of collapsing the entire overlay widget.
- Follow Last Interacted records the actual viewport under mouse-down/key interaction rather than polling hover or inheriting a stale startup ActiveViewport.
- Selected Viewports visibility is evaluated independently for every live pane and no longer depends on prior interaction.
- No Metric Profile / Human / Heavy data asset values changed.

## 0.16.9 HUD Display Control and Targeting

- **Follow Last Interacted Viewport** now reacts immediately to the viewport currently hovered or focused, with Unreal's active viewport used only as a first-frame fallback. No detour through another viewport should be required.
- **Selected Viewports** now keeps a lightweight overlay attached to every live Level Editor viewport and toggles visibility per Viewport 1-4 selection, avoiding fragile add/remove behavior during layout changes.
- The Legend now uses live color swatches sourced directly from Metric's configured visualization colors.
- Live Metric and each pinned benchmark now have independent **Edit** menus with a two-column **Visual / Results** matrix for Hull, Clearance, Ground, Fit, Threshold, Sensors, Sensor Field, Engagement Ranges, Locomotion, Trajectory, Camera, and Sightline.
- Live display choices are user settings. Each benchmark stores its own display mask and persists it with saved benchmark data.
- Selected benchmark world visualization respects that benchmark's Visual toggles, while pinned HUD results respect its Results toggles.
- The HUD was tightened toward native Unreal editor conventions with flatter grouping, compact headers, separators, stable-width result regions, and reduced explanatory text.


## 0.16.8 HUD / Benchmark / Legend UX

- Viewport 1-4 targeting is now assigned by visible viewport position, row-major from top-left, instead of Unreal's internal viewport array order.
- Benchmark selection is collapsed into a scrollable dropdown suitable for larger pin sets.
- Selected benchmarks now re-evaluate and render their captured Metric configuration at the pinned location, while the yellow benchmark hull remains the persistent identity marker.
- The Metric legend is now a screen-space Slate panel with its own anchor setting and a Show/Hide button on the HUD.
- Legend placement supports the same eight anchor positions as the HUD.

## 0.16.6 Viewport HUD Targeting Hotfix

- The Metric viewport HUD now follows the Level Editor viewport that was interacted with most recently.
- Multi-viewport layouts no longer use `GetFirstActiveViewport()` for HUD placement.
- Metric resolves the active `FLevelEditorViewportClient`, finds the matching `SLevelViewport`, removes the HUD from the previous viewport, and reattaches it to the newly active viewport.
- The analysis lens itself remains perspective-only, while the HUD can follow the user's most recently interacted Level Editor viewport.


## 0.16.1 Content Browser Polish

- `UMetricPreset` assets are presented as **Metric Profile** in the Content Browser instead of `Data Asset (Metric Preset)`.
- Metric Profile assets use the type color `#FF940A`.
- Metric registers `Resources/MetricProfile.svg` as the Metric Profile class icon and thumbnail when the SVG is present.
- Restored the correct `MetricModule.cpp` source after the 0.16.0 package accidentally contained README text in that file.

## 0.15.2 Compile Hotfix

- **Ctrl + Backslash** pins the current live Metric benchmark.
- **Ctrl + Shift + Backslash** clears pinned benchmarks for the current map/session.
- The two documented chords are now recognized directly by the input preprocessor instead of depending only on Slate command-chord registration.
- Chord precedence is explicit: clear pins, pin, cycle, then toggle. This prevents the more specific Ctrl-modified actions from falling through to Shift + Backslash or plain Backslash behavior.
- Rebound commands in **Editor Preferences > Keyboard Shortcuts > Metric** remain supported.

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




## Threshold Analysis

Metric presets now expose a compact **Threshold Analysis** layer for short entry/exit transitions. Unlike Fit Analysis, which samples a longer corridor, Threshold Analysis checks only the entry, midpoint, and far side of a configurable threshold depth.

Each preset exposes:

- **Show Threshold Analysis**
- **Probe Depth**
- **Max Side Probe Distance**
- **Max Ceiling Probe Height**
- **Warning Margin**
- **Max Step Up**
- **Max Step Down**
- **Floor Trace Height**
- **Floor Trace Depth**
- **Trace Channel**

At each of the three stations Metric measures lateral room and floor-to-ceiling height, while consecutive floor samples are compared against the configured step-up and step-down limits. Results are reported as `THRESHOLD OK`, `THRESHOLD TIGHT`, `THRESHOLD FAIL`, or `THRESHOLD NO FLOOR`. Failure readouts include width/height deficit and the worst observed step transition.

This is intended for doors, hatches, tunnel mouths, curb transitions, small portals, and similar short spatial transitions. It is a geometric approximation and does not replace navigation, animation, mantle, or project-specific traversal logic.

## Trajectory Envelope / Apex Clearance

Metric's existing Trajectory layer can now visualize the approximate **occupied hull envelope** around the sampled ballistic path instead of showing only the center arc. The envelope uses the active preset hull width and height, drawing quiet side/top/bottom rails plus periodic cross-sections along the trajectory.

Trajectory presets now expose:

- **Show Trajectory Envelope**
- **Envelope Sample Stride**
- **Apex Probe Distance**
- **Apex Warning Margin**

Metric also identifies the highest sampled hull-center point and measures vertical clearance above the top of the hull from that point. The readout adds an apex line such as `APEX +148u | HEADROOM 72u`, `APEX +148u | HEADROOM 32u TIGHT`, or `APEX +148u | HEADROOM 200u+`.

The apex probe is a local overhead measurement, not a second movement solver. The trajectory sweep remains the authoritative pass/fail path query. A jump may therefore be collision-free while still reporting tight apex headroom, which is useful for spotting low beams, ceilings, overhangs, and visually uncomfortable jump spaces before they actually block the hull.

Cylinder hulls retain Metric's existing capsule approximation for trajectory collision. Envelope rails are a visualization of the configured hull dimensions and do not claim exact continuous swept-volume rendering.

## Sensor Field / Field-of-View

Metric presets now expose an optional **Sensor Field** layer that expands the existing Sensor Heights reference into a forward-facing field-of-view volume. It uses the preset's Primary Sensor Height as its origin, draws the configured horizontal and vertical field angles, and samples horizontal visibility through the field using world traces.

Each preset exposes:

- **Show Sensor Field**
- **Range**
- **Horizontal Half Angle**
- **Vertical Half Angle**
- **Horizontal Ray Count**
- **Trace Channel**

The viewport draws the nominal field boundary in blue, individual sampled rays through the horizontal field, and red truncation markers where geometry blocks a sample. The readout reports either `SENSOR OPEN` or a compact coverage summary such as `SENSOR 67% CLEAR | 3/9 blocked | nearest 420u`.

The vertical angle is currently a visualization reference only. Obstruction percentage is derived from the horizontal ray fan at Primary Sensor Height, so Metric does not claim to reproduce a project's AI Perception component, sight affiliation rules, peripheral vision implementation, target filtering, vertical sampling, or gameplay-specific visibility logic.

## Pinned Benchmarks

- **Ctrl + Backslash** pins the current Metric hull at the live lens position.
- **Ctrl + Shift + Backslash** clears all pins in the current editor world.
- Pins are subsystem-owned visualization data, not Actors, so they never appear in the Scene Outliner and never become production level content.
- Pins can now persist per saved map when **Metric - User > Pinned Benchmarks > Persist Pinned Benchmarks Per Map** is enabled (default).
- Persistent pins are stored in editor user settings, never as level Actors or level-owned objects. Unsaved `/Temp/` maps remain session-only.
- Returning to a saved map automatically restores its pins. Clearing pins also clears the stored pin set for that map.
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


## Viewport HUD placement

Metric's screen-space HUD can be anchored to Top Left, Middle Left, Bottom Left, Top Middle, Bottom Middle, Top Right, Middle Right, or Bottom Right from **Editor Preferences > Plugins > Metric - User > Viewport HUD**.

HUD viewport targeting defaults to **Follow Last Interacted Viewport**. In this mode Metric waits for a real Level Editor viewport interaction/focus before attaching the HUD, then follows the last viewport you use. **Selected Viewports** mode can instead show the HUD in any combination of Viewport 1 through Viewport 4.
