
/*
 ============================================================
  Multi-Level Computer Memory Manager (Enhanced RAM Simulator)
  IN 1111 - Data Structures and Algorithms 1
  Faculty of Information Technology, University of Moratuwa
 ============================================================
  Data Structures Used:
    1. Array              - RAM Map (1024 blocks)
    2. Singly Linked List - PCB List (process metadata)
    3. Doubly Linked List - Memory Holes (free regions)
    4. Queue              - FIFO Page Replacement
    5. Stack              - Function Call Simulation
    6. Circular Linked List - CPU Round Robin Scheduling
    7. Priority Queue     - Interrupt / High-Priority Scheduler
 ============================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────────
   CONSTANTS
───────────────────────────────────────────── */
#define RAM_SIZE          1024
#define MAX_PROCESSES     50
#define TIME_QUANTUM      3
#define HASH_SIZE         64
#define MAX_STACK         20
#define MAX_PQUEUE        50
#define MAX_FIFO_QUEUE    50

/* ─────────────────────────────────────────────
   1. ARRAY  –  RAM Map
───────────────────────────────────────────── */
int RAM[RAM_SIZE];   /* 0 = free, positive int = process ID */

void initRAM() {
    memset(RAM, 0, sizeof(RAM));
}

/* Mark blocks [start, start+size) with pid */
void allocateRAM(int pid, int start, int size) {
    for (int i = start; i < start + size; i++) RAM[i] = pid;
}

/* Free blocks belonging to pid */
void freeRAM(int pid) {
    for (int i = 0; i < RAM_SIZE; i++)
        if (RAM[i] == pid) RAM[i] = 0;
}

/* Print a compact visual map (64 chars per row) */
void printRAMMap() {
    printf("\n  RAM Map (1024 blocks, each char = 16 blocks)\n  [");
    for (int i = 0; i < RAM_SIZE; i += 16) {
        int occupied = 0;
        for (int j = i; j < i + 16 && j < RAM_SIZE; j++)
            if (RAM[j]) { occupied = 1; break; }
        printf("%c", occupied ? '#' : '.');
    }
    printf("]\n  Legend: # = occupied   . = free\n\n");
}

/* ─────────────────────────────────────────────
   2. SINGLY LINKED LIST  –  PCB List
───────────────────────────────────────────── */
typedef struct PCB {
    int   pid;
    char  name[32];
    int   start;
    int   size;
    int   priority;   /* 1 = normal, higher = more urgent */
    struct PCB *next;
} PCB;

PCB *pcbHead = NULL;
int  nextPID  = 1;

PCB *createPCB(const char *name, int start, int size, int priority) {
    PCB *p = (PCB *)malloc(sizeof(PCB));
    p->pid      = nextPID++;
    strncpy(p->name, name, 31); p->name[31] = '\0';
    p->start    = start;
    p->size     = size;
    p->priority = priority;
    p->next     = NULL;
    return p;
}

void insertPCB(PCB *p) {
    p->next = pcbHead;
    pcbHead = p;
}

PCB *findPCB(int pid) {
    PCB *cur = pcbHead;
    while (cur) { if (cur->pid == pid) return cur; cur = cur->next; }
    return NULL;
}

void removePCB(int pid) {
    PCB **cur = &pcbHead;
    while (*cur) {
        if ((*cur)->pid == pid) {
            PCB *tmp = *cur;
            *cur = tmp->next;
            free(tmp);
            return;
        }
        cur = &(*cur)->next;
    }
}

void listPCB() {
    if (!pcbHead) { printf("  [PCB List is empty]\n"); return; }
    printf("  %-6s %-16s %-8s %-8s %-8s\n","PID","Name","Start","Size","Priority");
    printf("  %-6s %-16s %-8s %-8s %-8s\n","---","----","-----","----","--------");
    PCB *cur = pcbHead;
    while (cur) {
        printf("  %-6d %-16s %-8d %-8d %-8d\n",
               cur->pid, cur->name, cur->start, cur->size, cur->priority);
        cur = cur->next;
    }
    printf("\n");
}

/* ─────────────────────────────────────────────
   3. DOUBLY LINKED LIST  –  Memory Holes
───────────────────────────────────────────── */
typedef struct Hole {
    int start;
    int size;
    struct Hole *prev;
    struct Hole *next;
} Hole;

Hole *holeHead = NULL;

