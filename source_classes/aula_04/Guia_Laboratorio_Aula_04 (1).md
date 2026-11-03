# Guia de Laboratório — Aula 04

**Teclado matricial, varredura e interação humano-máquina**

PIC16F877A · MPLAB X · XC8 · cristal de 8 MHz

---

## Antes de começar

**O que você vai aprender**

- Como 16 teclas são lidas com apenas 8 pinos.
- Como o microcontrolador "pergunta" qual tecla foi apertada (varredura).
- Por que o botão "treme" ao ser apertado (*bounce*) e como lidar com isso.
- Como transformar um aperto de tecla em um dado que o seu código usa.
- Como juntar teclado e LCD para construir uma fechadura eletrônica.

**O que você precisa já dominar**

- Fuses (`FOSC`, `WDTE`, ...) e o ambiente MPLAB X (Aula 01).
- Escrever em `PORTD` para acender LEDs.
- Escrever caracteres no LCD (Aula 02/03).

**Arquivos auxiliares desta aula**

| Arquivo | Para que serve |
|---|---|
| `assets/esquema_teclado.png` | Esquema elétrico do teclado 4x4 da placa |
| `assets/teclado_matricial.html` | Demonstração interativa: clique numa tecla e veja o caminho da corrente |
| `codigos/usando_botoes_01` | Primeiro código de exemplo: lendo botões |

**Como usar este guia:** os blocos **Pense antes de continuar** pedem que você tente responder sozinho. As respostas e as soluções de código ficam recolhidas em **Spoiler**. Tente primeiro, abra depois.

---

## 1. Introdução

Até a Aula 03 nós só **enviamos** informações para fora: acendemos LEDs e escrevemos no LCD. Nesta aula o microcontrolador vai **escutar** o mundo externo, usando o painel de 16 botões da placa.

Este material foi pensado para dar **autonomia**. Vamos entender o hardware, os problemas físicos reais que aparecem quando alguém aperta um botão e, passo a passo, chegar a um sistema de controle de acesso.

---

## 2. O hardware: teclado matricial 4x4

### 2.1 Por que uma matriz?

Ligar cada um dos 16 botões a um pino próprio gastaria 16 pinos. Com uma **matriz** os botões ficam numa grade de 4 linhas por 4 colunas, e bastam **8 pinos**, todos no `PORTB`.

Cada botão fica no cruzamento de uma linha com uma coluna. Enquanto ninguém aperta, linhas e colunas **não se tocam**. Ao apertar, o botão fecha o contato entre a linha e a coluna dele.

![Esquema elétrico do teclado matricial 4x4](assets/esquema_teclado.png)

### 2.2 Mapa de pinos

| Pinos | Função | No `TRISB` |
|---|---|---|
| RB0 a RB3 | **Linhas** L0 a L3 (cada uma com um resistor de 470 Ω em série) | `0` = saída |
| RB4 a RB7 | **Colunas** C0 a C3 | `1` = entrada |

Quais teclas ficam em cada cruzamento:

| | C0 (RB4) | C1 (RB5) | C2 (RB6) | C3 (RB7) |
|---|:---:|:---:|:---:|:---:|
| **L0 (RB0)** | 1 | 2 | 3 | A |
| **L1 (RB1)** | 4 | 5 | 6 | B |
| **L2 (RB2)** | 7 | 8 | 9 | C |
| **L3 (RB3)** | * | 0 | # | D |

### 2.3 O caminho da corrente

Corrente só existe quando há **diferença de tensão** e um **caminho fechado** entre dois pontos. No teclado, o PIC monta essa situação de propósito:

1. As **colunas** ficam em nível 1 (+5 V) por um resistor de *pull-up*.
2. O PIC coloca **uma linha** em nível 0 (0 V) e deixa as outras em 1.
3. Ao apertar uma tecla **da linha que está em 0**, a coluna é ligada a essa linha e a corrente segue:

   `+5 V → pull-up → coluna → tecla → linha (0 V) → resistor de 470 Ω → pino do PIC`

4. A tensão da coluna cai para 0 V, e o PIC lê **0** nesse pino.

Se a tecla apertada estiver numa linha que está em 1, a coluna é ligada a um ponto que já tem a mesma tensão (+5 V). Não há diferença de tensão, não há corrente e a coluna continua em 1.

**Veja funcionando:** abra a demonstração interativa e clique em várias teclas.

<iframe src="assets/teclado_matricial.html" width="100%" height="520" style="border:1px solid #ccc;border-radius:8px" title="Demonstração interativa do teclado matricial"></iframe>

[Abrir a demonstração em outra aba](assets/teclado_matricial.html)

