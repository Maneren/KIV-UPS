def test_main_launches_gui(monkeypatch):
    import hive_client

    launched = {}
    monkeypatch.setattr(hive_client, "run", lambda **kwargs: launched.update(kwargs))
    assert hive_client.main(["--host", "127.0.0.1", "--port", "8080"]) == 0
    assert launched == {"host": "127.0.0.1", "port": 8080}


def test_gui_module_imports():
    import hive_client.gui

    assert callable(hive_client.gui.run)
