
%define FILE_RDONLY 1
%define FILE_WRONLY 2
%define FILE_RDWR   4
%define FILE_CREATE 8
%define FILE_APPEND 16
%define FILE_PERMS  32

%syscall putc       20
%syscall readc      21
%syscall readl      22

%syscall open       30
%syscall close      31
%syscall read       32
%syscall write      33

%syscall fsctl      60
%syscall vmctl      60
%syscall sysctl     70

%syscall breakpoint 90