void insertHole(int start, int size) {
    Hole *h = (Hole *)malloc(sizeof(Hole));
    h->start = start; h->size = size;
    h->prev = NULL;   h->next = holeHead;
    if (holeHead) holeHead->prev = h;
    holeHead = h;
}

void removeHole(Hole *h) {
    if (h->prev) h->prev->next = h->next;
    else         holeHead      = h->next;
    if (h->next) h->next->prev = h->prev;
    free(h);
}

/* Merge adjacent holes (called after freeing a process) */
void mergeHoles() {
    /* Simple O(n²) merge – acceptable for simulation */
    int changed = 1;
    while (changed) {
        changed = 0;
        Hole *a = holeHead;
        while (a) {
            Hole *b = holeHead;
            while (b) {
                if (b != a && a->start + a->size == b->start) {
                    a->size += b->size;
                    removeHole(b);
                    changed = 1;
                    break;
                }
                b = b->next;
            }
            a = a->next;
        }
    }
}

/* Strategy: 0=First Fit, 1=Best Fit, 2=Worst Fit */
Hole *findHole(int size, int strategy) {
    Hole *best = NULL, *cur = holeHead;
    while (cur) {
        if (cur->size >= size) {
            if (strategy == 0) return cur;           /* First Fit */
            if (strategy == 1) {                     /* Best Fit  */
                if (!best || cur->size < best->size) best = cur;
            }
            if (strategy == 2) {                     /* Worst Fit */
                if (!best || cur->size > best->size) best = cur;
            }
        }
        cur = cur->next;
    }
    return best;
}

void listHoles() {
    if (!holeHead) { printf("  [No free memory holes]\n"); return; }
    printf("  %-10s %-10s\n","Start","Size");
    printf("  %-10s %-10s\n","-----","----");
    Hole *cur = holeHead;
    while (cur) {
        printf("  %-10d %-10d\n", cur->start, cur->size);
        cur = cur->next;
    }
    printf("\n");
}

/* ─────────────────────────────────────────────
   4. QUEUE  –  FIFO Page Replacement
───────────────────────────────────────────── */
int  fifoQueue[MAX_FIFO_QUEUE];
int  fifoFront = 0, fifoRear = 0, fifoCount = 0;

void fifoEnqueue(int pid) {
    if (fifoCount >= MAX_FIFO_QUEUE) return;
    fifoQueue[fifoRear] = pid;
    fifoRear = (fifoRear + 1) % MAX_FIFO_QUEUE;
    fifoCount++;
}

int fifoDequeue() {
    if (fifoCount == 0) return -1;
    int pid = fifoQueue[fifoFront];
    fifoFront = (fifoFront + 1) % MAX_FIFO_QUEUE;
    fifoCount--;
    return pid;
}

void removeFIFO(int pid) {
    /* Remove a specific pid (used when process terminates normally) */
    int tmp[MAX_FIFO_QUEUE]; int n = 0;
    while (fifoCount > 0) {
        int p = fifoDequeue();
        if (p != pid) tmp[n++] = p;
    }
    for (int i = 0; i < n; i++) fifoEnqueue(tmp[i]);
}

/* ─────────────────────────────────────────────
   5. STACK  –  Function Call Simulation
───────────────────────────────────────────── */
typedef struct {
    char funcName[32];
    int  returnAddr;
} StackFrame;

StackFrame callStack[MAX_STACK];
int        stackTop = -1;

void pushStack(const char *fn, int retAddr) {
    if (stackTop >= MAX_STACK - 1) {
        printf("  [Stack Overflow!]\n"); return;
    }
    stackTop++;
    strncpy(callStack[stackTop].funcName, fn, 31);
    callStack[stackTop].returnAddr = retAddr;
    printf("  PUSH -> %s (return addr: %d)\n", fn, retAddr);
}

void popStack() {
    if (stackTop < 0) {
        printf("  [Stack Underflow!]\n"); return;
    }
    printf("  POP  <- %s (returned to: %d)\n",
           callStack[stackTop].funcName,
           callStack[stackTop].returnAddr);
    stackTop--;
}

void printStack() {
    if (stackTop < 0) { printf("  [Call Stack is empty]\n"); return; }
    printf("  Call Stack (top -> bottom):\n");
    for (int i = stackTop; i >= 0; i--)
        printf("    [%d] %s  (ret: %d)\n", i,
               callStack[i].funcName, callStack[i].returnAddr);
    printf("\n");
}

