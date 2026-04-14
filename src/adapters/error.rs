#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Error {
    pub code: u32,
    pub message: String,
}

impl Error {
    pub const OK: u32 = 0;
    pub const INVALID_ARGUMENT: u32 = 1;
    pub const OUT_OF_RANGE: u32 = 2;
    pub const NOT_FOUND: u32 = 3;
    pub const PERMISSION_DENIED: u32 = 4;
    pub const ALREADY_EXISTS: u32 = 5;
    pub const TIMEOUT: u32 = 6;
    pub const IO_ERROR: u32 = 7;
    pub const NETWORK_ERROR: u32 = 8;
    pub const PARSE_ERROR: u32 = 9;

    pub fn new(code: u32, message: impl Into<String>) -> Self {
        Self {
            code,
            message: message.into(),
        }
    }

    pub fn ok() -> Self {
        Self {
            code: Self::OK,
            message: String::new(),
        }
    }

    pub fn invalid_argument(msg: impl Into<String>) -> Self {
        Self::new(Self::INVALID_ARGUMENT, msg)
    }

    pub fn out_of_range(msg: impl Into<String>) -> Self {
        Self::new(Self::OUT_OF_RANGE, msg)
    }

    pub fn not_found(msg: impl Into<String>) -> Self {
        Self::new(Self::NOT_FOUND, msg)
    }

    pub fn permission_denied(msg: impl Into<String>) -> Self {
        Self::new(Self::PERMISSION_DENIED, msg)
    }

    pub fn already_exists(msg: impl Into<String>) -> Self {
        Self::new(Self::ALREADY_EXISTS, msg)
    }

    pub fn timeout(msg: impl Into<String>) -> Self {
        Self::new(Self::TIMEOUT, msg)
    }

    pub fn io_error(msg: impl Into<String>) -> Self {
        Self::new(Self::IO_ERROR, msg)
    }

    pub fn network_error(msg: impl Into<String>) -> Self {
        Self::new(Self::NETWORK_ERROR, msg)
    }

    pub fn parse_error(msg: impl Into<String>) -> Self {
        Self::new(Self::PARSE_ERROR, msg)
    }

    pub fn is_ok(&self) -> bool {
        self.code == Self::OK
    }

    pub fn is_err(&self) -> bool {
        self.code != Self::OK
    }

    pub fn same_code(&self, other: &Self) -> bool {
        self.code == other.code
    }
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Error({}): {}", self.code, self.message)
    }
}

impl std::error::Error for Error {}
