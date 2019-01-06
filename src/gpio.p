.origin 0
.entrypoint INIT

// load IEP timer to register
.macro ltimer
.mparam reg
        lbco reg, c26, 0x0c, 4
.endm

.macro sirq
.mparam irq
    mov r31, (irq)|1
.endm

#define PRU_GPINs   r31
#define BELL_PIN    0x2
#define BV_IRQ3     (1<<5)

// 0.04s/0.000000005s =
#define DEBOUNCE    8000000

INIT:
    zero    &r0, 120
    mov     r0, 0x00022000      // base address of PRU0 config registers
    //mov   r0, 0x00024000      // base address of PRU1 config registers

    mov     r1, 0x00000000      // set base addresses of constant-registers
    sbbo    r1, r0, 0x24, 4    // c26( IEP Timer )

    mov     r0, 0x000010011     // configure IEP counter (CMP_INC=1, DEF_INC=1, EN=1)
    sbco    r0, c26, 0x00, 4
    zero    &r0, 120

// registers:
// r0: current ts,
// r1: last ts,
// r2: time delta,
// r3: debounce value
// r4: interrupt emitted flag

repeat:
    qbbs    debounce, PRU_GPINs, BELL_PIN   // branch if bit is set

    // Pin not active
    ltimer  r1              // load last LOW ts
    mov     r4, 0           // reset irq marker
    jmp     repeat

debounce: // Pin active
    ltimer  r0              // load last HIGH ts
    sub     r2, r0, r1      // calculate delta
    mov     r3, DEBOUNCE
    qble    repeat, r3, r2  // jump if still not long enough active

    qbne    repeat, r4, 0   // check if irq already emitted
    sirq    BV_IRQ3         // emit irq
    mov     r4, 1           // mark irq as emitted
    jmp repeat

// Never should get here
    HALT
