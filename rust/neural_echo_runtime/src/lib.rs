use std::sync::OnceLock;
use tokio::runtime::Runtime;
use tracing::info;

pub mod session;

/// Global Tokio runtime — initialised once via `ne_runtime_init()`.
static RT: OnceLock<Runtime> = OnceLock::new();

/// Initialise the global Tokio runtime.
/// Must be called before any other `ne_*` FFI function.
pub fn init_runtime() {
    tracing_subscriber::fmt::init();
    RT.get_or_init(|| {
        info!("Initialising NeuralEcho Tokio runtime");
        Runtime::new().expect("Failed to create Tokio runtime")
    });
}

/// Return a reference to the global runtime.
pub fn runtime() -> &'static Runtime {
    RT.get().expect("NeuralEcho runtime not initialised — call ne_runtime_init() first")
}
