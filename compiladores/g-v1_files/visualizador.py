#!/usr/bin/env python3
"""
visualizador.py — Visualizador passo a passo da construção da AST do compilador g-v1

Uso:
    python3 visualizador.py <arquivo.g>

O script executa './g-v1 --step <arquivo.g>', captura os eventos de construção
da árvore e exibe cada passo numa janela gráfica tkinter.

Protocolo de eventos (um por linha em stdout do compilador):
    N <id> <tipo> <linha> <val>        — nó criado
    L <id_pai> <slot> <id_filho>       — ligação filho1/2/3
    P <id_no> <id_prox>                — ligação prox (lista)
"""

import sys
import os
import subprocess
import tkinter as tk
from tkinter import ttk, messagebox

# ── Constantes de layout ───────────────────────────────────────────
NODE_W   = 130   # largura do retângulo do nó
NODE_H   = 38    # altura do retângulo do nó
H_PAD    = 18    # espaçamento horizontal mínimo entre sub-árvores
V_GAP    = 72    # distância vertical pai → filho
MARGIN   = 40    # margem ao redor da árvore no canvas

# ── Cores por categoria de nó ─────────────────────────────────────
def _cor_no(tipo):
    """Retorna (cor_fundo, cor_texto) para o tipo de nó."""
    if tipo == 'NO_PROGRAMA':                    return '#1a6fa8', '#ffffff'
    if tipo == 'NO_BLOCO':                       return '#217a3d', '#ffffff'
    if tipo == 'NO_DECL_VAR':                    return '#b87c0a', '#ffffff'
    if tipo.startswith('NO_CMD_'):               return '#c05c1a', '#ffffff'
    if tipo in ('NO_ID', 'NO_INTCONST',
                'NO_CARCONST', 'NO_CADEIA'):     return '#aa2222', '#ffffff'
    return '#6b3fa0', '#ffffff'                  # expressões binárias/unárias

def _label_no(tipo, val):
    """Texto curto exibido dentro do retângulo."""
    short = (tipo.replace('NO_CMD_', '')
                 .replace('NO_', ''))
    if val and val != '-':
        # Limita o val a 14 chars para não extrapolar o retângulo
        v = val if len(val) <= 14 else val[:13] + '…'
        return f"{short}\n{v}"
    return short

# ── Parsing dos eventos ───────────────────────────────────────────
def parse_events(raw_lines):
    """
    Converte as linhas de texto do compilador numa lista de dicts.
    Cada dict tem uma chave 't' ('N', 'L' ou 'P') e os campos correspondentes.
    """
    events = []
    for line in raw_lines:
        line = line.rstrip('\n')
        if not line:
            continue
        parts = line.split(None, 4)   # split em no máximo 5 partes
        t = parts[0]
        if t == 'N' and len(parts) >= 5:
            events.append({'t': 'N', 'id': int(parts[1]),
                           'tipo': parts[2], 'linha': int(parts[3]),
                           'val': parts[4]})
        elif t == 'L' and len(parts) == 4:
            events.append({'t': 'L', 'pai': int(parts[1]),
                           'slot': parts[2], 'filho': int(parts[3])})
        elif t == 'P' and len(parts) == 3:
            events.append({'t': 'P', 'no': int(parts[1]),
                           'prox': int(parts[2])})
    return events

# ── Algoritmo de layout ───────────────────────────────────────────
#
# Convenção:
#   - filho1/2/3 ("filhos verticais"): aparecem um nível ABAIXO do pai
#   - prox ("irmão de lista"):         aparecem no MESMO nível, à direita
#
# A largura de uma sub-árvore com raiz N é:
#   largura(N) = max(NODE_W + H_PAD,
#                    soma dos larguras dos filhos verticais)
#              + largura(N.prox)
#
# Isso garante que os irmãos de lista ficam lado a lado no mesmo nível.

def _largura(nid, nodes):
    """Largura total da sub-árvore enraizada em nid (incluindo cadeia prox)."""
    if nid not in nodes:
        return 0
    n = nodes[nid]
    filhos = [n.get(s) for s in ('f1', 'f2', 'f3')
              if n.get(s) and n.get(s) in nodes]
    if filhos:
        w_propria = sum(_largura(c, nodes) for c in filhos)
    else:
        w_propria = NODE_W + H_PAD

    prox = n.get('prox')
    w_prox = _largura(prox, nodes) if prox and prox in nodes else 0
    return w_propria + w_prox


