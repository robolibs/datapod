use datapod::{
    PayloadLayoutBuilder, PayloadSection, SectionValidation, section_bytes, validate_sections,
    validate_sections_with_policy,
};

#[test]
fn payload_layout_builder_writes_sections_forward() {
    let mut builder = PayloadLayoutBuilder::new();
    let pixels = builder.push_section(b"pixels").expect("pixels section");
    let metadata = builder.push_section(b"meta").expect("metadata section");

    assert_eq!(pixels, PayloadSection::new(0, 6));
    assert_eq!(metadata, PayloadSection::new(6, 4));
    assert_eq!(builder.sections(), &[pixels, metadata]);
    assert_eq!(builder.payload(), b"pixelsmeta");

    let (sections, payload) = builder.finish();
    validate_sections(payload.len(), &sections).expect("builder output validates");
    assert_eq!(section_bytes(&payload, sections[0]).unwrap(), b"pixels");
    assert_eq!(section_bytes(&payload, sections[1]).unwrap(), b"meta");
}

#[test]
fn payload_layout_builder_exposes_fallible_reservation_path() {
    let mut builder =
        PayloadLayoutBuilder::try_with_capacity(2, 16).expect("small capacity reservation works");
    let first = builder.push_section(b"abc").expect("first section");
    let second = builder.push_section(b"def").expect("second section");
    assert_eq!(builder.sections(), &[first, second]);
    assert_eq!(builder.payload(), b"abcdef");

    assert!(
        PayloadLayoutBuilder::try_with_capacity(usize::MAX, 0).is_err(),
        "huge section reservations should report WireError instead of panicking"
    );
    assert!(
        PayloadLayoutBuilder::try_with_capacity(0, usize::MAX).is_err(),
        "huge payload reservations should report WireError instead of panicking"
    );
}

#[test]
fn payload_section_validation_rejects_bad_bounds_order_and_overlap() {
    assert!(validate_sections(8, &[PayloadSection::new(4, 5)]).is_err());

    assert!(validate_sections(8, &[PayloadSection::new(4, 1), PayloadSection::new(2, 1)]).is_err());

    assert!(validate_sections(8, &[PayloadSection::new(0, 4), PayloadSection::new(3, 2)]).is_err());

    validate_sections_with_policy(
        8,
        &[PayloadSection::new(4, 1), PayloadSection::new(0, 2)],
        SectionValidation {
            require_sorted: false,
            require_non_overlapping: true,
        },
    )
    .expect("unsorted non-overlapping sections allowed by policy");

    assert!(
        validate_sections_with_policy(
            8,
            &[PayloadSection::new(4, 3), PayloadSection::new(2, 3)],
            SectionValidation {
                require_sorted: false,
                require_non_overlapping: true,
            },
        )
        .is_err()
    );
}

#[test]
fn payload_section_is_plain_pod_for_headers() {
    assert_eq!(core::mem::size_of::<PayloadSection>(), 8);
    assert_eq!(core::mem::align_of::<PayloadSection>(), 4);

    let section = PayloadSection::new(9, 3);
    let bytes = bytemuck::bytes_of(&section);
    let round_trip = bytemuck::pod_read_unaligned::<PayloadSection>(bytes);
    assert_eq!(round_trip, section);
    assert_eq!(section.end().unwrap(), 12);
    assert_eq!(section.range().unwrap(), 9..12);
}

#[test]
fn payload_section_range_uses_checked_usize_conversions() {
    const LAYOUT_RS: &str = include_str!("../src/layout.rs");

    for needle in [
        "fn section_u32_to_usize(value: u32, field: &'static str) -> Result<usize, WireError>",
        "usize::try_from(value)",
        "section_u32_to_usize(self.offset, \"section offset\")?",
        "section_u32_to_usize(end, \"section end\")?",
    ] {
        assert!(
            LAYOUT_RS.contains(needle),
            "PayloadSection::range should keep checked conversion path {needle:?}"
        );
    }

    assert!(
        !LAYOUT_RS.contains("self.offset as usize..end as usize"),
        "PayloadSection::range should not retain unchecked u32-to-usize casts"
    );
}
