.data
_str_4: .asciiz "DESORDENADA"
_str_3: .asciiz "ORDENADA"
_str_2: .asciiz " numeros inteiros separados entre si por um espaco"
_str_1: .asciiz "digite uma sequencia de "
_str_0: .asciiz "digite o tamanho de uma sequencia de numeros inteiros - digite 0 para terminar."

.text
.globl main
main:
    move $fp, $sp
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    la $a0, _str_0
    li $v0, 4
    syscall
    li $v0, 5
    syscall
    sw $v0, -4($fp)
while_begin_0:
    lw $v0, -4($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 0
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    xor $v0, $t0, $v0
    sltu $v0, $zero, $v0
    beq $v0, $zero, while_end_1
    nop
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    li $v0, 118
    sw $v0, -16($fp)
    li $v0, 1
    sw $v0, -8($fp)
    li $v0, 5
    syscall
    sw $v0, -12($fp)
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
while_begin_2:
    lw $v0, -8($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    lw $v0, -4($fp)
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    slt $v0, $t0, $v0
    beq $v0, $zero, while_end_3
    nop
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    li $v0, 5
    syscall
    sw $v0, -20($fp)
    lw $v0, -12($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    lw $v0, -20($fp)
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    slt $v0, $t0, $v0
    beq $v0, $zero, else_4
    nop
    lw $v0, -20($fp)
    sw $v0, -12($fp)
    j endif_5
    nop
else_4:
    li $v0, 102
    sw $v0, -16($fp)
    lw $v0, -4($fp)
    sw $v0, -8($fp)
endif_5:
    lw $v0, -8($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 1
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    addu $v0, $t0, $v0
    sw $v0, -8($fp)
    addiu $sp, $sp, 4
    j while_begin_2
    nop
while_end_3:
    lw $v0, -16($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 118
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    xor $v0, $t0, $v0
    sltiu $v0, $v0, 1
    beq $v0, $zero, else_6
    nop
    la $a0, _str_3
    li $v0, 4
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    j endif_7
    nop
else_6:
    la $a0, _str_4
    li $v0, 4
    syscall
    li $a0, 10
    li $v0, 11
    syscall
endif_7:
    li $v0, 5
    syscall
    sw $v0, -4($fp)
    addiu $sp, $sp, 12
    j while_begin_0
    nop
while_end_1:
    addiu $sp, $sp, 4
    li $v0, 10
    syscall
