.data
_str_2: .asciiz " da sequencia de Fibonacci e: "
_str_1: .asciiz "O termo "
_str_0: .asciiz "Digite a posicao na sequencia de Fibonacci"

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
    li $v0, 0
    sw $v0, -8($fp)
    li $v0, 1
    sw $v0, -12($fp)
    li $v0, 0
    sw $v0, -20($fp)
while_begin_0:
    lw $v0, -20($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    lw $v0, -4($fp)
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    slt $v0, $t0, $v0
    beq $v0, $zero, while_end_1
    nop
    lw $v0, -12($fp)
    sw $v0, -16($fp)
    lw $v0, -8($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    lw $v0, -12($fp)
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    addu $v0, $t0, $v0
    sw $v0, -12($fp)
    lw $v0, -16($fp)
    sw $v0, -8($fp)
    lw $v0, -20($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 1
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    addu $v0, $t0, $v0
    sw $v0, -20($fp)
    j while_begin_0
    nop
while_end_1:
    la $a0, _str_1
    li $v0, 4
    syscall
    lw $v0, -4($fp)
    move $a0, $v0
    li $v0, 1
    syscall
    la $a0, _str_2
    li $v0, 4
    syscall
    lw $v0, -8($fp)
    move $a0, $v0
    li $v0, 1
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    addiu $sp, $sp, 20
    li $v0, 10
    syscall
