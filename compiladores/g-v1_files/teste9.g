principal {
    x: int;
} {
    x = 1 + 2;
    se (x >= 1) entao 
        se (x >= 2) entao 
            escreva 2;
        senao
            escreva 1;
        fimse
    fimse
    novalinha;
}