**Para que servem os 470 Ω?** Eles protegem os pinos. Se duas teclas da mesma coluna forem apertadas juntas, uma linha em 0 e outra em 1 ficariam ligadas pela coluna, o que seria um curto entre saídas. O resistor limita essa corrente.

### 2.4 A varredura

O PIC não consegue "ver" todas as teclas de uma vez. Ele faz uma **varredura**:

1. Põe a linha 0 em 0 V (as outras em 5 V) e **lê as 4 colunas**.
2. Põe a linha 1 em 0 V e lê as 4 colunas.
3. Repete para as linhas 2 e 3.

A **linha ativa no momento** mais a **coluna que caiu para 0** dizem exatamente qual tecla foi apertada. Isso é rápido a ponto de parecer simultâneo.

> **Pense antes de continuar:** com a linha 2 em 0, o PIC lê RB4 = 0. Qual tecla foi apertada? E se a linha ativa fosse a 3?

<details>
<summary><b>Spoiler: resposta</b></summary>

Com a linha 2 ativa e RB4 = 0, a tecla é o **7**. Com a linha 3 ativa e a mesma coluna, seria o **\***.

Repare: quem identifica a tecla não é o pino RB4 sozinho, e sim o **par** (linha ativa, coluna que caiu). O PIC só sabe a linha porque **foi ele quem a ativou**.

</details>

---

## 3. Configurando os pinos

Três configurações valem para todos os exercícios desta aula.

### 3.1 Direção dos pinos: `TRISB`

No registrador `TRIS`, **0 = saída** e **1 = entrada**.

```c
TRISB = 0b11110000;   // RB7-RB4 entradas (colunas), RB3-RB0 saídas (linhas)
```

### 3.2 Pull-up interno

Uma entrada "solta" não vale 0 nem 1: ela vira uma antena e capta ruído. As colunas precisam ser puxadas para +5 V. O seu esquema não desenha esses resistores porque eles **já existem dentro do PIC**, no `PORTB`. Para ativá-los, limpe o bit 7 (`nRBPU`) do `OPTION_REG`:

```c
OPTION_REGbits.nRBPU = 0;    // forma legível (recomendada)
// OPTION_REG &= 0b01111111; // mesmo efeito: zera só o bit 7 e mantém os outros
```

O "n" no nome indica lógica invertida: `nRBPU = 0` significa pull-ups **ligados**.

> **Cuidado:** `OPTION_REG = 0b01111111;` (com `=` em vez de `&=`) também zera o bit 7, mas sobrescreve o registrador inteiro, incluindo as configurações do Timer0.

### 3.3 Estado das linhas: `PORTB`

Para ativar a linha 0, escrevemos 0 só no bit dela:

```c
PORTB = 0b11111110;   // RB0 em 0, todas as outras em 1
```

| Linha ativa | Valor de `PORTB` |
|---|---|
| L0 | `0b11111110` |
| L1 | `0b11111101` |
| L2 | `0b11111011` |
| L3 | `0b11110111` |

Escrever `PORTB` mexe nas **linhas** (saídas). Os bits das colunas (RB4 a RB7) são entradas e ignoram o que você escreve neles.

### 3.4 Esqueleto de projeto

Use este esqueleto como ponto de partida nos exercícios (ele já tem as configurações do LCD, que usaremos adiante):

```c
#include <xc.h>

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000

void main(void) {
    ADCON1 = 0b00000110;          // pinos analógicos viram digitais
    OPTION_REGbits.nRBPU = 0;     // pull-ups internos do PORTB

    TRISB = 0b11110000;           // linhas = saída, colunas = entrada
    TRISD = 0b00000000;           // LEDs
    PORTD = 0;

    PORTB = 0b11111110;           // linha 0 ativa
    __delay_us(50);               // tempo para o sinal estabilizar

    while (1) {
        // sua lógica aqui
    }
}
```

> **Pense antes de continuar:** o que aconteceria com as colunas se você esquecesse a linha do pull-up? E se esquecesse de escrever `PORTB` antes de ler as colunas?

<details>
<summary><b>Spoiler: resposta</b></summary>

- **Sem o pull-up**, as colunas ficam flutuando: a leitura oscila sozinha e os LEDs acendem e apagam sem ninguém apertar nada.
- **Sem escrever `PORTB`**, nenhuma linha está em 0. Apertar uma tecla liga a coluna a um ponto em 1 e nada cai para 0, então o `if` nunca é verdadeiro.

</details>

---

## 4. Primeiro contato: um botão, um LED

Vamos ao mais simples: apertar a primeira tecla da matriz e acender um LED no `PORTD`.

**Lógica**

1. Coloque a linha 0 em 0 V.
2. Leia a coluna 0 (pino RB4).
3. Se RB4 estiver em 0 (tecla apertada), acenda o LED. Se estiver em 1 (solta), apague.

