# VPNX — Wireframe e especificação de UI

Frontend do port do Visual Pinball X para Nintendo Switch (HOS, homebrew). Este documento descreve duas telas: **Lista de mesas** e **Opções da mesa**. Use-o como fonte da verdade para implementar a UI.

Tela base: **1280×720 (modo portátil)**. No dock (1920×1080), todas as medidas escalam **1,5×**; o layout não muda. O modo vem de `appletGetOperationMode()`.

Mesmo esqueleto visual do Ruma: cabeçalho fino, lista densa com zebra, painel de detalhes à direita e rodapé com os atalhos do controle.

---

## 1. Tokens

### Cores

| Token | Hex | Uso |
|---|---|---|
| `bg.app` | `#0B0E13` | fundo da lista |
| `bg.chrome` | `#0F1319` | cabeçalho, rodapé, trilho A–Z |
| `bg.panel` | `#10141A` | painel de detalhes |
| `bg.card` | `#151A21` | caixa de recordes |
| `bg.control` | `#171C24` | busca, chips inativos |
| `bg.button` | `#1A2029` | botões secundários, hover de linha |
| `bg.zebra` | `#0E1218` | linhas ímpares |
| `line` | `#1E252F` | divisórias de 1 px |
| `glyph.bg` | `#2A323E` | fundo dos glifos de botão (A, B, ZL…) |
| `text.hi` | `#FFFFFF` | títulos |
| `text` | `#E6EAF0` | texto principal |
| `text.row` | `#D5DBE4` | texto das linhas |
| `text.mid` | `#AEB6C2` | rodapé, chips inativos |
| `text.dim` | `#8C96A5` | colunas secundárias |
| `text.label` | `#7A8494` | rótulos e cabeçalhos de coluna |
| `text.off` | `#3F4856` | letra sem mesas no trilho |
| `accent` | `#FFB020` | seleção, botão Jogar, letra ativa |
| `on.accent` | `#1A1000` | texto sobre o acento |
| `sel.bg` | `#E6EAF0` | fundo da linha selecionada |
| `sel.text` | `#0B0E13` | texto da linha selecionada |
| `star` | `#F5B544` | ★ em linha normal (`#A86A00` na selecionada) |
| `warn` | `#FF8A6B` | "ROM faltando" |
| `tag.fav.bg` / `tag.fav.fg` | `#2B2417` / `#F5C873` | selo Favorita |
| `tag.info.bg` / `tag.info.fg` | `#1D2733` / `#A9C6E8` | selo SS · 4 jog. |
| `placeholder` | `#1E1626` | fundo sem screenshot |

### Tipografia

O mockup usa Barlow Semi Condensed (UI), JetBrains Mono (números, ROMs) e Unbounded Bold (logo). No HOS, use `PlSharedFontType_Standard` para UI e uma monoespaçada embarcada em `romfs:` para números.

| Estilo | Tamanho | Peso |
|---|---|---|
| Logo | 15 | bold |
| H1 da tela | 20 | bold |
| H2 do painel | 22 | bold |
| Linha da lista | 13 | regular (selecionada: semibold) |
| Mono de linha | 11 | regular |
| Cabeçalho de coluna | 10, caixa alta, tracking 0.1em | semibold |
| Rodapé / chips | 12 | regular |
| Glifo de botão | 11 | semibold |

### Componentes recorrentes

- **Glifo de botão**: pílula com altura 18, largura mínima 18, padding horizontal 4, raio 9, fundo `glyph.bg`, texto `text`. Variante ativa: fundo `accent`, texto `on.accent`.
- **Chip de filtro**: padding 3×9, raio 11. Ativo: fundo `text`, texto `bg.app`, semibold. Inativo: fundo `bg.control`, texto `text.mid`.
- **Divisória**: 1 px `line`.

---

## 2. Tela: Lista de mesas

### Wireframe (1280×720)

