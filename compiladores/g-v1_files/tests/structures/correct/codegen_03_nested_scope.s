.text
.globl main
main:
    move $fp, $sp
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    li $v0, 1
    sw $v0, -4($fp)
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    lw $v0, -4($fp)
    addiu $sp, $sp, -4
    sw $v0, 0($sp)
    li $v0, 2
    lw $t0, 0($sp)
    addiu $sp, $sp, 4
    addu $v0, $t0, $v0
    sw $v0, -8($fp)
    lw $v0, -8($fp)
    move $a0, $v0
    li $v0, 1
    syscall
    addiu $sp, $sp, 4
    addiu $sp, $sp, 4
    li $v0, 10
    syscall
