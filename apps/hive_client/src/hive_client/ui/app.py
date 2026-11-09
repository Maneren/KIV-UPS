"""raylib window loop for the Hive client."""

from __future__ import annotations

from pyray import (
    ConfigFlags,
    MouseButton,
    close_window,
    get_mouse_position,
    get_screen_height,
    get_screen_width,
    init_window,
    is_key_pressed,
    is_mouse_button_pressed,
    is_mouse_button_released,
    set_config_flags,
    set_target_fps,
    window_should_close,
)

from hive_client.app.controller import GameController
from hive_client.ui.input import MouseEvent, MouseRouter
from hive_client.ui.renderer import HexRenderer, RenderContext
from hive_client.ui.theme import TUTORIAL_TOGGLE_KEY

__all__ = ["HiveGUI", "run"]


class HiveGUI:
    """Owns a controller and drives the raylib render loop."""

    def __init__(
        self,
        width: int = 800,
        height: int = 600,
        controller: GameController | None = None,
    ) -> None:
        """Create the window configuration and wire up the MVC pieces."""
        self.width = width
        self.height = height
        self.controller = controller or GameController()
        self.renderer = HexRenderer()
        self.mouse_router = MouseRouter(self.controller)
        self.tutorial_visible = True

    def run(self) -> None:
        """Open the client window and run the render loop."""
        set_config_flags(
            ConfigFlags.FLAG_MSAA_4X_HINT | ConfigFlags.FLAG_WINDOW_RESIZABLE,
        )
        init_window(self.width, self.height, "Hive Game")
        set_target_fps(60)

        while not window_should_close():
            self.controller.advance()

            width = float(get_screen_width())
            height = float(get_screen_height())
            center = (width * 0.5, height * 0.5)

            if is_key_pressed(TUTORIAL_TOGGLE_KEY):
                self.tutorial_visible = not self.tutorial_visible

            mouse = get_mouse_position()
            event = MouseEvent(
                pressed=is_mouse_button_pressed(MouseButton.MOUSE_BUTTON_LEFT),
                released=is_mouse_button_released(MouseButton.MOUSE_BUTTON_LEFT),
                position=(mouse.x, mouse.y),
            )
            self.mouse_router.handle(event, center)

            self.renderer.draw(
                RenderContext(
                    view=self.controller.view(),
                    width=width,
                    height=height,
                    center=center,
                    mouse=(mouse.x, mouse.y),
                    tutorial_visible=self.tutorial_visible,
                ),
            )

        close_window()


def run(
    _host: str = "127.0.0.1",
    _port: int = 8080,
    width: int = 800,
    height: int = 600,
) -> None:
    """
    Open the client window and run the render loop.

    ``host``/``port`` are accepted for future network integration; the GUI
    currently runs in local (hot-seat) mode.
    """
    HiveGUI(width, height).run()
