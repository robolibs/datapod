//! Procedural macros for `datapod`.
//!
//! `#[datapod]` annotates a struct that participates in the wire contract.
//! Two cases:
//!
//! ## Fixed-Pod case (no `#[dp(bytes)]` field)
//!
//! The struct itself is the wire layout. The macro injects `#[repr(C)]`,
//! `Debug/Clone/Copy/PartialEq/bytemuck::Pod/bytemuck::Zeroable`, an
//! `unsafe impl ZeroCopySend`, and `impl DataPod for X { type Header = X;
//! type Payload = (); fn header(&self) -> X { *self } fn payload_bytes(&self)
//! -> &[u8] { &[] } }`.
//!
//! ## Heap-bearing case (`#[dp(bytes)]` fields of type `Vec<T>`)
//!
//! - The user struct **keeps** every field exactly as written. The
//!   `#[dp(bytes)]` field is the heap-owned data buffer.
//! - The user struct is **not** `bytemuck::Pod` (it has a `Vec` inside);
//!   it derives `Debug/Clone/PartialEq` and any other derives the user
//!   added.
//! - The macro **generates a sibling `<Original>Header` struct** containing
//!   every NON-bytes field, copied verbatim. That header IS `#[repr(C)]`,
//!   Pod, Zeroable, ZeroCopySend — i.e. wire-shippable.
//! - `impl DataPod for Original` for the common single-payload case:
//!     - `type Header = <Original>Header`,
//!     - `type Payload = [u8]`,
//!     - `fn header(&self)` copies the non-bytes fields into a new header,
//!     - `fn payload_bytes(&self)` returns `bytemuck::cast_slice(&self.<bytes_field>)`.
//! - Multiple `#[dp(bytes, section = "...")]` fields are encoded as one
//!   payload blob written in field order. The generated header contains a
//!   `PayloadSection` field for each owned `Vec<T>` field.
//! - By default the macro also generates `<Original>View<'a>`,
//!   `DataPodValidate`, and `DataPodAccess`. Use `#[dp(manual_access)]` on
//!   the struct when a type provides semantic validation/view impls by hand.
//! - If generated header fields have type `PayloadSection`, default validation
//!   also checks that those sections are sorted, non-overlapping, and in-bounds.

use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use quote::{format_ident, quote};
use syn::{
    Attribute, Data, DeriveInput, Error, Field, Fields, FieldsNamed, GenericArgument, Generics,
    ItemStruct, LitStr, PathArguments, Token, Type, Visibility,
    parse::{Parse, ParseStream},
    parse_macro_input,
    spanned::Spanned,
};

#[derive(Debug, Clone)]
struct BytesFieldInfo {
    index: usize,
    section: Option<String>,
}

#[derive(Clone, Default)]
struct DatapodArgs {
    canonical_name: Option<LitStr>,
}

impl Parse for DatapodArgs {
    fn parse(input: ParseStream<'_>) -> Result<Self, Error> {
        let mut args = DatapodArgs::default();
        while !input.is_empty() {
            let ident: syn::Ident = input.parse()?;
            if ident == "name" || ident == "canonical_name" {
                input.parse::<Token![=]>()?;
                let value: LitStr = input.parse()?;
                if args.canonical_name.is_some() {
                    return Err(Error::new(ident.span(), "duplicate datapod canonical name"));
                }
                if value.value().is_empty() {
                    return Err(Error::new(value.span(), "datapod canonical name is empty"));
                }
                args.canonical_name = Some(value);
            } else {
                return Err(Error::new(
                    ident.span(),
                    "unsupported #[datapod(...)] argument; expected name = \"...\"",
                ));
            }
            if input.is_empty() {
                break;
            }
            input.parse::<Token![,]>()?;
        }
        Ok(args)
    }
}