<details>
<summary><b>Dica 1: como ativar a linha 0</b></summary>

```c
PORTB = 0b11111110;
```

</details>

<details>
<summary><b>Dica 2: como testar a coluna</b></summary>

```c
if (PORTBbits.RB4 == 0) {
    // o que fazer quando a tecla está apertada
}
```

</details>

**Sua vez.** Use o esqueleto da seção 3.4, escreva a lógica dentro do `while (1)`, compile e teste na placa.

<details>
<summary><b>Spoiler: solução</b></summary>

```c
// dentro do while (1)
PORTB = 0b11111110;            // linha 0 (RB0) em 0 V, resto em 5 V
if (PORTBbits.RB4 == 0) {      // coluna 0 (RB4) caiu: tecla "1" apertada
    PORTD = 0b00000001;        // acende o LED
} else {
    PORTD = 0b00000000;        // apaga o LED
}
```

</details>

> **Pense antes de continuar:** o LED acende porque o PIC *leu* o botão ou porque o PIC *escreveu* no `PORTD`? Quais pinos participam de cada coisa?

<details>
<summary><b>Spoiler: resposta</b></summary>

São duas coisas independentes. O pino **RB4 lê** o botão (e só serve de condição do `if`). O pino **RD0 comanda** o LED porque o código escreveu 1 nele. O PIC não "sente" o aperto: a cada passada do `while (1)` ele apenas consulta o nível de RB4.

</details>

---

## 5. Lendo várias teclas da mesma linha

Com a linha 0 ativa, as quatro colunas respondem **de forma independente**, cada uma à sua tecla:

| Tecla apertada | Coluna que cai para 0 |
|---|---|
| 1 | RB4 |
| 2 | RB5 |
| 3 | RB6 |
| A | RB7 |

Então dá para dar uma função diferente a cada tecla da linha. O trecho abaixo vai **dentro de um laço**:

```c
unsigned char leds;

while (1) {
    leds = 0;                                   // zera a cada passada

    if (!PORTBbits.RB4) leds = 0b11000000;      // tecla 1
    if (!PORTBbits.RB5) leds = 0b00000011;      // tecla 2
    if (!PORTBbits.RB6) leds = 0b00001100;      // tecla 3
    if (!PORTBbits.RB7) leds = 0b00110000;      // tecla A

    PORTD = leds;                               // escreve uma vez por passada
}
```

Por que montar o valor numa variável? Se você apagasse o `PORTD` no começo de cada passada e acendesse depois, os LEDs piscariam entre os dois estados. Escrevendo **uma vez só** por passada, isso não acontece.

> **Pense antes de continuar:** e as teclas 4, 5, 6, B? O que o programa faz se você apertá-las agora?

<details>
<summary><b>Spoiler: resposta</b></summary>

Nada. Essas teclas estão na linha 1, e o código só ativa a linha 0. A coluna delas é ligada a uma linha que está em 1, então não cai. Para ler as outras linhas é preciso **trocar a linha ativa** dentro do laço, e é isso que a varredura da seção 7 faz.

</details>

---

## 6. Mexendo um LED: deslocamento de bits e *bounce*

**Desafio:** com um LED aceso no meio do `PORTD`, a tecla **1** deve movê-lo para a **direita** e a tecla **2** para a **esquerda**.

**Ferramentas**

- `<<` desloca todos os bits **para a esquerda** (em direção a RD7).
- `>>` desloca todos os bits **para a direita** (em direção a RD0).
- `x <<= 1;` é atalho para `x = x << 1;` (e `>>=` vale o mesmo).

| `led` | Depois de `led <<= 1` | Depois de `led >>= 1` |
|---|---|---|
| `0b00001000` | `0b00010000` | `0b00000100` |

> "Direita" e "esquerda" aqui são as do número escrito em binário. Se os LEDs da placa estiverem montados na ordem inversa, o movimento que você vê será o oposto.

**Ponto de partida**

```c
unsigned char led = 0b00001000;   // LED começa no meio
PORTD = led;
```

> **Pense antes de continuar:** se você apenas fizer `if (!PORTBbits.RB4) led >>= 1;`, o que acontece quando apertar a tecla por meio segundo? Por quê?

<details>
<summary><b>Spoiler: resposta e a ideia do <i>bounce</i></b></summary>

O laço roda **milhares de vezes** enquanto você segura a tecla. A cada passada o teste continua verdadeiro e o LED é deslocado de novo, então ele atravessa tudo e para na borda antes de você soltar.

A solução é fazer cada aperto contar **uma única vez**: depois de agir, **espere a tecla ser solta**.

Há um segundo problema: ao apertar, o contato metálico vibra e gera vários 0 e 1 em poucos milissegundos. Isso é o **bounce**. Sem um pequeno atraso de filtro, um único aperto pode ser lido como vários.

