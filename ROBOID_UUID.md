# ROBOID + UUID (Robot Identity)

This note defines a small, reusable identity layer for robots/machines.

Goal:
- Stop using ad-hoc strings/maps for identity.
- Use a real `UUID` type everywhere.
- Keep `datapod::robot::Model` as the physical/kinematic model.
- Put identity and high-level semantics (role, steering type, works_on, rci) into a typed POD.

## 1) UUID

### 1.1 Why

We need a project-wide UUID type that:
- is cheap to store/copy
- serializes reliably
- converts to/from the common string form

### 1.2 Suggested POD

Implement `datapod::UUID` (in datapod core types):

```cpp
namespace datapod {

struct UUID {
    Array<u8, 16> bytes;

    auto members() noexcept { return std::tie(bytes); }
    auto members() const noexcept { return std::tie(bytes); }
};

} // namespace datapod
```

### 1.3 String format

Use standard UUID v4 textual format:

`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`

Where `x` is lowercase hex.

You can optionally enforce v4 version bits when generating.

Parsing:
- accept upper/lowercase
- ignore surrounding whitespace
- reject invalid length/characters

## 2) Robot Identity

This is the typed identity and high-level semantics that previously lived in JSON or in various structs.

### 2.1 Enums

```cpp
namespace datapod::robot {

enum class Role : u8 {
    Master = 0,
    Follower = 1,
    Slave = 2,
};

enum class SteeringType : u8 {
    Differential = 0,
    Ackermann = 1,
    Holonomic = 2,
    SkidSteer = 3,
};

} // namespace datapod::robot
```

### 2.2 Identity POD

```cpp
namespace datapod::robot {

struct Identity {
    String name;              // human-friendly display name
    UUID uuid;                // stable identifier
    u8 rci = 0;               // robot capability index (project-specific)
    Vector<String> works_on;  // semantic tags (e.g. "food", "cargo")
    Role role = Role::Master;
    SteeringType steering_type = SteeringType::Differential;

    auto members() noexcept {
        return std::tie(name, uuid, rci, works_on, role, steering_type);
    }
    auto members() const noexcept {
        return std::tie(name, uuid, rci, works_on, role, steering_type);
    }
};

} // namespace datapod::robot
```

Notes:
- Use `Vector<String>` instead of a set unless you have a POD set type.
- De-duplication can happen at load time.

## 3) How this relates to `datapod::robot::Model`

`datapod::robot::Model` is the URDF-like kinematic model:
- links, joints, tree structure
- geometry, inertial, limits, dynamics
- plus `props` for arbitrary extensions

Identity is NOT part of the kinematic model.

Recommended wrapper:

```cpp
namespace datapod::robot {

struct Robot {
    Identity id;
    Model model;
};

}
```

## 4) URDF mapping (props-based)

URDF is the single source file. Identity is stored in URDF extensions and parsed into `Identity`.

Example URDF:

```xml
<robot name="tractor">
  <flatsim>
    <info name="John Deere 8R" uuid="" rci="3" role="MASTER" steering="ACKERMANN"/>
    <works_on>
      <item>food</item>
    </works_on>
  </flatsim>
  ...
</robot>
```

With the props flattener, this becomes:
- `model.props["flatsim.info.name"] = "John Deere 8R"`
- `model.props["flatsim.info.uuid"] = "..."`
- `model.props["flatsim.info.rci"] = "3"`
- `model.props["flatsim.info.role"] = "MASTER"`
- `model.props["flatsim.info.steering"] = "ACKERMANN"`
- `model.props["flatsim.works_on.item"] = "food"`

Parsing into `Identity`:
- parse UUID string -> `UUID`
- parse `role` string -> `Role`
- parse `steering` string -> `SteeringType`
- parse `rci` as integer
- collect `works_on` values (you may want multiple items; if so, use indexed tags like
  `<item value="food"/>`, `<item value="cargo"/>` and flatten attributes).

## 5) Practical notes

- Prefer stable UUID stored in URDF for machines that must persist across sessions.
- If UUID is empty, generate one at load time (simulator) and optionally write it back.
- Keep `robot name` (URDF attribute) as a file/model name; keep `Identity.name` as display name.
