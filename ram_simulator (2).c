/*
 * ============================================================
 *  Multi-Level Computer Memory Manager — Enhanced RAM Simulator
 *  IN 1111 - Data Structures and Algorithms 1
 *  Faculty of Information Technology, University of Moratuwa
 * ============================================================
 *  Data Structures used (7):
 *    1. Array            — RAM Map (1024 blocks)
 *    2. Singly Linked List — PCB List
 *    3. Doubly Linked List — Memory Holes
 *    4. Queue            — FIFO Page Replacement
 *    5. Stack            — Function Call Simulation
 *    6. Circular Linked List — Round Robin Scheduler
 *    7. Priority Queue (Min-Heap) — Interrupt Scheduler
 *
 *  Sorting Algorithms (3):
 *    8.  Selection Sort — Memory Holes
 *    9.  Insertion Sort — PCB List
 *    10. Bubble Sort    — Ready Queue Ageing
 * ============================================================
 *  Compile:  gcc -Wall -Wextra -o ram_sim ram_simulator.c
 *  Run:      ./ram_sim
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 *  CONSTANTS
 * ============================================================ */
#define RAM_SIZE     1024
#define TIME_QUANTUM    3
#define PQ_MAX        128
#define MAX_DEPTH      64
#define FIFO_CAP       16

/* ============================================================
 *  1. ARRAY — RAM MAP
 * ============================================================ */
int ram[RAM_SIZE];

void initRAM(void) {
    memset(ram, 0, sizeof(ram));
}

int allocateRAM(int pid, int start, int size) {
    if (start < 0 || start + size > RAM_SIZE) return -1;
    for (int i = start; i < start + size; i++)
        if (ram[i] != 0) return -1;
    for (int i = start; i < start + size; i++)
        ram[i] = pid;
    return 0;
}

void freeRAM(int pid) {
    for (int i = 0; i < RAM_SIZE; i++)
        if (ram[i] == pid) ram[i] = 0;
}

/* ASCII visualisation — shows first `n` blocks */
void visualiseRAM(int n) {
    if (n > RAM_SIZE) n = RAM_SIZE;
    printf("\n=== RAM Visualisation (first %d blocks) ===\n", n);
    printf("   ");
    for (int c = 0; c < 16; c++) printf("%4d", c);
    printf("\n");
    for (int i = 0; i < n; i++) {
        if (i % 16 == 0) printf("\n%3d|", i);
        if (ram[i] == 0) printf("   .");
        else             printf("%4d", ram[i]);
    }
    printf("\n");
}

/* ============================================================
 *  2. SINGLY LINKED LIST — PCB LIST
 * ============================================================ */
typedef struct PCB {
    int    pid;
    char   name[20];
    int    startAddr;
    int    memSize;
    int    priority;
    struct PCB *next;
} PCB;

PCB *pcbHead = NULL;