/* ─────────────────────────────────────────────
   6. CIRCULAR LINKED LIST  –  Round Robin CPU
───────────────────────────────────────────── */
typedef struct CNode {
    int pid;
    int remainingBurst;
    struct CNode *next;
} CNode;

CNode *rrHead    = NULL;
CNode *rrCurrent = NULL;

void rrInsert(int pid, int burst) {
    CNode *n = (CNode *)malloc(sizeof(CNode));
    n->pid           = pid;
    n->remainingBurst = burst;
    if (!rrHead) {
        n->next  = n;
        rrHead   = n;
        rrCurrent = n;
    } else {
        /* Find tail */
        CNode *t = rrHead;
        while (t->next != rrHead) t = t->next;
        t->next = n;
        n->next = rrHead;
    }
}

void rrRemove(int pid) {
    if (!rrHead) return;
    CNode *cur = rrHead, *prev = NULL;
    /* Find tail first */
    CNode *tail = rrHead;
    while (tail->next != rrHead) tail = tail->next;
    do {
        if (cur->pid == pid) {
            if (cur == rrHead && cur->next == rrHead) {
                /* Only one node */
                rrHead = NULL; rrCurrent = NULL;
            } else {
                if (prev) prev->next = cur->next;
                else { tail->next = cur->next; rrHead = cur->next; }
                if (rrCurrent == cur) rrCurrent = cur->next;
            }
            free(cur);
            return;
        }
        prev = cur;
        cur  = cur->next;
    } while (cur != rrHead);
}

/* Execute one Round Robin tick */
void rrTick() {
    if (!rrCurrent) { printf("  [No processes in RR scheduler]\n"); return; }
    int pid   = rrCurrent->pid;
    PCB *proc = findPCB(pid);
    int  run  = (rrCurrent->remainingBurst > TIME_QUANTUM)
                ? TIME_QUANTUM : rrCurrent->remainingBurst;
    rrCurrent->remainingBurst -= run;
    printf("  RR: PID %d (%s) ran for %d unit(s). Remaining burst: %d\n",
           pid, proc ? proc->name : "?", run, rrCurrent->remainingBurst);
    if (rrCurrent->remainingBurst <= 0) {
        printf("  RR: PID %d finished execution.\n", pid);
        CNode *next = rrCurrent->next;
        rrRemove(pid);
        rrCurrent = (rrHead) ? next : NULL;
    } else {
        rrCurrent = rrCurrent->next;
    }
}

void listRR() {
    if (!rrHead) { printf("  [RR Scheduler is empty]\n"); return; }
    printf("  Round Robin Queue:\n");
    CNode *cur = rrHead;
    do {
        PCB *p = findPCB(cur->pid);
        printf("    PID %d (%s) - burst left: %d%s\n",
               cur->pid, p ? p->name : "?",
               cur->remainingBurst,
               cur == rrCurrent ? "  <-- CURRENT" : "");
        cur = cur->next;
    } while (cur != rrHead);
    printf("\n");
}

/* ─────────────────────────────────────────────
   7. PRIORITY QUEUE  –  Interrupt Scheduler
   (Max-Heap: highest priority served first)
───────────────────────────────────────────── */
typedef struct {
    int pid;
    int priority;
    char name[32];
} PQEntry;

PQEntry pq[MAX_PQUEUE];
int     pqSize = 0;

void pqSwap(int a, int b) {
    PQEntry t = pq[a]; pq[a] = pq[b]; pq[b] = t;
}

void pqPush(int pid, int priority, const char *name) {
    if (pqSize >= MAX_PQUEUE) { printf("  [Priority Queue full]\n"); return; }
    pq[pqSize].pid      = pid;
    pq[pqSize].priority = priority;
    strncpy(pq[pqSize].name, name, 31); pq[pqSize].name[31] = '\0';
    int i = pqSize++;
    while (i > 0 && pq[(i-1)/2].priority < pq[i].priority) {
        pqSwap(i, (i-1)/2); i = (i-1)/2;
    }
    printf("  [Interrupt] PID %d (%s) added with priority %d\n", pid, name, priority);
}

void pqPop() {
    if (pqSize == 0) { printf("  [Priority Queue empty – no interrupts]\n"); return; }
    printf("  [Interrupt Handled] PID %d (%s) priority %d\n",
           pq[0].pid, pq[0].name, pq[0].priority);
    pq[0] = pq[--pqSize];
    int i = 0;
    while (1) {
        int largest = i, l = 2*i+1, r = 2*i+2;
        if (l < pqSize && pq[l].priority > pq[largest].priority) largest = l;
        if (r < pqSize && pq[r].priority > pq[largest].priority) largest = r;
        if (largest == i) break;
        pqSwap(i, largest); i = largest;
    }
}

