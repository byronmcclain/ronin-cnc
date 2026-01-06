use std::env;
use std::path::PathBuf;

fn main() {
    // Only generate headers when explicitly requested via feature flag
    #[cfg(feature = "cbindgen-run")]
    generate_headers();

    // Tell Cargo to re-run if these change
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=cbindgen.toml");
}

#[cfg(feature = "cbindgen-run")]
fn generate_headers() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let output_file = PathBuf::from(&crate_dir)
        .parent()
        .unwrap()
        .join("include")
        .join("platform.h");

    // Create include directory if needed
    if let Some(parent) = output_file.parent() {
        std::fs::create_dir_all(parent).unwrap();
    }

    let config = cbindgen::Config::from_file("cbindgen.toml")
        .expect("Failed to read cbindgen.toml");

    cbindgen::Builder::new()
        .with_crate(&crate_dir)
        .with_config(config)
        .generate()
        .expect("Failed to generate bindings")
        .write_to_file(&output_file);

    println!("cargo:warning=Generated: {}", output_file.display());
}
