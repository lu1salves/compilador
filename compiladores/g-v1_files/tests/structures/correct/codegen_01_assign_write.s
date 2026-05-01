.text
.globl main
main:
    move $fp, $sp
    addiu $sp, $sp, -4
    sw $zero, 0($sp)
    li $v0, 5
    sw $v0, -4($fp)
    lw $v0, -4($fp)
    move $a0, $v0
    li $v0, 1
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    addiu $sp, $sp, 4
    li $v0, 10
    syscall
