"""从菜单点击起计时；计时报告只用于显示，不能影响运动。"""
from __future__ import annotations

from contextvars import ContextVar
from functools import wraps
import json
import logging
import math
import os
from pathlib import Path
import re
import time

from ec_26_e.tasks.contracts import TaskCancelledError

logger = logging.getLogger(__name__)
_current_timer = ContextVar('task_timer', default=None)


class TaskTimer:
    def __init__(self):
        now = time.monotonic()
        try:
            launched = float(os.environ.get('EC_TASK_LAUNCHED_MONOTONIC', now))
        except ValueError:
            launched = now
        self.started = launched if math.isfinite(launched) and 0 <= launched <= now else now
        self.calculation_seconds = None
        self.path = None
        self.token = os.environ.get('EC_TASK_DISPLAY_ID', '')
        directory = os.environ.get('EC_TASK_DISPLAY_DIR')
        if directory and re.fullmatch(r'[a-zA-Z0-9_-]{1,64}', self.token):
            self.path = Path(directory) / (self.token + '.timing.json')

    def publish(self, state):
        report = dict(token=self.token, state=state,
                      calculation_seconds=self.calculation_seconds,
                      total_seconds=time.monotonic() - self.started)
        if self.path is not None:
            try:
                self.path.parent.mkdir(parents=True, exist_ok=True)
                temporary = self.path.with_suffix('.json.tmp')
                temporary.write_text(json.dumps(report), encoding='utf-8')
                temporary.replace(self.path)
            except Exception:
                logger.exception('任务计时报告写入失败')
        if state in ('complete', 'cancelled', 'failed'):
            logger.info('TASK_TIMING %s', json.dumps(report))

    def calculation_finished(self):
        if self.calculation_seconds is None:
            self.calculation_seconds = time.monotonic() - self.started
            self.publish('executing')


def calculation_finished():
    """完整方案通过检查后、交给现有执行器之前固定计算耗时。"""
    timer = _current_timer.get()
    if timer is not None:
        timer.calculation_finished()


def record_task_timing(function):
    @wraps(function)
    def timed(*args, **kwargs):
        timer = TaskTimer()
        token = _current_timer.set(timer)
        try:
            timer.publish('calculating')
            result = function(*args, **kwargs)
        except BaseException as error:
            timer.publish('cancelled' if isinstance(error, (KeyboardInterrupt, TaskCancelledError)) else 'failed')
            raise
        else:
            timer.calculation_finished()
            timer.publish('complete')
            return result
        finally:
            _current_timer.reset(token)
    return timed
