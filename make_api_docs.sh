#!/bin/bash
cd "$(dirname "$0")"
ldoc -f backticks -p tove ../src/lua  -d api