#[proc_macro_attribute]
pub fn datapod(args: TokenStream, input: TokenStream) -> TokenStream {
    let args = parse_macro_input!(args as DatapodArgs);
    let input = parse_macro_input!(input as ItemStruct);
    expand_attribute(input, args)
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

fn expand_attribute(mut input: ItemStruct, args: DatapodArgs) -> Result<TokenStream2, Error> {
    let name = input.ident.clone();
    let manual_access = take_manual_access_attr(&mut input.attrs)?;

    // Find the #[dp(bytes)] fields if any.
    let bytes_info = find_bytes_fields(&input)?;

    match bytes_info.as_slice() {
        [] => expand_fixed(input, manual_access, &args),
        [info] if info.section.is_none() => {
            expand_heap(&mut input, info.index, &name, manual_access, &args)
        }
        infos => expand_sectioned_heap(&mut input, infos, &name, manual_access, &args),
    }
}

// ---------------------------------------------------------------------------
// Fixed-Pod case: struct itself is the wire layout.
// ---------------------------------------------------------------------------

fn expand_fixed(
    mut input: ItemStruct,
    manual_access: bool,
    args: &DatapodArgs,
) -> Result<TokenStream2, Error> {
    let name = input.ident.clone();
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();
    let le_wire_impl = le_wire_impl_for_item_struct(&name, &input.generics, &input.fields)?;
    let schema_fields = schema_fields_for_named_fields(&input.fields, &[]);
    let reserved_field_validation = fixed_reserved_field_validation(&input.fields, &name);
    let datapod_canonical_const = datapod_canonical_const(args);
    let inherent_archive_api = inherent_archive_api(&name, &input.generics);
    let inherent_canonical_api = inherent_canonical_api(
        &name,
        &input.generics,
        args.canonical_name.as_ref(),
        quote!(::datapod::registry::PayloadKind::Fixed),
        quote!(::datapod::registry::ArchiveShape::Fixed),
    );

    if !has_repr_c(&input.attrs) {
        input.attrs.push(parse_repr_c());
    }

    let access_impl = if manual_access {
        quote! {}
    } else {
        quote! {
            impl #impl_generics ::datapod::DataPodValidate for #name #ty_generics #where_clause {
                fn validate_wire_parts(
                    header: &<Self as ::datapod::DataPod>::Header,
                    payload: &[u8],
                ) -> ::core::result::Result<(), ::datapod::WireError> {
                    if !payload.is_empty() {
                        return Err(::datapod::WireError::InvalidPayloadSize {
                            type_name: ::core::any::type_name::<Self>(),
                            message: ::std::format!(
                                "fixed datapod payload must be empty, got {}",
                                payload.len()
                            ),
                        });
                    }
                    #reserved_field_validation
                    Ok(())
                }
            }

            impl #impl_generics ::datapod::DataPodAccess for #name #ty_generics #where_clause {
                type View<'a> = ::datapod::FixedView<Self> where Self: 'a;

                fn access_wire_parts<'a>(
                    header: <Self as ::datapod::DataPod>::Header,
                    payload: &'a [u8],
                ) -> ::core::result::Result<Self::View<'a>, ::datapod::WireError> {
                    <Self as ::datapod::DataPodValidate>::validate_wire_parts(&header, payload)?;
                    Ok(::datapod::FixedView { value: header })
                }

                unsafe fn access_wire_parts_unchecked<'a>(
                    header: <Self as ::datapod::DataPod>::Header,
                    _payload: &'a [u8],
                ) -> Self::View<'a> {
                    ::datapod::FixedView { value: header }
                }
            }
        }
    };
    let decode_validation = quote! {
        <Self as ::datapod::DataPodValidate>::validate_wire_parts(&header, &payload)?;
    };

    Ok(quote! {
        #[derive(
            ::core::fmt::Debug,
            ::core::clone::Clone,
            ::core::marker::Copy,
            ::core::cmp::PartialEq,
        )]
        #input

        // Marker traits are hand-implemented through datapod's re-export
        // rather than `#[derive(bytemuck::Pod)]`. bytemuck's derive expands
        // to code referencing a bare `::bytemuck`, which would force every
        // downstream crate to depend on bytemuck directly. These impls only
        // name `::datapod::bytemuck`, so `datapod` as a dependency suffices.
        // Safety: the macro keeps the type `#[repr(C)]` and all-Pod-fields,
        // so it is a valid plain-old-data type with no padding.
        unsafe impl #impl_generics ::datapod::bytemuck::Zeroable for #name #ty_generics #where_clause {}
        unsafe impl #impl_generics ::datapod::bytemuck::Pod for #name #ty_generics #where_clause {}

        unsafe impl #impl_generics ::datapod::ZeroCopySend for #name #ty_generics #where_clause {}

        impl #impl_generics ::datapod::DataPod for #name #ty_generics #where_clause {
            #datapod_canonical_const
            type Header = #name #ty_generics;
            type Payload = ();
            fn header(&self) -> Self::Header { *self }
            fn payload_bytes(&self) -> &[u8] { &[] }
        }

        impl #impl_generics ::datapod::DataPodDecode for #name #ty_generics #where_clause {
            fn from_wire_parts(
                header: <Self as ::datapod::DataPod>::Header,
                payload: ::std::vec::Vec<u8>,
            ) -> ::core::result::Result<Self, ::datapod::WireError> {
                #decode_validation
                Ok(header)
            }
        }

        impl #impl_generics ::datapod::schema::DataPodSchema for #name #ty_generics #where_clause {
            fn schema_fields() -> ::std::vec::Vec<::datapod::schema::SchemaField> {
                ::std::vec![#(#schema_fields),*]
            }
        }

        #le_wire_impl

        #inherent_archive_api

        #inherent_canonical_api

        #access_impl
    })
}

// ---------------------------------------------------------------------------
// Heap-bearing case: data inside the user struct, header generated alongside.
// ---------------------------------------------------------------------------