def _assign(nid, nodes, pos, x, y):
    """Atribui posições (x, y) a nid e a toda a sua sub-árvore."""
    if nid not in nodes:
        return
    n = nodes[nid]
    filhos = [n.get(s) for s in ('f1', 'f2', 'f3')
              if n.get(s) and n.get(s) in nodes]

    # Largura ocupada pelos filhos verticais (sem contar o prox)
    if filhos:
        w_propria = sum(_largura(c, nodes) for c in filhos)
    else:
        w_propria = NODE_W + H_PAD

    # Posição deste nó: centro da região w_propria
    pos[nid] = (x + w_propria / 2, y)

    # Posiciona filhos verticais
    cx = x
    for c in filhos:
        cw = _largura(c, nodes)
        _assign(c, nodes, pos, cx, y + V_GAP)
        cx += cw

    # Posiciona prox à direita (mesmo nível)
    prox = n.get('prox')
    if prox and prox in nodes:
        _assign(prox, nodes, pos, x + w_propria, y)


def compute_layout(nodes):
    """
    Calcula posições para todos os nós do estado atual.
    Nós desconectados (sem pai) são raízes e são empilhados
    horizontalmente na parte de cima do canvas.
    """
    # Determina quais nós têm algum pai
    referenciados = set()
    for n in nodes.values():
        for s in ('f1', 'f2', 'f3', 'prox'):
            v = n.get(s)
            if v:
                referenciados.add(v)

    raizes = [nid for nid in nodes if nid not in referenciados]
    pos = {}
    x = MARGIN
    for r in raizes:
        _assign(r, nodes, pos, x, MARGIN)
        x += _largura(r, nodes)

    return pos

# ── Aplicação ─────────────────────────────────────────────────────

