use std::collections::HashMap;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Trie<V> {
    pub values: HashMap<String, V>,
}