fn expand_heap(
    input: &mut ItemStruct,
    bytes_field_index: usize,
    name: &syn::Ident,
    manual_access: bool,
    args: &DatapodArgs,
) -> Result<TokenStream2, Error> {
    // Header name: `<Original>Header`.
    let header_name = format_ident!("{}Header", name);
    let view_name = format_ident!("{}View", name);
    let datapod_canonical_const = datapod_canonical_const(args);
    let inherent_archive_api = inherent_archive_api(name, &input.generics);
    let inherent_canonical_api = inherent_canonical_api(
        name,
        &input.generics,
        args.canonical_name.as_ref(),
        quote!(::datapod::registry::PayloadKind::Bytes),
        quote!(::datapod::registry::ArchiveShape::SinglePayload),
    );

    // Strip the `#[dp(bytes)]` attribute from the bytes field on the user
    // struct so it doesn't end up in the emitted source (the macro consumed
    // its meaning).
    let (bytes_field_ident, bytes_field_ty): (syn::Ident, Type) = match &mut input.fields {
        Fields::Named(named) => {
            let f = named
                .named
                .iter_mut()
                .nth(bytes_field_index)
                .ok_or_else(|| {
                    Error::new(name.span(), "#[dp(bytes)] field index is out of bounds")
                })?;
            let ty = f.ty.clone();
            f.attrs.retain(|a| !a.path().is_ident("dp"));
            (
                f.ident
                    .clone()
                    .ok_or_else(|| Error::new(f.span(), "#[dp(bytes)] field must be named"))?,
                ty,
            )
        }
        _ => {
            return Err(Error::new(
                input.span(),
                "#[datapod] heap-bearing structs require named fields.",
            ));
        }
    };
    let bytes_element_ty = vec_element_type(&bytes_field_ty)?;

    // Collect the non-bytes named fields — they become the header struct.
    let header_fields: Vec<&Field> = match &input.fields {
        Fields::Named(FieldsNamed { named, .. }) => named
            .iter()
            .enumerate()
            .filter(|(i, _)| *i != bytes_field_index)
            .map(|(_, f)| f)
            .collect(),
        _ => {
            return Err(Error::new(
                input.span(),
                "#[datapod] heap-bearing structs require named fields.",
            ));
        }
    };
    let reserved_field_validation =
        reserved_field_validation_for_fields(header_fields.iter().copied());
    let header_le_fields: Vec<(syn::Ident, Type)> = header_fields
        .iter()
        .map(|f| {
            Ok((
                f.ident
                    .clone()
                    .ok_or_else(|| Error::new(f.span(), "datapod header field must be named"))?,
                f.ty.clone(),
            ))
        })
        .collect::<Result<_, Error>>()?;
    let header_le_impl =
        le_wire_impl_for_type(&header_name, &Generics::default(), &header_le_fields);
    let schema_fields = schema_fields_for_header_and_payload(
        &header_le_fields,
        &[(bytes_field_ident.clone(), bytes_element_ty.clone())],
    );
    let header_field_idents: Vec<syn::Ident> = header_le_fields
        .iter()
        .map(|(ident, _)| ident.clone())
        .collect();

    // Build the generated header struct: same fields (cleaned of `#[dp(...)]`),
    // public visibility, #[repr(C)] + Pod derives.
    let header_field_defs =
        header_field_idents
            .iter()
            .zip(header_fields.iter())
            .map(|(ident, f)| {
                let ty = &f.ty;
                let vis = match &f.vis {
                    Visibility::Public(_) => quote!(pub),
                    _ => quote!(pub),
                };
                quote! { #vis #ident: #ty }
            });

    // header() method body: copy each non-bytes field by value.
    let header_field_copies = header_field_idents.iter().map(|ident| {
        quote! { #ident: self.#ident }
    });
    let header_field_decodes = header_field_idents.iter().map(|ident| {
        quote! { #ident: header.#ident }
    });
    let section_field_idents: Vec<syn::Ident> = header_fields
        .iter()
        .filter(|f| is_payload_section_type(&f.ty))
        .filter_map(|f| f.ident.clone())
        .collect();
    let section_validation = if section_field_idents.is_empty() {
        quote! {}
    } else {
        quote! {
            let sections = [#(header.#section_field_idents),*];
            ::datapod::validate_sections(payload.len(), &sections)?;
        }
    };

    // Outer visibility of the original struct → reuse for the header.
    let vis = match &input.vis {
        Visibility::Public(_) => quote!(pub),
        _ => quote!(pub),
    };

    let access_impl = if manual_access {
        quote! {}
    } else {
        quote! {
            /// Borrowed view over this datapod's wire payload.
            #[derive(::core::fmt::Debug, ::core::clone::Clone, ::core::marker::Copy, ::core::cmp::PartialEq)]
            #vis struct #view_name<'a> {
                pub header: #header_name,
                pub payload: &'a [u8],
            }

            impl<'a> #view_name<'a> {
                pub fn payload_bytes(&self) -> &'a [u8] {
                    self.payload
                }

                pub fn section_bytes(
                    &self,
                    section: ::datapod::PayloadSection,
                ) -> ::core::result::Result<&'a [u8], ::datapod::WireError> {
                    ::datapod::section_bytes(self.payload, section)
                }
            }

            impl ::datapod::DataPodValidate for #name {
                fn validate_wire_parts(
                    header: &<Self as ::datapod::DataPod>::Header,
                    payload: &[u8],
                ) -> ::core::result::Result<(), ::datapod::WireError> {
                    let elem_size = ::core::mem::size_of::<#bytes_element_ty>();
                    if elem_size == 0 {
                        return Err(::datapod::WireError::InvalidPayloadSize {
                            type_name: ::core::any::type_name::<Self>(),
                            message: "zero-sized payload elements are not supported".to_string(),
                        });
                    }
                    if payload.len() % elem_size != 0 {
                        return Err(::datapod::WireError::InvalidPayloadSize {
                            type_name: ::core::any::type_name::<Self>(),
                            message: ::std::format!(
                                "{} bytes is not a multiple of element size {elem_size}",
                                payload.len()
                            ),
                        });
                    }
                    #reserved_field_validation
                    #section_validation
                    Ok(())
                }
            }

            impl ::datapod::DataPodAccess for #name {
                type View<'a> = #view_name<'a>;

                fn access_wire_parts<'a>(
                    header: <Self as ::datapod::DataPod>::Header,
                    payload: &'a [u8],
                ) -> ::core::result::Result<Self::View<'a>, ::datapod::WireError> {
                    <Self as ::datapod::DataPodValidate>::validate_wire_parts(&header, payload)?;
                    Ok(#view_name { header, payload })
                }

                unsafe fn access_wire_parts_unchecked<'a>(
                    header: <Self as ::datapod::DataPod>::Header,
                    payload: &'a [u8],
                ) -> Self::View<'a> {
                    #view_name { header, payload }
                }
            }
        }
    };

    Ok(quote! {
        // User-facing struct: kept as-is (data inside). Not Pod (Vec inside).
        #[derive(::core::fmt::Debug, ::core::clone::Clone, ::core::cmp::PartialEq)]
        #input

        // Generated sibling header — IS Pod, lives on the wire.
        #[repr(C)]
        #[derive(
            ::core::fmt::Debug,
            ::core::clone::Clone,
            ::core::marker::Copy,
            ::core::cmp::PartialEq,
            ::core::default::Default,
        )]
        #vis struct #header_name {
            #(#header_field_defs),*
        }

        // Hand-implemented through datapod's re-export (see the fixed-Pod
        // case for the rationale): the header is a generated `#[repr(C)]`
        // all-Pod-fields struct, so these marker impls are sound and avoid
        // forcing a downstream `bytemuck` dependency.
        unsafe impl ::datapod::bytemuck::Zeroable for #header_name {}
        unsafe impl ::datapod::bytemuck::Pod for #header_name {}

        unsafe impl ::datapod::ZeroCopySend for #header_name {}

        #header_le_impl

        #inherent_archive_api

        #inherent_canonical_api

        impl ::datapod::DataPod for #name {
            #datapod_canonical_const
            type Header = #header_name;
            type Payload = [u8];
            fn header(&self) -> #header_name {
                #header_name {
                    #(#header_field_copies),*
                }
            }
            fn payload_bytes(&self) -> &[u8] {
                ::datapod::bytemuck::cast_slice(&self.#bytes_field_ident)
            }
        }

        impl ::datapod::DataPodDecode for #name {
            fn from_wire_parts(
                header: <Self as ::datapod::DataPod>::Header,
                payload: ::std::vec::Vec<u8>,
            ) -> ::core::result::Result<Self, ::datapod::WireError> {
                <Self as ::datapod::DataPodValidate>::validate_wire_parts(&header, &payload)?;
                Ok(Self {
                    #(#header_field_decodes,)*
                    #bytes_field_ident: ::datapod::decode_payload_vec::<#bytes_element_ty>(&payload)?,
                })
            }
        }

        impl ::datapod::schema::DataPodSchema for #name {
            fn schema_fields() -> ::std::vec::Vec<::datapod::schema::SchemaField> {
                ::std::vec![#(#schema_fields),*]
            }
        }

        #access_impl
    })
}

