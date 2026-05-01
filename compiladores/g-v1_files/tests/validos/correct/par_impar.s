.data
_str_2: .asciiz "IMPAR"
_str_1: .asciiz "PAR"
_str_0: .asciiz "Digite um numero inteiro"

.text
.globl main
main:
    move $fp, $sp
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
    lw $v0, -4($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    lw $v0, -4($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 2
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    div $t0, $v0
    mflo $v0
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 2
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    mul $v0, $t0, $v0
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    subu $v0, $t0, $v0
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 0
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    xor $v0, $t0, $v0
    sltiu $v0, $v0, 1
    beq $v0, $zero, else_0
    nop
    la $a0, _str_1
    li $v0, 4
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    j endif_1
    nop
else_0:
    la $a0, _str_2
    li $v0, 4
    syscall
    li $a0, 10
    li $v0, 11
    syscall
endif_1:
    addiu $sp, $sp, 4
    li $v0, 10
    syscall
