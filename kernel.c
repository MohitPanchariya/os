#include "io.h"
#include "types.h"
#include "memory.h"
#include "kernel.h"
#include "process.h"
#include "sbi.h"
#include "syscall.h"
#include "string.h"

extern char __stack_top[], __bss[], __bss_end[];
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];

struct virtio_virtq* blk_request_vq;
struct virtio_blk_req* blk_req;
paddr_t blk_req_paddr;
uint64_t blk_capacity;

void putchar(char ch) {
    // calls Console Putchar exposed by OpenSBI
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

long getchar(void) {
    // OpenSBI has a driver to read input from a serial connection
    struct sbi_ret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.error;
}

void handle_trap(struct trap_frame* f) {
    // cause of crash
    uint32_t scause = READ_CSR(scause);
    // additional info on crash
    uint32_t stval = READ_CSR(stval);
    // pc pointing to the point that caused the crash
    uint32_t user_pc = READ_CSR(sepc);

    if (scause == SCAUSE_ECALL) {
        handle_syscall(f);
        // move the PC by 4 bytes, pointing to the instruction after ecall
        user_pc += 4;
    } else {
        PANIC("trap triggered. scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
    }
    WRITE_CSR(sepc, user_pc);
}

void handle_syscall(struct trap_frame* f) {
    switch (f->a3)
    {
    case SYS_PUTCHAR:
        putchar(f->a0);
        break;
    case SYS_GETCHAR:
        while(1) {
            long ch = getchar();
            if (ch >= 0) {
                f->a0 = ch;
                break;
            }
            // yield the CPU to other processes instead of
            // waiting for a new character all the time
            yield();
        }
        break;
    case SYS_EXIT:
        printf("\nexiting process %d\n", current_proc->pid);
        current_proc->state = PROC_EXITED;
        yield();
        PANIC("exited process %d started running", current_proc->pid);
        break;
    default:
        PANIC("unexpected syscall a3=%x\n", f->a3);
        break;
    }
}

__attribute__((naked))
__attribute__((aligned(4)))
void exception_handler(void) {
    __asm__ __volatile__(
        // swap sscratch and sp
        // store kernel stack pointer in sp
        // and contents of sp in sscratch
        "csrrw sp, sscratch, sp\n" // save stack pointer to sscratch register
        "addi sp, sp, -4 * 31\n" // allocate space for the trap_frame struct
        // write out the general-purpose registers to stack
        "sw ra,  4 * 0(sp)\n"
        "sw gp,  4 * 1(sp)\n"
        "sw tp,  4 * 2(sp)\n"
        "sw t0,  4 * 3(sp)\n"
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

        /*
        The reason sscratch is read into a0, and then
        written to 4 * 30(sp) is because the sw instruction
        doesn't directly work with the sscratch register
        sw sscratch, 4 * 30(sp) doesn't work
        */
        "csrr a0, sscratch\n"
        // the trap_frame's last member is the stack pointer
        // store the original stack pointer at the end of the
        // trap_frame
        "sw a0, 4 * 30(sp)\n"

        // TODO: check if the following two instructions
        // serve the smae purpose as the following 3 instrutions
        // addi sp, sp, 4 * 31
        // mv sp, a0

        // reset the kernel stack pointer
        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

        // copy stack pointer into a0 register
        // a0 holds the first parameter for a
        // function being called
        "mv a0, sp\n"

        // invoke the trap handler function
        "call handle_trap\n"

        // restore register values
        "lw ra,  4 * 0(sp)\n"
        "lw gp,  4 * 1(sp)\n"
        "lw tp,  4 * 2(sp)\n"
        "lw t0,  4 * 3(sp)\n"
        "lw t1,  4 * 4(sp)\n"
        "lw t2,  4 * 5(sp)\n"
        "lw t3,  4 * 6(sp)\n"
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n"
    );
}


void delay(void) {
    for(int i = 0; i < 30000000; i++) {
        __asm__ __volatile__("nop");
    }
}

struct process* proc_a;
struct process* proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        yield();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        yield();
    }
}

void kmain(void) {
    // initialize the bss section with 0s
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    // setup exception handler
    // In riscv32, stvec stores the address of the exception
    // handler for U/S mode exceptions
    WRITE_CSR(stvec, (uint32_t) exception_handler);

    virtio_blk_init();

    char buf[SECTOR_SIZE];
    read_write_disk(buf, 0, false);
    printf("first sector: %s\n", buf);

    strcpy(buf, "hello from kernel!\n");
    read_write_disk(buf, 0, true);

    printf("Kernel says %s\n", "Hello!");

    proc_init();

    // start the shell, the first userspace process
    create_process(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);

    yield();
    PANIC("Returned to idle process");

    for (;;); // spin forever
}