void pcbInsert(int pid, const char *name, int start, int size, int prio) {
    PCB *node = (PCB *)malloc(sizeof(PCB));
    node->pid       = pid;
    strncpy(node->name, name, 19);
    node->name[19]  = '\0';
    node->startAddr = start;
    node->memSize   = size;
    node->priority  = prio;
    node->next      = NULL;
    if (!pcbHead) { pcbHead = node; return; }
    PCB *cur = pcbHead;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

int pcbDelete(int pid) {
    PCB *cur = pcbHead, *prev = NULL;
    while (cur) {
        if (cur->pid == pid) {
            if (prev) prev->next = cur->next;
            else      pcbHead    = cur->next;
            free(cur);
            return 0;
        }
        prev = cur; cur = cur->next;
    }
    return -1;
}

PCB *pcbSearch(int pid) {
    PCB *cur = pcbHead;
    while (cur) { if (cur->pid == pid) return cur; cur = cur->next; }
    return NULL;
}

void pcbPrint(void) {
    printf("\n%-6s %-15s %-8s %-8s %-8s\n",
           "PID","Name","Start","MemSz","Prio");
    printf("--------------------------------------------------\n");
    PCB *cur = pcbHead;
    while (cur) {
        printf("%-6d %-15s %-8d %-8d %-8d\n",
               cur->pid, cur->name, cur->startAddr,
               cur->memSize, cur->priority);
        cur = cur->next;
    }
}

/* ============================================================
 *  3. DOUBLY LINKED LIST — MEMORY HOLES
 * ============================================================ */
typedef struct Hole {
    int startAddr;
    int size;
    struct Hole *prev;
    struct Hole *next;
} Hole;

Hole *holeHead = NULL;

void holeInsert(int start, int size) {
    Hole *node = (Hole *)malloc(sizeof(Hole));
    node->startAddr = start; node->size = size;
    node->prev = node->next = NULL;
    if (!holeHead || start < holeHead->startAddr) {
        node->next = holeHead;
        if (holeHead) holeHead->prev = node;
        holeHead = node;
    } else {
        Hole *cur = holeHead;
        while (cur->next && cur->next->startAddr < start)
            cur = cur->next;
        node->next = cur->next;
        node->prev = cur;
        if (cur->next) cur->next->prev = node;
        cur->next = node;
    }
}

void holeDelete(Hole *h) {
    if (!h) return;
    if (h->prev) h->prev->next = h->next;
    else         holeHead      = h->next;
    if (h->next) h->next->prev = h->prev;
    free(h);
}

void holeMerge(void) {
    Hole *cur = holeHead;
    while (cur && cur->next) {
        if (cur->startAddr + cur->size == cur->next->startAddr) {
            cur->size += cur->next->size;
            Hole *tmp  = cur->next;
            cur->next  = tmp->next;
            if (tmp->next) tmp->next->prev = cur;
            free(tmp);
        } else cur = cur->next;
    }
}

/* 8. SELECTION SORT on hole list (by size ascending) */
void selectionSortHoles(void) {
    if (!holeHead) return;
    for (Hole *i = holeHead; i->next != NULL; i = i->next) {
        Hole *minNode = i;
        for (Hole *j = i->next; j != NULL; j = j->next)
            if (j->size < minNode->size) minNode = j;
        if (minNode != i) {
            int ta = i->startAddr, ts = i->size;
            i->startAddr = minNode->startAddr; i->size = minNode->size;
            minNode->startAddr = ta; minNode->size = ts;
        }
    }
}

Hole *firstFit(int reqSize) {
    Hole *c = holeHead;
    while (c) { if (c->size >= reqSize) return c; c = c->next; }
    return NULL;
}

Hole *bestFitSorted(int reqSize) {
    selectionSortHoles();          /* sort ascending by size */
    Hole *c = holeHead;
    while (c) { if (c->size >= reqSize) return c; c = c->next; }
    return NULL;
}

Hole *worstFit(int reqSize) {
    /* sort descending for worst fit */
    selectionSortHoles();          /* ascending first, then reverse scan */
    Hole *best = NULL;
    Hole *c = holeHead;
    while (c) {
        if (c->size >= reqSize)
            if (!best || c->size > best->size) best = c;
        c = c->next;
    }
    return best;
}

void holePrint(void) {
    printf("\n--- Free Memory Holes ---\n");
    printf("%-10s %-8s\n", "StartAddr", "Size");
    printf("-------------------\n");
    Hole *c = holeHead;
    if (!c) { printf("  (none)\n"); return; }
    while (c) { printf("%-10d %-8d\n", c->startAddr, c->size); c = c->next; }
}

/* ============================================================
 *  4. QUEUE — FIFO PAGE REPLACEMENT
 * ============================================================ */
typedef struct QNode { int pid; struct QNode *next; } QNode;
typedef struct {
    QNode *front, *rear;
    int count, capacity;
} FIFOQueue;

void qInit(FIFOQueue *q, int cap) { q->front = q->rear = NULL; q->count = 0; q->capacity = cap; }

void qEnqueue(FIFOQueue *q, int pid) {
    QNode *node = (QNode *)malloc(sizeof(QNode));
    node->pid = pid; node->next = NULL;
    if (!q->rear) q->front = q->rear = node;
    else { q->rear->next = node; q->rear = node; }
    q->count++;
}

int qDequeue(FIFOQueue *q) {
    if (!q->front) return -1;
    QNode *tmp = q->front; int pid = tmp->pid;
    q->front = tmp->next;
    if (!q->front) q->rear = NULL;
    free(tmp); q->count--;
    return pid;
}

int pageIn(FIFOQueue *q, int pid) {
    int evicted = -1;
    if (q->count >= q->capacity) evicted = qDequeue(q);
    qEnqueue(q, pid);
    return evicted;
}

/* ============================================================
 *  5. STACK — FUNCTION CALL SIMULATION
 * ============================================================ */
typedef struct { char funcName[32]; int returnAddr; } Frame;
typedef struct { Frame frames[MAX_DEPTH]; int top; } CallStack;

void  stackInit(CallStack *s) { s->top = -1; }
int   stackPush(CallStack *s, const char *name, int retAddr) {
    if (s->top >= MAX_DEPTH - 1) return -1;
    s->top++;
    strncpy(s->frames[s->top].funcName, name, 31);
    s->frames[s->top].funcName[31] = '\0';
    s->frames[s->top].returnAddr   = retAddr;
    return 0;
}
int   stackPop(CallStack *s) { if (s->top < 0) return -1; s->top--; return 0; }
void  stackPrint(CallStack *s) {
    printf("\n--- Call Stack (top -> bottom) ---\n");
    for (int i = s->top; i >= 0; i--)
        printf("  [%d] %s  (ret=%d)\n", i, s->frames[i].funcName, s->frames[i].returnAddr);
    if (s->top < 0) printf("  (empty)\n");
}

/* ============================================================
 *  6. CIRCULAR LINKED LIST — ROUND ROBIN SCHEDULER
 * ============================================================ */
typedef struct RRNode {
    int pid, burstRemaining, waitTime;
    struct RRNode *next;
} RRNode;

RRNode *rrTail = NULL;

int rrCount(void) {
    if (!rrTail) return 0;
    int n = 1; RRNode *c = rrTail->next;
    while (c != rrTail) { n++; c = c->next; }
    return n;
}

void rrInsert(int pid, int burst) {
    RRNode *node = (RRNode *)malloc(sizeof(RRNode));
    node->pid = pid; node->burstRemaining = burst; node->waitTime = 0;
    if (!rrTail) { node->next = node; rrTail = node; }
    else { node->next = rrTail->next; rrTail->next = node; rrTail = node; }
}

void rrRemove(int pid) {
    if (!rrTail) return;
    RRNode *cur = rrTail->next, *prev = rrTail;
    do {
        if (cur->pid == pid) {
            if (cur == cur->next) rrTail = NULL;
            else {
                prev->next = cur->next;
                if (cur == rrTail) rrTail = prev;
            }
            free(cur); return;
        }
        prev = cur; cur = cur->next;
    } while (cur != rrTail->next);
}

/* 10. BUBBLE SORT — ageing pass on ready queue */
void bubbleSortAgeing(void) {
    int n = rrCount();
    if (n <= 1) return;
    for (int i = 0; i < n - 1; i++) {
        RRNode *cur = rrTail->next; int swapped = 0;
        for (int j = 0; j < n - 1 - i; j++) {
            RRNode *nxt = cur->next;
            if (cur->waitTime < nxt->waitTime) {
                int tp = cur->pid;              cur->pid = nxt->pid;              nxt->pid = tp;
                int tb = cur->burstRemaining;   cur->burstRemaining = nxt->burstRemaining; nxt->burstRemaining = tb;
                int tw = cur->waitTime;         cur->waitTime = nxt->waitTime;    nxt->waitTime = tw;
                swapped = 1;
            }
            cur = cur->next;
        }
        if (!swapped) break;
    }
}

void ageAllProcesses(int runningPid) {
    if (!rrTail) return;
    RRNode *c = rrTail->next;
    do { if (c->pid != runningPid) c->waitTime++; c = c->next; } while (c != rrTail->next);
}

void rrPrint(void) {
    printf("\n--- Ready Queue (Round Robin) ---\n");
    printf("%-6s %-8s %-8s\n", "PID", "Burst", "WaitT");
    printf("-------------------------\n");
    if (!rrTail) { printf("  (empty)\n"); return; }
    RRNode *c = rrTail->next;
    do { printf("%-6d %-8d %-8d\n", c->pid, c->burstRemaining, c->waitTime); c = c->next; }
    while (c != rrTail->next);
}

/* ============================================================
 *  7. PRIORITY QUEUE (MIN-HEAP) — INTERRUPT SCHEDULER
 * ============================================================ */
typedef struct { int pid, priority; char name[20]; } PQEntry;
typedef struct { PQEntry data[PQ_MAX]; int size; } PriorityQueue;

void pqInit(PriorityQueue *pq) { pq->size = 0; }

void pqInsert(PriorityQueue *pq, int pid, int prio, const char *name) {
    if (pq->size >= PQ_MAX) return;
    int i = pq->size++;
    pq->data[i].pid = pid; pq->data[i].priority = prio;
    strncpy(pq->data[i].name, name, 19); pq->data[i].name[19] = '\0';
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pq->data[i].priority < pq->data[parent].priority) {
            PQEntry t = pq->data[i]; pq->data[i] = pq->data[parent]; pq->data[parent] = t;
            i = parent;
        } else break;
    }
}

