//! Panic-safety smoke tests for malformed registered wire input.
//!
//! These are intentionally dependency-free, deterministic "fuzz-ish" tests:
//! validators may accept or reject arbitrary bytes depending on the schema, but
//! they must always return normally instead of panicking.

use std::panic::{AssertUnwindSafe, catch_unwind};

use datapod::{
    Archived, DataPodAccess, DataPodDecode, DataPodValidate, LeWireHeader, WireFrame, WireMessage,
    access_wire, access_wire_bytes, access_wire_bytes_v1, access_wire_frame, from_archive,
    from_wire_frame, from_wire_message, from_wire_message_v1, split_wire_frame, split_wire_parts,
    validate_registered_wire, validate_registered_wire_frame, validate_wire, validate_wire_bytes,
    validate_wire_bytes_v1, view_archive,
};

fn bytes_for(type_hash: u64, len: usize, seed: u64) -> Vec<u8> {
    let mut state = seed ^ type_hash ^ ((len as u64) << 32);
    let mut bytes = Vec::with_capacity(len);
    for _ in 0..len {
        state = state
            .wrapping_mul(6364136223846793005)
            .wrapping_add(1442695040888963407);
        bytes.push((state >> 32) as u8);
    }
    bytes
}

#[test]
fn registered_message_validators_do_not_panic_on_malformed_bytes() {
    for info in datapod::registry::all_type_infos()
        .into_iter()
        .filter(|info| info.validator == datapod::registry::ValidatorKind::BuiltIn)
    {
        let mut lengths = vec![
            0,
            info.header_size.saturating_sub(1),
            info.header_size,
            info.header_size.saturating_add(1),
            info.header_size.saturating_add(17),
        ];
        lengths.sort_unstable();
        lengths.dedup();

        for (case, len) in lengths.into_iter().enumerate() {
            let bytes = bytes_for(info.type_hash, len, case as u64);
            let result = catch_unwind(AssertUnwindSafe(|| {
                let _ = validate_registered_wire(info.type_hash, &bytes);
            }));
            assert!(
                result.is_ok(),
                "{} registered message validator panicked for len {len}",
                info.canonical_name
            );
        }
    }
}

#[test]
fn registered_frame_validators_do_not_panic_on_malformed_slices() {
    for info in datapod::registry::all_type_infos()
        .into_iter()
        .filter(|info| info.validator == datapod::registry::ValidatorKind::BuiltIn)
    {
        let mut header_lengths = vec![
            0,
            info.header_size.saturating_sub(1),
            info.header_size,
            info.header_size.saturating_add(1),
        ];
        header_lengths.sort_unstable();
        header_lengths.dedup();

        for (header_case, header_len) in header_lengths.into_iter().enumerate() {
            for payload_len in [0_usize, 1, 7, 16] {
                let header = bytes_for(info.type_hash, header_len, header_case as u64);
                let payload = bytes_for(info.type_hash, payload_len, payload_len as u64);
                let frame = WireFrame {
                    type_hash: info.type_hash,
                    header: &header,
                    payload: &payload,
                };
                let result = catch_unwind(AssertUnwindSafe(|| {
                    let _ = validate_registered_wire_frame(frame);
                }));
                assert!(
                    result.is_ok(),
                    "{} registered frame validator panicked for header_len {header_len}, payload_len {payload_len}",
                    info.canonical_name
                );
            }
        }
    }
}

#[test]
fn registered_helpers_do_not_panic_on_unknown_type_hashes() {
    for type_hash in [0, 1, u64::MAX, 0xdead_beef_dead_beef] {
        for len in [0_usize, 1, 7, 16, 64] {
            let bytes = bytes_for(type_hash, len, len as u64);
            let header = bytes_for(type_hash, len.min(16), 17 + len as u64);
            let payload = bytes_for(type_hash, len, 31 + len as u64);
            let frame = WireFrame {
                type_hash,
                header: &header,
                payload: &payload,
            };

            for (label, result) in [
                (
                    "split_wire_parts",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = split_wire_parts(type_hash, &bytes);
                    })),
                ),
                (
                    "split_wire_frame",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = split_wire_frame(type_hash, &bytes);
                    })),
                ),
                (
                    "validate_registered_wire",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = validate_registered_wire(type_hash, &bytes);
                    })),
                ),
                (
                    "validate_registered_wire_frame",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = validate_registered_wire_frame(frame);
                    })),
                ),
            ] {
                assert!(
                    result.is_ok(),
                    "registered {label} panicked for unknown type_hash {type_hash:#x}, len {len}"
                );
            }
        }
    }
}

