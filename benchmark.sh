#!/bin/bash

function usage() {
    echo "Usage: $0 [OPTIONS] <APP> <TAG>"
    echo ""
    echo "Options:"
    echo "    -h                      Display this message"
    echo "    -r <NUMBER>             Number of runs (default: 10)"
    echo "    -t <OP_COUNT>           Number of operations (default: 100000)"
    echo ""
    echo "Arguments:"
    echo "    <APP>                   Executable file"
    echo "    <TAG>                   Tag of this evalutaion"
    echo ""
}

while getopts "r:t:h" opt
do
   case "$opt" in
      r) runs="$OPTARG" ;;
      t) op_count="$OPTARG" ;;
      h | ?)
        usage
        exit 0
        ;;
   esac
done

shift $((OPTIND - 1))
if [ -z "$1" -o -z "$2" ]; then
    usage
    exit 0
fi

if [ -z "$runs" ]; then
    runs=10
fi

if [ -z "$op_count" ]; then
    op_count=100000
fi

app=$1
res_dir=result-$2-$(date +%y%m%d-%H%M%S)
rec_counts="5 10 20 40 60 80 100 120 140 160 180 200"
workload=./YCSB-C/workloads/workloada.spec

declare -A all_tput

mkdir -p "$res_dir"

for ((i=1; i<=$runs; i++))
do
    for n in $rec_counts
    do
        rec_count=$(expr $n \* 1000)
        resfile=$res_dir/${rec_count}_$i.res

        $app -db sqlite -P $workload -n $rec_count -t $op_count > $resfile || exit $?
        
        tput=$(tail -n1 $resfile)
        all_tput[$n]="${all_tput[$n]} $tput"
        echo "[$i] Record count: $rec_count, Throughput (KTPS): $tput"
        sleep 2
    done
done

function stats() {
    list=$(echo "$*" | tr " " ",")
    median=$(echo "$*" | tr " " "\n" | sort -n | awk '{a[NR]=$0} END {if(NR%2==1) print a[int(NR/2)+1]; else print(a[NR/2+1]+a[NR/2])/2}')
    average=$(echo "$*" | tr " " "\n" | awk '{sum+=$0; n++} END {print sum/n}')
}

echo "rec_count,$(seq -s, $runs),average,median" > $res_dir/summary_throughput.csv
for n in $rec_counts
do
    stats ${all_tput[$n]}
    echo "$(expr $n \* 1000),$list,$average,$median" >> $res_dir/summary_throughput.csv
done

echo ""
echo "Summary (Throughput):"
cat $res_dir/summary_throughput.csv