fn expand_sectioned_heap(
    input: &mut ItemStruct,
    bytes_infos: &[BytesFieldInfo],
    name: &syn::Ident,
    manual_access: bool,
    args: &DatapodArgs,
) -> Result<TokenStream2, Error> {
    let header_name = format_ident!("{}Header", name);
    let view_name = format_ident!("{}View", name);
    let datapod_canonical_const = datapod_canonical_const(args);
    let inherent_archive_api = inherent_archive_api(name, &input.generics);
    let inherent_canonical_api = inherent_canonical_api(
        name,
        &input.generics,
        args.canonical_name.as_ref(),
        quote!(::datapod::registry::PayloadKind::Bytes),
        quote!(::datapod::registry::ArchiveShape::SegmentedPayload),
    );

    let bytes_indices: Vec<usize> = bytes_infos.iter().map(|info| info.index).collect();
    let mut bytes_fields = Vec::new();
    if let Fields::Named(named) = &mut input.fields {
        for info in bytes_infos {
            let field = named.named.iter_mut().nth(info.index).ok_or_else(|| {
                Error::new(name.span(), "#[dp(bytes)] field index is out of bounds")
            })?;
            field.attrs.retain(|attr| !attr.path().is_ident("dp"));
            let ident = field
                .ident
                .clone()
                .ok_or_else(|| Error::new(field.span(), "#[dp(bytes)] field must be named"))?;
            let ty = field.ty.clone();
            let elem_ty = vec_element_type(&ty)?;
            bytes_fields.push((ident, ty, elem_ty));
        }
    } else {
        return Err(Error::new(
            input.span(),
            "#[datapod] heap-bearing structs require named fields.",
        ));
    }

    let header_fields: Vec<&Field> = match &input.fields {
        Fields::Named(FieldsNamed { named, .. }) => named
            .iter()
            .enumerate()
            .filter(|(i, _)| !bytes_indices.contains(i))
            .map(|(_, f)| f)
            .collect(),
        _ => {
            return Err(Error::new(
                input.span(),
                "#[datapod] heap-bearing structs require named fields.",
            ));
        }
    };
    let reserved_field_validation =
        reserved_field_validation_for_fields(header_fields.iter().copied());

    let regular_header_fields: Vec<(syn::Ident, Type)> = header_fields
        .iter()
        .map(|f| {
            Ok((
                f.ident
                    .clone()
                    .ok_or_else(|| Error::new(f.span(), "datapod header field must be named"))?,
                f.ty.clone(),
            ))
        })
        .collect::<Result<_, Error>>()?;

    let regular_header_field_defs = regular_header_fields.iter().map(|(ident, ty)| {
        quote! { pub #ident: #ty }
    });
    let section_header_field_defs = bytes_fields.iter().map(|(ident, _, _)| {
        quote! { pub #ident: ::datapod::PayloadSection }
    });
    let mut header_le_fields: Vec<(syn::Ident, Type)> = regular_header_fields.clone();
    header_le_fields.extend(
        bytes_fields
            .iter()
            .map(|(ident, _, _)| (ident.clone(), syn::parse_quote!(::datapod::PayloadSection))),
    );
    let header_le_impl =
        le_wire_impl_for_type(&header_name, &Generics::default(), &header_le_fields);
    let schema_fields = schema_fields_for_header_and_payload(
        &header_le_fields,
        &bytes_fields
            .iter()
            .map(|(ident, _, elem_ty)| (ident.clone(), elem_ty.clone()))
            .collect::<Vec<_>>(),
    );

    let regular_header_decodes = regular_header_fields.iter().map(|(ident, _)| {
        quote! { #ident: header.#ident }
    });
    let regular_header_copies = regular_header_fields.iter().map(|(ident, _)| {
        quote! { #ident: self.#ident }
    });

    let section_vars: Vec<syn::Ident> = bytes_fields
        .iter()
        .map(|(ident, _, _)| format_ident!("__dp_{}_section", ident))
        .collect();
    let section_byte_vars: Vec<syn::Ident> = bytes_fields
        .iter()
        .map(|(ident, _, _)| format_ident!("__dp_{}_bytes", ident))
        .collect();
    let section_idents: Vec<syn::Ident> = bytes_fields
        .iter()
        .map(|(ident, _, _)| ident.clone())
        .collect();
    let section_elem_tys: Vec<Type> = bytes_fields
        .iter()
        .map(|(_, _, elem_ty)| elem_ty.clone())
        .collect();

    let section_header_build = bytes_fields
        .iter()
        .zip(section_vars.iter())
        .zip(section_byte_vars.iter())
        .map(|(((ident, _, _), section_var), bytes_var)| {
            quote! {
                let #bytes_var: &[u8] = ::datapod::bytemuck::cast_slice(&self.#ident);
                let __dp_len: u32 = match #bytes_var.len().try_into() {
                    ::core::result::Result::Ok(__dp_len) => __dp_len,
                    ::core::result::Result::Err(_) => {
                        return ::core::result::Result::Err(::datapod::WireError::InvalidPayloadSize {
                            type_name: ::core::any::type_name::<Self>(),
                            message: ::std::format!(
                                "section {} has {} bytes, exceeds u32::MAX",
                                ::core::stringify!(#ident),
                                #bytes_var.len()
                            ),
                        });
                    }
                };
                let #section_var = ::datapod::PayloadSection::new(__dp_offset, __dp_len);
                __dp_offset = match __dp_offset.checked_add(__dp_len) {
                    ::core::option::Option::Some(__dp_offset) => __dp_offset,
                    ::core::option::Option::None => {
                        return ::core::result::Result::Err(::datapod::WireError::InvalidPayloadSize {
                            type_name: ::core::any::type_name::<Self>(),
                            message: "section payload offsets exceed u32::MAX".to_string(),
                        });
                    }
                };
            }
        });

    let section_header_copies = section_idents
        .iter()
        .zip(section_vars.iter())
        .map(|(ident, section_var)| quote! { #ident: #section_var });
    let payload_len_steps = section_idents.iter().map(|ident| {
        quote! {
            let __dp_section_payload: &[u8] = ::datapod::bytemuck::cast_slice(&self.#ident);
            __dp_len = match __dp_len.checked_add(__dp_section_payload.len()) {
                ::core::option::Option::Some(__dp_len) => __dp_len,
                ::core::option::Option::None => {
                    return ::core::result::Result::Err(::datapod::WireError::InvalidPayloadSize {
                        type_name: ::core::any::type_name::<Self>(),
                        message: "sectioned payload length overflows usize".to_string(),
                    });
                }
            };
        }
    });
    let payload_write_steps: Vec<_> = section_idents
        .iter()
        .map(|ident| {
            quote! {
                let __dp_section_payload: &[u8] = ::datapod::bytemuck::cast_slice(&self.#ident);
                out.extend_from_slice(__dp_section_payload);
            }
        })
        .collect();
    let payload_try_write_steps: Vec<_> = section_idents
        .iter()
        .map(|ident| {
            quote! {
                let __dp_section_payload: &[u8] = ::datapod::bytemuck::cast_slice(&self.#ident);
                out.extend_from_slice(__dp_section_payload);
            }
        })
        .collect();
    let section_decodes =
        section_idents
            .iter()
            .zip(section_elem_tys.iter())
            .map(|(ident, elem_ty)| {
                quote! {
                    #ident: ::datapod::decode_payload_vec::<#elem_ty>(
                        ::datapod::section_bytes(&payload, header.#ident)?
                    )?
                }
            });

    let section_validation_items =
        section_idents
            .iter()
            .zip(section_elem_tys.iter())
            .map(|(ident, elem_ty)| {
                quote! {
                    let __dp_section_bytes = ::datapod::section_bytes(payload, header.#ident)?;
                    let __dp_elem_size = ::core::mem::size_of::<#elem_ty>();
                    if __dp_elem_size == 0 {
                        return Err(::datapod::WireError::InvalidPayloadSize {
                            type_name: ::core::any::type_name::<Self>(),
                            message: "zero-sized payload elements are not supported".to_string(),
                        });
                    }
                    if __dp_section_bytes.len() % __dp_elem_size != 0 {
                        return Err(::datapod::WireError::InvalidPayloadSize {
                            type_name: ::core::any::type_name::<Self>(),
                            message: ::std::format!(
                                "section {} has {} bytes, not a multiple of element size {}",
                                ::core::stringify!(#ident),
                                __dp_section_bytes.len(),
                                __dp_elem_size
                            ),
                        });
                    }
                }
            });

    let vis = match &input.vis {
        Visibility::Public(_) => quote!(pub),
        _ => quote!(pub),
    };

    let access_impl = if manual_access {
        quote! {}
    } else {
        quote! {
            /// Borrowed view over this datapod's sectioned wire payload.
            #[derive(::core::fmt::Debug, ::core::clone::Clone, ::core::marker::Copy, ::core::cmp::PartialEq)]
            #vis struct #view_name<'a> {
                pub header: #header_name,
                pub payload: &'a [u8],
            }

            impl<'a> #view_name<'a> {
                pub fn payload_bytes(&self) -> &'a [u8] {
                    self.payload
                }

                pub fn section_bytes(
                    &self,
                    section: ::datapod::PayloadSection,
                ) -> ::core::result::Result<&'a [u8], ::datapod::WireError> {
                    ::datapod::section_bytes(self.payload, section)
                }
            }

            impl ::datapod::DataPodValidate for #name {
                fn validate_wire_parts(
                    header: &<Self as ::datapod::DataPod>::Header,
                    payload: &[u8],
                ) -> ::core::result::Result<(), ::datapod::WireError> {
                    let sections = [#(header.#section_idents),*];
                    ::datapod::validate_sections(payload.len(), &sections)?;
                    #reserved_field_validation
                    #(#section_validation_items)*
                    Ok(())
                }
            }

            impl ::datapod::DataPodAccess for #name {
                type View<'a> = #view_name<'a>;

                fn access_wire_parts<'a>(
                    header: <Self as ::datapod::DataPod>::Header,
                    payload: &'a [u8],
                ) -> ::core::result::Result<Self::View<'a>, ::datapod::WireError> {
                    <Self as ::datapod::DataPodValidate>::validate_wire_parts(&header, payload)?;
                    Ok(#view_name { header, payload })
                }

                unsafe fn access_wire_parts_unchecked<'a>(
                    header: <Self as ::datapod::DataPod>::Header,
                    payload: &'a [u8],
                ) -> Self::View<'a> {
                    #view_name { header, payload }
                }
            }
        }
    };

    Ok(quote! {
        #[derive(::core::fmt::Debug, ::core::clone::Clone, ::core::cmp::PartialEq)]
        #input

        #[repr(C)]
        #[derive(
            ::core::fmt::Debug,
            ::core::clone::Clone,
            ::core::marker::Copy,
            ::core::cmp::PartialEq,
            ::core::default::Default,
        )]
        #vis struct #header_name {
            #(#regular_header_field_defs,)*
            #(#section_header_field_defs),*
        }

        unsafe impl ::datapod::bytemuck::Zeroable for #header_name {}
        unsafe impl ::datapod::bytemuck::Pod for #header_name {}

        unsafe impl ::datapod::ZeroCopySend for #header_name {}

        #header_le_impl

        #inherent_archive_api

        #inherent_canonical_api

        impl ::datapod::DataPod for #name {
            #datapod_canonical_const
            type Header = #header_name;
            type Payload = [u8];

            fn header(&self) -> #header_name {
                self.try_header().unwrap_or_default()
            }

            fn try_header(&self) -> ::core::result::Result<#header_name, ::datapod::WireError> {
                let mut __dp_offset: u32 = 0;
                #(#section_header_build)*
                let _ = __dp_offset;
                ::core::result::Result::Ok(#header_name {
                    #(#regular_header_copies,)*
                    #(#section_header_copies),*
                })
            }

            fn payload_bytes(&self) -> &[u8] {
                &[]
            }

            fn payload_len(&self) -> usize {
                self.try_payload_len().unwrap_or(0)
            }

            fn try_payload_len(&self) -> ::core::result::Result<usize, ::datapod::WireError> {
                let mut __dp_len = 0usize;
                #(#payload_len_steps)*
                ::core::result::Result::Ok(__dp_len)
            }

            fn write_payload_bytes(&self, out: &mut ::std::vec::Vec<u8>) {
                #(#payload_write_steps)*
            }

            fn try_write_payload_bytes(
                &self,
                out: &mut ::std::vec::Vec<u8>,
            ) -> ::core::result::Result<(), ::datapod::WireError> {
                let __dp_len = self.try_payload_len()?;
                out.try_reserve_exact(__dp_len).map_err(|err| {
                    ::datapod::WireError::InvalidPayloadSize {
                        type_name: ::core::any::type_name::<Self>(),
                        message: ::std::format!(
                            "failed to reserve {} sectioned payload bytes: {err}",
                            __dp_len
                        ),
                    }
                })?;
                #(#payload_try_write_steps)*
                ::core::result::Result::Ok(())
            }

            fn with_payload_segments<R>(&self, f: impl FnOnce(&[&[u8]]) -> R) -> R {
                let __dp_payload_segments = [
                    #(::datapod::bytemuck::cast_slice(&self.#section_idents),)*
                ];
                f(&__dp_payload_segments)
            }
        }

        impl ::datapod::DataPodDecode for #name {
            fn from_wire_parts(
                header: <Self as ::datapod::DataPod>::Header,
                payload: ::std::vec::Vec<u8>,
            ) -> ::core::result::Result<Self, ::datapod::WireError> {
                <Self as ::datapod::DataPodValidate>::validate_wire_parts(&header, &payload)?;
                Ok(Self {
                    #(#regular_header_decodes,)*
                    #(#section_decodes),*
                })
            }
        }

        impl ::datapod::schema::DataPodSchema for #name {
            fn schema_fields() -> ::std::vec::Vec<::datapod::schema::SchemaField> {
                ::std::vec![#(#schema_fields),*]
            }
        }

        #access_impl
    })
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

