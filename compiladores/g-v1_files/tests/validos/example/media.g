principal
/* Calcula a media inteira de N numeros digitados pelo usuario. */
{
    n, soma, i: int;
}
{
    escreva "Digite a quantidade de numeros";
    novalinha;
    leia n;
    soma = 0;
    i = 0;
    enquanto (i < n)
    {
        val: int;
    }
    {
        escreva "Digite um numero";
        novalinha;
        leia val;
        soma = soma + val;
        i = i + 1;
    }
    escreva "Media: ";
    escreva soma / n;
    novalinha;
}
