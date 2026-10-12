def test_main_returns_zero():
    from hive_client import main

    assert main(["--host", "127.0.0.1", "--port", "8080"]) == 0
