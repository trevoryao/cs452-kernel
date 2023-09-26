#include "task-alloc.h"

void task_alloc_init(task_alloc *alloc) {
    // Initialize the free list to empty
    alloc->free_list = NULL;

    // Init all slabs to be inactive
    alloc->active_slab = 0;
}

task_t *task_alloc_new(task_alloc *alloc) {
    // no free memory in the free list
    if (!alloc->free_list) {
        // go thorough all slab and test if they could be used
        for (int i = 0; i < 8; i++) {
            // consider first free slab
            if ((alloc->active_slab & (1 << i)) == 0) {
                // Initialize slab if not in use
                for (int j = 0; j < N_TASK_T; j++) {
                    task_t *new_task = &alloc->slabs[i][j];
                    // set the slab index
                    new_task->slab_index = i;
                    new_task->next = alloc->free_list;
                    alloc->free_list = new_task;
                }

                // set slab in use
                alloc->active_slab |= (1 << i);
                alloc->task_count_in_slab[i] = 0;
                // break, because only activating one slab at a time
                break;
            }
        }
    }

    // if the list is still empty, then all slabs are already in use
    if (alloc->free_list) {
        task_t *new_task = alloc->free_list;
        alloc->free_list = new_task->next;

        // set slab info
        alloc->task_count_in_slab[new_task->slab_index]++;

        // zero the task
        task_clear(new_task); // TODO: might be the cause of zero value returns, alloc is not behaving properly?

        return new_task;
    }
    // return NULL, if memory is full
    return NULL;
}

void task_alloc_free(task_alloc *alloc, task_t *task) {
    if (task) {
        task->ready_state = STATE_KILLED;

        // Add the task to the free list
        task->next = alloc->free_list;
        alloc->free_list = task;

        // decrease slab counter
        alloc->task_count_in_slab[task->slab_index]--;

        // Check if the slab can be freed
        if (alloc->task_count_in_slab[task->slab_index] == 0) {
            alloc->active_slab &= ~(1 << task->slab_index);

            // going through the entire free list and delete all which belong to the deactivted slab
            task_t *last_task = NULL;
            task_t *curr_task = alloc->free_list;

            while(curr_task) {
                if (curr_task->slab_index == task->slab_index) {
                    if (last_task) {
                        last_task->next = curr_task->next;
                    } else {
                        alloc->free_list = curr_task->next;
                    }
                    curr_task = curr_task->next;
                } else {
                    last_task = curr_task;
                    curr_task = curr_task->next;
                }
            }
        }
    }
}

