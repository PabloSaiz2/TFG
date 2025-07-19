/*=============================================================
 * Malleability scheduling plugin for the llvm's openmp runtime
 * Author Pablo Saiz Blazquez
 *=============================================================
*/

#include <pmc/pmcsched.h>


static void tfg_sched_kthread_periodic(sized_list_t* migration_list){

}
sched_ops_t tfg_plugin = {
    .policy = SCHED_MALLEABILITY,
    .description = "plugin for omp malleability",
    .flags = PMCSCHED_CPUGROUP_LOCK,
    .sched_kthread_periodic=,
    .on_exec_thread = ,
    .on_active_thread=,
    .on_inactive_thread=,
    .on_fork_thread=,
    .on_exit_thread=,
    .on_migrate_thread=,

};
