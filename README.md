# YCSB-C-SGX

## Build for native

```
make native
```

## Build for SGX

```
make clean
make SGX_MODE=HW SGX_DEBUG=0 SGX_PRERELEASE=1
```

## Run

```
./app -db sqlite -P YCSB-C/workloads/workloada.spec
```
