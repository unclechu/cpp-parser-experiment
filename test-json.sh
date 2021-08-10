#! /usr/bin/env bash

set -Eeuo pipefail


set -x
JSON_TO_TEST=$(jq -S < /dev/stdin)
set +x

if (( $# == 0 )); then
	set -x
	REFERENCE_JSON=$(jq -S < example.json)
	set +x
elif (( $# == 1 )) && [[ $1 == --model ]]; then
	set -x
	REFERENCE_JSON=$(
		jq -S \
			'to_entries | map(select(.key != "spouse" and .key != "children")) | from_entries' \
			< example.json
	)
	set +x
else
	>&2 echo Incorrect arguments
	exit 1
fi

if ! diff -- <(printf '%s' "$JSON_TO_TEST") <(printf '%s' "$REFERENCE_JSON")
then
	>&2 echo \
		'The difference between parsed+serialized JSON' \
		'and reference JSON is not empty!'
	exit 1
fi

echo 'The test was successful'
