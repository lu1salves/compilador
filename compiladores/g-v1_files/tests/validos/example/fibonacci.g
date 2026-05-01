principal
/* Calcula o n-esimo termo da sequencia de Fibonacci. */
{
    n, a, b, temp, i: int;
}
{
    escreva "Digite a posicao na sequencia de Fibonacci";
    novalinha;
    leia n;
    a = 0;
    b = 1;
    i = 0;
    enquanto (i < n)
    {
        temp = b;
        b = a + b;
        a = temp;
        i = i + 1;
    }
    escreva "O termo ";
    escreva n;
    escreva " da sequencia de Fibonacci e: ";
    escreva a;
    novalinha;
}