PQEntry pqExtractMin(PriorityQueue *pq) {
    PQEntry min = pq->data[0];
    pq->data[0] = pq->data[--pq->size];
    int i = 0;
    while (1) {
        int l = 2*i+1, r = 2*i+2, smallest = i;
        if (l < pq->size && pq->data[l].priority < pq->data[smallest].priority) smallest = l;
        if (r < pq->size && pq->data[r].priority < pq->data[smallest].priority) smallest = r;
        if (smallest != i) {
            PQEntry t = pq->data[i]; pq->data[i] = pq->data[smallest]; pq->data[smallest] = t;
            i = smallest;
        } else break;
    }
    return min;
}

void pqPrint(PriorityQueue *pq) {
    printf("\n--- Interrupt / Priority Queue ---\n");
    printf("%-6s %-15s %-6s\n", "PID", "Name", "Prio");
    printf("-------------------------------\n");
    if (pq->size == 0) { printf("  (empty)\n"); return; }
    for (int i = 0; i < pq->size; i++)
        printf("%-6d %-15s %-6d\n", pq->data[i].pid, pq->data[i].name, pq->data[i].priority);
}

/* ============================================================
 *  9. INSERTION SORT — PCB LIST
 * ============================================================ */
void insertionSortPCBbyAddr(void) {
    if (!pcbHead || !pcbHead->next) return;
    PCB *sorted = NULL, *cur = pcbHead;
    while (cur) {
        PCB *nxt = cur->next;
        if (!sorted || cur->startAddr < sorted->startAddr) { cur->next = sorted; sorted = cur; }
        else {
            PCB *s = sorted;
            while (s->next && s->next->startAddr < cur->startAddr) s = s->next;
            cur->next = s->next; s->next = cur;
        }
        cur = nxt;
    }
    pcbHead = sorted;
}