```
x:0      40                                                     860                         1280
y:0  ┌──────────────────────────────────────────────────────────────────────────────────────────┐
     │ ● vpnx  [L] Mesas Recentes Favoritas Recordes Coleções [R]   (🔍 Buscar em 1.142 mesas [−]) │
     │                                          PORTÁTIL·720p  wifi  bat 61%  08:43  ⚙ Config  (G) │
y:36 ├────┬───────────────────────────────────────────────────────┬─────────────────────────────┤
     │ #  │ Mesas  1.142 mesas · 64★ · 38,6 GB   (Todas)(Fav)(SS)(EM)(ROM faltando)   Ordem: A–Z │
y:80 │ A  ├───────────────────────────────────────────────────────┤ ┌─────────────────────────┐ │
     │ B  │ ★ MESA       FABRIC. ANO TIPO ROM  B2S DMD PUP JOG TEMPO RECORDE│ │                         │ │
y:104│ C  ├───────────────────────────────────────────────────────┤ │   SCREENSHOT 16:9       │ │
     │ D  │ ★ Addams Family, The  Bally 1992 SS taf_l7 ● ● · 4 22h40m 188.402.330 │ │   392×220               │ │
     │ E  │   Attack from Mars    Bally 1995 SS afm_113b ● ● · 4 …                 │ │                         │ │
     │ …  │   …                                                   │ └─────────────────────────┘ │
     │[M] │▌★ Medieval Madness   Williams 1997 SS mm_109c ● ● ● 4 …  ← selecionada│ Medieval Madness  [★][SS·4]│
     │ N  │   …                                                   │ Fabricante …   ROM …        │
     │ …  │   …                                                 ▐ │ Tempo …        Última …     │
     │ Z  │   (24 linhas visíveis)                                │ Extras …                    │
     │    │                                                       │ ┌ RECORDES LOCAIS   nvram ┐ │
     │    │                                                       │ │ 1. GIL      412.880.150 │ │
     │    │                                                       │ └─────────────────────────┘ │
     │    │                                                       │ [ (A) Jogar               ] │
     │    │                                                       │ [ ▯ Jogar em retrato ] [+]  │
y:688├────┴───────────────────────────────────────────────────────┴─────────────────────────────┤
     │ 17 / 1.142        [L][R] Letra [ZL][ZR] Página [Y] Grade [X] Favoritar [+] Opções [B] Voltar [A] Jogar │
y:720└──────────────────────────────────────────────────────────────────────────────────────────┘
```

### Regiões

| Região | x | y | w | h | Fundo |
|---|---|---|---|---|---|
| Cabeçalho | 0 | 0 | 1280 | 36 | `bg.chrome`, borda inferior `line` |
| Trilho A–Z | 0 | 36 | 40 | 652 | `bg.chrome`, borda direita `line` |
| Lista | 40 | 36 | 820 | 652 | `bg.app` |
| Painel | 860 | 36 | 420 | 652 | `bg.panel`, borda esquerda `line` |
| Rodapé | 0 | 688 | 1280 | 32 | `bg.chrome`, borda superior `line` |

### Cabeçalho (h 36, padding horizontal 14, gap 18, itens centralizados verticalmente)

Da esquerda para a direita:
1. Logo: círculo de 15 px em `accent` com brilho branco de 4 px no canto superior esquerdo, seguido de "vpnx" (logo 15).
2. Abas: glifo `L`, abas "Mesas · Recentes · Favoritas · Recordes · Coleções", glifo `R`. Aba ativa: padding 4×10, raio 4, fundo `#1F2630`, texto branco semibold. As inativas usam `#9AA4B2`.
3. Espaço flexível.
4. Busca: pílula de 200×24, raio 12, fundo `bg.control`. Contém lupa, "Buscar em {N} mesas" e o glifo `−` encostado à direita.
5. Selo "PORTÁTIL · 720p" (ou "DOCK · 1080p"): 11 px, borda 1 px `glyph.bg`, raio 3.
6. Wi-Fi, bateria + % (mono 13), relógio HH:MM (mono 13).
7. Botão "Configurações": pílula de altura 24 com engrenagem.
8. Avatar 22×22 com a inicial do perfil.

### Trilho A–Z (w 40, padding vertical 8)

- 27 botões (`#`, A–Z), cada um 24×19, centralizados, gap 1, raio 4, texto 11 semibold.
- Estados: **ativa** (letra da mesa selecionada) com fundo `accent` e texto `on.accent`; **com mesas** com texto `text.mid`; **vazia** com texto `text.off`, desabilitada e pulada pelo L/R.