class Visualizador:
    def __init__(self, root_tk, events, nome_arquivo):
        self.root     = root_tk
        self.events   = events
        self.passo    = 0             # próximo evento a aplicar
        self.nodes    = {}            # id → dict com tipo, val, f1, f2, f3, prox
        self.ultimo_id = None         # id do último nó ou aresta destacado
        self.tocando  = False
        self.apos_id  = None          # after() handle para auto-play

        root_tk.title(f"Visualizador AST — {os.path.basename(nome_arquivo)}")
        root_tk.configure(bg='#1e1e2e')
        self._build_ui()
        self._atualiza_tudo()

    # ── Construção da interface ────────────────────────────────────

    def _build_ui(self):
        # ── Barra superior com info ────────────────────────────────
        top = tk.Frame(self.root, bg='#1e1e2e')
        top.pack(side='top', fill='x', padx=8, pady=(8, 0))

        self.lbl_passo = ttk.Label(top, text="", font=('Courier', 10))
        self.lbl_passo.pack(side='left')

        self.lbl_evento = ttk.Label(top, text="", font=('Courier', 10),
                                    foreground='#a6e3a1')
        self.lbl_evento.pack(side='left', padx=16)

        # ── Canvas com scrollbars ──────────────────────────────────
        frame_canvas = tk.Frame(self.root, bg='#1e1e2e')
        frame_canvas.pack(fill='both', expand=True, padx=8, pady=8)

        self.canvas = tk.Canvas(frame_canvas, bg='#181825',
                                highlightthickness=0)
        sb_v = ttk.Scrollbar(frame_canvas, orient='vertical',
                              command=self.canvas.yview)
        sb_h = ttk.Scrollbar(frame_canvas, orient='horizontal',
                              command=self.canvas.xview)
        self.canvas.configure(yscrollcommand=sb_v.set,
                              xscrollcommand=sb_h.set)

        sb_v.pack(side='right', fill='y')
        sb_h.pack(side='bottom', fill='x')
        self.canvas.pack(fill='both', expand=True)

        # Scroll com roda do mouse
        self.canvas.bind('<MouseWheel>',
            lambda e: self.canvas.yview_scroll(-1*(e.delta//120), 'units'))
        self.canvas.bind('<Shift-MouseWheel>',
            lambda e: self.canvas.xview_scroll(-1*(e.delta//120), 'units'))

        # ── Painel de controles ────────────────────────────────────
        ctrl = tk.Frame(self.root, bg='#313244', pady=6)
        ctrl.pack(side='bottom', fill='x')

        style = ttk.Style()
        style.configure('Ctrl.TButton', font=('Courier', 11, 'bold'),
                        padding=4)

        self.btn_prev = ttk.Button(ctrl, text='◀  Anterior',
                                   style='Ctrl.TButton',
                                   command=self.passo_anterior)
        self.btn_prev.pack(side='left', padx=8)

        self.btn_next = ttk.Button(ctrl, text='▶  Próximo',
                                   style='Ctrl.TButton',
                                   command=self.proximo_passo)
        self.btn_next.pack(side='left', padx=4)

        self.btn_auto = ttk.Button(ctrl, text='▶▶ Auto',
                                   style='Ctrl.TButton',
                                   command=self.toggle_auto)
        self.btn_auto.pack(side='left', padx=4)

        ttk.Button(ctrl, text='⏮ Início', style='Ctrl.TButton',
                   command=self.ir_inicio).pack(side='left', padx=4)

        ttk.Button(ctrl, text='⏭ Fim', style='Ctrl.TButton',
                   command=self.ir_fim).pack(side='left', padx=4)

        # Velocidade
        ttk.Label(ctrl, text='  Velocidade:', background='#313244',
                  foreground='#cdd6f4', font=('Courier', 9)).pack(side='left')
        self.vel = tk.IntVar(value=400)
        ttk.Scale(ctrl, from_=50, to=1500, orient='horizontal',
                  variable=self.vel, length=130).pack(side='left', padx=4)

        # Legenda
        self._legenda(ctrl)

    def _legenda(self, parent):
        cores = [
            ('PROGRAMA/BLOCO',  '#1a6fa8'),
            ('DECL',            '#b87c0a'),
            ('COMANDO',         '#c05c1a'),
            ('FOLHA',           '#aa2222'),
            ('EXPRESSÃO',       '#6b3fa0'),
        ]
        for label, cor in cores:
            f = tk.Frame(parent, bg=cor, width=12, height=12)
            f.pack(side='right', padx=(0, 2))
            ttk.Label(parent, text=label, background='#313244',
                      foreground='#cdd6f4',
                      font=('Courier', 7)).pack(side='right', padx=(0, 2))

    # ── Lógica de passos ──────────────────────────────────────────

    def _aplica_evento(self, ev):
        """Aplica um evento ao estado interno self.nodes."""
        if ev['t'] == 'N':
            self.nodes[ev['id']] = {
                'tipo': ev['tipo'], 'linha': ev['linha'],
                'val': ev['val'],
                'f1': None, 'f2': None, 'f3': None, 'prox': None,
            }
            self.ultimo_id = ('N', ev['id'])
        elif ev['t'] == 'L':
            if ev['pai'] in self.nodes:
                self.nodes[ev['pai']][ev['slot']] = ev['filho']
            self.ultimo_id = ('L', ev['pai'], ev['slot'], ev['filho'])
        elif ev['t'] == 'P':
            if ev['no'] in self.nodes:
                self.nodes[ev['no']]['prox'] = ev['prox']
            self.ultimo_id = ('P', ev['no'], ev['prox'])

    def _reconstroi_ate(self, n):
        """Reconstrói self.nodes do zero até o passo n (exclusive)."""
        self.nodes    = {}
        self.ultimo_id = None
        for i in range(n):
            self._aplica_evento(self.events[i])

    def proximo_passo(self):
        if self.passo >= len(self.events):
            return
        self._aplica_evento(self.events[self.passo])
        self.passo += 1
        self._atualiza_tudo()

    def passo_anterior(self):
        if self.passo <= 0:
            return
        self.passo -= 1
        self._reconstroi_ate(self.passo)
        self._atualiza_tudo()

    def ir_inicio(self):
        self._parar_auto()
        self.passo = 0
        self._reconstroi_ate(0)
        self._atualiza_tudo()

    def ir_fim(self):
        self._parar_auto()
        self._reconstroi_ate(len(self.events))
        self.passo = len(self.events)
        self._atualiza_tudo()

    def toggle_auto(self):
        if self.tocando:
            self._parar_auto()
        else:
            self.tocando = True
            self.btn_auto.configure(text='■  Parar')
            self._tick_auto()

    def _tick_auto(self):
        if not self.tocando:
            return
        if self.passo >= len(self.events):
            self._parar_auto()
            return
        self.proximo_passo()
        self.apos_id = self.root.after(self.vel.get(), self._tick_auto)

    def _parar_auto(self):
        self.tocando = False
        self.btn_auto.configure(text='▶▶ Auto')
        if self.apos_id:
            self.root.after_cancel(self.apos_id)
            self.apos_id = None

    # ── Desenho ───────────────────────────────────────────────────

    def _atualiza_tudo(self):
        total = len(self.events)
        self.lbl_passo.configure(
            text=f"Passo {self.passo}/{total}",
            foreground='#cdd6f4', background='#1e1e2e')
        self.lbl_evento.configure(
            text=self._desc_ultimo_evento(), background='#1e1e2e')

        self.btn_prev.state(['disabled'] if self.passo == 0 else ['!disabled'])
        self.btn_next.state(['disabled'] if self.passo >= total else ['!disabled'])

        self._desenha()

    def _desc_ultimo_evento(self):
        u = self.ultimo_id
        if u is None:
            return "← use ▶ Próximo para avançar"
        if u[0] == 'N':
            n = self.nodes.get(u[1], {})
            t = n.get('tipo', '?').replace('NO_', '')
            v = n.get('val', '-')
            return f"Criado: {t}({v}) [id={u[1]}, linha={n.get('linha','?')}]"
        if u[0] == 'L':
            return f"Ligação: nó {u[1]} .{u[2]} → nó {u[3]}"
        if u[0] == 'P':
            return f"Prox: nó {u[1]} → nó {u[2]}"
        return ""

    def _desenha(self):
        c = self.canvas
        c.delete('all')

        if not self.nodes:
            c.create_text(300, 180, text="Nenhum nó ainda.\nClique em ▶ Próximo.",
                          fill='#585b70', font=('Courier', 14), justify='center')
            return

        pos = compute_layout(self.nodes)

        # Ajusta scroll region
        if pos:
            xs = [p[0] for p in pos.values()]
            ys = [p[1] for p in pos.values()]
            c.configure(scrollregion=(
                min(xs) - MARGIN - NODE_W,
                min(ys) - MARGIN - NODE_H,
                max(xs) + MARGIN + NODE_W,
                max(ys) + MARGIN + NODE_H * 3,
            ))

        # Determina o nó/aresta destacado
        destaque_nos   = set()
        destaque_arestas = set()
        u = self.ultimo_id
        if u:
            if u[0] == 'N':
                destaque_nos.add(u[1])
            elif u[0] == 'L':
                destaque_arestas.add((u[1], u[2], u[3]))
            elif u[0] == 'P':
                destaque_arestas.add((u[1], 'prox', u[2]))

        # Desenha arestas primeiro (ficam por baixo dos nós)
        for nid, n in self.nodes.items():
            px, py = pos.get(nid, (0, 0))
            for slot, fid in [('f1', n.get('f1')), ('f2', n.get('f2')),
                               ('f3', n.get('f3')), ('prox', n.get('prox'))]:
                if not fid or fid not in pos:
                    continue
                cx2, cy2 = pos[fid]
                highlight = (nid, slot, fid) in destaque_arestas
                self._desenha_aresta(px, py, cx2, cy2, slot, highlight)

        # Desenha nós
        for nid, n in self.nodes.items():
            x, y = pos.get(nid, (0, 0))
            self._desenha_no(x, y, n['tipo'], n['val'],
                             nid in destaque_nos)

    def _desenha_aresta(self, px, py, cx, cy, slot, highlight):
        c = self.canvas
        eh_prox = (slot == 'prox')

        if eh_prox:
            # Aresta horizontal: lado direito → lado esquerdo
            sx, sy = px + NODE_W // 2,  py
            ex, ey = cx - NODE_W // 2,  cy
            cor    = '#f9e2af' if highlight else '#585b70'
            dash   = (6, 4)
            largura = 1
        else:
            # Aresta vertical: base → topo
            sx, sy = px, py + NODE_H // 2
            ex, ey = cx, cy - NODE_H // 2
            cor    = '#f9e2af' if highlight else '#6c7086'
            dash   = ()
            largura = 1 if not highlight else 2

        c.create_line(sx, sy, ex, ey,
                      fill=cor, dash=dash, width=largura,
                      arrow=tk.LAST, arrowshape=(8, 10, 4))

        # Rótulo da aresta (só para filhos verticais)
        if not eh_prox:
            mx = (sx + ex) / 2 + 8
            my = (sy + ey) / 2
            rotulo = slot.replace('f', 'filho')
            c.create_text(mx, my, text=rotulo,
                          fill='#7f849c', font=('Courier', 7))

    def _desenha_no(self, x, y, tipo, val, highlight):
        c = self.canvas
        bg, fg = _cor_no(tipo)

        x0 = x - NODE_W // 2
        y0 = y - NODE_H // 2
        x1 = x + NODE_W // 2
        y1 = y + NODE_H // 2

        if highlight:
            # Glow amarelo ao redor do nó
            c.create_rectangle(x0 - 4, y0 - 4, x1 + 4, y1 + 4,
                                fill='#f9e2af', outline='', width=0)

        # Retângulo com cantos arredondados (simulado com sobreposição)
        r = 6
        c.create_rectangle(x0 + r, y0, x1 - r, y1, fill=bg, outline='')
        c.create_rectangle(x0, y0 + r, x1, y1 - r, fill=bg, outline='')
        c.create_oval(x0, y0, x0 + 2*r, y0 + 2*r, fill=bg, outline='')
        c.create_oval(x1 - 2*r, y0, x1, y0 + 2*r, fill=bg, outline='')
        c.create_oval(x0, y1 - 2*r, x0 + 2*r, y1, fill=bg, outline='')
        c.create_oval(x1 - 2*r, y1 - 2*r, x1, y1, fill=bg, outline='')

        # Borda
        borda = '#f9e2af' if highlight else '#45475a'
        w_borda = 2 if highlight else 1
        for (ax, ay, bx, by) in [
            (x0+r, y0, x1-r, y0), (x0+r, y1, x1-r, y1),
            (x0, y0+r, x0, y1-r), (x1, y0+r, x1, y1-r),
        ]:
            c.create_line(ax, ay, bx, by, fill=borda, width=w_borda)

        # Texto
        c.create_text(x, y, text=_label_no(tipo, val),
                      fill=fg, font=('Courier', 8, 'bold'),
                      justify='center', width=NODE_W - 8)

# ── Ponto de entrada ──────────────────────────────────────────────

def main():
    if len(sys.argv) != 2:
        print(f"Uso: python3 {sys.argv[0]} <arquivo.g>", file=sys.stderr)
        sys.exit(1)

    arquivo = sys.argv[1]
    compilador = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'g-v1')

    if not os.path.isfile(compilador):
        messagebox.showerror("Erro", f"Compilador não encontrado: {compilador}\nRode 'make' primeiro.")
        sys.exit(1)

    # Executa o compilador em modo --step
    try:
        resultado = subprocess.run(
            [compilador, '--step', arquivo],
            capture_output=True, text=True
        )
    except FileNotFoundError:
        messagebox.showerror("Erro", f"Não foi possível executar: {compilador}")
        sys.exit(1)

    # Erros do compilador vão para stderr
    if resultado.returncode != 0:
        erro = resultado.stderr.strip()
        messagebox.showerror(
            "Erro de compilação",
            f"O compilador reportou:\n\n{erro}"
        )
        sys.exit(1)

    events = parse_events(resultado.stdout.splitlines())

    if not events:
        messagebox.showinfo("Aviso", "Nenhum evento gerado. O arquivo está vazio?")
        sys.exit(0)

    root = tk.Tk()
    root.geometry('1100x720')

    # Tema escuro via ttk
    style = ttk.Style(root)
    style.theme_use('clam')
    style.configure('.', background='#1e1e2e', foreground='#cdd6f4')
    style.configure('TLabel', background='#1e1e2e', foreground='#cdd6f4')
    style.configure('TButton', background='#313244', foreground='#cdd6f4',
                    relief='flat', borderwidth=1)
    style.map('TButton',
              background=[('active', '#45475a'), ('disabled', '#181825')],
              foreground=[('disabled', '#45475a')])
    style.configure('TScrollbar', background='#313244', troughcolor='#181825')
    style.configure('Horizontal.TScale', background='#313244',
                    troughcolor='#45475a')
    style.configure('Vertical.TScale', background='#313244',
                    troughcolor='#45475a')

    Visualizador(root, events, arquivo)
    root.mainloop()


if __name__ == '__main__':
    main()
