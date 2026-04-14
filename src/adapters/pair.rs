pub type Pair<A, B> = (A, B);

pub trait PairExt<A, B> {
    fn first(&self) -> &A;
    fn second(&self) -> &B;
    fn first_mut(&mut self) -> &mut A;
    fn second_mut(&mut self) -> &mut B;
    fn into_first(self) -> A;
    fn into_second(self) -> B;
    fn swap_pair(self) -> Pair<B, A>;
    fn map_first<U, F: FnOnce(A) -> U>(self, f: F) -> Pair<U, B>;
    fn map_second<U, F: FnOnce(B) -> U>(self, f: F) -> Pair<A, U>;
}

impl<A, B> PairExt<A, B> for Pair<A, B> {
    fn first(&self) -> &A {
        &self.0
    }

    fn second(&self) -> &B {
        &self.1
    }

    fn first_mut(&mut self) -> &mut A {
        &mut self.0
    }

    fn second_mut(&mut self) -> &mut B {
        &mut self.1
    }

    fn into_first(self) -> A {
        self.0
    }

    fn into_second(self) -> B {
        self.1
    }

    fn swap_pair(self) -> Pair<B, A> {
        (self.1, self.0)
    }

    fn map_first<U, F: FnOnce(A) -> U>(self, f: F) -> Pair<U, B> {
        (f(self.0), self.1)
    }

    fn map_second<U, F: FnOnce(B) -> U>(self, f: F) -> Pair<A, U> {
        (self.0, f(self.1))
    }
}

pub fn make_pair<A, B>(first: A, second: B) -> Pair<A, B> {
    (first, second)
}