fn datapod_canonical_const(args: &DatapodArgs) -> TokenStream2 {
    match args.canonical_name.as_ref() {
        Some(name) => quote! {
            const CANONICAL_NAME: ::core::option::Option<&'static str> = ::core::option::Option::Some(#name);
        },
        None => quote! {},
    }
}

fn inherent_archive_api(name: &syn::Ident, generics: &Generics) -> TokenStream2 {
    let (impl_generics, ty_generics, where_clause) = generics.split_for_impl();
    quote! {
        impl #impl_generics #name #ty_generics #where_clause {
            pub fn archive<R>(
                &self,
                f: impl FnOnce(::datapod::Archived<'_, Self>) -> R,
            ) -> ::core::result::Result<R, ::datapod::WireError>
            where
                Self: ::datapod::DataPodValidate,
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::archive(self, f)
            }

            pub fn with_archive<R>(
                &self,
                f: impl FnOnce(::datapod::Archived<'_, Self>) -> R,
            ) -> ::core::result::Result<R, ::datapod::WireError>
            where
                Self: ::datapod::DataPodValidate,
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::with_archive(self, f)
            }

            pub fn segmented_archive<R>(
                &self,
                f: impl FnOnce(::datapod::SegmentedArchived<'_, Self>) -> R,
            ) -> ::core::result::Result<R, ::datapod::WireError>
            where
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::segmented_archive(self, f)
            }

            pub fn view_archive<'a>(
                archive: ::datapod::Archived<'a, Self>,
            ) -> ::core::result::Result<
                <Self as ::datapod::DataPodAccess>::View<'a>,
                ::datapod::WireError,
            >
            where
                Self: ::datapod::DataPodAccess,
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::view_archive::<Self>(archive)
            }

            pub fn from_archive(
                archive: ::datapod::Archived<'_, Self>,
            ) -> ::core::result::Result<Self, ::datapod::WireError>
            where
                Self: ::datapod::DataPodDecode + ::datapod::DataPodValidate,
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::from_archive::<Self>(archive)
            }
        }
    }
}