fn assert_typed_message_paths_do_not_panic<T>()
where
    T: DataPodAccess + DataPodDecode + DataPodValidate,
    T::Header: LeWireHeader,
{
    let type_hash = datapod::bind::type_hash::<T>();
    let header_size = T::Header::LE_WIRE_SIZE;
    let mut header_lengths = vec![
        0,
        header_size.saturating_sub(1),
        header_size,
        header_size + 1,
    ];
    header_lengths.sort_unstable();
    header_lengths.dedup();

    for (header_case, header_len) in header_lengths.into_iter().enumerate() {
        for payload_len in [0_usize, 1, 3, 16, 31] {
            let mut bytes = bytes_for(type_hash, header_len, header_case as u64);
            bytes.extend(bytes_for(type_hash, payload_len, payload_len as u64 + 199));
            let msg = WireMessage { type_hash, bytes };

            for (label, result) in [
                (
                    "split_wire_parts",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = split_wire_parts(msg.type_hash, &msg.bytes);
                    })),
                ),
                (
                    "split_wire_frame",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = split_wire_frame(msg.type_hash, &msg.bytes);
                    })),
                ),
                (
                    "validate_wire",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = validate_wire::<T>(&msg);
                    })),
                ),
                (
                    "validate_wire_bytes",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = validate_wire_bytes::<T>(msg.type_hash, &msg.bytes);
                    })),
                ),
                (
                    "validate_wire_bytes_v1",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = validate_wire_bytes_v1::<T>(msg.type_hash, &msg.bytes);
                    })),
                ),
                (
                    "access_wire",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = access_wire::<T>(&msg);
                    })),
                ),
                (
                    "access_wire_bytes",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = access_wire_bytes::<T>(msg.type_hash, &msg.bytes);
                    })),
                ),
                (
                    "access_wire_bytes_v1",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = access_wire_bytes_v1::<T>(msg.type_hash, &msg.bytes);
                    })),
                ),
                (
                    "from_wire_message",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = from_wire_message::<T>(&msg);
                    })),
                ),
                (
                    "from_wire_message_v1",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = from_wire_message_v1::<T>(&msg);
                    })),
                ),
            ] {
                assert!(
                    result.is_ok(),
                    "{} typed message {label} panicked for header_len {header_len}, payload_len {payload_len}",
                    core::any::type_name::<T>()
                );
            }
        }
    }
}

fn assert_typed_archive_paths_do_not_panic<T>()
where
    T: DataPodAccess + DataPodDecode + DataPodValidate,
    T::Header: LeWireHeader,
{
    let type_hash = datapod::bind::type_hash::<T>();
    let header_size = T::Header::LE_WIRE_SIZE;
    let mut header_lengths = vec![
        0,
        header_size.saturating_sub(1),
        header_size,
        header_size + 1,
    ];
    header_lengths.sort_unstable();
    header_lengths.dedup();

    for (header_case, header_len) in header_lengths.into_iter().enumerate() {
        for payload_len in [0_usize, 1, 3, 16, 31] {
            let header = bytes_for(type_hash, header_len, header_case as u64);
            let payload = bytes_for(type_hash, payload_len, payload_len as u64 + 99);
            let frame = WireFrame {
                type_hash,
                header: &header,
                payload: &payload,
            };
            let archive = Archived::<T>::from_frame(frame);

            for (label, result) in [
                (
                    "access_wire_frame",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = access_wire_frame::<T>(frame);
                    })),
                ),
                (
                    "from_wire_frame",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = from_wire_frame::<T>(frame);
                    })),
                ),
                (
                    "view_archive",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = view_archive::<T>(archive);
                    })),
                ),
                (
                    "from_archive",
                    catch_unwind(AssertUnwindSafe(|| {
                        let _ = from_archive::<T>(archive);
                    })),
                ),
            ] {
                assert!(
                    result.is_ok(),
                    "{} typed {label} panicked for header_len {header_len}, payload_len {payload_len}",
                    core::any::type_name::<T>()
                );
            }
        }
    }
}

