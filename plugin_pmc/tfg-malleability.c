/*=============================================================
 * Malleability scheduling plugin for the llvm's openmp runtime
 * Author Pablo Saiz Blazquez
 *=============================================================
*/

#include <pmc/pmcsched.h>
#include <linux/rwlock.h>
#include <linux/spinlock.h>
#include <linux/list.h>
// Global data structure for malleability
typedef struct mall_global{
    spinlock_t mall_lock;
    sized_list_t activa_processes; //Activos
    sized_list_t idle_processes; // Idle
    sized_list_t signal_list; // Procesos a mandar senial
    unsigned long nr_total_runnable_threads;
} mall_global_t;
#define AMP_CORE_TYPES 2
// Struct con los datos para calcular que cores dar
typedef struct tmon_global_app{
    int nr_runnable_threads;
    int nr_alive_threads;
    //Estimacion
    int effective_thread_count;
    //Ultimo conteo de hilos aplicado para proc maleable
    int last_enforced_thread_count;
    rwlock_t lock;
    sized_list_t threads_per_coretype[AMP_CORE_TYPES];
    struct list_head active_app_link;
    struct list_head inactive_app_link;
    struct list_head signal_links;
    struct list_head malleable_links;
    thread_count_stats_t tcount_stats;
    thread_count_stats_t tcount_periodic_stats;
    unsigned long flags;
    struct task_struct* signal_recipient;
} tmon_global_app_t;
static void sched_timer_periodic_fair_mall(void){
    sched_thread_group_t *cur_group = get_cur_group_sched();
    unsigned long flags;
    tmon_global_app_t *tsk;
    pmcsched_thread_data_t *thr;
    schedctl_t *shared_region;
    unsigned char is_malleable=0;
    unsigned int nr_remaining_cores = num_online_cpus();
    int nr_fair_cores;
    int nr_remaining_apps;
    sized_list_t malleable_list;
    ktime_t threshold = ktime_set(0,1000000);

    init_sized_list(&malleable_list,offsetof(tmon_global_app_t,malleable_links));

    spin_lock_irqsave(&mall_gbl.mall_lock,flags);

    for(tsk=head_sized_list(&mall_gbl.active_processes);tsk!=NULL;tsk = next_sized_list(&mall_gbl.active_processes,tsk)){
        shared_region = get_schedctl_task(tsk);
        is_malleable = shared_region && shared_region->sc_malleable;

        //Actualizacion de stats conseguir conteo de hilos y reset de los stats

        update_thread_count_stats(&tsk->tcount_periodic_stats,tsk->nr_runnable_threads,tsk->nr_runnable_threads);

        tsk->effective_thread_count = get_typical_thread_count(&tsk->tcount_periodic_stats,threshold);
        reset_thread_count_stats(&tsk->tcount_periodic_stats,tsk->nr_runnable_threads);

        if(is_malleable)
            insert_sized_list_tail(&malleable_list,tsk);
        else
            nr_remaining_cores-=tsk->effective_thread_count;
    }
    nr_remaining_cores-=sized_list_length(&mall_gbl.idle_processes);
    nr_remaining_apps = sized_list_length(&malleable_list);
    //Reparto de cores en procesos maleables
    for(tsk=head_sized_list(&malleable_list);tsk!=NULL;tsk=next_sized_list(&malleable_list,tsk)){
        nr_fair_cores = MAX(1,(nr_remaining_cores+nr_remaining_apps-1)/nr_remaining_apps);
        nr_remaining_apps--;
        nr_remaining_cores-=nr_fair_cores;

        //Comprobar si se debe enviar senial
        if(nr_fair_cores!=tsk->last_enforced_thread_count){
            tsk->last_enforced_thread_count = nr_fair_cores;
            //Actualizacion memoria compartida
            shared_region = get_schedctl_task(tsk);
            shared_region->sc_num_threads=nr_fair_cores;
            if(tsk->flags & TMON_APP_TO_BE_SIGNALED){
                trace_printk("Aplication to be signaled with singal pending");
            }
            else {
                tsk->flags |= TMON_APP_TO_BE_SIGNALED;
                insert_sized_list_tail(&&mall_gbl.signal_list,tsk);
            }
        }
        // Senializacion
        if(sized_list_length(&mall_gbl.signal_list>0))
            cur_group->activate_kthread =1;
        spin_unlock_irqstire(&mall_gbl,mall_lock,flags);
    }
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
