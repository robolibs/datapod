pub type Tuple<T> = T;

pub trait Tuple1Ext<A> {
    fn get_0(&self) -> &A;
    fn into_0(self) -> A;
}

impl<A> Tuple1Ext<A> for (A,) {
    fn get_0(&self) -> &A {
        &self.0
    }

    fn into_0(self) -> A {
        self.0
    }
}

pub trait Tuple2Ext<A, B> {
    fn get_0(&self) -> &A;
    fn get_1(&self) -> &B;
    fn into_0(self) -> A;
    fn into_1(self) -> B;
    fn apply_fn<R, F: FnOnce(A, B) -> R>(self, f: F) -> R;
}

impl<A, B> Tuple2Ext<A, B> for (A, B) {
    fn get_0(&self) -> &A {
        &self.0
    }

    fn get_1(&self) -> &B {
        &self.1
    }

    fn into_0(self) -> A {
        self.0
    }

    fn into_1(self) -> B {
        self.1
    }

    fn apply_fn<R, F: FnOnce(A, B) -> R>(self, f: F) -> R {
        f(self.0, self.1)
    }
}

pub trait Tuple3Ext<A, B, C> {
    fn get_0(&self) -> &A;
    fn get_1(&self) -> &B;
    fn get_2(&self) -> &C;
    fn into_0(self) -> A;
    fn into_1(self) -> B;
    fn into_2(self) -> C;
    fn apply_fn<R, F: FnOnce(A, B, C) -> R>(self, f: F) -> R;
}

impl<A, B, C> Tuple3Ext<A, B, C> for (A, B, C) {
    fn get_0(&self) -> &A {
        &self.0
    }

    fn get_1(&self) -> &B {
        &self.1
    }

    fn get_2(&self) -> &C {
        &self.2
    }

    fn into_0(self) -> A {
        self.0
    }

    fn into_1(self) -> B {
        self.1
    }

    fn into_2(self) -> C {
        self.2
    }

    fn apply_fn<R, F: FnOnce(A, B, C) -> R>(self, f: F) -> R {
        f(self.0, self.1, self.2)
    }
}

pub trait Tuple4Ext<A, B, C, D> {
    fn get_0(&self) -> &A;
    fn get_1(&self) -> &B;
    fn get_2(&self) -> &C;
    fn get_3(&self) -> &D;
    fn apply_fn<R, F: FnOnce(A, B, C, D) -> R>(self, f: F) -> R;
}

impl<A, B, C, D> Tuple4Ext<A, B, C, D> for (A, B, C, D) {
    fn get_0(&self) -> &A {
        &self.0
    }

    fn get_1(&self) -> &B {
        &self.1
    }

    fn get_2(&self) -> &C {
        &self.2
    }

    fn get_3(&self) -> &D {
        &self.3
    }

    fn apply_fn<R, F: FnOnce(A, B, C, D) -> R>(self, f: F) -> R {
        f(self.0, self.1, self.2, self.3)
    }
}

pub fn make_tuple2<A, B>(a: A, b: B) -> (A, B) {
    (a, b)
}

pub fn make_tuple3<A, B, C>(a: A, b: B, c: C) -> (A, B, C) {
    (a, b, c)
}

pub fn make_tuple4<A, B, C, D>(a: A, b: B, c: C, d: D) -> (A, B, C, D) {
    (a, b, c, d)
}