### Lista (x 40, w 820)

**Barra de título** (h 44, padding 0 14, gap 12, borda inferior `line`):
- H1 "Mesas", depois o resumo em mono 12 `text.dim`: "{total} mesas · {fav} ★ · {GB} GB".
- Espaço flexível, chips (Todas, Favoritas, Solid state, Eletromecânicas, ROM faltando) e "Ordem: A–Z" em 12 `text.mid`.

**Cabeçalho de colunas** (h 24, padding 0 12, borda inferior `line`). Grade com gap de 8 px:

| # | Coluna | Largura | Alinhamento | Conteúdo |
|---|---|---|---|---|
| 1 | ★ | 22 | esq. | "★" ou vazio |
| 2 | MESA | flex (≈214) | esq. | nome, com reticências |
| 3 | FABRICANTE | 78 | esq. | com reticências |
| 4 | ANO | 36 | esq. | mono |
| 5 | TIPO | 30 | esq. | mono: `SS` / `EM` |
| 6 | ROM | 72 | esq. | mono; "—" em EM; "ROM faltando" em `warn` |
| 7 | B2S | 28 | esq. | `●` presente / `·` ausente |
| 8 | DMD | 28 | esq. | idem |
| 9 | PUP | 28 | esq. | idem |
| 10 | JOG. | 30 | esq. | número de jogadores |
| 11 | TEMPO | 56 | dir. | "14h 06m" ou "—" |
| 12 | RECORDE | 84 | dir. | com separador de milhar "." ou "—" |

**Linhas**: altura 24, a partir de y 104, com 24 visíveis.
- Normal: fundo transparente, e as linhas ímpares usam `bg.zebra`. A coluna MESA usa `text.row`; as colunas 3 a 12 usam `text.dim`.
- Selecionada: fundo `sel.bg`, todo o texto em `sel.text` semibold, faixa de 3 px em `accent` na borda esquerda e ★ em `#A86A00`.
- Barra de rolagem: 4 px de largura, raio 2, cor `#3A4350`, a 3 px da borda direita. A altura é proporcional à fração visível.

### Painel de detalhes (x 860, w 420, padding 14, gap 10, coluna vertical)

Medidas absolutas em tela:

| Elemento | x | y | w | h | Notas |
|---|---|---|---|---|---|
| **Screenshot do playfield** | 874 | 50 | 392 | 220 | **16:9, maior destaque da tela.** Raio 6, sombra `0 8 22 rgba(0,0,0,.45)`, `object-fit: cover` |
| Linha de título | 874 | 280 | 392 | 26 | H2 com reticências + selo "★ Favorita" + selo "SS · 4 jog." |
| Grade de dados | 874 | 316 | 392 | ~60 | 4 colunas: rótulo · valor · rótulo · valor; gap linha 6 / coluna 10 |
| Recordes locais | 874 | ~386 | 392 | ~70 | `bg.card`, raio 6, padding 8×10 |
| (espaço flexível) | | | | | |
| Botão Jogar | 874 | 578 | 392 | 44 | `accent`, texto `on.accent` 16 bold, glifo A invertido, raio 6 |
| Jogar em retrato | 874 | 630 | 340 | 44 | `bg.button`, ícone de celular em pé + texto 13 semibold |
| Botão + (Opções) | 1222 | 630 | 44 | 44 | `bg.button`, glifo `+`, abre **Opções da mesa** |

- **Screenshot**: arquivo PNG/JPG 16:9 (ex.: 1920×1080) na vista desktop da mesa, que aparece em perspectiva no centro com o backglass nas laterais. Ele é reduzido para 392×220 (588×330 no dock). É a **única imagem** do painel; backglass e DMD não aparecem aqui.
- **Fallback sem screenshot**: retângulo `placeholder` com o rótulo mono 10 "sem screenshot · + para capturar".
- **Grade de dados**: Fabricante ("Williams · 1997"), ROM, Tempo jogado, Última ("ontem 23:10"), Extras ("B2S · DMD · PuP").
- **Recordes**: cabeçalho "RECORDES LOCAIS" (11, tracking 0.08em, `text.label`) com "nvram" à direita, seguido das 2 melhores pontuações ("1. GIL ··· 412.880.150", sendo a segunda em `text.mid`).

