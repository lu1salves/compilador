principal
/* Verifica se um numero inteiro e par ou impar. */
{
    n: int;
}
{
    escreva "Digite um numero inteiro";
    novalinha;
    leia n;
    se (n - (n / 2) * 2 == 0)
    entao {
        escreva "PAR";
        novalinha;
    }
    senao {
        escreva "IMPAR";
        novalinha;
    }
    fimse
}
