#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Either<L, R> {
    Left(L),
    Right(R),
}

impl<L, R> Either<L, R> {
    pub fn left(value: L) -> Self {
        Self::Left(value)
    }

    pub fn right(value: R) -> Self {
        Self::Right(value)
    }

    pub fn is_left(&self) -> bool {
        matches!(self, Self::Left(_))
    }

    pub fn is_right(&self) -> bool {
        matches!(self, Self::Right(_))
    }

    pub fn left_value(&self) -> &L {
        match self {
            Self::Left(v) => v,
            Self::Right(_) => panic!("Either::left_value called on Right"),
        }
    }

    pub fn right_value(&self) -> &R {
        match self {
            Self::Right(v) => v,
            Self::Left(_) => panic!("Either::right_value called on Left"),
        }
    }

    pub fn into_left(self) -> L {
        match self {
            Self::Left(v) => v,
            Self::Right(_) => panic!("Either::into_left called on Right"),
        }
    }

    pub fn into_right(self) -> R {
        match self {
            Self::Right(v) => v,
            Self::Left(_) => panic!("Either::into_right called on Left"),
        }
    }

    pub fn map_right<U, F: FnOnce(R) -> U>(self, f: F) -> Either<L, U> {
        match self {
            Self::Right(v) => Either::Right(f(v)),
            Self::Left(v) => Either::Left(v),
        }
    }

    pub fn map_left<U, F: FnOnce(L) -> U>(self, f: F) -> Either<U, R> {
        match self {
            Self::Left(v) => Either::Left(f(v)),
            Self::Right(v) => Either::Right(v),
        }
    }

    pub fn bimap<UL, UR, FL, FR>(self, fl: FL, fr: FR) -> Either<UL, UR>
    where
        FL: FnOnce(L) -> UL,
        FR: FnOnce(R) -> UR,
    {
        match self {
            Self::Left(v) => Either::Left(fl(v)),
            Self::Right(v) => Either::Right(fr(v)),
        }
    }

    pub fn fold<T, FL, FR>(self, fl: FL, fr: FR) -> T
    where
        FL: FnOnce(L) -> T,
        FR: FnOnce(R) -> T,
    {
        match self {
            Self::Left(v) => fl(v),
            Self::Right(v) => fr(v),
        }
    }

    pub fn swap(self) -> Either<R, L> {
        match self {
            Self::Left(v) => Either::Right(v),
            Self::Right(v) => Either::Left(v),
        }
    }

    pub fn right_or(self, default_value: R) -> R {
        match self {
            Self::Right(v) => v,
            Self::Left(_) => default_value,
        }
    }

    pub fn left_or(self, default_value: L) -> L {
        match self {
            Self::Left(v) => v,
            Self::Right(_) => default_value,
        }
    }

    pub fn inspect_right<F: FnOnce(&R)>(self, f: F) -> Self {
        if let Self::Right(ref v) = self {
            f(v);
        }
        self
    }

    pub fn inspect_left<F: FnOnce(&L)>(self, f: F) -> Self {
        if let Self::Left(ref v) = self {
            f(v);
        }
        self
    }
}

pub fn left<L, R>(value: L) -> Either<L, R> {
    Either::Left(value)
}

pub fn right<L, R>(value: R) -> Either<L, R> {
    Either::Right(value)
}
