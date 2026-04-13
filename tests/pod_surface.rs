use datapod::Vector;
use datapod::adapters::*;
use datapod::associative::*;
use datapod::lockfree::RingBuffer;
use datapod::mat;
use datapod::memory::*;
use datapod::sequential::*;
use datapod::spatial::*;
use datapod::sugar::*;
use datapod::temporal::*;
use datapod::trees::*;

#[test]
fn pod_surface_is_present() {
    let _optional: Optional<i32> = Some(1);
    let _result: Result<i32, Error> = Ok(1);
    let _either: Either<i32, &str> = Either::Left(1);
    let _pair: Pair<i32, i32> = (1, 2);
    let _variant = Variant(3);
    let mut _bitset = Bitset::<8>::default();
    _bitset.set(0, true);

    let mut _map: Map<&str, i32> = Map::default();
    _map.insert("a", 1);
    let mut _set: Set<i32> = Set::default();
    _set.insert(1);
    let mut _fws: FwsMultimap<&str, i32> = FwsMultimap::default();
    _fws.insert("k", 1);
    let mut _mfws: MutableFwsMultimap<&str, i32> = MutableFwsMultimap::default();
    _mfws.insert("k", 2);

    let mut _ring = RingBuffer::new(4);
    _ring.push(1);

    let _scalar = mat::Scalar::new(1.0_f64);
    let _vector = mat::Vector3d::from([1.0, 2.0, 3.0]);
    let _matrix = mat::Matrix::<f64, 2, 2>::default();
    let _tensor: mat::DTensor<f64> = mat::DTensor::default();
    let _dynamic: mat::DMatrix<f64> = mat::DMatrix::default();
    let _complex = mat::Complex { re: 1.0, im: 2.0 };
    let _fraction = mat::Fraction {
        numerator: 1_i32,
        denominator: 2_i32,
    };
    let _quaternion = mat::Quaternion {
        w: 1.0,
        x: 0.0,
        y: 0.0,
        z: 0.0,
    };

    let _arena: Arena<i32> = Arena::default();
    let _pool: Pool<i32> = Pool::default();
    let _mmap: MmapVec<i32> = MmapVec::default();
    let _paged: Paged<i32> = Paged::default();
    let _offset: OffsetPtr<i32> = OffsetPtr::new(0);
    let _ptr: Ptr<i32> = std::boxed::Box::new(1);

    let _array: Array<i32, 2> = [1, 2];
    let _bitvec: BitVec = vec![true, false];
    let _bytes: Bytes = vec![1, 2, 3];
    let _deque: Deque<i32> = Deque::default();
    let _forward_list: ForwardList<i32> = ForwardList::default();
    let _heap: Heap<i32> = Heap::default();
    let _indexed_heap: IndexedHeap<i32> = IndexedHeap::default();
    let _list: List<i32> = List::default();
    let _nvec: NVec<i32, 2> = [1, 2];
    let _queue: Queue<i32> = Queue::default();
    let _stack: Stack<i32> = vec![1];
    let _string: String = "x".into();
    let _vectra: Vectra<i32> = vec![1, 2];
    let _vecvec: Vecvec<i32> = vec![vec![1]];
    let _paged_vecvec: PagedVecvec<i32> = vec![vec![1]];
    let _fixed_queue: FixedQueue<i32> = FixedQueue::default();
    let _flat_matrix: FlatMatrix<i32> = FlatMatrix::default();
    let _vector_container: Vector<i32> = Vector::default();

    let point = Point::new(1.0, 2.0, 3.0);
    let _segment = Segment::new(point, Point::default());
    let polygon = Polygon {
        vertices: Vector::from([
            Point::default(),
            Point::new(1.0, 0.0, 0.0),
            Point::new(0.0, 1.0, 0.0),
        ]),
    };
    let _aabb = Aabb::default();
    let _acceleration = Acceleration::default();
    let _bounding_sphere = BoundingSphere::default();
    let _box = Box::default();
    let _bs = Bs::default();
    let _grid: Grid<f64> = Grid::default();
    let _layer: Layer<f64> = Layer::default();
    let _path = Path::default();
    let _trajectory = Trajectory::default();
    let _gaussian_box = GaussianBox::default();
    let _gaussian_circle = GaussianCircle::default();
    let _gaussian_point = GaussianPoint::default();
    let _gaussian_rectangle = GaussianRectangle::default();
    let _linestring = Linestring::default();
    let _loc = Loc::default();
    let _multi_point: MultiPoint = Vector::default();
    let _multi_linestring: MultiLinestring = Vector::default();
    let _multi_polygon: MultiPolygon = Vector::default();
    let _obb = Obb::default();
    let _pose = Pose::default();
    let _circle = Circle::default();
    let _line = Line::default();
    let _rectangle = Rectangle::default();
    let _square = Square::default();
    let _triangle = Triangle::default();
    let _quadtree: Quadtree<Point> = Quadtree::default();
    let _ring = Ring::default();
    let _identity = Identity::default();
    let _inertial = Inertial::default();
    let _joint = Joint::default();
    let _link = Link::default();
    let _model = Model::default();
    let _odom = Odom::default();
    let _robot = Robot::default();
    let _sensor = Sensor::default();
    let _transmission = Transmission::default();
    let _twist = Twist::default();
    let _visual = Visual::default();
    let _wrench = Wrench::default();
    let _rtree: RTree<Point> = RTree::default();
    let _size = Size::default();
    let _state = State::default();
    let _transform = Transform::default();
    let _utm = Utm::default();
    let _velocity = Velocity::default();
    let _accel = Accel::default();
    let _geometry = Geometry::Sphere(SphereShape { radius: 1.0 });
    let _point_map: PointMap<i32> = PointMap::default();
    let _point_set: PointSet = PointSet::default();
    let _point_key: PointKey = point.into();
    let _geo = Geo::default();
    let _euler = Euler::default();
    let _quat = Quaternion::default();
    let _polygon = polygon;

    let _ip = Ip::default();
    let _mac = MacAddr::default();
    let _uuid = Uuid::default();

    let _stamp = Stamp::new(0);
    let _event = Event {
        stamp: Stamp::new(0),
        value: 1_i32,
    };
    let _window: Window<i32> = Window {
        start: Stamp::new(0),
        end: Stamp::new(1),
        values: Vector::default(),
    };
    let _circular: CircularBuffer<i32> = CircularBuffer::default();
    let _series: TimeSeries<i32> = TimeSeries::default();
    let _multi_series: MultiSeries<i32> = MultiSeries::default();
    let _financial = Financial::default();

    let _binary_tree: BinaryTree<i32> = BinaryTree::default();
    let _nary_tree: NaryTree<i32> = NaryTree::default();
    let _trie: Trie<i32> = Trie::default();
    let _ordered_map: datapod::trees::OrderedMap<i32, i32> = datapod::trees::OrderedMap::default();
    let _ordered_set: OrderedSet<i32> = OrderedSet::default();
}
