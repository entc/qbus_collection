#!/bin/sh

# run benchmark
wrk -t4 -c100 -d30s http://127.0.0.1:8000/