</details>

**Sua vez.** Implemente a solução com: deslocamento, limite nas bordas, espera de soltar e filtro de *bounce*.

<details>
<summary><b>Spoiler: solução</b></summary>

```c
unsigned char led = 0b00001000;   // LED começa no meio
PORTD = led;

while (1) {
    if (!PORTBbits.RB4) {                       // tecla 1: direita
        if (led > 0b00000001) led >>= 1;        // não deixa sair pela borda
        PORTD = led;
        __delay_ms(20);                         // filtra o ruído do contato
        while (!PORTBbits.RB4);                 // espera soltar
        __delay_ms(20);
    }

    if (!PORTBbits.RB5) {                       // tecla 2: esquerda
        if (led < 0b10000000) led <<= 1;
        PORTD = led;
        __delay_ms(20);
        while (!PORTBbits.RB5);
        __delay_ms(20);
    }
}
```

O `while (!PORTBbits.RB4);` fica preso enquanto o pino valer 0, ou seja, enquanto a tecla estiver apertada.

Os limites existem porque `led` tem 8 bits: um bit empurrado para fora do registrador some e `led` vira 0. Se preferir que o LED "dê a volta", use rotação:

```c
led = (unsigned char)((led << 1) | (led >> 7));   // rotaciona à esquerda
led = (unsigned char)((led >> 1) | (led << 7));   // rotaciona à direita
```

</details>

---

## 7. Varredura completa: `lerTeclado()`

Até aqui só lemos a linha 0. Vamos criar uma função que varre as 4 linhas e **devolve o caractere da tecla apertada** (ou `0` se nenhuma estiver apertada). Esse valor de retorno é o **dado** que o resto do programa vai usar.

### 7.1 Versão sem laços (mais fácil de entender)

Uma linha por bloco; o padrão se repete:

```c
char lerTeclado(void) {
    PORTB = 0b11111110;  __delay_us(50);   // linha 0
    if (!PORTBbits.RB4) return '1';
    if (!PORTBbits.RB5) return '2';
    if (!PORTBbits.RB6) return '3';
    if (!PORTBbits.RB7) return 'A';

    PORTB = 0b11111101;  __delay_us(50);   // linha 1
    if (!PORTBbits.RB4) return '4';
    if (!PORTBbits.RB5) return '5';
    if (!PORTBbits.RB6) return '6';
    if (!PORTBbits.RB7) return 'B';

    PORTB = 0b11111011;  __delay_us(50);   // linha 2
    if (!PORTBbits.RB4) return '7';
    if (!PORTBbits.RB5) return '8';
    if (!PORTBbits.RB6) return '9';
    if (!PORTBbits.RB7) return 'C';

    PORTB = 0b11110111;  __delay_us(50);   // linha 3
    if (!PORTBbits.RB4) return '*';
    if (!PORTBbits.RB5) return '0';
    if (!PORTBbits.RB6) return '#';
    if (!PORTBbits.RB7) return 'D';

    return 0;                              // nenhuma tecla apertada
}
```

### 7.2 Versão compacta (com laços)

Faz exatamente a mesma coisa. O mapa de teclas vira uma **string**, e o índice `linha * 4 + coluna` aponta a posição certa:

```c
const char teclas[] = "123A456B789C*0#D";

char lerTeclado(void) {
    unsigned char linha, coluna;

    for (linha = 0; linha < 4; linha++) {
        PORTB = (unsigned char)(~(1 << linha));         // só a linha da vez vai a 0
        __delay_us(50);                                 // sinal estabiliza
        for (coluna = 0; coluna < 4; coluna++) {
            if (!(PORTB & (0b00010000 << coluna))) {    // coluna em 0 = tecla apertada
                PORTB = 0b11111111;                     // desativa as linhas
                return teclas[linha * 4 + coluna];
            }
        }
    }
    PORTB = 0b11111111;
    return 0;
}
```

Como ler as contas:

- `1 << linha` cria um bit 1 na posição da linha; o `~` inverte tudo, deixando **só esse bit em 0**.
- `0b00010000 << coluna` cria uma máscara que aponta para RB4, RB5, RB6 ou RB7.
- Para a tecla 7: `linha = 2`, `coluna = 0`, então o índice é `2 * 4 + 0 = 8`, e a posição 8 da string é `'7'`.

> **Pense antes de continuar:** com `linha = 2`, qual valor `~(1 << linha)` coloca no `PORTB`? A qual bloco da versão sem laços isso corresponde?

<details>
<summary><b>Spoiler: resposta</b></summary>

`1 << 2` vale `0b00000100`; invertido dá `0b11111011`. Corresponde ao bloco da **linha 2** (teclas 7, 8, 9, C).

