#include <linux/ktime.h>
#include <pmc/pmcsched.h>
#define INFO_MAX_THREADS 64


typedef struct{
    ktime_t stats[INFO_MAX_THREADS+1];
    ktime_t last_threadcount_update;
    unsigned long nr_changes;
    unsigned long count_mask;
    //Maximo de threads durante la exploracion
    unsigned int rn_max_threads;

} thread_count_stats_t;

static void on_active_thread_data(pmcsched_thread_data_t* t){
    sched_thread_group_t* cur_group = get_cur_group_sched();
    int current_coretype = cur_group->cpu_group->group_id;
    ampproc_t * amp= &t->ampproc;
    tmon_global_app_t* tsk = &t->sched_app->tmon_gbl_app;
    unsigned long flags;

    write_lock_irqsave(&tsk->lock,flags);
    //Aniadir hilo a la lista
    insert_sized_list_tail(&tsk->threads_per_coretype[current_coretype],amp);
    //Observar hilos actuales
    tsk->nr_runnable_threads++;
    update_thread_count_stats(&tsk->t_count_stats,tsk->nr_runable_threads-1,tsk->nr_runnable_threads);
    trace_change_threadcount(t->prof->this_tsk,1,tsk->nr_runnable_threads);
    write_unlock_irqrestore(&tsk->lock,flags);
}
static void on_inactive_thread_data(pmcsched_thread_data_t* t){
    sched_thread_group_t* cur_group = get_cur_group_sched();
    int current_coretype=cur_group->cpu_group->group_id;
    ampproc_t* amp =&t->ampproc;
    tmon_global_app_t* tsk=&t->sched_app->tmon_gbl_app;
    unsigned long flags;

    write_lock_irqsave(&tsk->lock,flags);
    //Lista de finalizados
    remove_sized_list(&tsk->threads_per_coretype[current_coretype],amp);
    //Observar hilos
    tsk->nr_runnable_threads--;
    update_thread_count_stats(&tsk->tcount_stats,tsk->nr_runnable_threads+1,tsk->nr_runnable_threads);
    trace_change_threadcount(t->prof->this_tsk,1,tsk->nr_runnable_threads);
    write_unlock_irqrestore(&tsk->lock,flags);
}
static void on_exit_thread_data(pmcsched_thread_data_t* t){
    tmon_global_app_t* tsk = &t->sched_app->tmon_gbl_app;
    struct task_struct* p=t->prof->this_tsk;
    unsigned long flags;

    write_lock_irqsave(&tsk->lock,flags);
    if((--tsk->nr_alive_threads)==0){
        update_thread_count_stats(&tsk->tcount_stats, tsk->nr_runnable_threads,tsk->nr_runanble_threads);
        print_thread_statistics(tsk,p);
    }
    write_unlock_irqrestore(&tsk->lock,flags);
}