fn inherent_canonical_api(
    name: &syn::Ident,
    generics: &Generics,
    canonical_name: Option<&LitStr>,
    payload_kind: TokenStream2,
    archive_shape: TokenStream2,
) -> TokenStream2 {
    let Some(canonical_name) = canonical_name else {
        return quote! {};
    };
    let type_hash = fnv1a_64(canonical_name.value().as_bytes());
    let (impl_generics, ty_generics, where_clause) = generics.split_for_impl();
    quote! {
        impl #impl_generics #name #ty_generics #where_clause {
            pub const CANONICAL_NAME: &'static str = #canonical_name;
            pub const TYPE_HASH: u64 = #type_hash;

            pub fn register_schema() -> ::core::result::Result<(), ::datapod::registry::RegistryError>
            where
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::registry::register_datapod_type::<Self>(
                    Self::CANONICAL_NAME,
                    #payload_kind,
                    #archive_shape,
                )
            }

            pub fn to_wire_message(&self) -> ::datapod::WireMessage
            where
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::to_wire_message(self)
            }

            pub fn with_wire_frame<R>(
                &self,
                f: impl FnOnce(::datapod::WireFrame<'_>) -> R,
            ) -> ::core::result::Result<R, ::datapod::WireError>
            where
                Self: ::datapod::DataPodValidate,
            {
                ::datapod::with_wire_frame(self, f)
            }

            pub fn validate_wire_frame(
                frame: ::datapod::WireFrame<'_>,
            ) -> ::core::result::Result<(), ::datapod::WireError>
            where
                Self: ::datapod::DataPodValidate,
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::validate_wire_frame::<Self>(frame)
            }

            pub fn view_from_wire_frame<'a>(
                frame: ::datapod::WireFrame<'a>,
            ) -> ::core::result::Result<
                <Self as ::datapod::DataPodAccess>::View<'a>,
                ::datapod::WireError,
            >
            where
                Self: ::datapod::DataPodAccess,
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::access_wire_frame::<Self>(frame)
            }

            pub fn from_wire_frame(
                frame: ::datapod::WireFrame<'_>,
            ) -> ::core::result::Result<Self, ::datapod::WireError>
            where
                Self: ::datapod::DataPodDecode + ::datapod::DataPodValidate,
                <Self as ::datapod::DataPod>::Header: ::datapod::LeWireHeader,
            {
                ::datapod::from_wire_frame::<Self>(frame)
            }
        }
    }
}

fn fnv1a_64(bytes: &[u8]) -> u64 {
    const OFFSET: u64 = 0xcbf29ce484222325;
    const PRIME: u64 = 0x00000100000001b3;
    let mut hash = OFFSET;
    for byte in bytes {
        hash ^= u64::from(*byte);
        hash = hash.wrapping_mul(PRIME);
    }
    hash
}

fn fixed_reserved_field_validation(fields: &Fields, _type_name: &syn::Ident) -> TokenStream2 {
    let Fields::Named(FieldsNamed { named, .. }) = fields else {
        return quote! {};
    };
    reserved_field_validation_for_fields(named.iter())
}

fn reserved_field_validation_for_fields<'a>(
    fields: impl IntoIterator<Item = &'a Field>,
) -> TokenStream2 {
    let checks = fields.into_iter().filter_map(|field| {
        let ident = field.ident.as_ref()?;
        if ident == "_pad" {
            let ty = &field.ty;
            Some(quote! {
                let __dp_reserved_zero: #ty = ::core::default::Default::default();
                if header.#ident != __dp_reserved_zero {
                    return Err(::datapod::WireError::InvalidHeader {
                        type_name: ::core::any::type_name::<Self>(),
                        message: "reserved _pad field must be zero".to_string(),
                    });
                }
            })
        } else {
            None
        }
    });
    quote! {
        #(#checks)*
    }
}

fn le_wire_impl_for_item_struct(
    name: &syn::Ident,
    generics: &Generics,
    fields: &Fields,
) -> Result<TokenStream2, Error> {
    let fields = match fields {
        Fields::Named(FieldsNamed { named, .. }) => named
            .iter()
            .map(|field| {
                Ok((
                    field.ident.clone().ok_or_else(|| {
                        Error::new(field.span(), "#[datapod] header field must be named")
                    })?,
                    field.ty.clone(),
                ))
            })
            .collect::<Result<Vec<_>, Error>>()?,
        Fields::Unit => Vec::new(),
        Fields::Unnamed(_) => {
            return Err(Error::new(
                name.span(),
                "#[datapod] stable little-endian header generation requires named fields",
            ));
        }
    };
    Ok(le_wire_impl_for_type(name, generics, &fields))
}

