use datapod::{
    Aabb, Accel, Acceleration, Actuator, Archived, BitVec, BoundingSphere, Box as DatapodBox,
    BoxShape, Bytes, Circle, Collision, CylinderShape, DataPod, DataPodAccess, DataPodDecode,
    DataPodValidate, Deque, DpStr, DpString, Encoding, Envelope, Euler, ForwardList, GaussianBox,
    GaussianCircle, GaussianPoint, GaussianRectangle, Geo, Geometry, GeometryKind, Gnss, Grid,
    Heap, Identity, Imu, IndexedHeap, Inertial, Ip, Joint, JointCalibration, JointDynamics,
    JointLimits,
    JointMimic, JointSafetyController, JointType, KV, Layer, LeWireHeader, Line, Linestring, Link,
    List, Loc, MacAddr, Map, Material, Matrix, MatrixHeader, MeshShape, Model, MultiPoint, Obb,
    Odom, PagedVecvec, Path, Point, PointKey, Polygon, Pose, Quaternion, Queue, Rectangle, Ring,
    Robot, Segment, Sensor, Set, Size, SphereShape, Square, Stack, State, Tensor, Trajectory,
    Transform, Transmission, TransmissionJoint, Triangle, TurnRadius, Twist, Utm, Uuid, Vector,
    Vecvec, Velocity, Visual, WheelEncoder, WheelEncoders, Wrench, archive, bytemuck, datapod,
    from_archive, segmented_archive, split_wire_frame, to_wire_message, view_archive,
};

fn assert_archive_view_owned<T>()
where
    T: DataPod + DataPodValidate + DataPodAccess + DataPodDecode,
    T::Header: LeWireHeader,
{
}

macro_rules! all_builtin_archive_datapods {
    ($m:ident) => {
        $m!(Envelope);
        $m!(Encoding);
        $m!(Point);
        $m!(PointKey);
        $m!(Geo);
        $m!(Loc);
        $m!(Utm);
        $m!(Segment);
        $m!(Linestring);
        $m!(MultiPoint);
        $m!(Ring);
        $m!(Polygon);
        $m!(Path);
        $m!(Trajectory);
        $m!(Line);
        $m!(Size);
        $m!(Rectangle);
        $m!(Square);
        $m!(Aabb);
        $m!(Obb);
        $m!(DatapodBox);
        $m!(BoundingSphere);
        $m!(Circle);
        $m!(Triangle);
        $m!(GaussianPoint);
        $m!(GaussianCircle);
        $m!(GaussianRectangle);
        $m!(GaussianBox);
        $m!(Euler);
        $m!(Quaternion);
        $m!(Pose);
        $m!(Transform);
        $m!(Velocity);
        $m!(Acceleration);
        $m!(State);
        $m!(Grid);
        $m!(Layer);
        $m!(Bytes);
        $m!(DpStr);
        $m!(DpString);
        $m!(BitVec);
        $m!(Deque);
        $m!(Queue);
        $m!(Stack);
        $m!(List);
        $m!(ForwardList);
        $m!(Heap);
        $m!(IndexedHeap);
        $m!(Vector);
        $m!(Matrix);
        $m!(Tensor);
        $m!(Vecvec);
        $m!(PagedVecvec);
        $m!(Map);
        $m!(Set);
        $m!(Uuid);
        $m!(Ip);
        $m!(MacAddr);
        $m!(Twist);
        $m!(Wrench);
        $m!(Odom);
        $m!(Inertial);
        $m!(JointLimits);
        $m!(Accel);
        $m!(JointDynamics);
        $m!(JointMimic);
        $m!(JointSafetyController);
        $m!(JointCalibration);
        $m!(KV);
        $m!(BoxShape);
        $m!(SphereShape);
        $m!(CylinderShape);
        $m!(MeshShape);
        $m!(GeometryKind);
        $m!(Geometry);
        $m!(Identity);
        $m!(Material);
        $m!(Visual);
        $m!(Collision);
        $m!(JointType);
        $m!(Joint);
        $m!(Link);
        $m!(Sensor);
        $m!(Model);
        $m!(Robot);
        $m!(Actuator);
        $m!(TransmissionJoint);
        $m!(Transmission);
        $m!(WheelEncoder);
        $m!(WheelEncoders);
        $m!(Imu);
        $m!(TurnRadius);
        $m!(Gnss);
    };
}

fn covered_archive_view_owned_type_hashes() -> Vec<u64> {
    let mut hashes = Vec::new();
    macro_rules! push_hash {
        ($ty:ty) => {
            hashes.push(datapod::bind::type_hash::<$ty>());
        };
    }
    all_builtin_archive_datapods!(push_hash);
    hashes
}