fn assert_typed_wrong_hash_paths_do_not_panic<T>()
where
    T: DataPodAccess + DataPodDecode + DataPodValidate,
    T::Header: LeWireHeader,
{
    let expected_hash = datapod::bind::type_hash::<T>();
    let header_size = T::Header::LE_WIRE_SIZE;
    let header = bytes_for(expected_hash, header_size, 401);
    let payload = bytes_for(expected_hash, 17, 409);
    let mut bytes = header.clone();
    bytes.extend_from_slice(&payload);

    for wrong_hash in [
        0_u64,
        1,
        expected_hash ^ 1,
        expected_hash.wrapping_add(0x9e37_79b9_7f4a_7c15),
        u64::MAX,
    ]
    .into_iter()
    .filter(|hash| *hash != expected_hash)
    {
        let msg = WireMessage {
            type_hash: wrong_hash,
            bytes: bytes.clone(),
        };
        let frame = WireFrame {
            type_hash: wrong_hash,
            header: &header,
            payload: &payload,
        };
        let archive = Archived::<T>::from_frame(frame);

        for (label, result) in [
            (
                "validate_wire",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = validate_wire::<T>(&msg);
                })),
            ),
            (
                "validate_wire_bytes",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = validate_wire_bytes::<T>(msg.type_hash, &msg.bytes);
                })),
            ),
            (
                "validate_wire_bytes_v1",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = validate_wire_bytes_v1::<T>(msg.type_hash, &msg.bytes);
                })),
            ),
            (
                "access_wire",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = access_wire::<T>(&msg);
                })),
            ),
            (
                "access_wire_bytes",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = access_wire_bytes::<T>(msg.type_hash, &msg.bytes);
                })),
            ),
            (
                "access_wire_bytes_v1",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = access_wire_bytes_v1::<T>(msg.type_hash, &msg.bytes);
                })),
            ),
            (
                "from_wire_message",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = from_wire_message::<T>(&msg);
                })),
            ),
            (
                "from_wire_message_v1",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = from_wire_message_v1::<T>(&msg);
                })),
            ),
            (
                "access_wire_frame",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = access_wire_frame::<T>(frame);
                })),
            ),
            (
                "from_wire_frame",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = from_wire_frame::<T>(frame);
                })),
            ),
            (
                "view_archive",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = view_archive::<T>(archive);
                })),
            ),
            (
                "from_archive",
                catch_unwind(AssertUnwindSafe(|| {
                    let _ = from_archive::<T>(archive);
                })),
            ),
        ] {
            assert!(
                result.is_ok(),
                "{} typed {label} panicked for wrong type_hash {wrong_hash:#x}",
                core::any::type_name::<T>()
            );
        }
    }
}