void insertionSortPCBbySize(void) {
    if (!pcbHead || !pcbHead->next) return;
    PCB *sorted = NULL, *cur = pcbHead;
    while (cur) {
        PCB *nxt = cur->next;
        if (!sorted || cur->memSize < sorted->memSize) { cur->next = sorted; sorted = cur; }
        else {
            PCB *s = sorted;
            while (s->next && s->next->memSize < cur->memSize) s = s->next;
            cur->next = s->next; s->next = cur;
        }
        cur = nxt;
    }
    pcbHead = sorted;
}

/* ============================================================
 *  GLOBAL STATE
 * ============================================================ */
static int      nextPID  = 1;
static FIFOQueue fifoQ;
static CallStack callStk;
static PriorityQueue intPQ;
static int      allocMode = 0;   /* 0=FirstFit 1=BestFit 2=WorstFit */

/* ============================================================
 *  HELPER — allocate memory for a new process
 *  Returns start address or -1 on failure
 * ============================================================ */
int allocateForProcess(int size) {
    Hole *h = NULL;
    if      (allocMode == 0) h = firstFit(size);
    else if (allocMode == 1) h = bestFitSorted(size);
    else                     h = worstFit(size);

    if (!h) return -1;          /* no suitable hole found */

    int start = h->startAddr;
    int hSize = h->size;
    holeDelete(h);

    /* if hole was larger, re-insert remainder */
    if (hSize > size)
        holeInsert(start + size, hSize - size);

    allocateRAM(nextPID, start, size);
    return start;
}

/* ============================================================
 *  MENU HELPERS
 * ============================================================ */
void clearInput(void) { int c; while ((c = getchar()) != '\n' && c != EOF); }

void printBanner(void) {
    printf("\n");
    printf("##############################################\n");
    printf("#   Multi-Level Memory Manager  v1.0        #\n");
    printf("#   IN1111 - Data Structures & Algorithms   #\n");
    printf("##############################################\n");
}

void printMainMenu(void) {
    printf("\n========== MAIN MENU ==========\n");
    printf(" [1] Create Process\n");
    printf(" [2] Terminate Process\n");
    printf(" [3] Run Round Robin (1 quantum)\n");
    printf(" [4] Add Interrupt (Priority Queue)\n");
    printf(" [5] Dispatch Next Interrupt\n");
    printf(" [6] Push Call Stack Frame\n");
    printf(" [7] Pop Call Stack Frame\n");
    printf(" [8] Memory & System Status\n");
    printf(" [9] Sort & View Reports\n");
    printf(" [0] Exit\n");
    printf("================================\n");
    printf(" Alloc Mode: %s\n",
           allocMode==0?"First Fit": allocMode==1?"Best Fit":"Worst Fit");
    printf("================================\n");
    printf("Choice: ");
}

