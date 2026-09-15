use datapod::{
    Bytes, DataPod, Encoding, Grid, Matrix, Point, Pose, WireMessage, from_wire_message,
    to_wire_message,
};

fn encode_hex(bytes: &[u8]) -> String {
    const HEX: &[u8; 16] = b"0123456789abcdef";
    let mut out = String::with_capacity(bytes.len() * 2);
    for byte in bytes {
        out.push(HEX[(byte >> 4) as usize] as char);
        out.push(HEX[(byte & 0x0f) as usize] as char);
    }
    out
}

fn decode_hex(input: &str) -> Result<Vec<u8>, String> {
    if !input.len().is_multiple_of(2) {
        return Err("hex input has odd length".to_owned());
    }
    let mut bytes = Vec::with_capacity(input.len() / 2);
    for chunk in input.as_bytes().chunks_exact(2) {
        let hi = hex_nibble(chunk[0])?;
        let lo = hex_nibble(chunk[1])?;
        bytes.push((hi << 4) | lo);
    }
    Ok(bytes)
}

fn hex_nibble(byte: u8) -> Result<u8, String> {
    match byte {
        b'0'..=b'9' => Ok(byte - b'0'),
        b'a'..=b'f' => Ok(byte - b'a' + 10),
        b'A'..=b'F' => Ok(byte - b'A' + 10),
        _ => Err(format!("invalid hex byte {byte}")),
    }
}

fn print_message(message: WireMessage) {
    println!("{} {}", message.type_hash, encode_hex(&message.bytes));
}

fn sample_grid() -> Grid {
    Grid::new(
        2,
        2,
        Encoding::Rgba8,
        0.5,
        false,
        Pose::default(),
        (0u8..16).collect(),
    )
}

fn sample_matrix() -> Matrix {
    Matrix::from_bytes::<u8>(2, 3, vec![1, 2, 3, 4, 5, 6])
}

fn sample_bytes() -> Bytes {
    Bytes::from_slice(&[9, 8, 7])
}

fn wire_from_args(args: &[String]) -> Result<WireMessage, String> {
    if args.len() != 4 {
        return Err(format!(
            "usage: {} <decode-command> <type_hash> <hex_wire>",
            args[0]
        ));
    }
    let type_hash = args[2]
        .parse::<u64>()
        .map_err(|error| format!("invalid type_hash: {error}"))?;
    let bytes = decode_hex(&args[3])?;
    Ok(WireMessage { type_hash, bytes })
}

fn main() -> Result<(), String> {
    let args: Vec<String> = std::env::args().collect();
    let Some(command) = args.get(1).map(String::as_str) else {
        return Err(format!("usage: {} <command>", args[0]));
    };

    match command {
        "encode-point" => print_message(to_wire_message(&Point::new(1.0, 2.0, 3.0))),
        "decode-point" => {
            let point = from_wire_message::<Point>(&wire_from_args(&args)?)
                .map_err(|error| error.to_string())?;
            assert_eq!(point, Point::new(1.0, 2.0, 3.0));
            println!("ok point");
        }
        "encode-grid" => print_message(to_wire_message(&sample_grid())),
        "decode-grid" => {
            let grid =
                from_wire_message::<Grid>(&wire_from_args(&args)?).map_err(|e| e.to_string())?;
            assert_eq!(grid.rows, 2);
            assert_eq!(grid.cols, 2);
            assert_eq!(grid.encoding, Encoding::Rgba8);
            assert_eq!(grid.resolution, 0.5);
            assert_eq!(grid.payload_bytes(), &(0u8..16).collect::<Vec<_>>());
            println!("ok grid");
        }
        "encode-matrix" => print_message(to_wire_message(&sample_matrix())),
        "decode-matrix" => {
            let matrix =
                from_wire_message::<Matrix>(&wire_from_args(&args)?).map_err(|e| e.to_string())?;
            assert_eq!(matrix.rows, 2);
            assert_eq!(matrix.cols, 3);
            assert_eq!(matrix.element_size, 1);
            assert_eq!(matrix.payload_bytes(), &[1, 2, 3, 4, 5, 6]);
            println!("ok matrix");
        }
        "encode-bytes" => print_message(to_wire_message(&sample_bytes())),
        "decode-bytes" => {
            let bytes =
                from_wire_message::<Bytes>(&wire_from_args(&args)?).map_err(|e| e.to_string())?;
            assert_eq!(bytes.payload_bytes(), &[9, 8, 7]);
            println!("ok bytes");
        }
        _ => return Err(format!("unknown command: {command}")),
    }

    Ok(())
}
