// #ifdef __APPLE__
// #include <algorithm>
// #include <atomic>
// #include <chrono>
// #include <random>
// #include <thread>
// #include <unordered_map>
// #include <vector>
// #include <set>


// #include <mach/mach_types.h>
// #include <mach/thread_policy.h>
// #include <mach/thread_act.h>


// #include <sys/types.h>
// #include <sys/sysctl.h>

// #define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

// typedef struct cpu_set {
//     uint32_t    count;
// } cpu_set_t;

// static inline void
// CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

// static inline void
// CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

// static inline int
// CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

// int sched_getaffinity(pid_t pid, size_t cpu_size, cpu_set_t *cpu_set)
// {
//   int32_t core_count = 0;
//   size_t  len = sizeof(core_count);
//   int ret = sysctlbyname(SYSCTL_CORE_COUNT, &core_count, &len, 0, 0);
//   if (ret) {
//     printf("error while get core count %d\n", ret);
//     return -1;
//   }
//   cpu_set->count = 0;
//   for (int i = 0; i < core_count; i++) {
//     cpu_set->count |= (1 << i);
//   }

//   return 0;
// }

// int pthread_setaffinity_np(pthread_t thread, size_t cpu_size,
//                            cpu_set_t *cpu_set)
// {
//   thread_port_t mach_thread;
//   int core = 0;

//   for (core = 0; core < 8 * cpu_size; core++) {
//     if (CPU_ISSET(core, cpu_set)) break;
//   }
//   printf("binding to core %d\n", core);
//   thread_affinity_policy_data_t policy = { core };
//   mach_thread = pthread_mach_thread_np(thread);
//   thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
//                     (thread_policy_t)&policy, 1);
//   return 0;
// }

// void print_affinity() {
//     cpu_set_t mask;
//     long nproc, i;

//     if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
//         perror("sched_getaffinity");
//         assert(false);
//     }
//     nproc = 16 ;// sysconf(_SC_NPROCESSORS_ONLN);
//     printf("sched_getaffinity = ");
//     for (i = 0; i < nproc; i++) {
//         printf("%d ", CPU_ISSET(i, &mask));
//     }
//     printf("\n");
// }
// #endif