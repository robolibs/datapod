//! Tests for the heap-bearing spatial types under the data-inside design.
//!
//! Each type owns its `Vec<…>` internally; methods take `&self`. The
//! `#[dp(bytes)]` attribute on the field is just metadata for the macro to
//! generate the sibling Pod header. On the wire, `T::header()` produces
//! that header and `T::payload_bytes()` produces the bytes-cast Vec.

use datapod::{
    DataPod, Encoding, Grid, Layer, Linestring, MultiPoint, Path, Point, Pose, Ring, Trajectory,
};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

#[test]
fn ring_reports_length_area_and_closedness() {
    let ring = Ring::new(vec![
        Point::new(0.0, 0.0, 0.0),
        Point::new(1.0, 0.0, 0.0),
        Point::new(1.0, 1.0, 0.0),
        Point::new(0.0, 1.0, 0.0),
        Point::new(0.0, 0.0, 0.0),
    ]);
    assert!(ring.is_closed());
    assert_eq!(ring.num_points(), 5);
    approx_eq(ring.length(), 4.0, 1e-9);
    approx_eq(ring.area(), 1.0, 1e-9);
}

#[test]
fn linestring_length_works_on_inside_buffer() {
    let ls = Linestring::new(vec![Point::new(0.0, 0.0, 0.0), Point::new(3.0, 4.0, 0.0)]);
    assert_eq!(ls.num_points(), 2);
    approx_eq(ls.length(), 5.0, 1e-9);
}

#[test]
fn grid_geometry_methods_use_self_data() {
    let g = Grid::new(
        2,
        3,
        Encoding::I32,
        1.0,
        true,
        Pose::default(),
        vec![0u8; 2 * 3 * 4],
    );
    assert_eq!(g.size(), 6);
    // 6 cells × 4 bytes/i32 = 24 bytes
    assert!(g.is_valid(4));
    assert_eq!(g.flat_index(1, 2), 5);

    // Wire: header is the generated Pod sibling.
    let h = g.header();
    let bytes = bytemuck::bytes_of(&h);
    let back: &datapod::GridHeader = bytemuck::from_bytes(bytes);
    assert_eq!(back.rows, 2);
    assert_eq!(back.cols, 3);

    let point = g.get_point(0, 0);
    approx_eq(point.x, -1.0, 1e-9);
    approx_eq(point.y, -0.5, 1e-9);
    assert_eq!(g.world_to_grid(point), (0, 0));
}

#[test]
fn layer_geometry_methods_use_self_data() {
    let layer = Layer {
        rows: 2,
        cols: 2,
        layers: 2,
        encoding: Encoding::I32,
        centered: 0,
        _pad: 0,
        resolution: 1.0,
        layer_height: 0.5,
        pose: Pose {
            point: Point::new(10.0, 0.0, 0.0),
            ..Default::default()
        },
        data: vec![0u8; 2 * 2 * 2 * 4],
    };
    assert_eq!(layer.size(), 8);
    assert!(layer.is_valid(4));
    assert_eq!(layer.flat_index(1, 1, 1), 7);

    let point = layer.get_point(0, 0, 1);
    approx_eq(point.x, 10.5, 1e-9);
    approx_eq(point.y, 0.5, 1e-9);
    approx_eq(point.z, 0.75, 1e-9);
    assert_eq!(layer.world_to_voxel(point), (0, 0, 1));
}

#[test]
fn path_and_trajectory_carry_their_buffers() {
    let path = Path::new(vec![Pose::default()]);
    let trajectory = Trajectory::new(vec![datapod::State::default()]);
    assert_eq!(path.size(), 1);
    assert!(!path.is_empty());
    assert_eq!(trajectory.size(), 1);
    assert!(!trajectory.is_empty());
}

#[test]
fn multi_point_bbox_uses_inside_buffer() {
    let mp = MultiPoint::new(vec![
        Point::new(0.0, 0.0, 0.0),
        Point::new(3.0, 4.0, 5.0),
        Point::new(-1.0, 2.0, 1.0),
    ]);
    assert_eq!(mp.num_points(), 3);
    let (min, max) = mp.bbox().unwrap();
    approx_eq(min.x, -1.0, 1e-9);
    approx_eq(min.y, 0.0, 1e-9);
    approx_eq(min.z, 0.0, 1e-9);
    approx_eq(max.x, 3.0, 1e-9);
    approx_eq(max.y, 4.0, 1e-9);
    approx_eq(max.z, 5.0, 1e-9);
}
