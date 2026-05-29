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
//! ## Heap-bearing case (one `#[dp(bytes)]` field of type `Vec<T>`)
//!
//! - The user struct **keeps** every field exactly as written. The
//!   `#[dp(bytes)]` field is the heap-owned data buffer.
//! - The user struct is **not** `bytemuck::Pod` (it has a `Vec` inside);
//!   it derives `Debug/Clone/PartialEq` and any other derives the user
//!   added.
//! - The macro **generates a sibling `<Original>Header` struct** containing
//!   every NON-bytes field, copied verbatim. That header IS `#[repr(C)]`,
//!   Pod, Zeroable, ZeroCopySend — i.e. wire-shippable.
//! - `impl DataPod for Original`:
//!     - `type Header = <Original>Header`,
//!     - `type Payload = [u8]`,
//!     - `fn header(&self)` copies the non-bytes fields into a new header,
//!     - `fn payload_bytes(&self)` returns `bytemuck::cast_slice(&self.<bytes_field>)`.

use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use quote::{format_ident, quote};
use syn::{
    Attribute, Data, DeriveInput, Error, Field, Fields, FieldsNamed, ItemStruct, Visibility,
    parse_macro_input, spanned::Spanned,
};

#[proc_macro_attribute]
pub fn datapod(_args: TokenStream, input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as ItemStruct);
    expand_attribute(input)
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

fn expand_attribute(mut input: ItemStruct) -> Result<TokenStream2, Error> {
    let name = input.ident.clone();

    // Find the #[dp(bytes)] field if any.
    let bytes_info = find_bytes_field(&input)?;

    match bytes_info {
        None => expand_fixed(input),
        Some(bytes_field_index) => expand_heap(&mut input, bytes_field_index, &name),
    }
}

// ---------------------------------------------------------------------------
// Fixed-Pod case: struct itself is the wire layout.
// ---------------------------------------------------------------------------

fn expand_fixed(mut input: ItemStruct) -> Result<TokenStream2, Error> {
    let name = input.ident.clone();
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();

    if !has_repr_c(&input.attrs) {
        input.attrs.push(parse_repr_c());
    }

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
            type Header = #name #ty_generics;
            type Payload = ();
            fn header(&self) -> Self::Header { *self }
            fn payload_bytes(&self) -> &[u8] { &[] }
        }
    })
}

// ---------------------------------------------------------------------------
// Heap-bearing case: data inside the user struct, header generated alongside.
// ---------------------------------------------------------------------------

fn expand_heap(
    input: &mut ItemStruct,
    bytes_field_index: usize,
    name: &syn::Ident,
) -> Result<TokenStream2, Error> {
    // Header name: `<Original>Header`.
    let header_name = format_ident!("{}Header", name);

    // Strip the `#[dp(bytes)]` attribute from the bytes field on the user
    // struct so it doesn't end up in the emitted source (the macro consumed
    // its meaning).
    let bytes_field_ident: syn::Ident = match &mut input.fields {
        Fields::Named(named) => {
            let f = named.named.iter_mut().nth(bytes_field_index).unwrap();
            f.attrs.retain(|a| !a.path().is_ident("dp"));
            f.ident.clone().unwrap()
        }
        _ => {
            return Err(Error::new(
                input.span(),
                "#[datapod] heap-bearing structs require named fields.",
            ));
        }
    };

    // Collect the non-bytes named fields — they become the header struct.
    let header_fields: Vec<&Field> = match &input.fields {
        Fields::Named(FieldsNamed { named, .. }) => named
            .iter()
            .enumerate()
            .filter(|(i, _)| *i != bytes_field_index)
            .map(|(_, f)| f)
            .collect(),
        _ => unreachable!(),
    };

    // Build the generated header struct: same fields (cleaned of `#[dp(...)]`),
    // public visibility, #[repr(C)] + Pod derives.
    let header_field_defs = header_fields.iter().map(|f| {
        let ident = f.ident.as_ref().unwrap();
        let ty = &f.ty;
        let vis = match &f.vis {
            Visibility::Public(_) => quote!(pub),
            _ => quote!(pub),
        };
        quote! { #vis #ident: #ty }
    });

    // header() method body: copy each non-bytes field by value.
    let header_field_copies = header_fields.iter().map(|f| {
        let ident = f.ident.as_ref().unwrap();
        quote! { #ident: self.#ident }
    });

    // Outer visibility of the original struct → reuse for the header.
    let vis = match &input.vis {
        Visibility::Public(_) => quote!(pub),
        _ => quote!(pub),
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

        impl ::datapod::DataPod for #name {
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
    })
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

fn find_bytes_field(input: &ItemStruct) -> Result<Option<usize>, Error> {
    let fields = match &input.fields {
        Fields::Named(named) => &named.named,
        Fields::Unnamed(_) | Fields::Unit => return Ok(None),
    };
    let mut indices: Vec<usize> = fields
        .iter()
        .enumerate()
        .filter(|(_, f)| has_dp_bytes(f))
        .map(|(i, _)| i)
        .collect();
    if indices.len() > 1 {
        return Err(Error::new(
            fields[indices[1]].span(),
            "#[datapod] allows at most one #[dp(bytes)] field per struct.",
        ));
    }
    Ok(indices.pop())
}

fn has_dp_bytes(field: &Field) -> bool {
    field.attrs.iter().any(|attr| {
        if !attr.path().is_ident("dp") {
            return false;
        }
        let mut found = false;
        let _ = attr.parse_nested_meta(|meta| {
            if meta.path.is_ident("bytes") {
                found = true;
            }
            Ok(())
        });
        found
    })
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

fn expand_derive(input: &DeriveInput) -> Result<TokenStream2, Error> {
    let name = &input.ident;
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();

    match &input.data {
        Data::Struct(_) => {}
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
    }

    // The derive-only path covers the fixed-Pod case. Header = Self.
    Ok(quote! {
        impl #impl_generics ::datapod::DataPod for #name #ty_generics #where_clause {
            type Header = #name #ty_generics;
            type Payload = ();
            fn header(&self) -> Self::Header { *self }
            fn payload_bytes(&self) -> &[u8] { &[] }
        }
    })
}
