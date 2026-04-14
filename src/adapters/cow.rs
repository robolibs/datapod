pub type Cow<'a, T> = std::borrow::Cow<'a, T>;

pub trait CowExt<'a, T: ToOwned + ?Sized + 'a> {
    fn borrowed(value: &'a T) -> std::borrow::Cow<'a, T>;
    fn owned(value: <T as ToOwned>::Owned) -> std::borrow::Cow<'a, T>;
    fn is_borrowed_ext(&self) -> bool;
    fn is_owned_ext(&self) -> bool;
    fn get(&self) -> &T;
    fn to_mut_ext(&mut self) -> &mut <T as ToOwned>::Owned;
    fn make_owned(&mut self) -> &mut Self;
    fn into_owned_ext(self) -> <T as ToOwned>::Owned;
}

impl<'a, T: ToOwned + ?Sized + 'a> CowExt<'a, T> for std::borrow::Cow<'a, T> {
    fn borrowed(value: &'a T) -> std::borrow::Cow<'a, T> {
        std::borrow::Cow::Borrowed(value)
    }

    fn owned(value: <T as ToOwned>::Owned) -> std::borrow::Cow<'a, T> {
        std::borrow::Cow::Owned(value)
    }

    fn is_borrowed_ext(&self) -> bool {
        matches!(self, std::borrow::Cow::Borrowed(_))
    }

    fn is_owned_ext(&self) -> bool {
        matches!(self, std::borrow::Cow::Owned(_))
    }

    fn get(&self) -> &T {
        self.as_ref()
    }

    fn to_mut_ext(&mut self) -> &mut <T as ToOwned>::Owned {
        self.to_mut()
    }

    fn make_owned(&mut self) -> &mut Self {
        if let std::borrow::Cow::Borrowed(b) = self {
            *self = std::borrow::Cow::Owned((*b).to_owned());
        }
        self
    }

    fn into_owned_ext(self) -> <T as ToOwned>::Owned {
        self.into_owned()
    }
}
