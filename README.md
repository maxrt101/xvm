```
 ██╗  ██╗██╗   ██╗███╗   ███╗
 ╚██╗██╔╝██║   ██║████╗ ████║
  ╚███╔╝ ██║   ██║██╔████╔██║
  ██╔██╗ ╚██╗ ██╔╝██║╚██╔╝██║
 ██╔╝ ██╗ ╚████╔╝ ██║ ╚═╝ ██║
 ╚═╝  ╚═╝  ╚═══╝  ╚═╝     ╚═╝
 
 X Virtual Machine
```

### About
This is a custom virtual machine with custom instruction set & assmbly language.  

### How To Install
Prerequisites: `clang` or `gcc`, `make`  
Steps:  
  - Clone the repo
  - Run `make`
  - Run `./build/bin/xvm help`

### Instruction Set

```
MNEMONIC      ARGS          MODES         DESC
push          VALUE         IMM           Pushes value onto stack
pop           [COUNT]       IMM           Pops COUNT values from stack (defalt 1)
dup                         IMM           Dups top value
rol                         IMM           Rolls 2 top values
deref8        [ADDR]        IMM     STK   Dereferences an address (pointer to byte)
deref16       [ADDR]        IMM     STK   Dereferences an address (pointer to short)
deref32       [ADDR]        IMM     STK   Dereferences an address (pointer to int)
store8        [ADDR]        IMM     STK   Stores 8 bits from top stack value to ADDR
store16       [ADDR]        IMM     STK   Stores 16 bits from top stack value to ADDR
store32       [ADDR]        IMM     STK   Stores 32 bits from top stack value to ADDR
load8         [ADDR]        IMM     STK   Pushes 8 bits from ADDR onto stack
load16        [ADDR]        IMM     STK   Pushes 16 bits from ADDR onto stack
load32        [ADDR]        IMM     STK   Pushes 32 bits from ADDR onto stack
add           [OP1 [OP2]]   IMM IND STK   Adds 2 values (IMM or from stack)
sub           [OP1 [OP2]]   IMM IND STK   Subtracts 2 values (IMM or from stack)
mul           [OP1 [OP2]]   IMM IND STK   Multiplies 2 values (IMM or from stack)
div           [OP1 [OP2]]   IMM IND STK   Divides 2 values (IMM or from stack)
equ           [OP1 [OP2]]   IMM IND STK   Compares 2 values (IMM or from stack)
lt            [OP1 [OP2]]   IMM IND STK   Less Then 2 values (IMM or from stack)
gt            [OP1 [OP2]]   IMM IND STK   Greater Then 2 values (IMM or from stack)
dec           [ADDR]            IND STK   Decrement
inc           [ADDR]            IND STK   Increment
shl           [COUNT]           IND STK   Shift Left
shr           [COUNT]           IND STK   Shift Right
and           [OP1 [OP2]]   IMM     STK   Bitwise And
or            [OP1 [OP2]]   IMM     STK   Bitwise Or
jump          [ADDR/LABEL]  IMM IND STK   Jumps to address/label (IMM or from stack)
jumpt         [ADDR/LABEL]  IMM IND STK   Jumps if top stack value is true
jumpf         [ADDR/LABEL]  IMM IND STK   Jumps if top stack value is false
call          [ADDR/LABEL]  IMM IND STK   Calls an address/label (IMM or from stack)
syscall       NUMBER        IMM           Calls a system function
ret                         IMM           Returns from procedure
nop                         IMM           No Operation
halt                        IMM           Halts execution
reset                       IMM           Resets VM
```
```
IMM - Immediate
IND - Indirect
STK - Stack
```

### Assembly
#### Instructions
Instruction's mnemonics are typed in followed by 0 or more parameters, that given instruction can take
```
push    10
add     50
sub     9   5
mul
halt
```

#### Lables
Labels are declared by typing in lable name folowed by a colon. When a label is mentioned somewhere in code, its address is used
```
label:
jump    label
```

#### Stack
Stack is key a component of the vm. It serves as a temporary storage for variables. There are several stack manipulation instructions
```
push    10      ; Pushes a value onto the stack
pop             ; Pops a value from the stack
rol             ; Switches 2 top values
```

#### Variables & Data
Data is stored in RAM (which also holds program code). There are 4 basic types `i8` `i16` `i32` `str`. Variables are declared using `%def` helper instruction. Usage: `%def TYPE NAME VALUE`. When the variable is referenced, its address is used.
```
%def i8  c  'A'
%def i8  x  156
%def i16 y  1024
%def i32 z  16384
%def str s  "Hello, World!"
```
To load & store values on the stack there are `loadX` and `storeX` instructions
```
load8     c
load8     x
load16    y
load32    z
load8     s    ; Will load first character of 's'

push      10
store8    c
push      10
store8    x
push      10
store16   y
push      10
store32   z
```
To manually dereference an address use `derefX` instruction
```
push      s
deref8
push      y
deref16
push      z
deref32
```
To declare an array, use `%data`. Usage: `%data TYPE NAME VALUE..`
```
%data i16 a 10 38 183 28
```

#### Conditions
There are `equ`, `lt` and `gt` for comparing values

#### Control Flow
Unconditional jump is performed using `jump` instruction. To jump only if top stack value is true, use `jumpt`, otherwise - `jumpf`

#### Functions & Calls
Functions are declared with labels. To call a function use `call` instruction, to return from a function `ret` is used.
To pass or return parameters, use the stack.
```
call print_newline
...
print_newline:
  push '\n'
  syscall putc
  ret
```

#### Syscalls
Syscalls are the bridge to the C++, through them xvm program can interface with the host system and perform, for example, io operations.
To invoke a syscall use `syscall` instruction. To create an alias for syscall, use `%syscall`. Usage: `%syscall NAME NUMBER`
```
%syscall putc 20
syscall putc
```