void listPQ() {
    if (pqSize == 0) { printf("  [Priority Queue is empty]\n"); return; }
    printf("  Priority Queue (not sorted for display):\n");
    printf("  %-6s %-16s %-8s\n","PID","Name","Priority");
    printf("  %-6s %-16s %-8s\n","---","----","--------");
    for (int i = 0; i < pqSize; i++)
        printf("  %-6d %-16s %-8d\n", pq[i].pid, pq[i].name, pq[i].priority);
    printf("\n");
}

/* ─────────────────────────────────────────────
   HASH TABLE  –  Fast PCB Lookup (Enhancement)
───────────────────────────────────────────── */
PCB *hashTable[HASH_SIZE];

void hashInsert(PCB *p) {
    int idx = p->pid % HASH_SIZE;
    p->next      = hashTable[idx];
    hashTable[idx] = p;
}

PCB *hashFind(int pid) {
    PCB *cur = hashTable[pid % HASH_SIZE];
    while (cur) { if (cur->pid == pid) return cur; cur = cur->next; }
    return NULL;
}

/* ─────────────────────────────────────────────
   CORE OPERATIONS
───────────────────────────────────────────── */

int strategy = 0;   /* 0=First Fit, 1=Best Fit, 2=Worst Fit */

/* Try to allocate a process – returns 1 on success, 0 on failure */
int allocateProcess(const char *name, int size, int priority, int burst) {
    /* Check if any hole is big enough */
    Hole *h = findHole(size, strategy);
    if (!h) {
        /* FIFO page replacement: evict oldest */
        if (fifoCount == 0) {
            printf("  [ERROR] Not enough memory and nothing to evict!\n");
            return 0;
        }
        int evict = fifoDequeue();
        PCB *ep   = findPCB(evict);
        if (ep) {
            printf("  [FIFO] Evicting PID %d (%s) to free %d blocks\n",
                   evict, ep->name, ep->size);
            freeRAM(evict);
            insertHole(ep->start, ep->size);
            mergeHoles();
            rrRemove(evict);
            removePCB(evict);
        }
        h = findHole(size, strategy);
        if (!h) {
            printf("  [ERROR] Still not enough memory after eviction!\n");
            return 0;
        }
    }

    /* Allocate */
    int start = h->start;
    if (h->size > size) {
        /* Shrink the hole */
        h->start += size;
        h->size  -= size;
    } else {
        removeHole(h);
    }

    allocateRAM(0, start, size);   /* mark in array */
    PCB *p = createPCB(name, start, size, priority);
    allocateRAM(p->pid, start, size);
    insertPCB(p);
    fifoEnqueue(p->pid);
    rrInsert(p->pid, burst);

    printf("  [ALLOC] PID %d (%s) -> Start: %d  Size: %d  Burst: %d\n",
           p->pid, p->name, start, size, burst);
    return 1;
}

void terminateProcess(int pid) {
    PCB *p = findPCB(pid);
    if (!p) { printf("  [ERROR] PID %d not found.\n", pid); return; }
    printf("  [TERM] Terminating PID %d (%s)\n", pid, p->name);
    freeRAM(pid);
    insertHole(p->start, p->size);
    mergeHoles();
    removeFIFO(pid);
    rrRemove(pid);
    removePCB(pid);
}

/* ─────────────────────────────────────────────
   INITIALIZATION
───────────────────────────────────────────── */
void initSystem() {
    initRAM();
    memset(hashTable, 0, sizeof(hashTable));
    /* One big free hole covering all of RAM */
    insertHole(0, RAM_SIZE);
    printf("  System initialized. RAM: %d blocks. One free hole.\n\n", RAM_SIZE);
}

/* ─────────────────────────────────────────────
   MENUS & UI
───────────────────────────────────────────── */
void printSeparator() {
    printf("  ============================================================\n");
}

void printHeader() {
    system("cls || clear");
    printSeparator();
    printf("    Multi-Level Computer Memory Manager (RAM Simulator)\n");
    printf("    IN 1111 - Data Structures and Algorithms 1\n");
    printf("    Faculty of IT, University of Moratuwa\n");
    printSeparator();
}

