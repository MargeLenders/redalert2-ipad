/*
 * arena_selftest.c - Verification of 4 GB Arena, memory partitions,
 * and unaligned access macros on LP64 (arm64 / x86_64).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include <unistd.h>
#include "recomp_types.h"
#include "win32hle.h"

const recomp_dispatch_entry_t recomp_dispatch_table[] = { { 0, 0 } };
const uint32_t recomp_dispatch_count = 0;

static sigjmp_buf g_segv_jmp;
static volatile sig_atomic_t g_segv_caught = 0;

static void segv_handler(int sig) {
    (void)sig;
    g_segv_caught = 1;
    siglongjmp(g_segv_jmp, 1);
}

static void test_initialization_and_translation(void) {
    printf("[*] Testing guest memory initialization and address translation...\n");
    guest_memory_init();
    assert(g_mem_base != 0 && "g_mem_base must be initialized");

    guest_ptr_t va = 0x00400000u;
    void *host_p = guest_to_host(va);
    assert(host_p == (void *)((uintptr_t)g_mem_base + va));
    assert(host_to_guest(host_p) == va);

    /* Test boundary VAs */
    assert(guest_to_host(0) == (void *)((uintptr_t)g_mem_base));
    assert(host_to_guest((void *)((uintptr_t)g_mem_base)) == 0);

    assert(guest_to_host(0xFFFFFFFFu) == (void *)((uintptr_t)g_mem_base + 0xFFFFFFFFu));
    assert(host_to_guest((void *)((uintptr_t)g_mem_base + 0xFFFFFFFFu)) == 0xFFFFFFFFu);

    printf("    -> Address translation OK\n");
}

static void test_partition_allocations(void) {
    printf("[*] Testing memory partition bounds and reservations...\n");

    /* 1. Guard page cannot be reserved */
    uint32_t bad_res = hle_vm_reserve(0x00001000u, 0x1000);
    assert(bad_res == 0 && "Reservation in guard page must fail");

    /* 2. PE Image reservation at 0x00400000 */
    uint32_t pe_res = hle_vm_reserve(0x00400000u, 0x00100000u);
    assert(pe_res == 0x00400000u && "PE image reservation must succeed at 0x00400000");

    /* 3. Default dynamic reservation must be in HEAP [0x05000000, 0x80000000) */
    uint32_t heap_res = hle_vm_reserve(0, 0x10000);
    assert(heap_res >= 0x05000000u && heap_res < 0x80000000u && "Dynamic alloc must land in HEAP");

    /* 4. Range-specific reservation in STACK [0x01000000, 0x05000000) */
    uint32_t stack_res = hle_vm_reserve_in(0x01000000u, 0x05000000u, 0x20000);
    assert(stack_res >= 0x01000000u && (stack_res + 0x20000) <= 0x05000000u && "Stack alloc must land in STACK range");

    /* 5. Range-specific reservation in COM [0xE0000000, 0xF0000000) */
    uint32_t com_res = hle_vm_reserve_in(0xE0000000u, 0xF0000000u, 0x10000);
    assert(com_res >= 0xE0000000u && (com_res + 0x10000) <= 0xF0000000u && "COM alloc must land in COM range");

    /* 6. Release allocated regions */
    hle_vm_release(stack_res, 0x20000);
    hle_vm_release(com_res, 0x10000);
    hle_vm_release(heap_res, 0x10000);
    hle_vm_release(pe_res, 0x00100000u);

    printf("    -> Memory partitions OK\n");
}

static void test_guard_page_protection(void) {
    printf("[*] Testing guard page PROT_NONE catching null dereference...\n");
    struct sigaction sa, old_sa_segv, old_sa_bus;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = segv_handler;
    sigaction(SIGSEGV, &sa, &old_sa_segv);
    sigaction(SIGBUS, &sa, &old_sa_bus);

    g_segv_caught = 0;
    if (sigsetjmp(g_segv_jmp, 1) == 0) {
        /* Attempt to read guest VA 0 (null dereference) */
        volatile uint32_t val = MEM_U32(0);
        (void)val;
        assert(false && "Read from VA 0 should have faulted!");
    } else {
        assert(g_segv_caught == 1 && "SIGSEGV/SIGBUS must be caught on null deref");
    }

    g_segv_caught = 0;
    if (sigsetjmp(g_segv_jmp, 1) == 0) {
        /* Attempt to read guest VA 0x0000FFF0 (inside 64KB guard page) */
        volatile uint32_t val = MEM_U32(0x0000FFF0u);
        (void)val;
        assert(false && "Read from VA 0x0000FFF0 should have faulted!");
    } else {
        assert(g_segv_caught == 1 && "SIGSEGV/SIGBUS must be caught inside guard page");
    }

    /* Restore handlers */
    sigaction(SIGSEGV, &old_sa_segv, NULL);
    sigaction(SIGBUS, &old_sa_bus, NULL);

    printf("    -> Guard page protection OK\n");
}

static void test_unaligned_memory_access(void) {
    printf("[*] Testing unaligned memory access macros on arm64...\n");

    /* Allocate a buffer in guest heap for read/write testing */
    uint32_t buf_va = hle_vm_reserve(0, 0x10000);
    assert(buf_va != 0);

    /* Test unaligned 16-bit access */
    MEM_U16(buf_va + 1) = 0x1234;
    assert(MEM_U16(buf_va + 1) == 0x1234);
    assert(MEM_U8(buf_va + 1) == 0x34); /* little-endian */
    assert(MEM_U8(buf_va + 2) == 0x12);

    /* Test unaligned 32-bit access */
    MEM_U32(buf_va + 3) = 0xAABBCCDDu;
    assert(MEM_U32(buf_va + 3) == 0xAABBCCDDu);
    assert(MEM_I32(buf_va + 3) == (int32_t)0xAABBCCDDu);
    assert(MEM_U8(buf_va + 3) == 0xDD);
    assert(MEM_U8(buf_va + 4) == 0xCC);
    assert(MEM_U8(buf_va + 5) == 0xBB);
    assert(MEM_U8(buf_va + 6) == 0xAA);

    /* Test unaligned 64-bit access */
    MEM_U64(buf_va + 7) = 0x0123456789ABCDEFULL;
    assert(MEM_U64(buf_va + 7) == 0x0123456789ABCDEFULL);
    assert(MEM_I64(buf_va + 7) == (int64_t)0x0123456789ABCDEFULL);

    /* Test unaligned float / double */
    MEM_F32(buf_va + 9) = 3.14159265f;
    float f = MEM_F32(buf_va + 9);
    assert(f > 3.14159f && f < 3.14160f);

    MEM_F64(buf_va + 11) = 2.718281828459045;
    double d = MEM_F64(buf_va + 11);
    assert(d > 2.718281828459 && d < 2.718281828460);

    hle_vm_release(buf_va, 0x10000);
    printf("    -> Unaligned memory access OK\n");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("=== RUNNING GATE 1: ARENA & LP64 SELFTEST ===\n");
    test_initialization_and_translation();
    test_partition_allocations();
    test_guard_page_protection();
    test_unaligned_memory_access();
    printf("=== GATE 1 SELFTEST PASSED SUCCESSFULLY! ===\n");
    return 0;
}
