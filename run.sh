#/bin/bash

app=$1
repeat_num=5
workload_dir=./YCSB/workloads

mv result.output result.output.bak
for file_name in $workload_dir/workload*.spec; do
    for ((i=1; i<=repeat_num; ++i)); do
        echo "Running $i: $file_name"
        $app -db sqlite -P $file_name 2>> result.output
    done
    echo "" >> result.output
done