void printMainMenu() {
    printf("\n  MAIN MENU\n");
    printf("  ---------\n");
    printf("  1. Process Management\n");
    printf("  2. Memory Management\n");
    printf("  3. CPU Scheduler (Round Robin)\n");
    printf("  4. Interrupt Scheduler (Priority Queue)\n");
    printf("  5. Function Call Stack\n");
    printf("  6. System Status\n");
    printf("  7. Settings\n");
    printf("  0. Exit\n");
    printf("\n  Choice: ");
}

void menuProcess() {
    int c;
    do {
        printHeader();
        printf("\n  PROCESS MANAGEMENT\n");
        printf("  ------------------\n");
        printf("  1. Create Process\n");
        printf("  2. Terminate Process\n");
        printf("  3. List All Processes (PCB List)\n");
        printf("  0. Back\n");
        printf("\n  Choice: ");
        scanf("%d", &c); getchar();

        if (c == 1) {
            char name[32]; int size, priority, burst;
            printf("  Process name : "); fgets(name, 32, stdin);
            name[strcspn(name,"\n")] = 0;
            printf("  Memory size  : "); scanf("%d", &size);
            printf("  Priority(1+) : "); scanf("%d", &priority);
            printf("  CPU burst    : "); scanf("%d", &burst);
            getchar();
            allocateProcess(name, size, priority, burst);
            printf("\n  Press Enter..."); getchar();
        } else if (c == 2) {
            int pid;
            printf("  Enter PID to terminate: "); scanf("%d", &pid); getchar();
            terminateProcess(pid);
            printf("\n  Press Enter..."); getchar();
        } else if (c == 3) {
            printHeader();
            printf("\n  PCB LIST\n"); printf("  --------\n");
            listPCB();
            printf("  Press Enter..."); getchar();
        }
    } while (c != 0);
}

void menuMemory() {
    int c;
    do {
        printHeader();
        printf("\n  MEMORY MANAGEMENT\n");
        printf("  -----------------\n");
        printf("  1. View RAM Map\n");
        printf("  2. View Memory Holes (Free Regions)\n");
        printf("  3. View FIFO Queue (Page Replacement Order)\n");
        printf("  0. Back\n");
        printf("\n  Choice: ");
        scanf("%d", &c); getchar();

        if (c == 1) {
            printHeader();
            printRAMMap();
            printf("  Press Enter..."); getchar();
        } else if (c == 2) {
            printHeader();
            printf("\n  FREE MEMORY HOLES\n"); printf("  -----------------\n");
            listHoles();
            printf("  Press Enter..."); getchar();
        } else if (c == 3) {
            printHeader();
            printf("\n  FIFO PAGE REPLACEMENT QUEUE\n");
            printf("  ---------------------------\n");
            if (fifoCount == 0) { printf("  [Queue is empty]\n"); }
            else {
                printf("  Order (oldest first): ");
                for (int i = 0; i < fifoCount; i++)
                    printf("PID %d  ", fifoQueue[(fifoFront + i) % MAX_FIFO_QUEUE]);
                printf("\n");
            }
            printf("\n  Press Enter..."); getchar();
        }
    } while (c != 0);
}

void menuRR() {
    int c;
    do {
        printHeader();
        printf("\n  CPU ROUND ROBIN SCHEDULER  (Quantum = %d)\n", TIME_QUANTUM);
        printf("  ------------------------------------------\n");
        printf("  1. View Scheduler Queue\n");
        printf("  2. Execute One Tick\n");
        printf("  3. Execute Until All Finish\n");
        printf("  0. Back\n");
        printf("\n  Choice: ");
        scanf("%d", &c); getchar();

        if (c == 1) {
            printHeader();
            printf("\n  ROUND ROBIN QUEUE\n"); printf("  -----------------\n");
            listRR();
            printf("  Press Enter..."); getchar();
        } else if (c == 2) {
            rrTick();
            printf("  Press Enter..."); getchar();
        } else if (c == 3) {
            int limit = 200, count = 0;
            while (rrHead && count++ < limit) rrTick();
            if (count >= limit) printf("  [Warning] Stopped after %d ticks.\n", limit);
            printf("  Press Enter..."); getchar();
        }
    } while (c != 0);
}