fn le_wire_impl_for_type(
    name: &syn::Ident,
    generics: &Generics,
    fields: &[(syn::Ident, Type)],
) -> TokenStream2 {
    let (impl_generics, ty_generics, where_clause) = generics.split_for_impl();
    let field_idents: Vec<_> = fields.iter().map(|(ident, _)| ident).collect();
    let field_tys: Vec<_> = fields.iter().map(|(_, ty)| ty).collect();
    quote! {
        impl #impl_generics ::datapod::LeWireHeader for #name #ty_generics #where_clause {
            const LE_WIRE_SIZE: usize = 0 #( + <#field_tys as ::datapod::LeWireHeader>::LE_WIRE_SIZE )*;

            fn write_le(&self, out: &mut ::std::vec::Vec<u8>) {
                #(
                    <#field_tys as ::datapod::LeWireHeader>::write_le(&self.#field_idents, out);
                )*
            }

            fn read_le(bytes: &[u8]) -> ::core::result::Result<Self, ::datapod::WireError> {
                if bytes.len() != Self::LE_WIRE_SIZE {
                    return Err(::datapod::WireError::InvalidHeader {
                        type_name: ::core::any::type_name::<Self>(),
                        message: ::std::format!(
                            "little-endian header has {} bytes, expected {}",
                            bytes.len(),
                            Self::LE_WIRE_SIZE,
                        ),
                    });
                }
                let mut __dp_offset = 0usize;
                Ok(Self {
                    #(
                        #field_idents: ::datapod::read_le_field::<#field_tys>(
                            bytes,
                            &mut __dp_offset,
                            ::core::any::type_name::<Self>(),
                        )?,
                    )*
                })
            }
        }
    }
}

fn find_bytes_fields(input: &ItemStruct) -> Result<Vec<BytesFieldInfo>, Error> {
    let fields = match &input.fields {
        Fields::Named(named) => &named.named,
        Fields::Unnamed(_) | Fields::Unit => return Ok(Vec::new()),
    };
    let infos: Result<Vec<_>, Error> = fields
        .iter()
        .enumerate()
        .filter_map(|(index, field)| match dp_bytes_section(field) {
            Ok(Some(section)) => Some(Ok(BytesFieldInfo { index, section })),
            Ok(None) => None,
            Err(error) => Some(Err(error)),
        })
        .collect();
    let infos = infos?;
    if infos.len() > 1 && infos.iter().any(|info| info.section.is_none()) {
        return Err(Error::new(
            fields[infos[1].index].span(),
            "multiple #[dp(bytes)] fields require #[dp(bytes, section = \"...\")]",
        ));
    }
    Ok(infos)
}

fn take_manual_access_attr(attrs: &mut Vec<Attribute>) -> Result<bool, Error> {
    let mut manual_access = false;
    let mut remove = Vec::new();
    for (index, attr) in attrs.iter().enumerate() {
        if !attr.path().is_ident("dp") {
            continue;
        }
        let mut attr_manual_access = false;
        attr.parse_nested_meta(|meta| {
            if meta.path.is_ident("manual_access") {
                attr_manual_access = true;
                Ok(())
            } else {
                Err(meta.error("unsupported #[dp(...)] item on struct; expected manual_access"))
            }
        })?;
        if attr_manual_access {
            manual_access = true;
            remove.push(index);
        }
    }
    for index in remove.into_iter().rev() {
        attrs.remove(index);
    }
    Ok(manual_access)
}

fn dp_bytes_section(field: &Field) -> Result<Option<Option<String>>, Error> {
    for attr in &field.attrs {
        if !attr.path().is_ident("dp") {
            continue;
        }
        let mut found = false;
        let mut section = None;
        attr.parse_nested_meta(|meta| {
            if meta.path.is_ident("bytes") {
                found = true;
                return Ok(());
            }
            if meta.path.is_ident("section") {
                let value = meta.value()?;
                let literal: LitStr = value.parse()?;
                section = Some(literal.value());
                return Ok(());
            }
            Err(meta.error("unsupported #[dp(...)] item on field; expected bytes or section"))
        })?;
        if found {
            return Ok(Some(section));
        }
    }
    Ok(None)
}

fn vec_element_type(ty: &Type) -> Result<Type, Error> {
    let Type::Path(path) = ty else {
        return Err(Error::new(
            ty.span(),
            "#[dp(bytes)] field must be Vec<T> with a Pod element type.",
        ));
    };
    let Some(segment) = path.path.segments.last() else {
        return Err(Error::new(
            ty.span(),
            "#[dp(bytes)] field must be Vec<T> with a Pod element type.",
        ));
    };
    if segment.ident != "Vec" {
        return Err(Error::new(
            ty.span(),
            "#[dp(bytes)] field must be Vec<T> with a Pod element type.",
        ));
    }
    let PathArguments::AngleBracketed(args) = &segment.arguments else {
        return Err(Error::new(
            ty.span(),
            "#[dp(bytes)] field must be Vec<T> with a Pod element type.",
        ));
    };
    let Some(GenericArgument::Type(inner)) = args.args.first() else {
        return Err(Error::new(
            ty.span(),
            "#[dp(bytes)] field must be Vec<T> with a Pod element type.",
        ));
    };
    Ok(inner.clone())
}

fn is_payload_section_type(ty: &Type) -> bool {
    let Type::Path(path) = ty else {
        return false;
    };
    path.path
        .segments
        .last()
        .is_some_and(|segment| segment.ident == "PayloadSection")
}

fn schema_fields_for_named_fields(
    fields: &Fields,
    payloads: &[(syn::Ident, Type)],
) -> Vec<TokenStream2> {
    let header_fields: Vec<(syn::Ident, Type)> = match fields {
        Fields::Named(FieldsNamed { named, .. }) => named
            .iter()
            .filter_map(|field| Some((field.ident.clone()?, field.ty.clone())))
            .collect(),
        _ => Vec::new(),
    };
    schema_fields_for_header_and_payload(&header_fields, payloads)
}

