.data
_str_2: .asciiz "Resultado: "
_str_1: .asciiz "Digite o expoente"
_str_0: .asciiz "Digite a base"

.text
.globl main
main:
    move $fp, $sp
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    la $a0, _str_0
    li $v0, 4
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    li $v0, 5
    syscall
    sw $v0, -4($fp)
    la $a0, _str_1
    li $v0, 4
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    li $v0, 5
    syscall
    sw $v0, -8($fp)
    li $v0, 1
    sw $v0, -12($fp)
while_begin_0:
    lw $v0, -8($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 0
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    slt $v0, $v0, $t0
    beq $v0, $zero, while_end_1
    nop
    lw $v0, -12($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    lw $v0, -4($fp)
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    mul $v0, $t0, $v0
    sw $v0, -12($fp)
    lw $v0, -8($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 1
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    subu $v0, $t0, $v0
    sw $v0, -8($fp)
    j while_begin_0
    nop
while_end_1:
    la $a0, _str_2
    li $v0, 4
    syscall
    lw $v0, -12($fp)
    move $a0, $v0
    li $v0, 1
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    addiu $sp, $sp, 12
    li $v0, 10
    syscall
