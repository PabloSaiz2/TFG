#include <linux/ktime.h>
#include <pmc/pmcsched.h>
#define TMON_MAX_THREADS 64


typedef struct{
    ktime_t stats[TMON_MAX_THREADS+1];
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
void noinline trace_data_stats(struct task_struct* p,char* buf){
    asm(" ");
}

static void print_threads_statistics(tmon_global_app_t* gbl_app,struct task_struct* p){
    char comm[TASK_COMM_LEN];
    char *dest = tracebuf;
    thread_count_stats_t* tcount_stats;
    int i=0;
    static unsigned char first_time=1;
    static spinlock_t lock;

    if(first_time){
        first_time=0;
        spin_lock_init(&lock);
    }
    spin_lock(&lock);
    tcount_stats=&gbl_app->tcount_stats;
    get_task_comm(comm,p);
    dest +=sprintf(dest,"%s,%d",comm,p->tgid);
    for(i=0< MAX_THREADS_OUT+1;i++){
        dest+= sprintf(dest,",%llu",(tcount_stats->stats[i]/1000));
    }
    trace_data_stats(p,tracebuf);
    spin_unlock(&lock);
}