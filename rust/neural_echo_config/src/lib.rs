pub mod schema;
pub use schema::*;

use anyhow::Result;
use std::path::Path;

pub fn load(path: impl AsRef<Path>) -> Result<NeuralEchoConfig> {
    let text = std::fs::read_to_string(path)?;
    let cfg: NeuralEchoConfig = toml::from_str(&text)?;
    Ok(cfg)
}

pub fn load_or_default(path: impl AsRef<Path>) -> NeuralEchoConfig {
    load(path).unwrap_or_default()
}
