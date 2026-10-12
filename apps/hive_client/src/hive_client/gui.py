"""Raylib-based GUI for the Hive client."""

from pyray import (
    DARKGRAY,
    RAYWHITE,
    ConfigFlags,
    begin_drawing,
    clear_background,
    close_window,
    draw_text,
    end_drawing,
    init_window,
    set_config_flags,
    set_target_fps,
    window_should_close,
)

__all__ = ["run"]

DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 600
TARGET_FPS = 60


def run(
    host: str = "127.0.0.1",
    port: int = 8080,
    width: int = DEFAULT_WIDTH,
    height: int = DEFAULT_HEIGHT,
) -> None:
    """Open the client window and run the render loop (stub)."""
    set_config_flags(ConfigFlags.FLAG_MSAA_4X_HINT | ConfigFlags.FLAG_WINDOW_RESIZABLE)
    init_window(width, height, f"Hive Game ({host}:{port})")
    set_target_fps(TARGET_FPS)

    while not window_should_close():
        begin_drawing()
        clear_background(RAYWHITE)
        draw_text("Hive client (stub)", 190, 200, 20, DARKGRAY)
        end_drawing()

    close_window()