void printStatusMenu(void) {
    printf("\n--- Status / View ---\n");
    printf(" [1] RAM Visualisation (first 128 blocks)\n");
    printf(" [2] PCB List\n");
    printf(" [3] Memory Holes\n");
    printf(" [4] Ready Queue\n");
    printf(" [5] Priority (Interrupt) Queue\n");
    printf(" [6] Call Stack\n");
    printf(" [7] Change Allocation Strategy\n");
    printf(" [0] Back\n");
    printf("Choice: ");
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void) {
    /* Initialise everything */
    initRAM();
    qInit(&fifoQ, FIFO_CAP);
    stackInit(&callStk);
    pqInit(&intPQ);

    /* Seed one large free hole covering all of RAM */
    holeInsert(0, RAM_SIZE);

    printBanner();
    printf("RAM initialised: %d blocks free.\n", RAM_SIZE);

    int choice;
    while (1) {
        printMainMenu();
        if (scanf("%d", &choice) != 1) { clearInput(); continue; }
        clearInput();

        /* ---- 1. CREATE PROCESS ---- */
        if (choice == 1) {
            char name[20];
            int  size, burst, prio;
            printf("Process name (max 19 chars): ");
            if (scanf("%19s", name) != 1) { clearInput(); continue; }
            clearInput();
            printf("Memory size (blocks): ");
            if (scanf("%d", &size) != 1 || size <= 0 || size > RAM_SIZE) {
                printf("Invalid size.\n"); clearInput(); continue;
            }
            clearInput();
            printf("CPU burst time (ticks): ");
            if (scanf("%d", &burst) != 1 || burst <= 0) {
                printf("Invalid burst.\n"); clearInput(); continue;
            }
            clearInput();
            printf("Priority (1=highest, 10=lowest): ");
            if (scanf("%d", &prio) != 1) { clearInput(); prio = 5; }
            clearInput();

            int start = allocateForProcess(size);
            if (start == -1) {
                printf("[!] No suitable hole found. Attempting FIFO page eviction...\n");
                int evicted = pageIn(&fifoQ, nextPID);
                if (evicted != -1) {
                    printf("[!] Evicted PID %d from memory.\n", evicted);
                    freeRAM(evicted);
                    holeInsert(pcbSearch(evicted) ? pcbSearch(evicted)->startAddr : 0,
                               pcbSearch(evicted) ? pcbSearch(evicted)->memSize  : size);
                    pcbDelete(evicted);
                    rrRemove(evicted);
                    holeMerge();
                    start = allocateForProcess(size);
                }
                if (start == -1) { printf("[!] Allocation failed even after eviction.\n"); continue; }
            } else {
                pageIn(&fifoQ, nextPID);   /* track in FIFO regardless */
            }

            pcbInsert(nextPID, name, start, size, prio);
            rrInsert(nextPID, burst);
            printf("[+] Created PID %d (%s) at addr %d, size %d, burst %d\n",
                   nextPID, name, start, size, burst);
            nextPID++;
        }

        /* ---- 2. TERMINATE PROCESS ---- */
        else if (choice == 2) {
            int pid;
            printf("PID to terminate: ");
            if (scanf("%d", &pid) != 1) { clearInput(); continue; }
            clearInput();
            PCB *p = pcbSearch(pid);
            if (!p) { printf("[!] PID %d not found.\n", pid); continue; }
            int start = p->startAddr, sz = p->memSize;
            freeRAM(pid);
            rrRemove(pid);
            pcbDelete(pid);
            holeInsert(start, sz);
            holeMerge();
            printf("[-] PID %d terminated. %d blocks freed at addr %d.\n", pid, sz, start);
        }

        /* ---- 3. ROUND ROBIN TICK ---- */
        else if (choice == 3) {
            if (!rrTail) { printf("[!] Ready queue is empty.\n"); continue; }

            /* Bubble sort ageing pass before each quantum */
            bubbleSortAgeing();

            RRNode *running = rrTail->next;
            int pid = running->pid;
            int exec = (running->burstRemaining > TIME_QUANTUM) ? TIME_QUANTUM : running->burstRemaining;
            running->burstRemaining -= exec;
            printf("[RR] Executing PID %d for %d tick(s). Remaining burst: %d\n",
                   pid, exec, running->burstRemaining);

            ageAllProcesses(pid);

            if (running->burstRemaining == 0) {
                printf("[RR] PID %d completed execution.\n", pid);
                int start = -1, sz = 0;
                PCB *p = pcbSearch(pid);
                if (p) { start = p->startAddr; sz = p->memSize; }
                freeRAM(pid); rrRemove(pid); pcbDelete(pid);
                if (start >= 0) { holeInsert(start, sz); holeMerge(); }
            } else {
                /* Advance tail so next process runs next quantum */
                rrTail = rrTail->next;
            }
        }

        /* ---- 4. ADD INTERRUPT ---- */
        else if (choice == 4) {
            char name[20]; int pid, prio;
            printf("Interrupt name: ");
            if (scanf("%19s", name) != 1) { clearInput(); continue; }
            clearInput();
            printf("Interrupt PID (use 9xx convention): ");
            if (scanf("%d", &pid) != 1) { clearInput(); continue; }
            clearInput();
            printf("Priority (1=urgent): ");
            if (scanf("%d", &prio) != 1) { clearInput(); continue; }
            clearInput();
            pqInsert(&intPQ, pid, prio, name);
            printf("[+] Interrupt '%s' (PID %d, prio %d) queued.\n", name, pid, prio);
        }

        /* ---- 5. DISPATCH INTERRUPT ---- */
        else if (choice == 5) {
            if (intPQ.size == 0) { printf("[!] No interrupts pending.\n"); continue; }
            PQEntry e = pqExtractMin(&intPQ);
            printf("[!] Dispatching interrupt: '%s' (PID %d, prio %d)\n",
                   e.name, e.pid, e.priority);
        }

        /* ---- 6. PUSH CALL FRAME ---- */
        else if (choice == 6) {
            char fn[32]; int retAddr;
            printf("Function name: ");
            if (scanf("%31s", fn) != 1) { clearInput(); continue; }
            clearInput();
            printf("Return address: ");
            if (scanf("%d", &retAddr) != 1) { clearInput(); continue; }
            clearInput();
            if (stackPush(&callStk, fn, retAddr) == 0)
                printf("[+] Pushed '%s' (ret=%d) onto call stack.\n", fn, retAddr);
            else
                printf("[!] Call stack overflow.\n");
        }

        /* ---- 7. POP CALL FRAME ---- */
        else if (choice == 7) {
            if (stackPop(&callStk) == 0)
                printf("[-] Popped top frame from call stack.\n");
            else
                printf("[!] Call stack is empty.\n");
        }

        /* ---- 8. STATUS ---- */
        else if (choice == 8) {
            int sub;
            printStatusMenu();
            if (scanf("%d", &sub) != 1) { clearInput(); continue; }
            clearInput();
            if      (sub == 1) visualiseRAM(128);
            else if (sub == 2) pcbPrint();
            else if (sub == 3) holePrint();
            else if (sub == 4) rrPrint();
            else if (sub == 5) pqPrint(&intPQ);
            else if (sub == 6) stackPrint(&callStk);
            else if (sub == 7) {
                printf("Allocation strategy: [0] First Fit  [1] Best Fit  [2] Worst Fit\nChoice: ");
                if (scanf("%d", &allocMode) != 1 || allocMode < 0 || allocMode > 2) allocMode = 0;
                clearInput();
                printf("Strategy set to %s.\n",
                       allocMode==0?"First Fit": allocMode==1?"Best Fit":"Worst Fit");
            }
        }

        /* ---- 9. SORT REPORTS ---- */
        else if (choice == 9) {
            int sub;
            printf("\n--- Sort & Report ---\n");
            printf(" [1] PCB sorted by start address\n");
            printf(" [2] PCB sorted by memory size\n");
            printf(" [3] Holes sorted by size (selection sort)\n");
            printf(" [4] Ageing pass on ready queue (bubble sort)\n");
            printf("Choice: ");
            if (scanf("%d", &sub) != 1) { clearInput(); continue; }
            clearInput();
            if      (sub == 1) { insertionSortPCBbyAddr(); pcbPrint(); }
            else if (sub == 2) { insertionSortPCBbySize(); pcbPrint(); }
            else if (sub == 3) { selectionSortHoles();     holePrint(); }
            else if (sub == 4) { bubbleSortAgeing();       rrPrint();  }
        }

        /* ---- 0. EXIT ---- */
        else if (choice == 0) {
            printf("Goodbye.\n");
            break;
        }
        else {
            printf("Unknown option.\n");
        }
    }
    return 0;
}
