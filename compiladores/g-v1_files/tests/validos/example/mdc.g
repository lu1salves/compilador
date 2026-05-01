principal
/* Calcula o maximo divisor comum de dois numeros pelo algoritmo de Euclides. */
{
    a, b: int;
}
{
    escreva "Digite o primeiro numero";
    novalinha;
    leia a;
    escreva "Digite o segundo numero";
    novalinha;
    leia b;
    enquanto (b != 0)
    {
        resto: int;
    }
    {
        resto = a - (a / b) * b;
        a = b;
        b = resto;
    }
    escreva "MDC: ";
    escreva a;
    novalinha;
}
