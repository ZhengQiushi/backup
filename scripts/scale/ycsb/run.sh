/home/star/bench_ycsb --logtostderr=1 --id=8 --servers="10.77.70.143:22010;10.77.70.145:22010;10.77.70.252:22010;10.77.70.253:22010;10.77.70.111:22010;10.77.70.113:22010;10.77.70.114:22010;10.77.70.203:22010;10.77.110.144:22010" --protocol=Lion --partition_num=20 --partitioner=Lion --threads=2 --batch_size=10000 --batch_flush=500   --cross_ratio=100 --lion_with_metis_init=0 --time_to_run=60   --sample_time_interval=3 --migration_only=0 --n_nop=0 --v=5 --data_src_path_dir="/home/star/data/ycsb/ips8/"  --read_on_replica=true  --random_router=0 --workload_time=30 --lion_self_remaster=0  --v=2 --skew_factor=0 --lion_with_trace_log=true