### Rodapé (h 32, padding 0 14, gap 16, texto 12 `text.mid`)

- Esquerda: posição em mono 11 `text.label`, "{índice} / {total}".
- Direita, nesta ordem: `L` `R` Letra · `ZL` `ZR` Página · `Y` Grade · `X` Favoritar · `+` Opções · `B` Voltar · **`A` Jogar** (glifo ativo, texto branco semibold).

### Controles

| Entrada | Ação |
|---|---|
| D-pad ↑/↓, analógico esq. | move a seleção 1 linha (repetição após 300 ms, depois a cada 50 ms) |
| ZL / ZR | página anterior/próxima (24 linhas) |
| L / R | letra anterior/próxima com mesas (pula letras vazias) |
| Toque no trilho | salta para a letra |
| A | joga a mesa com as opções salvas |
| X | alterna favorita |
| Y | alterna para a grade de screenshots (tela futura) |
| + | abre Opções da mesa |
| − | abre a busca (teclado do sistema, swkbd) |
| B | volta / fecha |
| Toque numa linha | seleciona; segundo toque joga |

---

## 3. Tela: Opções da mesa

### Wireframe

```
x:0                    360                                                              1280
y:0  ┌──────────────────────────────────────────────────────────────────────────────────────┐
     │ ● vpnx   Mesas › Medieval Madness › Opções                PORTÁTIL·720p bat 08:44    │
y:36 ├──────────────────────┬───────────────────────────────────────────────────────────────┤
     │ ┌──────────────────┐ │ (Esta mesa)(Padrão global)              3 opções diferem…     │
     │ │ screenshot 16:9  │ │ TELA                                                          │
     │ │ 328×184          │ │▌Orientação                    [‹]  Retrato (console de lado) [›]│
     │ └──────────────────┘ │  Vista da câmera                ‹   Definida pela mesa      ›  │
     │ Medieval Madness     │  Posição do DMD                 ‹   Sobre o backglass       ›  │
     │ Fabricante  …        │  Backglass na tela              ‹   Topo · 25% da altura    ›  │
     │ ROM         …        │ CONTROLES                                                     │
     │ Arquivo     ….vpx    │  Flippers                       ‹        ZL  ZR             ›  │
     │ Extras      …        │  Lançador                       ‹ Puxar analógico direito   ›  │
     │                      │  Nudge pelo giroscópio          ━━━━━━━━●─────────    60%      │
     │                      │  Vibração no flipper            ‹         Leve              ›  │
     │                      │ SOM E DESEMPENHO                                              │
     │ [(+) Jogar com estas]│  Volume mesa / ROM                    80% · 70%               │
     │                      │  Resolução de render            ‹ Nativa (720p/1080p dock) ›  │
y:688├──────────────────────┴───────────────────────────────────────────────────────────────┤
     │ Salvo em sdmc:/vpnx/tables/Medieval Madness.ini   [L][R] Esta mesa/global [X] Restaurar [B] Voltar [A] Alterar │
y:720└──────────────────────────────────────────────────────────────────────────────────────┘
```

> No mockup, a coluna esquerda desta tela ainda mostra playfield, backglass e DMD separados. Aqui ela já está alinhada à lista e usa **o mesmo screenshot 16:9** (328×184).

### Regiões

| Região | x | y | w | h |
|---|---|---|---|---|
| Cabeçalho (com breadcrumb em vez das abas) | 0 | 0 | 1280 | 36 |
| Coluna da mesa | 0 | 36 | 360 | 652 |
| Lista de opções | 360 | 36 | 920 | 652 |
| Rodapé | 0 | 688 | 1280 | 32 |

### Coluna da mesa (padding 16, gap 12)
- Screenshot 16:9 de 328×184, raio 5.
- H1 24 bold com o nome da mesa.
- Grade rótulo/valor (12 px): Fabricante, ROM, Arquivo (mono, com reticências), Extras.
- Espaço flexível, depois o botão "Jogar com estas opções" (h 44, `accent`, glifo `+`).

