#!/usr/bin/env bash
src/invent -a -p json | jq -r '.[] | .bin'|sort -u