/*
The boot function is placed at .text.boot since
the first section in the linked executable is .text.boot
*/
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // place stack top in stack pointer
        "j kmain\n" // jump to kmain function
        :
        : [stack_top] "r" (__stack_top)
    );
}

uint32_t virtio_reg_read32(unsigned offset) {
    return *((volatile uint32_t*) (VIRTIO_BLK_PADDR + offset));
}

uint64_t virtio_reg_read64(unsigned offset) {
    return *((volatile uint64_t*) (VIRTIO_BLK_PADDR + offset));
}

void virtio_reg_write32(unsigned offset, uint32_t value) {
    *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset)) = value;
}

void virtio_reg_fetch_and_or32(unsigned offset, uint32_t value) {
    virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}

// initialize a block device
void virtio_blk_init(void) {
  // docs on block device - https://docs.oasis-open.org/virtio/virtio/v1.4/virtio-v1.4.pdf
  // Section 4.2 defines Virtio over MMIO
  if(virtio_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976) {
    PANIC("virtio: invalid magic value");
  }
  if(virtio_reg_read32(VIRTIO_REG_VERSION) != 1) {
    PANIC("virtio: invalid version");
  }
  if(virtio_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK) {
    PANIC("virtio: invalid device id");
  }

  // reset the device
  virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);
  // set ACKNOWLEDGE status bit: kernel found the device
  virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
  // set the DRIVER status bit: kernel knows how to use the device
  virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);
  // set page size
  virtio_reg_write32(VIRTIO_REG_PAGE_SIZE, PAGE_SIZE);
  // initialize a queue for disk read/write requests
  blk_request_vq = virtq_init(0);
  // set DRIVER_OK status bit: the device is ready to be used
  virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK); 

  blk_capacity = virtio_reg_read64(VIRTIO_REG_DEVICE_CONFIG + 0) * SECTOR_SIZE; 

  printf("virtio-blk device capacity is %d bytes\n", (int) blk_capacity);

  blk_req_paddr = allocate_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
  blk_req = (struct virtio_blk_req *) blk_req_paddr;
}

struct virtio_virtq *virtq_init(unsigned index) {
    // Allocate a region for the virtqueue.
    paddr_t virtq_paddr = allocate_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
    struct virtio_virtq *vq = (struct virtio_virtq *) virtq_paddr;
    vq->queue_index = index;
    vq->used_index = (volatile uint16_t *) &vq->used.index;
    // Select the queue: Write the virtqueue index (first queue is 0).
    virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);
    // Specify the queue size: Write the # of descriptors we'll use.
    virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);
    // Write the physical page frame number (not physical address!) of the queue.
    virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr / PAGE_SIZE);
    return vq;
}

// Notifies the device that there is a new request. `desc_index` is the index
// of the head descriptor of the new request.
void virtq_kick(struct virtio_virtq *vq, int desc_index) {
    vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
    vq->avail.index++;
    __sync_synchronize();
    virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
    vq->last_used_index++;
}

// Returns whether there are requests being processed by the device.
bool virtq_is_busy(struct virtio_virtq *vq) {
    return vq->last_used_index != *vq->used_index;
}

// Reads/writes from/to virtio-blk device.
void read_write_disk(void *buf, unsigned sector, int is_write) {
    if (sector >= blk_capacity / SECTOR_SIZE) {
        printf("virtio: tried to read/write sector=%d, but capacity is %d\n",
              sector, blk_capacity / SECTOR_SIZE);
        return;
    }

    // Construct the request according to the virtio-blk specification.
    blk_req->sector = sector;
    blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    if (is_write)
        memcpy(blk_req->data, buf, SECTOR_SIZE);

    // Construct the virtqueue descriptors (using 3 descriptors).
    struct virtio_virtq *vq = blk_request_vq;
    vq->descs[0].addr = blk_req_paddr;
    vq->descs[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
    vq->descs[0].flags = VIRTQ_DESC_F_NEXT;
    vq->descs[0].next = 1;

    vq->descs[1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
    vq->descs[1].len = SECTOR_SIZE;
    vq->descs[1].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
    vq->descs[1].next = 2;

    vq->descs[2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
    vq->descs[2].len = sizeof(uint8_t);
    vq->descs[2].flags = VIRTQ_DESC_F_WRITE;

    // Notify the device that there is a new request.
    virtq_kick(vq, 0);

    // Wait until the device finishes processing.
    while (virtq_is_busy(vq))
        ;

    // virtio-blk: If a non-zero value is returned, it's an error.
    if (blk_req->status != 0) {
        printf("virtio: warn: failed to read/write sector=%d status=%d\n",
               sector, blk_req->status);
        return;
    }

    // For read operations, copy the data into the buffer.
    if (!is_write)
        memcpy(buf, blk_req->data, SECTOR_SIZE);
}
