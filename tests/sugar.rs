use datapod::{Ip, MacAddr, Uuid};

#[test]
fn uuid_formats_and_parses_round_trip() {
    let uuid = Uuid {
        bytes: [
            0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0x4d, 0xef, 0x80, 0x12, 0x34, 0x56, 0x78, 0x9a,
            0xbc, 0xde,
        ],
    };
    let text = uuid.to_string();
    assert_eq!(text, "12345678-9abc-4def-8012-3456789abcde");
    assert_eq!(Uuid::from_string(&text).unwrap(), uuid);
    assert_eq!(
        Uuid::nil().to_string(),
        "00000000-0000-0000-0000-000000000000"
    );

    let generated = Uuid::generate_v4();
    assert_eq!(generated.bytes[6] >> 4, 0x4);
    assert_eq!(generated.bytes[8] >> 6, 0x2);

    let generated = Uuid::try_generate_v4().expect("OS random UUID generation should work");
    assert_eq!(generated.bytes[6] >> 4, 0x4);
    assert_eq!(generated.bytes[8] >> 6, 0x2);
}

#[test]
fn mac_addr_formats_and_parses_round_trip() {
    let mac = MacAddr::new([0xde, 0xad, 0xbe, 0xef, 0x12, 0x34]);
    assert_eq!(mac.to_string(), "de:ad:be:ef:12:34");
    assert_eq!(MacAddr::from_string("de:ad:be:ef:12:34").unwrap(), mac);
    assert_eq!(MacAddr::from_string("de-ad-be-ef-12-34").unwrap(), mac);
}

#[test]
fn mac_addr_parser_rejects_malformed_octet_shapes_without_allocating_parts() {
    for input in [
        "",
        "de:ad:be:ef:12",
        "de:ad:be:ef:12:34:56",
        "de:ad:be:ef:12:",
        ":de:ad:be:ef:12",
        "de::be:ef:12:34",
        "de:ad:be:ef:12:3",
        "de:ad:be:ef:12:345",
        "de:ad:be:ef:12:zz",
        "de:ad:be-ef:12:34",
    ] {
        assert!(
            MacAddr::from_string(input).is_err(),
            "malformed MAC address {input:?} should be rejected"
        );
    }
}

#[test]
fn ip_formats_and_parses_v4_and_v6() {
    let v4 = Ip::from_v4_bytes([192, 168, 0, 1]);
    assert!(v4.is_v4());
    assert_eq!(v4.v4_bytes(), Some([192, 168, 0, 1]));
    assert_eq!(v4.to_string(), "192.168.0.1");
    assert_eq!(v4.try_to_ip_addr().unwrap().to_string(), "192.168.0.1");
    assert_eq!(Ip::from_string("192.168.0.1").unwrap(), v4);

    let v6 = Ip::from_v6_bytes([
        0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01,
    ]);
    assert!(v6.is_v6());
    assert_eq!(v6.to_string(), "2001:0db8:0000:0000:0000:0000:0000:0001");
    assert_eq!(
        Ip::from_string("2001:0db8:0000:0000:0000:0000:0000:0001").unwrap(),
        v6
    );
    assert_eq!(v6.try_to_ip_addr().unwrap().to_string(), "2001:db8::1");
    assert!(Ip::from_string("2001:db8::1").is_err());

    let unset = Ip {
        family: 0,
        _pad: 0,
        bytes: [0; 16],
    };
    assert!(unset.try_to_ip_addr().is_err());
    assert_eq!(unset.to_ip_addr().to_string(), "0.0.0.0");

    let malformed_v4 = Ip {
        family: 4,
        _pad: 0,
        bytes: [1; 16],
    };
    assert!(malformed_v4.try_to_ip_addr().is_err());
    assert_eq!(malformed_v4.to_ip_addr().to_string(), "0.0.0.0");
}
