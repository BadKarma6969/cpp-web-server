#!/usr/bin/env bash

REQUESTS=500

start=$(date +%s%N)

for ((i=1; i<=REQUESTS; i++)); do
    curl -s http://localhost:8080 > /dev/null &
done

wait

end=$(date +%s%N)

elapsed_ns=$((end - start))

elapsed=$(awk "BEGIN {printf \"%.3f\", $elapsed_ns / 1000000000}")
throughput=$(awk "BEGIN {printf \"%.0f\", $REQUESTS / ($elapsed_ns / 1000000000)}")

echo "Requests: $REQUESTS"
echo "Elapsed: ${elapsed}s"
echo "Approx. throughput: ${throughput} req/s"