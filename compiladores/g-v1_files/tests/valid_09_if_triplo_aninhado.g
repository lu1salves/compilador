principal {
    x: int;
} {
    x = 1 + 2;
    se (x >= 1) entao 
        se (x >= 2) entao 
            se (x>=3) entao
                escreva "maiorigual a 3";
            senao 
                escreva 2;
            fimse
        senao
            escreva 1;
        fimse
    fimse
}
