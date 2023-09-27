#include "task-alloc.h"

void task_alloc_init(task_alloc *alloc) {
    // Initialize the free list to empty
    alloc->free_list = NULL;

    // memset the slab indexes
    memset(alloc->active_slabs, 0, sizeof(uint8_t) * N_SLABS);
}

task_t *task_alloc_new(task_alloc *alloc) {
    // no free memory in the free list
    if (!alloc->free_list) {
        // go thorough all slab and test if they could be used
        for (int i = 0; i < N_SLABS; i++) {
            // consider first free slab
            if (alloc->active_slabs[i] == 0) {
                // Initialize slab if not in use
                for (int j = 0; j < N_TASK_T; j++) {
                    task_t *new_task = &alloc->slabs[i][j];
                    // set the slab index
                    task_clear(new_task);
                    new_task->slab_index = i;
                    

                    // add it to the free list
                    task_alloc_add_to_free_list(alloc, new_task);
                }

                // set slab in use
                alloc->active_slabs[i] = 1;
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

void task_alloc_add_to_free_list(task_alloc *alloc, task_t *task) {
    // make sure the next pointer of the task is zero
    task->next = NULL;
    
    // Case 1: Free list is empty
    if (alloc->free_list == NULL) {
        alloc->free_list = task;
    } else {  
        // Case 2: Iterate over the list and add it according to pointer
        task_t *last = NULL;
        task_t *current = alloc->free_list;
        while ((current != NULL) && (current->slab_index < task->slab_index)) {
            last = current;
            current = current->next;
        }
        
        if (current == alloc->free_list) {
            // Case 2.1: Task is new head of free list
            alloc->free_list = task;
            task->next = current;
        } else {
            // Case 2.2: Task is added somewhere in the queue
            last->next = task;
            task->next = current;
        }
    }
}

void task_alloc_free(task_alloc *alloc, task_t *task) {
    if (task != NULL) {
        task->ready_state = STATE_KILLED;

        // Add the task to the free list
        task_alloc_add_to_free_list(alloc, task);

        // decrease slab counter
        alloc->task_count_in_slab[task->slab_index]--;
        
        // Check if the slab can be freed
        if (alloc->task_count_in_slab[task->slab_index] == 0) {
            // going through the entire free list and delete all which belong to the deactivted slab
            uint8_t slab_index = task->slab_index;
            task_t *prev = NULL;
            task_t *current = alloc->free_list;

            while (current != NULL) {
                if (current->slab_index == slab_index) {
                    if (prev == NULL) {
                        alloc->free_list = current->next;  // if current is head of list
                    } else {
                        prev->next = current->next;  // unlink current from list
                    }

                    task_t *temp = current;
                    current = current->next;
                    temp->next = NULL;  // optional, clear next pointer of removed task
                } else {
                    prev = current;
                    current = current->next;
                }
            }
        
            // set slab to inactive
            alloc->active_slabs[task->slab_index] = 0;
        } 
    }
}
