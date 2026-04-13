use datapod::{Grid, Layer, Path, Point, Pose, Quaternion, Ring, State, Trajectory, Vector};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

#[test]
fn ring_reports_length_area_and_closedness() {
    let ring = Ring {
        points: Vector::from([
            Point::new(0.0, 0.0, 0.0),
            Point::new(1.0, 0.0, 0.0),
            Point::new(1.0, 1.0, 0.0),
            Point::new(0.0, 1.0, 0.0),
            Point::new(0.0, 0.0, 0.0),
        ]),
    };
    assert!(ring.is_closed());
    assert_eq!(ring.num_points(), 5);
    approx_eq(ring.length(), 4.0, 1e-9);
    approx_eq(ring.area(), 1.0, 1e-9);
}

#[test]
fn grid_supports_indexing_and_coordinate_conversion() {
    let mut grid = Grid {
        rows: 2,
        cols: 3,
        resolution: 1.0,
        centered: true,
        pose: Pose::default(),
        data: Vector::from([0_i32, 1, 2, 3, 4, 5]),
    };
    assert!(grid.is_valid());
    assert_eq!(grid.size(), 6);
    assert_eq!(grid[(1, 2)], 5);
    grid[(0, 0)] = 9;
    assert_eq!(grid[(0, 0)], 9);

    let point = grid.get_point(0, 0);
    approx_eq(point.x, -1.0, 1e-9);
    approx_eq(point.y, -0.5, 1e-9);
    assert_eq!(grid.world_to_grid(point), (0, 0));
}

#[test]
fn layer_supports_voxel_indexing_and_grid_extraction() {
    let pose = Pose {
        point: Point::new(10.0, 0.0, 0.0),
        rotation: Quaternion::identity(),
    };
    let mut layer = Layer {
        rows: 2,
        cols: 2,
        layers: 2,
        resolution: 1.0,
        layer_height: 0.5,
        centered: false,
        pose,
        data: Vector::from([0_i32, 1, 2, 3, 4, 5, 6, 7]),
    };
    assert!(layer.is_valid());
    assert_eq!(layer.size(), 8);
    assert_eq!(layer[(1, 1, 1)], 7);

    let point = layer.get_point(0, 0, 1);
    approx_eq(point.x, 10.5, 1e-9);
    approx_eq(point.y, 0.5, 1e-9);
    approx_eq(point.z, 0.75, 1e-9);
    assert_eq!(layer.world_to_voxel(point), (0, 0, 1));

    let slice = layer.extract_grid(1);
    assert_eq!(slice.rows, 2);
    assert_eq!(slice.cols, 2);
    assert_eq!(slice.data.as_slice(), &[4, 5, 6, 7]);

    let replacement = Grid {
        rows: 2,
        cols: 2,
        resolution: 1.0,
        centered: false,
        pose,
        data: Vector::from([9_i32, 8, 7, 6]),
    };
    layer.set_grid(0, &replacement);
    assert_eq!(layer.data.as_slice(), &[9, 8, 7, 6, 4, 5, 6, 7]);
}

#[test]
fn path_and_trajectory_match_upstream_element_types() {
    let path = Path {
        waypoints: Vector::from([Pose::default()]),
    };
    let trajectory = Trajectory {
        states: Vector::from([State::default()]),
    };
    assert_eq!(path.size(), 1);
    assert!(!path.is_empty());
    assert_eq!(trajectory.size(), 1);
    assert!(!trajectory.is_empty());
}
