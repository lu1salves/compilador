.data
_str_1: .asciiz "no"
_str_0: .asciiz "ok"

.text
.globl main
main:
    move $fp, $sp
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    li $v0, 1
    sw $v0, -4($fp)
    lw $v0, -4($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 2
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    slt $v0, $t0, $v0
    beq $v0, $zero, else_0
    nop
    la $a0, _str_0
    li $v0, 4
    syscall
    j endif_1
    nop
else_0:
    la $a0, _str_1
    li $v0, 4
    syscall
endif_1:
    addiu $sp, $sp, 4
    li $v0, 10
    syscall