macro_rules! all_builtin_datapods {
    ($m:ident) => {
        $m!(datapod::Envelope);
        $m!(datapod::Encoding);
        $m!(datapod::Point);
        $m!(datapod::PointKey);
        $m!(datapod::Geo);
        $m!(datapod::Loc);
        $m!(datapod::Utm);
        $m!(datapod::Segment);
        $m!(datapod::Linestring);
        $m!(datapod::MultiPoint);
        $m!(datapod::Ring);
        $m!(datapod::Polygon);
        $m!(datapod::Path);
        $m!(datapod::Trajectory);
        $m!(datapod::Line);
        $m!(datapod::Size);
        $m!(datapod::Rectangle);
        $m!(datapod::Square);
        $m!(datapod::Aabb);
        $m!(datapod::Obb);
        $m!(datapod::Box);
        $m!(datapod::BoundingSphere);
        $m!(datapod::Circle);
        $m!(datapod::Triangle);
        $m!(datapod::GaussianPoint);
        $m!(datapod::GaussianCircle);
        $m!(datapod::GaussianRectangle);
        $m!(datapod::GaussianBox);
        $m!(datapod::Euler);
        $m!(datapod::Quaternion);
        $m!(datapod::Pose);
        $m!(datapod::Transform);
        $m!(datapod::Velocity);
        $m!(datapod::Acceleration);
        $m!(datapod::State);
        $m!(datapod::Grid);
        $m!(datapod::Layer);
        $m!(datapod::Bytes);
        $m!(datapod::DpStr);
        $m!(datapod::DpString);
        $m!(datapod::BitVec);
        $m!(datapod::Deque);
        $m!(datapod::Queue);
        $m!(datapod::Stack);
        $m!(datapod::List);
        $m!(datapod::ForwardList);
        $m!(datapod::Heap);
        $m!(datapod::IndexedHeap);
        $m!(datapod::Vector);
        $m!(datapod::Matrix);
        $m!(datapod::Tensor);
        $m!(datapod::Vecvec);
        $m!(datapod::PagedVecvec);
        $m!(datapod::Map);
        $m!(datapod::Set);
        $m!(datapod::Uuid);
        $m!(datapod::Ip);
        $m!(datapod::MacAddr);
        $m!(datapod::Twist);
        $m!(datapod::Wrench);
        $m!(datapod::Odom);
        $m!(datapod::Inertial);
        $m!(datapod::JointLimits);
        $m!(datapod::Accel);
        $m!(datapod::JointDynamics);
        $m!(datapod::JointMimic);
        $m!(datapod::JointSafetyController);
        $m!(datapod::JointCalibration);
        $m!(datapod::KV);
        $m!(datapod::BoxShape);
        $m!(datapod::SphereShape);
        $m!(datapod::CylinderShape);
        $m!(datapod::MeshShape);
        $m!(datapod::GeometryKind);
        $m!(datapod::Geometry);
        $m!(datapod::Identity);
        $m!(datapod::Material);
        $m!(datapod::Visual);
        $m!(datapod::Collision);
        $m!(datapod::JointType);
        $m!(datapod::Joint);
        $m!(datapod::Link);
        $m!(datapod::Sensor);
        $m!(datapod::Model);
        $m!(datapod::Robot);
        $m!(datapod::Actuator);
        $m!(datapod::TransmissionJoint);
        $m!(datapod::Transmission);
        $m!(datapod::WheelEncoder);
        $m!(datapod::WheelEncoders);
        $m!(datapod::Imu);
        $m!(datapod::TurnRadius);
        $m!(datapod::Gnss);
    };
}

macro_rules! assert_all_builtin_datapods {
    ($assertion:ident) => {{
        macro_rules! apply_assertion {
            ($ty:ty) => {
                $assertion::<$ty>();
            };
        }
        all_builtin_datapods!(apply_assertion);
    }};
}

fn covered_builtin_type_hashes() -> Vec<u64> {
    let mut hashes = Vec::new();
    macro_rules! push_hash {
        ($ty:ty) => {
            hashes.push(datapod::bind::type_hash::<$ty>());
        };
    }
    all_builtin_datapods!(push_hash);
    hashes
}

#[test]
fn malformed_wire_exhaustive_type_list_matches_builtin_registry() {
    let covered_hashes = covered_builtin_type_hashes();
    let covered: std::collections::BTreeSet<_> = covered_hashes.iter().copied().collect();
    assert_eq!(
        covered.len(),
        covered_hashes.len(),
        "malformed-wire exhaustive list must not contain duplicate datapods"
    );

    let registered: std::collections::BTreeSet<_> = datapod::registry::all_type_infos()
        .into_iter()
        .filter(|info| info.validator == datapod::registry::ValidatorKind::BuiltIn)
        .map(|info| info.type_hash)
        .collect();

    assert_eq!(
        covered, registered,
        "malformed-wire panic-safety list must stay in lockstep with registry::datapod_types!"
    );
}

#[test]
fn every_builtin_typed_message_path_does_not_panic_on_malformed_bytes() {
    assert_all_builtin_datapods!(assert_typed_message_paths_do_not_panic);
}

#[test]
fn every_builtin_typed_path_does_not_panic_on_wrong_type_hashes() {
    assert_all_builtin_datapods!(assert_typed_wrong_hash_paths_do_not_panic);
}

#[test]
fn every_builtin_archive_view_owned_path_does_not_panic_on_malformed_frames() {
    assert_all_builtin_datapods!(assert_typed_archive_paths_do_not_panic);
}
