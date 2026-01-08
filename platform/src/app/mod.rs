//! Application management for Red Alert
//!
//! Provides:
//! - Application lifecycle state tracking
//! - Background/foreground transitions
//! - Quit request handling

pub mod lifecycle;

pub use lifecycle::{
    AppState,
    get_app_state,
    set_app_state,
    is_app_active,
    is_app_background,
    request_quit,
    should_quit,
    clear_quit_request,
    handle_app_event,
};

// Re-export FFI functions for cbindgen
pub use lifecycle::{
    Platform_GetAppState,
    Platform_IsAppActive,
    Platform_IsAppBackground,
    Platform_RequestQuit,
    Platform_ShouldQuit,
    Platform_ClearQuitRequest,
};