void menuPQ() {
    int c;
    do {
        printHeader();
        printf("\n  INTERRUPT SCHEDULER (Priority Queue)\n");
        printf("  ------------------------------------\n");
        printf("  1. View Priority Queue\n");
        printf("  2. Add Interrupt\n");
        printf("  3. Handle Next Interrupt\n");
        printf("  0. Back\n");
        printf("\n  Choice: ");
        scanf("%d", &c); getchar();

        if (c == 1) {
            printHeader();
            printf("\n  PRIORITY QUEUE\n"); printf("  --------------\n");
            listPQ();
            printf("  Press Enter..."); getchar();
        } else if (c == 2) {
            char name[32]; int pid, prio;
            printf("  Interrupt name : "); fgets(name, 32, stdin);
            name[strcspn(name,"\n")] = 0;
            printf("  PID            : "); scanf("%d", &pid); getchar();
            printf("  Priority (1-10): "); scanf("%d", &prio); getchar();
            pqPush(pid, prio, name);
            printf("  Press Enter..."); getchar();
        } else if (c == 3) {
            pqPop();
            printf("  Press Enter..."); getchar();
        }
    } while (c != 0);
}

void menuStack() {
    int c;
    do {
        printHeader();
        printf("\n  FUNCTION CALL STACK\n");
        printf("  -------------------\n");
        printf("  1. View Call Stack\n");
        printf("  2. Push (call function)\n");
        printf("  3. Pop  (return from function)\n");
        printf("  0. Back\n");
        printf("\n  Choice: ");
        scanf("%d", &c); getchar();

        if (c == 1) {
            printHeader();
            printf("\n  CALL STACK\n"); printf("  ----------\n");
            printStack();
            printf("  Press Enter..."); getchar();
        } else if (c == 2) {
            char fn[32]; int ret;
            printf("  Function name   : "); fgets(fn, 32, stdin);
            fn[strcspn(fn,"\n")] = 0;
            printf("  Return address  : "); scanf("%d", &ret); getchar();
            pushStack(fn, ret);
            printf("  Press Enter..."); getchar();
        } else if (c == 3) {
            popStack();
            printf("  Press Enter..."); getchar();
        }
    } while (c != 0);
}

void menuStatus() {
    printHeader();
    printf("\n  FULL SYSTEM STATUS\n");
    printf("  ------------------\n");
    printf("  [1] PCB List:\n"); listPCB();
    printf("  [2] Memory Holes:\n"); listHoles();
    printf("  [3] RR Scheduler:\n"); listRR();
    printf("  [4] Priority Queue:\n"); listPQ();
    printf("  [5] Call Stack:\n"); printStack();
    printRAMMap();
    printf("  Press Enter..."); getchar();
}

void menuSettings() {
    int c;
    printHeader();
    printf("\n  SETTINGS\n");
    printf("  --------\n");
    printf("  Current allocation strategy: %s\n",
           strategy == 0 ? "First Fit" : strategy == 1 ? "Best Fit" : "Worst Fit");
    printf("  1. First Fit\n");
    printf("  2. Best Fit\n");
    printf("  3. Worst Fit\n");
    printf("  0. Back\n");
    printf("\n  Choice: ");
    scanf("%d", &c); getchar();
    if (c >= 1 && c <= 3) {
        strategy = c - 1;
        printf("  Strategy set to: %s\n",
               strategy == 0 ? "First Fit" : strategy == 1 ? "Best Fit" : "Worst Fit");
    }
    printf("  Press Enter..."); getchar();
}

/* ─────────────────────────────────────────────
   MAIN
───────────────────────────────────────────── */
int main() {
    initSystem();

    /* Seed with a few demo processes */
    printf("  Loading demo processes...\n");
    allocateProcess("kernel",    128, 10, 5);
    allocateProcess("init",       64,  5, 8);
    allocateProcess("scheduler",  32,  3, 6);
    printf("\n  Press Enter to start...\n"); getchar();

    int choice;
    do {
        printHeader();
        printMainMenu();
        scanf("%d", &choice); getchar();
        switch (choice) {
            case 1: menuProcess();  break;
            case 2: menuMemory();   break;
            case 3: menuRR();       break;
            case 4: menuPQ();       break;
            case 5: menuStack();    break;
            case 6: menuStatus();   break;
            case 7: menuSettings(); break;
            case 0: break;
            default: printf("  Invalid choice.\n"); getchar();
        }
    } while (choice != 0);

    printf("\n  Exiting RAM Simulator. Goodbye!\n\n");
    return 0;
}
