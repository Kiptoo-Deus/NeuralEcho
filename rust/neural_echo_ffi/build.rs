fn main() {
    let crate_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    let out_dir   = std::path::PathBuf::from(&crate_dir).join("include");
    std::fs::create_dir_all(&out_dir).unwrap();

    cbindgen::Builder::new()
        .with_crate(&crate_dir)
        .with_language(cbindgen::Language::C)
        .with_include_guard("NEURAL_ECHO_FFI_H")
        .with_documentation(true)
        .generate()
        .expect("Unable to generate C bindings")
        .write_to_file(out_dir.join("neural_echo_ffi.h"));
}
