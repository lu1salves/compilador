.data
_str_1: .asciiz "Conceito: "
_str_0: .asciiz "Digite um valor inteiro para a nota de um aluno"

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
    li $v0, 6
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    slt $v0, $t0, $v0
    beq $v0, $zero, else_0
    nop
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    li $v0, 68
    sw $v0, -8($fp)
    la $a0, _str_1
    li $v0, 4
    syscall
    lw $v0, -8($fp)
    move $a0, $v0
    li $v0, 11
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    addiu $sp, $sp, 4
    j endif_1
    nop
else_0:
    lw $v0, -4($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 7
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    slt $v0, $t0, $v0
    beq $v0, $zero, else_2
    nop
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    li $v0, 67
    sw $v0, -8($fp)
    la $a0, _str_1
    li $v0, 4
    syscall
    lw $v0, -8($fp)
    move $a0, $v0
    li $v0, 11
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    addiu $sp, $sp, 4
    j endif_3
    nop
else_2:
    lw $v0, -4($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 9
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    slt $v0, $t0, $v0
    beq $v0, $zero, else_4
    nop
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    li $v0, 66
    sw $v0, -8($fp)
    la $a0, _str_1
    li $v0, 4
    syscall
    lw $v0, -8($fp)
    move $a0, $v0
    li $v0, 11
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    addiu $sp, $sp, 4
    j endif_5
    nop
else_4:
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    li $v0, 65
    sw $v0, -8($fp)
    la $a0, _str_1
    li $v0, 4
    syscall
    lw $v0, -8($fp)
    move $a0, $v0
    li $v0, 11
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    addiu $sp, $sp, 4
endif_5:
endif_3:
endif_1:
    addiu $sp, $sp, 4
    li $v0, 10
    syscall
