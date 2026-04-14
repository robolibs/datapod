#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Financial {
    pub open: f64,
    pub high: f64,
    pub low: f64,
    pub close: f64,
    pub volume: f64,
}

impl Financial {
    pub const fn new(open: f64, high: f64, low: f64, close: f64, volume: f64) -> Self {
        Self {
            open,
            high,
            low,
            close,
            volume,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Tick {
    pub timestamp: i64,
    pub sequence: i64,
    pub price: f64,
    pub volume: u64,
    pub side: u8,
}

impl Tick {
    pub const BID: u8 = 0;
    pub const ASK: u8 = 1;
    pub const TRADE: u8 = 2;

    pub const fn new(timestamp: i64, sequence: i64, price: f64, volume: u64, side: u8) -> Self {
        Self {
            timestamp,
            sequence,
            price,
            volume,
            side,
        }
    }

    pub const fn is_bid(&self) -> bool {
        self.side == Self::BID
    }
    pub const fn is_ask(&self) -> bool {
        self.side == Self::ASK
    }
    pub const fn is_trade(&self) -> bool {
        self.side == Self::TRADE
    }

    pub const fn side_str(&self) -> &'static str {
        match self.side {
            Self::BID => "BID",
            Self::ASK => "ASK",
            Self::TRADE => "TRADE",
            _ => "UNKNOWN",
        }
    }

    pub fn total_value(&self) -> f64 {
        self.price * self.volume as f64
    }
}

impl PartialOrd for Tick {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        match self.timestamp.cmp(&other.timestamp) {
            std::cmp::Ordering::Equal => Some(self.sequence.cmp(&other.sequence)),
            o => Some(o),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct OHLCV {
    pub timestamp: i64,
    pub open: f64,
    pub high: f64,
    pub low: f64,
    pub close: f64,
    pub volume: u64,
}

impl OHLCV {
    pub const fn new(
        timestamp: i64,
        open: f64,
        high: f64,
        low: f64,
        close: f64,
        volume: u64,
    ) -> Self {
        Self {
            timestamp,
            open,
            high,
            low,
            close,
            volume,
        }
    }

    pub fn range(&self) -> f64 {
        self.high - self.low
    }

    pub fn body(&self) -> f64 {
        self.close - self.open
    }

    pub fn is_bullish(&self) -> bool {
        self.close > self.open
    }

    pub fn is_bearish(&self) -> bool {
        self.close < self.open
    }

    pub fn is_doji(&self) -> bool {
        self.close == self.open
    }

    pub fn upper_wick(&self) -> f64 {
        self.high - self.open.max(self.close)
    }

    pub fn lower_wick(&self) -> f64 {
        self.open.min(self.close) - self.low
    }

    pub fn typical_price(&self) -> f64 {
        (self.high + self.low + self.close) / 3.0
    }

    pub fn vwap(&self) -> f64 {
        self.typical_price()
    }
}

impl PartialOrd for OHLCV {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.timestamp.cmp(&other.timestamp))
    }
}