</details>

> **Sobre apertar várias teclas ao mesmo tempo:** `lerTeclado()` devolve a primeira que encontrar e ignora o resto, o que basta para este projeto. Em teclados matriciais, apertar várias teclas em linhas e colunas diferentes pode criar caminhos que não existem de fato (*ghosting*). Para uso normal, uma tecla por vez, você não precisa se preocupar.

---

## 8. Teclado + LCD

Agora use o LCD que você construiu nas aulas anteriores. **Desafio:** ao apertar uma tecla, mostrar no LCD qual foi.

### 8.1 Funções do LCD

Cole este bloco no seu projeto. Ele reúne o padrão que você já repetia (`RS`, `PORTD`, `pulsaLCD()`) em funções pequenas:

```c
#define RS RE0
#define EN RE1

void pulsaLCD(void) {
    EN = 1;
    __delay_us(100);
    EN = 0;
    __delay_ms(5);
}

void lcdComando(unsigned char c) {
    RS = 0;                       // modo comando
    PORTD = c;
    pulsaLCD();
}

void lcdChar(char c) {
    RS = 1;                       // modo caractere
    PORTD = c;
    pulsaLCD();
}

void lcdTexto(const char *texto) {
    while (*texto) {
        lcdChar(*texto);
        texto++;
    }
}

void lcdLimpa(void) {
    lcdComando(0b00000001);
}

void lcdCursor(unsigned char linha, unsigned char coluna) {
    // linha 1 começa em 0x80, linha 2 em 0xC0
    lcdComando((linha == 1 ? 0x80 : 0xC0) + coluna);
}

void lcdInit(void) {
    __delay_ms(50);
    lcdComando(0b00111000);       // 8 bits, 2 linhas
    lcdComando(0b00001100);       // display ligado, sem cursor
    lcdComando(0b00000110);       // cursor avança para a direita
    lcdLimpa();
}
```

**Atenção aos pinos:** o LCD usa `PORTD` (dados) e `RE0`/`RE1` (controle). O teclado usa o `PORTB`. Não há conflito, mas o `PORTD` **não pode mais ser usado para LEDs** nesta etapa.

### 8.2 Sua vez

Monte o programa: configure os pinos (`TRISB`, `TRISD`, `TRISE0`, `TRISE1`, pull-up, `ADCON1`), inicialize o LCD, escreva `Tecla:` na linha 1 e, no `while (1)`, mostre na linha 2 a tecla lida.

> **Pense antes de continuar:** `lerTeclado()` devolve `0` quando nada está apertado. O que aconteceria se você escrevesse o retorno no LCD sem testar isso antes?

<details>
<summary><b>Spoiler: resposta</b></summary>

Você mandaria o caractere de código 0 ao LCD o tempo todo, e a tecla sumiria da tela assim que fosse solta (ou apareceria lixo). Por isso o `if (tecla != 0)`: só atualiza o LCD quando alguém apertou, e a última tecla continua na tela depois de solta.

</details>

<details>
<summary><b>Spoiler: código completo</b></summary>

```c
#include <xc.h>

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000

#define RS RE0
#define EN RE1

const char teclas[] = "123A456B789C*0#D";

/* ---------- LCD ---------- */

void pulsaLCD(void) {
    EN = 1;
    __delay_us(100);
    EN = 0;
    __delay_ms(5);
}

void lcdComando(unsigned char c) {
    RS = 0;
    PORTD = c;
    pulsaLCD();
}

void lcdChar(char c) {
    RS = 1;
    PORTD = c;
    pulsaLCD();
}

void lcdTexto(const char *texto) {
    while (*texto) {
        lcdChar(*texto);
        texto++;
    }
}

void lcdLimpa(void) {
    lcdComando(0b00000001);
}

void lcdCursor(unsigned char linha, unsigned char coluna) {
    lcdComando((linha == 1 ? 0x80 : 0xC0) + coluna);
}

void lcdInit(void) {
    __delay_ms(50);
    lcdComando(0b00111000);
    lcdComando(0b00001100);
    lcdComando(0b00000110);
    lcdLimpa();
}

/* ---------- Teclado ---------- */

char lerTeclado(void) {
    unsigned char linha, coluna;

    for (linha = 0; linha < 4; linha++) {
        PORTB = (unsigned char)(~(1 << linha));
        __delay_us(50);
        for (coluna = 0; coluna < 4; coluna++) {
            if (!(PORTB & (0b00010000 << coluna))) {
                PORTB = 0b11111111;
                return teclas[linha * 4 + coluna];
            }
        }
    }
    PORTB = 0b11111111;
    return 0;
}

/* ---------- Programa ---------- */

void main(void) {
    char tecla;

    ADCON1 = 0b00000110;         // tudo digital
    OPTION_REGbits.nRBPU = 0;    // pull-ups internos do PORTB

    TRISB = 0b11110000;          // RB0-3 saída, RB4-7 entrada
    PORTB = 0b11111111;
    TRISD = 0b00000000;          // dados do LCD
    TRISE0 = 0;                  // RS
    TRISE1 = 0;                  // EN

    lcdInit();
    lcdTexto("Tecla:");

    while (1) {
        tecla = lerTeclado();

        if (tecla != 0) {        // só atualiza se alguém apertou
            lcdCursor(2, 0);
            lcdChar(tecla);
        }
    }
}
```

