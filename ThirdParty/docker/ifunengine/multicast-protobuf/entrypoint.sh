#!/bin/sh
service zookeeper start
sleep 1
/home/test/test-build/debug/test-local -session_message_logging_level=2