#[datapod]
pub struct ArchiveCameraFrame {
    pub width: u32,
    pub height: u32,
    pub encoding: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

#[datapod(name = "robolibs.archive_split_payload.v1")]
pub struct ArchiveSplitPayload {
    pub id: u32,
    #[dp(bytes, section = "left")]
    pub left: Vec<u8>,
    #[dp(bytes, section = "right")]
    pub right: Vec<u16>,
}

#[test]
fn representative_datapods_implement_archive_view_owned_traits() {
    assert_archive_view_owned::<Point>();
    assert_archive_view_owned::<Matrix>();
    assert_archive_view_owned::<ArchiveCameraFrame>();
}

#[test]
fn exported_datapods_implement_archive_view_owned_traits() {
    macro_rules! assert_type {
        ($ty:ty) => {
            assert_archive_view_owned::<$ty>();
        };
    }
    all_builtin_archive_datapods!(assert_type);
}

#[test]
fn archive_view_owned_trait_coverage_matches_builtin_registry() {
    let covered_hashes = covered_archive_view_owned_type_hashes();
    let covered: std::collections::BTreeSet<_> = covered_hashes.iter().copied().collect();
    assert_eq!(
        covered.len(),
        covered_hashes.len(),
        "archive/view/owned trait coverage must not contain duplicate datapods"
    );

    let registered: std::collections::BTreeSet<_> = datapod::registry::all_type_infos()
        .into_iter()
        .filter(|info| info.validator == datapod::registry::ValidatorKind::BuiltIn)
        .map(|info| info.type_hash)
        .collect();

    assert_eq!(
        covered, registered,
        "archive/view/owned compile-time coverage must stay in lockstep with registry::datapod_types!"
    );
}

#[test]
fn rust_archive_view_owned_round_trips_without_payload_copy_in_view() {
    let matrix = Matrix::from_bytes::<u16>(2, 3, vec![1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0]);
    let original_payload_ptr = matrix.data.as_ptr();
    let original_payload_len = matrix.data.len();
    let expected_hash = datapod::bind::type_hash::<Matrix>();

    let owned_from_method = matrix
        .archive(|archive| {
            assert_eq!(archive.type_hash(), expected_hash);
            assert_eq!(archive.header_bytes().len(), MatrixHeader::LE_WIRE_SIZE);
            assert_eq!(archive.payload_bytes().as_ptr(), original_payload_ptr);
            assert_eq!(archive.payload_bytes().len(), original_payload_len);

            archive.validate()?;
            let view = archive.view()?;
            assert_eq!(view.rows(), 2);
            assert_eq!(view.cols(), 3);
            assert_eq!(view.payload_bytes().as_ptr(), original_payload_ptr);

            let view_from_free_fn = view_archive::<Matrix>(archive)?;
            assert_eq!(
                view_from_free_fn.payload_bytes().as_ptr(),
                original_payload_ptr
            );

            let owned = archive.to_owned()?;
            assert_eq!(owned.data, matrix.data);
            assert_ne!(owned.data.as_ptr(), original_payload_ptr);

            let owned_from_free_fn = from_archive::<Matrix>(archive)?;
            assert_eq!(owned_from_free_fn.data, matrix.data);
            Ok::<_, datapod::WireError>(owned)
        })
        .expect("archive callback succeeds")
        .expect("archive/view/owned operations succeed");

    assert_eq!(owned_from_method.rows, matrix.rows);
    assert_eq!(owned_from_method.cols, matrix.cols);
    assert_eq!(owned_from_method.data, matrix.data);
}

#[test]
fn message_can_be_split_into_archive_for_view_and_owned_decode() {
    let matrix = Matrix::from_bytes::<u16>(1, 2, vec![9, 0, 8, 0]);
    let msg = to_wire_message(&matrix);
    let frame = split_wire_frame(msg.type_hash, &msg.bytes).expect("built-in message splits");
    let archive = Archived::<Matrix>::from_frame(frame);

    let view = view_archive::<Matrix>(archive).expect("archive views");
    assert_eq!(view.rows(), 1);
    assert_eq!(view.cols(), 2);
    assert_eq!(view.payload_bytes(), &[9, 0, 8, 0]);

    let owned = from_archive::<Matrix>(archive).expect("archive decodes to owned");
    assert_eq!(owned.data, matrix.data);
}

#[test]
fn macro_generated_single_payload_type_has_archive_view_owned_methods() {
    let frame = ArchiveCameraFrame {
        width: 640,
        height: 480,
        encoding: 11,
        data: vec![1, 2, 3, 4],
    };
    let original_payload_ptr = frame.data.as_ptr();

    frame
        .archive(|archive| {
            assert_eq!(archive.payload_bytes().as_ptr(), original_payload_ptr);
            let view = ArchiveCameraFrame::view_archive(archive)?;
            assert_eq!(view.header.width, 640);
            assert_eq!(view.payload_bytes().as_ptr(), original_payload_ptr);

            let owned = ArchiveCameraFrame::from_archive(archive)?;
            assert_eq!(owned.width, 640);
            assert_eq!(owned.data, vec![1, 2, 3, 4]);
            Ok::<_, datapod::WireError>(())
        })
        .expect("macro archive callback succeeds")
        .expect("macro archive/view/owned operations succeed");
}

#[test]
fn segmented_archive_preserves_payload_segments_without_implicit_join() {
    let packet = ArchiveSplitPayload {
        id: 7,
        left: b"left".to_vec(),
        right: vec![10_u16, 11_u16],
    };
    let right_bytes: &[u8] = bytemuck::cast_slice(&packet.right);
    ArchiveSplitPayload::register_schema().expect("sectioned custom schema registers");
    let info = datapod::registry::find_type_info(ArchiveSplitPayload::TYPE_HASH)
        .expect("sectioned custom schema is discoverable");
    assert_eq!(
        info.archive_shape,
        datapod::registry::ArchiveShape::SegmentedPayload
    );

    packet
        .segmented_archive(|archive| {
            assert_eq!(archive.payload_segments().len(), 2);
            assert_eq!(archive.payload_segments()[0], b"left");
            assert_eq!(archive.payload_segments()[0].as_ptr(), packet.left.as_ptr());
            assert_eq!(archive.payload_segments()[1], right_bytes);
            assert_eq!(archive.payload_segments()[1].as_ptr(), right_bytes.as_ptr());
            assert!(archive.as_single_payload_archive().is_err());
        })
        .expect("segmented archive callback succeeds");

    segmented_archive(&packet, |archive| {
        assert_eq!(archive.payload_len(), b"left".len() + right_bytes.len());
        assert_eq!(
            archive
                .frame()
                .try_payload_len()
                .expect("segmented archive payload length is exact"),
            archive.payload_len()
        );
        assert_eq!(
            archive
                .frame()
                .try_joined_len()
                .expect("segmented archive joined length is exact"),
            archive.header_bytes().len() + archive.payload_len()
        );
        assert_eq!(archive.payload_segments().len(), 2);
    })
    .expect("free segmented archive callback succeeds");

    assert!(archive(&packet, |_| ()).is_err());
}

#[test]
fn single_segmented_archive_can_be_reborrowed_as_single_payload_without_copy() {
    let frame = ArchiveCameraFrame {
        width: 320,
        height: 240,
        encoding: 7,
        data: vec![9, 8, 7, 6],
    };
    let payload_ptr = frame.data.as_ptr();

    frame
        .segmented_archive(|archive| {
            assert_eq!(archive.payload_segments().len(), 1);
            let single = archive
                .as_single_payload_archive()
                .expect("single segment is safe to expose as an archive");
            assert_eq!(single.payload_bytes(), &[9, 8, 7, 6]);
            assert_eq!(single.payload_bytes().as_ptr(), payload_ptr);
        })
        .expect("single segmented archive succeeds");
}

#[test]
fn empty_segmented_archive_refuses_implicit_join() {
    let header = [0_u8; 4];
    let payloads: [&[u8]; 0] = [];
    let frame = datapod::WireSegmentedFrame {
        type_hash: datapod::bind::type_hash::<ArchiveCameraFrame>(),
        header: &header,
        payloads: &payloads,
    };

    let message = datapod::wire_segmented_frame_to_message(frame);
    assert_eq!(message.bytes, header);

    let archive = datapod::SegmentedArchived::<ArchiveCameraFrame>::from_frame(frame);
    assert!(archive.as_single_payload_archive().is_err());
}

#[test]
fn archive_view_and_owned_decode_reject_wrong_type_hash() {
    let matrix = Matrix::from_bytes::<u8>(1, 3, vec![1, 2, 3]);
    matrix
        .archive(|archive| {
            let bad_frame = datapod::ArchiveFrame {
                type_hash: datapod::bind::type_hash::<Vector>(),
                header: archive.header_bytes(),
                payload: archive.payload_bytes(),
            };
            let bad = Archived::<Matrix>::from_frame(bad_frame);

            assert!(matches!(
                bad.view(),
                Err(datapod::WireError::WrongTypeHash { .. })
            ));
            assert!(matches!(
                bad.to_owned(),
                Err(datapod::WireError::WrongTypeHash { .. })
            ));
        })
        .expect("archive callback succeeds");
}

#[test]
fn archive_view_and_owned_decode_reject_malformed_header_lengths() {
    let matrix = Matrix::from_bytes::<u8>(1, 3, vec![1, 2, 3]);
    matrix
        .archive(|archive| {
            let short_header = &archive.header_bytes()[..archive.header_bytes().len() - 1];
            let short = Archived::<Matrix>::from_frame(datapod::ArchiveFrame {
                type_hash: datapod::bind::type_hash::<Matrix>(),
                header: short_header,
                payload: archive.payload_bytes(),
            });
            assert!(matches!(
                short.view(),
                Err(datapod::WireError::ShortHeader { .. })
            ));
            assert!(matches!(
                short.to_owned(),
                Err(datapod::WireError::ShortHeader { .. })
            ));

            let mut long_header = archive.header_bytes().to_vec();
            long_header.push(0);
            let long = Archived::<Matrix>::from_frame(datapod::ArchiveFrame {
                type_hash: datapod::bind::type_hash::<Matrix>(),
                header: &long_header,
                payload: archive.payload_bytes(),
            });
            assert!(matches!(
                long.view(),
                Err(datapod::WireError::ShortHeader { .. })
            ));
            assert!(matches!(
                long.to_owned(),
                Err(datapod::WireError::ShortHeader { .. })
            ));
        })
        .expect("archive callback succeeds");
}

#[test]
fn archive_view_and_owned_decode_reject_malformed_payload_lengths() {
    let matrix = Matrix::from_bytes::<u16>(1, 2, vec![1, 0, 2, 0]);
    matrix
        .archive(|archive| {
            let malformed = Archived::<Matrix>::from_frame(datapod::ArchiveFrame {
                type_hash: datapod::bind::type_hash::<Matrix>(),
                header: archive.header_bytes(),
                payload: &archive.payload_bytes()[..archive.payload_bytes().len() - 1],
            });

            assert!(matches!(
                malformed.validate(),
                Err(datapod::WireError::InvalidPayloadSize { .. })
            ));
            assert!(matches!(
                malformed.view(),
                Err(datapod::WireError::InvalidPayloadSize { .. })
            ));
            assert!(matches!(
                malformed.to_owned(),
                Err(datapod::WireError::InvalidPayloadSize { .. })
            ));
        })
        .expect("archive callback succeeds");
}

#[test]
fn fixed_size_archive_has_empty_payload_and_valid_owned_message() {
    let point = Point::new(1.0, 2.0, 3.0);
    point
        .archive(|archive| {
            assert_eq!(
                archive.header_bytes().len(),
                datapod::bind::header_size::<Point>()
            );
            assert!(archive.payload_bytes().is_empty());
            archive.validate()?;
            let view = archive.view()?;
            assert_eq!(view.value.x, point.x);
            assert_eq!(view.value.y, point.y);
            assert_eq!(view.value.z, point.z);

            let owned = archive.to_owned()?;
            assert_eq!(owned, point);
            Ok::<_, datapod::WireError>(())
        })
        .expect("point archive callback succeeds")
        .expect("fixed archive/view/owned succeeds");
}

#[test]
fn runtime_fixed_schema_validation_rejects_payload_bytes() {
    let type_hash =
        datapod::registry::type_hash_name("robolibs.test.runtime_fixed_archive_payload.v1");
    datapod::registry::register_type(
        type_hash,
        "robolibs.test.runtime_fixed_archive_payload.v1",
        2,
        datapod::registry::PayloadKind::Fixed,
    )
    .expect("runtime fixed schema registers");

    let bytes = [1_u8, 2, 3];
    assert!(datapod::validate_registered_wire_v1(type_hash, &bytes[..2]).is_ok());
    assert!(matches!(
        datapod::validate_registered_wire_v1(type_hash, &bytes),
        Err(datapod::WireError::InvalidPayloadSize { .. })
    ));

    let good_frame = datapod::ArchiveFrame {
        type_hash,
        header: &bytes[..2],
        payload: &[],
    };
    assert!(datapod::validate_registered_wire_frame_v1(good_frame).is_ok());

    let bad_frame = datapod::ArchiveFrame {
        type_hash,
        header: &bytes[..2],
        payload: &bytes[2..],
    };
    assert!(matches!(
        datapod::validate_registered_wire_frame_v1(bad_frame),
        Err(datapod::WireError::InvalidPayloadSize { .. })
    ));
}