</details>

---

## 9. Desafio final: a fechadura eletrônica

Agora vem o projeto completo, **construído em etapas**. Cada etapa tem um objetivo, um teste para você saber se deu certo e um spoiler. **Só avance quando o teste da etapa funcionar na placa.** Construir devagar facilita encontrar onde está um erro.

### A missão

Um cofre que pede uma senha de 4 teclas.

1. A tela inicial mostra `Digite a Senha:`.
2. A cada tecla digitada, aparece um `*` no LCD (a senha fica escondida).
3. Senha **correta**: limpa a tela e mostra `DESBLOQUEADA`.
4. Senha **errada**: limpa a tela e mostra `ERRO!` na linha 1 e `Tente novamente` na linha 2.
5. Após **3 segundos**, o sistema volta sozinho à tela inicial.

> O LCD 16x2 tem só 16 colunas por linha. Por isso o texto `ERRO! Tente novamente` (21 caracteres) é dividido em duas linhas.

Todas as etapas partem do código da seção 8 (funções do LCD, `teclas[]` e `lerTeclado()`). Só o `main` e uma nova função mudam.

---

### Etapa 1: uma tecla por aperto

**Problema:** `lerTeclado()` responde **enquanto** a tecla está apertada. Se o seu programa chamá-la em laço, um único aperto é lido milhares de vezes (o mesmo problema do LED na seção 6).

**Objetivo:** criar a função `esperaTecla()`, que **espera apertar, espera soltar e devolve a tecla uma única vez**.

**Passos que ela precisa fazer**

1. Ficar chamando `lerTeclado()` até vir algo diferente de 0.
2. Esperar uns 20 ms (filtro de *bounce*).
3. Ficar esperando até `lerTeclado()` voltar a 0 (tecla solta).
4. Esperar mais uns 20 ms e devolver a tecla.

<details>
<summary><b>Spoiler: esperaTecla()</b></summary>

```c
char esperaTecla(void) {
    char t;
    do { t = lerTeclado(); } while (t == 0);   // espera apertar
    __delay_ms(20);                            // filtra o bounce
    while (lerTeclado() != 0);                 // espera soltar
    __delay_ms(20);
    return t;
}
```

</details>

**Teste de sucesso:** escreva um `main` que mostre as teclas **em sequência**, uma ao lado da outra:

```c
lcdTexto("Teclas:");
lcdCursor(2, 0);
while (1) {
    lcdChar(esperaTecla());
}
```

Cada aperto deve acrescentar **um** caractere, mesmo se você segurar a tecla. Se aparecerem caracteres repetidos, o problema está na espera de soltar ou no filtro.

---

### Etapa 2: tela inicial e os asteriscos

**Objetivo:** mostrar `Digite a Senha:` e, a cada tecla, escrever um `*` na linha 2. Depois de 4 teclas, esperar 1 segundo e recomeçar a tela.

**Dicas**

- Use `lcdLimpa()` e `lcdTexto()` para a tela inicial.
- Use `lcdCursor(2, 0)` para ir ao começo da linha 2.
- Um `for` de 0 a 3 conta as 4 teclas.
- Por enquanto **ignore o valor** devolvido por `esperaTecla()`.

<details>
<summary><b>Spoiler: main da etapa 2</b></summary>

```c
void main(void) {
    unsigned char i;

    ADCON1 = 0b00000110;
    OPTION_REGbits.nRBPU = 0;
    TRISB = 0b11110000;
    PORTB = 0b11111111;
    TRISD = 0b00000000;
    TRISE0 = 0;
    TRISE1 = 0;

    lcdInit();

    while (1) {
        lcdLimpa();
        lcdTexto("Digite a Senha:");
        lcdCursor(2, 0);

        for (i = 0; i < 4; i++) {
            esperaTecla();
            lcdChar('*');
        }
        __delay_ms(1000);
    }
}
```

</details>

**Teste de sucesso:** aparecem exatamente 4 asteriscos, um por aperto. Depois de 1 segundo a tela reinicia.

---

### Etapa 3: guardar as teclas num vetor

