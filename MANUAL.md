# SKIFF — manual

**betobullet** · 4 HP · pulador de gates por probabilidade, duplo

O Skiff deixa passar cada gate da entrada com uma probabilidade. São dois canais independentes,
que podem também funcionar de forma exclusiva. Ele recria o comportamento publicado do Ladik S-090.

---

## 1. Painel (por canal)

| Controle / jack | O que faz |
|---|---|
| **PROB** | Probabilidade de passar, de 0 % (nenhum) a 100 % (todos). |
| **CV** | CV de probabilidade. Com cabo, o knob **atenua** o CV: no máximo, 0 V ou menos dá 0 % e 5 V ou mais dá 100 %. |
| **IN** | Entrada de gate. O LED ao lado acende com o gate. |
| **IN 2** | A entrada do canal 2 é **normalizada** na entrada do canal 1. Sem cabo, os dois canais leem o mesmo gate e decidem cada um por si. |
| **OUT** | Saída de gate (10 V). Quando o gate passa, a saída acompanha a duração do gate de entrada. O LED ao lado acende. |

## 2. Os "jumpers" (menu de contexto)

O original tem dois jumpers atrás. Aqui eles ficam no menu:

- **Exclusive (one output per pulse)**: só uma saída pode passar cada pulso.
  - Com entrada compartilhada, o canal 1 decide primeiro e tem prioridade; o canal 2 só passa se o
    1 não passou.
  - Com entradas separadas, uma saída ocupada bloqueia a outra.
- **Regular randomness**: deixa o aleatório mais regular, com menos agrupamentos de pulsos
  seguidos ou de silêncios. Com essa opção, 0 % e 100 % **não são exatos**. Desligue quando
  precisar dos extremos.

## 3. Dicas

- **Hi-hat humano:** um clock de 16 avos em IN e PROB em 70 %.
- **Divisor aleatório entre dois destinos:** use Exclusive com entrada compartilhada. Cada pulso vai
  para OUT 1 ou para OUT 2, nunca para os dois.
- **Fills que crescem:** use uma rampa lenta no CV.

## 4. Menu de contexto

- **Theme**: Black ou White.
