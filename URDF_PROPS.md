# URDF Props Extensions (Model/Link/Joint)

This note describes a simplified extension mechanism for URDF parsing where **all non-core URDF data is stored as
key/value properties** (`props`) directly on:

- `datapod::robot::Model`
- `datapod::robot::Link`
- `datapod::robot::Joint`

The intent is to fully replace legacy machine JSON files with a single URDF file that contains both:

- Standard URDF physics/geometry.
- Simulator- or domain-specific metadata stored as `props`.

This design intentionally does **not** keep raw XML extension blocks. It is best for extensions expressed as simple tags
with attributes (like `<flatsim side="left" throttle_max="0.2"/>`).

If you later need nested Gazebo plugin structures, you will want a separate raw XML capture mechanism. For now, props is
the simplest thing that works.

## 1) What URDF is (quick recap)

Core URDF structure:
- `<robot>` contains `<link>` and `<joint>` elements.
- Geometry lives under `<link>`:
  - `<visual><geometry>...</geometry></visual>`
  - `<collision><geometry>...</geometry></collision>`
- Mass/inertia lives under `<link><inertial>...</inertial>`.
- Limits/dynamics live under `<joint>`:
  - `<limit .../>`, `<dynamics .../>`

Everything else in the XML is "extra".

## 2) Goal: store extras as `props`

We want to allow URDF files to carry simulator metadata in a way that:
- does not change URDF core types every time we add a new field
- does not require multiple files (no sidecar JSON)
- keeps access simple in the simulator code

We do this by adding a map of string→string properties (`props`) to the URDF data model types.

## 3) Data model changes (datapod)

Add a property map to:

- `datapod::robot::Model`
- `datapod::robot::Link`
- `datapod::robot::Joint`

Suggested field name:

```cpp
Map<String, String> props;
```

Notes:
- Use whatever map POD container datapod supports (e.g. `datapod::Map`, or `Vector<Pair<String,String>>` if a map POD is
  not available).
- Keep it in `members()` so it serializes.

## 4) Parsing rules (robomod)

During URDF parsing:

### 4.1 Robot-level props

At `<robot>` level, parse custom tags and write into `model.props`.

Example URDF:

```xml
<robot name="tractor">
  <flatsim turning_radius="2.4"/>
  ...
</robot>
```

Props:
- `model.props["flatsim.turning_radius"] = "2.4"`

If you prefer nested tags:

```xml
<flatsim>
  <turning radius="2.4"/>
</flatsim>
```

Flatten by concatenating tag names:
- `model.props["flatsim.turning.radius"] = "2.4"`

### 4.2 Link-level props

Inside each `<link name="...">`, any non-core child element becomes `Link.props`.

Example:

```xml
<link name="rear_left_wheel_link">
  <flatsim side="left"/>
  ...
</link>
```

Props:
- `link.props["flatsim.side"] = "left"`

### 4.3 Joint-level props

Inside each `<joint name="...">`, any non-core child element becomes `Joint.props`.

Example:

```xml
<joint name="rear_left_wheel_joint" type="continuous">
  ...
  <flatsim side="left" throttle_max="0.2" throttle_diff="-0.8"/>
</joint>
```

Props:
- `joint.props["flatsim.side"] = "left"`
- `joint.props["flatsim.throttle_max"] = "0.2"`
- `joint.props["flatsim.throttle_diff"] = "-0.8"`

### 4.4 What counts as "core" elements

When scanning children, skip core URDF tags:

Robot:
- `link`, `joint`, `material`

Link:
- `inertial`, `visual`, `collision`

Joint:
- `origin`, `parent`, `child`, `axis`, `limit`, `dynamics`, `mimic`, `safety_controller`, `calibration`

Everything else becomes props.

### 4.5 Key format

Use `{tag}.{attribute}`:
- `flatsim.side`
- `flatsim.throttle_max`
- `gazebo.mu1`

This keeps keys namespaced and avoids collisions.

Values remain strings; the simulator parses them into floats/bools/enums as needed.

## 5) Flatsim mapping examples

These are typical flatsim keys:

Robot props:
- `flatsim.color.rgba = "0 255 100 255"` (or split into 4 keys)
- `flatsim.turning_radius = "2.4"`

Wheel joint props:
- `flatsim.side = "left"|"right"`
- `flatsim.throttle_max = "0.2"`
- `flatsim.throttle_diff = "-0.8"` (optional)

Steering joint props:
- `flatsim.steering_diff = "4"` (optional)

Tank joint props:
- `flatsim.tank_name = "harvest_bin"`
- `flatsim.tank_type = "HARVEST"`
- `flatsim.tank_capacity = "10000"`

Power joint props:
- `flatsim.power_name = "fuel_tank"`
- `flatsim.power_type = "FUEL"|"BATTERY"`
- `flatsim.power_capacity = "150"`
- `flatsim.power_consumption_rate = "0.03"`
- `flatsim.power_charge_rate = "0"`

Hitch joint props:
- `flatsim.hitch_name = "rear_hitch"`
- `flatsim.hitch_is_master = "true"|"false"`

Karosserie fixed-joint props:
- `flatsim.karosserie_name = "front"`
- `flatsim.karosserie_sections = "5"`
- `flatsim.karosserie_has_physics = "true"|"false"`

## 6) Tradeoffs

Pros:
- Single URDF file; no JSON sidecar.
- Very easy to consume in simulator code.
- No need for `Urdf.ext` raw XML capture.

Cons:
- Loses structured XML blocks (e.g. nested `<gazebo><plugin>...</plugin></gazebo>`).
- All values are strings; the consumer must parse types.