Hoje o programa joga fora o que o usuário digitou. Para comparar com a senha, é preciso **guardar** as 4 teclas.

**Objetivo:** criar o vetor `char digitado[4];` e, no `for`, guardar cada tecla na posição `i`.

**Dica:** `esperaTecla()` **devolve** a tecla. Basta atribuir esse retorno a `digitado[i]`.

<details>
<summary><b>Spoiler: o laço da etapa 3</b></summary>

```c
char digitado[4];
// ...
for (i = 0; i < 4; i++) {
    digitado[i] = esperaTecla();    // guarda a tecla
    lcdChar('*');                   // mostra * no lugar dela
}
```

</details>

**Teste de sucesso (provisório):** depois do `for`, mostre `digitado` na tela por 1 segundo e confirme que bate com o que você apertou (apague esse trecho depois):

```c
lcdLimpa();
for (i = 0; i < 4; i++) lcdChar(digitado[i]);
__delay_ms(1000);
```

---

### Etapa 4: comparar com a senha correta

**Objetivo:** criar a senha correta e descobrir se o que foi digitado é igual.

Defina a senha fora do `main` (troque pelos dígitos que quiser):

```c
const char SENHA[4] = {'1', '5', '9', '3'};
```

> **Pense antes de continuar:** por que o código abaixo **não** funciona para comparar as duas senhas?
>
> ```c
> if (digitado == SENHA) { ... }
> ```

<details>
<summary><b>Spoiler: resposta</b></summary>

Em C, o nome de um vetor representa o **endereço** onde ele está na memória. O `==` compararia os endereços dos dois vetores (que sempre são diferentes), e não o conteúdo. É preciso comparar **posição por posição**.

</details>

**Estratégia (a da "bandeira")**

1. Crie `certo` e comece com `certo = 1` (supõe que está certa).
2. Percorra as 4 posições. Se **alguma** for diferente, faça `certo = 0`.
3. No fim, `certo == 1` significa que **todas** bateram.

<details>
<summary><b>Spoiler: comparação</b></summary>

```c
certo = 1;
for (i = 0; i < 4; i++) {
    if (digitado[i] != SENHA[i]) certo = 0;
}
```

</details>

**Teste de sucesso (provisório):** acenda um LED no `PORTD`... ou melhor, como o `PORTD` agora é do LCD, escreva `'S'` no LCD se `certo == 1` e `'N'` se não. Teste uma senha certa e várias erradas, incluindo uma que acerte 3 dígitos e erre só o último.

---

### Etapa 5: mensagens e volta à tela inicial

**Objetivo:** mostrar o resultado e voltar ao início sozinho.

**Regras**

- Se `certo`: `lcdLimpa()` e `DESBLOQUEADA`.
- Se não: `lcdLimpa()`, `ERRO!` na linha 1 e `Tente novamente` na linha 2.
- Esperar **3 segundos** e o `while (1)` recomeça sozinho.

<details>
<summary><b>Spoiler: código completo da fechadura</b></summary>

```c
#include <xc.h>

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000

#define RS RE0
#define EN RE1

const char teclas[] = "123A456B789C*0#D";
const char SENHA[4] = {'1', '5', '9', '3'};

/* ---------- LCD ---------- */

void pulsaLCD(void) {
    EN = 1;
    __delay_us(100);
    EN = 0;
    __delay_ms(5);
}

void lcdComando(unsigned char c) {
    RS = 0;
    PORTD = c;
    pulsaLCD();
}

void lcdChar(char c) {
    RS = 1;
    PORTD = c;
    pulsaLCD();
}

void lcdTexto(const char *texto) {
    while (*texto) {
        lcdChar(*texto);
        texto++;
    }
}

void lcdLimpa(void) {
    lcdComando(0b00000001);
}

void lcdCursor(unsigned char linha, unsigned char coluna) {
    lcdComando((linha == 1 ? 0x80 : 0xC0) + coluna);
}

void lcdInit(void) {
    __delay_ms(50);
    lcdComando(0b00111000);
    lcdComando(0b00001100);
    lcdComando(0b00000110);
    lcdLimpa();
}

/* ---------- Teclado ---------- */

char lerTeclado(void) {
    unsigned char linha, coluna;

    for (linha = 0; linha < 4; linha++) {
        PORTB = (unsigned char)(~(1 << linha));
        __delay_us(50);
        for (coluna = 0; coluna < 4; coluna++) {
            if (!(PORTB & (0b00010000 << coluna))) {
                PORTB = 0b11111111;
                return teclas[linha * 4 + coluna];
            }
        }
    }
    PORTB = 0b11111111;
    return 0;
}

char esperaTecla(void) {
    char t;
    do { t = lerTeclado(); } while (t == 0);   // espera apertar
    __delay_ms(20);                            // filtra o bounce
    while (lerTeclado() != 0);                 // espera soltar
    __delay_ms(20);
    return t;
}

/* ---------- Programa ---------- */

void main(void) {
    char digitado[4];
    unsigned char i, certo;

    ADCON1 = 0b00000110;         // tudo digital
    OPTION_REGbits.nRBPU = 0;    // pull-ups internos do PORTB

    TRISB = 0b11110000;          // teclado: RB0-3 saída, RB4-7 entrada
    PORTB = 0b11111111;
    TRISD = 0b00000000;          // dados do LCD
    TRISE0 = 0;                  // RS
    TRISE1 = 0;                  // EN

    lcdInit();

    while (1) {
        lcdLimpa();
        lcdTexto("Digite a Senha:");
        lcdCursor(2, 0);

        for (i = 0; i < 4; i++) {
            digitado[i] = esperaTecla();
            lcdChar('*');                 // esconde o dígito
        }

        certo = 1;
        for (i = 0; i < 4; i++) {
            if (digitado[i] != SENHA[i]) certo = 0;
        }

        lcdLimpa();
        if (certo) {
            lcdTexto("DESBLOQUEADA");
        } else {
            lcdTexto("ERRO!");
            lcdCursor(2, 0);
            lcdTexto("Tente novamente");
        }
        __delay_ms(3000);                 // 3 segundos e volta ao início
    }
}
```

