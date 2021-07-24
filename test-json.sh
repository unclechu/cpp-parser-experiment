#! /usr/bin/env bash

set -Eeuo pipefail

set -x
JSON_TO_TEST=$(jq -S < /dev/stdin)
REFERENCE_JSON=$(jq -S < example.json)
set +x

if ! diff -- <(printf '%s' "$JSON_TO_TEST") <(printf '%s' "$REFERENCE_JSON")
then
	>&2 echo \
		'The difference between parsed+serialized JSON' \
		'and reference JSON is not empty!'
	exit 1
fi

echo 'The test was successful'
