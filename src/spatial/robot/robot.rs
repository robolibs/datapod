use crate::associative::Map;

use super::{Identity, Model};

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Robot {
    pub id: Identity,
    pub model: Model,
    pub props: Map<String, String>,
}

impl Robot {
    pub fn new(id: Identity, model: Model) -> Self {
        Self { id, model, props: Map::default() }
    }
}