</details>

**Teste de sucesso:** senha certa mostra `DESBLOQUEADA`; senha errada mostra o erro; nos dois casos a tela inicial volta sozinha após 3 segundos.

> **Pense:** o laço de comparação percorre os 4 dígitos mesmo depois de achar um errado. Isso é proposital: o tempo de resposta não revela em qual posição você errou. Em sistemas de segurança reais esse cuidado importa.

---

### Etapa 6: desafios extras (escolha os que quiser)

Cada item é independente. Tente sem spoiler.

1. **Tecla de apagar.** Use `*` para apagar o último dígito: se a tecla for `*` e `i > 0`, faça `i--`, posicione o cursor nessa coluna, escreva um espaço e posicione de novo.
2. **Tecla de confirmar.** Deixe a senha ter tamanho variável e use `#` para confirmar.
3. **Três tentativas.** Conte os erros seguidos. No terceiro, mostre `BLOQUEADO` por 10 segundos antes de aceitar nova tentativa.
4. **Trava de verdade.** Ligue um pino de saída (por exemplo RC0) por alguns segundos quando a senha estiver certa. Atenção: relé ou solenoide **não** devem ser ligados direto ao PIC; é preciso um transistor ou driver, e um diodo de proteção.
5. **Senha trocável.** Guarde a senha na EEPROM interna para poder mudá-la sem regravar o programa.

---

## 10. Problemas comuns

| Sintoma | Causa provável |
|---|---|
| LEDs ou teclas mudam sozinhos, sem ninguém apertar | Faltou ligar o pull-up (`nRBPU = 0`) |
| Só as teclas 1, 2, 3 e A funcionam | Está usando uma linha fixa (`PORTB = 0b11111110`) em vez de varrer as 4 linhas |
| Uma tecla é lida várias vezes num aperto só | Falta esperar soltar e/ou o filtro de *bounce* |
| Um aperto conta como vários (às vezes) | Falta o `__delay_ms(20)` depois de detectar o aperto |
| Nenhuma tecla é detectada | Falta escrever `PORTB` antes de ler, ou `TRISB` está errado |
| LCD mostra caracteres estranhos | Inicialização rápida demais: aumente o atraso de `lcdInit()` para 100 ms |
| Não compila: "instrução fora de função" | Uma linha como `OPTION_REGbits.nRBPU = 0;` ficou fora do `main` |
| LEDs do `PORTD` acendem sozinhos no projeto do LCD | O `PORTD` é o barramento de dados do LCD; não use para LEDs ao mesmo tempo |

---

## 11. Resumo

- Uma **matriz** lê 16 teclas com 8 pinos: 4 linhas (saída) e 4 colunas (entrada).
- Só existe corrente quando há **diferença de tensão** e **caminho fechado**: por isso a linha ativa fica em 0 e as colunas em 1.
- A tecla é identificada pelo par **(linha ativa, coluna que caiu)**.
- O **pull-up interno** (`nRBPU = 0`) evita colunas flutuando.
- O **bounce** exige filtro de tempo e espera de soltar para cada aperto contar uma vez.
- `lerTeclado()` transforma o aperto num **dado** (um caractere), e `esperaTecla()` garante que ele chegue uma única vez.
- Um programa grande se constrói **por etapas testáveis**: cada uma funcionando antes da próxima.
