"""受控时钟验证点击到计算/运动结束的边界，不触碰真实设备。"""
import json
from types import SimpleNamespace
from unittest.mock import Mock

import pytest

from ec_26_e.gui.board_menu import BoardMenu
from ec_26_e.tasks import timing
from ec_26_e.tasks.contracts import TaskCancelledError


@pytest.fixture
def clock(tmp_path, monkeypatch):
    value = SimpleNamespace(now=100.)
    monkeypatch.setattr(timing.time, 'monotonic', lambda: value.now)
    monkeypatch.setenv('EC_TASK_LAUNCHED_MONOTONIC', '90')
    monkeypatch.setenv('EC_TASK_DISPLAY_ID', 'current')
    monkeypatch.setenv('EC_TASK_DISPLAY_DIR', str(tmp_path))
    return value


def report(tmp_path):
    return json.loads((tmp_path/'current.timing.json').read_text())


def test_compute_boundary_and_total_include_click_launch_and_motion(clock, tmp_path):
    @timing.record_task_timing
    def task():
        clock.now = 105.
        timing.calculation_finished()
        assert report(tmp_path) == dict(token='current', state='executing',
                                       calculation_seconds=15., total_seconds=15.)
        clock.now = 126.
        return 'plan'
    assert task() == 'plan'
    assert report(tmp_path) == dict(token='current', state='complete',
                                   calculation_seconds=15., total_seconds=36.)
    clock.now = 200.
    assert report(tmp_path)['total_seconds'] == 36.  # 结束后保持，不随菜单继续增长。


@pytest.mark.parametrize('error', [KeyboardInterrupt(), TaskCancelledError('cancel'), RuntimeError('fault')])
@pytest.mark.parametrize('motion_started', [False, True])
def test_cancel_or_failure_is_never_reported_as_complete(clock, tmp_path, error, motion_started):
    @timing.record_task_timing
    def task():
        if motion_started:
            clock.now = 104.
            timing.calculation_finished()
        clock.now = 110.
        raise error
    with pytest.raises(type(error)):
        task()
    actual = report(tmp_path)
    assert actual['state'] == ('failed' if isinstance(error, RuntimeError) and not isinstance(error, TaskCancelledError) else 'cancelled')
    assert actual['calculation_seconds'] == (14. if motion_started else None)
    assert actual['total_seconds'] == 20.
    assert timing._current_timer.get() is None


def test_plan_only_and_second_task_do_not_reuse_old_timing(clock, tmp_path, monkeypatch):
    @timing.record_task_timing
    def task():
        clock.now += 2.
    task()
    assert report(tmp_path)['calculation_seconds'] == 12.
    monkeypatch.setenv('EC_TASK_LAUNCHED_MONOTONIC', '102')
    task()
    assert report(tmp_path)['calculation_seconds'] == 2.
    assert report(tmp_path)['total_seconds'] == 2.


@pytest.mark.parametrize('launch', ['nan', 'inf', '-1', '9999', 'invalid'])
def test_invalid_start_uses_current_monotonic_time(clock, monkeypatch, launch):
    monkeypatch.setenv('EC_TASK_LAUNCHED_MONOTONIC', launch)
    assert timing.TaskTimer().started == 100.


def test_timing_write_failure_cannot_stop_task_or_replace_original_error(clock, monkeypatch):
    monkeypatch.setattr(timing.Path, 'mkdir', Mock(side_effect=OSError('read-only')))
    execute = Mock()
    @timing.record_task_timing
    def task():
        timing.calculation_finished()
        execute()
    task()
    execute.assert_called_once()


def test_menu_freezes_both_times_and_ignores_old_token(clock, tmp_path):
    menu = BoardMenu.__new__(BoardMenu)
    menu.display_directory, menu.display_token = tmp_path, 'current'
    menu._task_requested_at = 90.
    menu.timing_report = menu._timing_text = None
    menu.model, menu.timing = SimpleNamespace(busy=True), Mock()
    path = tmp_path/'current.timing.json'
    path.write_text(json.dumps(dict(token='old', state='complete', calculation_seconds=999, total_seconds=999)))
    menu.poll_timing()
    menu.timing.configure.assert_called_with(text='计算时间：计算中    总时间：10.00 s')
    path.write_text(json.dumps(dict(token='current', state='complete', calculation_seconds=12., total_seconds=35.)))
    menu.model.busy = False
    menu.poll_timing()
    menu.timing.configure.assert_called_with(text='计算时间：12.00 s    总时间：35.00 s')
    menu.timing.configure.reset_mock()
    clock.now = 500.
    menu.poll_timing()
    menu.timing.configure.assert_not_called()


def test_repeated_click_during_motion_does_not_reset_start(clock):
    menu = BoardMenu.__new__(BoardMenu)
    menu.transition = None
    menu.model, menu.camera = SimpleNamespace(busy=True), Mock(active=False)
    menu._task_requested_at, menu.events = 90., []
    menu._apply_action = Mock()
    menu.action('confirm', 'mouse')
    assert menu._task_requested_at == 90.


def test_spawn_preserves_click_before_camera_release_in_child_environment(clock, tmp_path, monkeypatch):
    menu = BoardMenu.__new__(BoardMenu)
    menu._task_requested_at, menu.display_directory = 90., tmp_path
    spawn = Mock()
    monkeypatch.setattr('ec_26_e.gui.board_menu.subprocess.Popen', spawn)
    menu._spawn_task(['python', 'task'], {'EC_TASK_LAUNCHED_MONOTONIC': '999'})
    environment = spawn.call_args.kwargs['env']
    assert environment['EC_TASK_LAUNCHED_MONOTONIC'] == '90.0'
    assert environment['EC_TASK_DISPLAY_ID'] == menu.display_token
    assert menu.timing_report is None