### Lista de opções (padding 4 18 12)
- Linha de chips no topo: "Esta mesa" / "Padrão global", com "{n} opções diferem do padrão" à direita.
- Seção: rótulo 11 semibold, tracking 0.1em, `text.label`, margem 14 em cima e 4 embaixo, padding-x 14.
- Linha de opção: altura 38, padding 0 14, raio 5, grade `1fr | 300px`, texto 14.
  - Valor de **ciclo**: `‹  valor centralizado  ›` em 13 `text.mid`.
  - Valor de **slider**: trilho de 4 px `glyph.bg`, preenchimento em `accent`, botão de 14 px `text`, e o % em mono 11 à direita.
  - Selecionada: fundo `sel.bg`, texto `sel.text` semibold, faixa de 3 px `accent`, setas como glifos invertidos.

### Opções e valores

| Seção | Opção | Tipo | Valores |
|---|---|---|---|
| Tela | Orientação | ciclo | Paisagem · Retrato (console de lado) |
| Tela | Vista da câmera | ciclo | Definida pela mesa · Desktop · Cabinet (FSS) |
| Tela | Posição do DMD | ciclo | Sobre o backglass · Flutuante · Oculto |
| Tela | Backglass na tela | ciclo | Topo · 25% da altura · Oculto |
| Controles | Flippers | ciclo | ZL/ZR · L/R |
| Controles | Lançador | ciclo | Puxar analógico direito · Segurar A |
| Controles | Nudge pelo giroscópio | slider | 0–100%, passo 10 |
| Controles | Vibração no flipper | ciclo | Desligada · Leve · Forte |
| Som e desempenho | Volume mesa / ROM | 2 sliders | 0–100% cada |
| Som e desempenho | Resolução de render | ciclo | Nativa · 720p fixo |

### Controles

| Entrada | Ação |
|---|---|
| ↑/↓ | move entre opções (os títulos de seção não são selecionáveis) |
| ←/→ ou A | altera o valor (ciclo / slider) |
| L / R | alterna "Esta mesa" / "Padrão global" |
| X | restaura a opção selecionada para o padrão global |
| + | joga com estas opções |
| B | salva e volta para a lista |

---

## 4. Dados

```c
typedef enum { TABLE_SS, TABLE_EM } TableKind;

typedef struct {
    char     name[128];        // "Medieval Madness"
    char     manufacturer[32]; // "Williams"
    uint16_t year;             // 1997
    TableKind kind;
    char     rom[32];          // "mm_109c"; vazio = EM
    bool     rom_present;      // false => "ROM faltando"
    bool     has_b2s, has_dmd, has_pup;
    uint8_t  players;          // 1–4
    bool     favorite;
    uint32_t play_seconds;     // contado pelo próprio VPNX
    int64_t  last_played;      // epoch; 0 = nunca
    uint64_t high_scores[2];   // lidos da nvram do PinMAME
    char     vpx_path[256];    // sdmc:/vpnx/tables/Medieval Madness.vpx
    char     shot_path[256];   // sdmc:/vpnx/media/playfield/Medieval Madness.png
} Table;
```

Estrutura sugerida no SD:

```
sdmc:/vpnx/
  tables/           *.vpx, *.directb2s, *.ini (opções por mesa)
  roms/             *.zip (PinMAME)
  nvram/            *.nv
  media/playfield/  <nome da mesa>.png  (screenshot 16:9)
  pup/              <pacotes PuP>
  vpnx.ini          opções globais + favoritas
  stats.json        tempo jogado, última sessão
```

## 5. Estados a cobrir

- **Lista vazia**: mensagem centralizada "Nenhuma mesa em sdmc:/vpnx/tables" e o rodapé só com `B`.
- **ROM faltando**: valor em `warn` na coluna ROM. "Jogar" fica desabilitado, com o rótulo "ROM ausente".
- **Sem screenshot**: placeholder descrito acima.
- **Filtro sem resultados**: "Nenhuma mesa com este filtro" na área da lista.
- **Dock**: escala 1,5× em todo o layout. O selo do cabeçalho muda para "DOCK · 1080p".
