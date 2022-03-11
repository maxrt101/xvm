
%include "syscall.x"

puts:      ; [strptr]
  dup
  deref8
  equ       0
  jumpt     puts_end
  dup
  deref8
  syscall   putc
  inc
  jump      puts

puts_end:
  pop
  ret
