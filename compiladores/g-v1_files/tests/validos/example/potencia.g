principal
/* Calcula base elevado a expoente usando multiplicacoes sucessivas. */
{
    base, exp, resultado: int;
}
{
    escreva "Digite a base";
    novalinha;
    leia base;
    escreva "Digite o expoente";
    novalinha;
    leia exp;
    resultado = 1;
    enquanto (exp > 0)
    {
        resultado = resultado * base;
        exp = exp - 1;
    }
    escreva "Resultado: ";
    escreva resultado;
    novalinha;
}
