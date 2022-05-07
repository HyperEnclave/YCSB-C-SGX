#!/bin/bash

cd $(dirname "$0")

app=$1
res_dir=results-$2
op_count=100000
repeat_num=5
workload_dir=./YCSB-C/workloads
workload=$workload_dir/workloada.spec
rec_counts="5 10 20 40 60 80 100 120 140 160 180 200"

mkdir -p "$res_dir"

for n in $rec_counts
do
    rec=$(expr $n \* 1000)
    tput_arr=
    for ((i=1; i<=repeat_num; ++i)); do
        echo "Record count: $rec ($i):"
        resfile=$res_dir/${rec}_$i.res
        $app -db sqlite -P $workload -n $rec -t $op_count > $resfile
        
        tput=$(tail -n1 $resfile)
        tput_arr="$tput_arr $tput"
        echo "Throughput (KTPS): $tput"
        echo "$rec,$tput" >> $res_dir/summary.txt
    done
    median=$(echo $tput_arr | tr " " "\n" | sort -n | awk '{a[NR]=$0}END{if(NR%2==1)print a[int(NR/2)+1];else print(a[NR/2+1]+a[NR/2])/2}')
    echo $median
    echo "$rec,$median" >> $res_dir/median.txt
done