fn schema_fields_for_header_and_payload(
    header_fields: &[(syn::Ident, Type)],
    payloads: &[(syn::Ident, Type)],
) -> Vec<TokenStream2> {
    let mut fields = Vec::new();
    for (index, (ident, ty)) in header_fields.iter().enumerate() {
        let previous_tys = header_fields
            .iter()
            .take(index)
            .map(|(_, ty)| ty.clone())
            .collect::<Vec<_>>();
        let offset = header_offset_expr(&previous_tys);
        let field_type = schema_field_type_expr(ty);
        fields.push(quote! {
            ::datapod::schema::SchemaField {
                name: ::core::stringify!(#ident),
                role: ::datapod::schema::FieldRole::Header,
                offset: #offset,
                ty: #field_type,
            }
        });
    }
    for (ident, elem_ty) in payloads {
        let field_type = schema_payload_field_type_expr(elem_ty);
        fields.push(quote! {
            ::datapod::schema::SchemaField {
                name: ::core::stringify!(#ident),
                role: ::datapod::schema::FieldRole::Payload,
                offset: 0usize,
                ty: #field_type,
            }
        });
    }
    fields
}

fn header_offset_expr(previous_tys: &[Type]) -> TokenStream2 {
    let tys: Vec<_> = previous_tys.iter().collect();
    quote! { 0usize #( + <#tys as ::datapod::LeWireHeader>::LE_WIRE_SIZE )* }
}

fn schema_payload_field_type_expr(elem_ty: &Type) -> TokenStream2 {
    quote! {
        ::datapod::schema::FieldType::BytesElements {
            type_hash: ::datapod::schema::bytes_element_type_hash(
                <#elem_ty as ::datapod::schema::SchemaFieldType>::field_type(),
            ),
        }
    }
}

fn schema_field_type_expr(ty: &Type) -> TokenStream2 {
    if is_payload_section_type(ty) {
        return quote! { ::datapod::schema::FieldType::PayloadSection };
    }
    quote! { <#ty as ::datapod::schema::SchemaFieldType>::field_type() }
}

fn has_repr_c(attrs: &[Attribute]) -> bool {
    attrs.iter().any(|attr| {
        if !attr.path().is_ident("repr") {
            return false;
        }
        let mut found = false;
        let _ = attr.parse_nested_meta(|meta| {
            if meta.path.is_ident("C") {
                found = true;
            }
            Ok(())
        });
        found
    })
}

fn parse_repr_c() -> Attribute {
    syn::parse_quote!(#[repr(C)])
}

// ---------------------------------------------------------------------------
// #[derive(DataPod)] — explicit form, only emits the trait impl.
//
// The user is responsible for writing `#[repr(C)]` + Pod/Zeroable derives
// themselves. We trust them; the type-system check on `Pod + Zeroable +
// ZeroCopySend` ensures they got it right.
// ---------------------------------------------------------------------------

#[proc_macro_derive(DataPod, attributes(dp))]
pub fn derive_data_pod(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);
    expand_derive(&input)
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

// ---------------------------------------------------------------------------
// #[derive(ZeroCopySend)] — emits the marker impl for datapod's own trait.
//
// Stands in for iceoryx2's old `#[derive(ZeroCopySend)]` so the explicit
// `#[derive(..., ZeroCopySend, DataPod)]` form keeps working without an
// iceoryx2 dependency. `ZeroCopySend` is an empty `unsafe` marker, so the
// impl body is empty; the `Pod + Zeroable` bounds on `DataPod::Header`
// are what actually enforce the wire-shippable contract.
// ---------------------------------------------------------------------------

#[proc_macro_derive(ZeroCopySend)]
pub fn derive_zero_copy_send(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);
    let name = &input.ident;
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();
    quote! {
        unsafe impl #impl_generics ::datapod::ZeroCopySend for #name #ty_generics #where_clause {}
    }
    .into()
}

fn expand_derive(input: &DeriveInput) -> Result<TokenStream2, Error> {
    let name = &input.ident;
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();

    let (le_wire_impl, reserved_field_validation) = match &input.data {
        Data::Struct(data) => (
            le_wire_impl_for_item_struct(name, &input.generics, &data.fields)?,
            fixed_reserved_field_validation(&data.fields, name),
        ),
        Data::Enum(e) => {
            return Err(Error::new(
                e.enum_token.span(),
                "#[derive(DataPod)] only supports structs. For enums, use \
                 #[repr(uN)] and implement DataPod manually.",
            ));
        }
        Data::Union(u) => {
            return Err(Error::new(
                u.union_token.span(),
                "#[derive(DataPod)] does not support unions.",
            ));
        }
    };

    // The derive-only path covers the fixed-Pod case. Header = Self.
    Ok(quote! {
        impl #impl_generics ::datapod::DataPod for #name #ty_generics #where_clause {
            type Header = #name #ty_generics;
            type Payload = ();
            fn header(&self) -> Self::Header { *self }
            fn payload_bytes(&self) -> &[u8] { &[] }
        }

        impl #impl_generics ::datapod::DataPodDecode for #name #ty_generics #where_clause {
            fn from_wire_parts(
                header: <Self as ::datapod::DataPod>::Header,
                payload: ::std::vec::Vec<u8>,
            ) -> ::core::result::Result<Self, ::datapod::WireError> {
                <Self as ::datapod::DataPodValidate>::validate_wire_parts(&header, &payload)?;
                Ok(header)
            }
        }

        impl #impl_generics ::datapod::DataPodValidate for #name #ty_generics #where_clause {
            fn validate_wire_parts(
                header: &<Self as ::datapod::DataPod>::Header,
                payload: &[u8],
            ) -> ::core::result::Result<(), ::datapod::WireError> {
                if !payload.is_empty() {
                    return Err(::datapod::WireError::InvalidPayloadSize {
                        type_name: ::core::any::type_name::<Self>(),
                        message: ::std::format!(
                            "fixed datapod payload must be empty, got {}",
                            payload.len()
                        ),
                    });
                }
                #reserved_field_validation
                Ok(())
            }
        }

        #le_wire_impl

        impl #impl_generics ::datapod::DataPodAccess for #name #ty_generics #where_clause {
            type View<'a> = ::datapod::FixedView<Self> where Self: 'a;

            fn access_wire_parts<'a>(
                header: <Self as ::datapod::DataPod>::Header,
                payload: &'a [u8],
            ) -> ::core::result::Result<Self::View<'a>, ::datapod::WireError> {
                <Self as ::datapod::DataPodValidate>::validate_wire_parts(&header, payload)?;
                Ok(::datapod::FixedView { value: header })
            }

            unsafe fn access_wire_parts_unchecked<'a>(
                header: <Self as ::datapod::DataPod>::Header,
                _payload: &'a [u8],
            ) -> Self::View<'a> {
                ::datapod::FixedView { value: header }
            }
        }
    })
}
