#!/usr/bin/env python3
"""
visualizador.py — Visualizador passo a passo da construção da AST (PyQt6)

Uso:
    python3 visualizador.py <arquivo.g>

Protocolo de eventos (um por linha em stdout do compilador):
    N <id> <tipo> <linha> <val>        — nó criado
    L <id_pai> <slot> <id_filho>       — ligação filho1/2/3
    P <id_no> <id_prox>                — ligação prox (lista)
"""

import sys
import os
import math
import subprocess

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget,
    QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QSlider,
    QGraphicsScene, QGraphicsView, QGraphicsItem,
    QMessageBox, QFrame,
)
from PyQt6.QtCore import Qt, QRectF, QTimer
from PyQt6.QtGui import (
    QPainter, QPainterPath, QColor, QPen, QBrush,
    QFont, QTransform,
)

# ── Constantes de layout ──────────────────────────────────────────────────────
NODE_W = 130
NODE_H = 42
H_PAD  = 20
V_GAP  = 75
MARGIN = 50

# ── Paleta (Catppuccin Mocha) ─────────────────────────────────────────────────
BG_WINDOW = QColor('#1e1e2e')
BG_SCENE  = QColor('#181825')
BG_CTRL   = QColor('#313244')
TEXT_MAIN = QColor('#cdd6f4')
TEXT_DIM  = QColor('#7f849c')
HIGHLIGHT = QColor('#f9e2af')


def _cor_no(tipo):
    if tipo == 'NO_PROGRAMA':                   return QColor('#1a6fa8'), QColor('#ffffff')
    if tipo == 'NO_BLOCO':                      return QColor('#217a3d'), QColor('#ffffff')
    if tipo == 'NO_DECL_VAR':                   return QColor('#b87c0a'), QColor('#ffffff')
    if tipo.startswith('NO_CMD_'):              return QColor('#c05c1a'), QColor('#ffffff')
    if tipo in ('NO_ID', 'NO_INTCONST',
                'NO_CARCONST', 'NO_CADEIA'):    return QColor('#aa2222'), QColor('#ffffff')
    return QColor('#6b3fa0'), QColor('#ffffff')


def _label_no(tipo, val):
    short = tipo.replace('NO_CMD_', '').replace('NO_', '')
    if val and val != '-':
        v = val if len(val) <= 14 else val[:13] + '…'
        return f"{short}\n{v}"
    return short


# ── Parsing dos eventos ───────────────────────────────────────────────────────
def parse_events(raw_lines):
    events = []
    for line in raw_lines:
        line = line.rstrip('\n')
        if not line:
            continue
        parts = line.split(None, 4)
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


# ── Algoritmo de layout ───────────────────────────────────────────────────────
def _largura(nid, nodes):
    if nid not in nodes:
        return 0
    n = nodes[nid]
    filhos = [n.get(s) for s in ('f1', 'f2', 'f3')
              if n.get(s) and n.get(s) in nodes]
    w = sum(_largura(c, nodes) for c in filhos) if filhos else NODE_W + H_PAD
    prox = n.get('prox')
    return w + (_largura(prox, nodes) if prox and prox in nodes else 0)


def _assign(nid, nodes, pos, x, y):
    if nid not in nodes:
        return
    n = nodes[nid]
    filhos = [n.get(s) for s in ('f1', 'f2', 'f3')
              if n.get(s) and n.get(s) in nodes]
    w = sum(_largura(c, nodes) for c in filhos) if filhos else NODE_W + H_PAD
    pos[nid] = (x + w / 2, y)
    cx = x
    for c in filhos:
        cw = _largura(c, nodes)
        _assign(c, nodes, pos, cx, y + V_GAP)
        cx += cw
    prox = n.get('prox')
    if prox and prox in nodes:
        _assign(prox, nodes, pos, x + w, y)


def compute_layout(nodes):
    referenciados = set()
    for n in nodes.values():
        for s in ('f1', 'f2', 'f3', 'prox'):
            v = n.get(s)
            if v:
                referenciados.add(v)
    raizes = [nid for nid in nodes if nid not in referenciados]
    pos, x = {}, MARGIN
    for r in raizes:
        _assign(r, nodes, pos, x, MARGIN)
        x += _largura(r, nodes)
    return pos


# ── Item gráfico de nó ────────────────────────────────────────────────────────
class NodeItem(QGraphicsItem):
    RADIUS = 8

    def __init__(self, tipo, val, highlight=False):
        super().__init__()
        self.tipo      = tipo
        self.val       = val
        self.highlight = highlight
        self.bg, self.fg = _cor_no(tipo)

    def boundingRect(self):
        extra = 6 if self.highlight else 0
        return QRectF(-NODE_W / 2 - extra, -NODE_H / 2 - extra,
                       NODE_W + 2 * extra,  NODE_H + 2 * extra)

    def paint(self, painter, option, widget=None):
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        r    = self.RADIUS
        rect = QRectF(-NODE_W / 2, -NODE_H / 2, NODE_W, NODE_H)

        if self.highlight:
            glow = rect.adjusted(-5, -5, 5, 5)
            path = QPainterPath()
            path.addRoundedRect(glow, r + 3, r + 3)
            painter.setPen(Qt.PenStyle.NoPen)
            painter.setBrush(QBrush(HIGHLIGHT))
            painter.drawPath(path)

        path = QPainterPath()
        path.addRoundedRect(rect, r, r)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QBrush(self.bg))
        painter.drawPath(path)

        border_color = HIGHLIGHT if self.highlight else QColor('#45475a')
        painter.setPen(QPen(border_color, 2 if self.highlight else 1))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawPath(path)

        painter.setFont(QFont('Courier', 8, QFont.Weight.Bold))
        painter.setPen(QPen(self.fg))
        painter.drawText(rect, Qt.AlignmentFlag.AlignCenter,
                         _label_no(self.tipo, self.val))


# ── QGraphicsView com zoom e pan ──────────────────────────────────────────────
class ASTView(QGraphicsView):
    def __init__(self, scene):
        super().__init__(scene)
        self.setRenderHint(QPainter.RenderHint.Antialiasing)
        self.setDragMode(QGraphicsView.DragMode.ScrollHandDrag)
        self.setTransformationAnchor(QGraphicsView.ViewportAnchor.AnchorUnderMouse)
        self.setResizeAnchor(QGraphicsView.ViewportAnchor.AnchorViewCenter)
        self.setBackgroundBrush(QBrush(BG_SCENE))
        self.setStyleSheet('border: none;')
        self._zoom = 0

    def wheelEvent(self, event):
        delta = event.angleDelta().y()
        if delta == 0:
            return
        step = 1 if delta > 0 else -1
        self._zoom = max(-10, min(15, self._zoom + step))
        factor = 1.15 if delta > 0 else 1 / 1.15
        self.scale(factor, factor)

    def fit(self):
        self.fitInView(self.scene().sceneRect(),
                       Qt.AspectRatioMode.KeepAspectRatio)
        self._zoom = 0


# ── Janela principal ──────────────────────────────────────────────────────────
class Visualizador(QMainWindow):
    def __init__(self, events, nome_arquivo):
        super().__init__()
        self.events    = events
        self.passo     = 0
        self.nodes     = {}
        self.ultimo_id = None
        self.tocando   = False
        self._timer    = QTimer(self)
        self._timer.timeout.connect(self._tick_auto)

        self.setWindowTitle(f"Visualizador AST — {os.path.basename(nome_arquivo)}")
        self.resize(1200, 750)
        self._apply_style()
        self._build_ui()
        self._atualiza_tudo()

    # ── Estilo ────────────────────────────────────────────────────────────────
    def _apply_style(self):
        self.setStyleSheet(f"""
            QMainWindow, QWidget {{
                background-color: {BG_WINDOW.name()};
                color: {TEXT_MAIN.name()};
            }}
            QPushButton {{
                background-color: {BG_CTRL.name()};
                color: {TEXT_MAIN.name()};
                border: 1px solid #45475a;
                border-radius: 4px;
                padding: 5px 14px;
                font-family: Courier;
                font-size: 11px;
                font-weight: bold;
            }}
            QPushButton:hover    {{ background-color: #45475a; }}
            QPushButton:disabled {{ color: #45475a; background-color: #1e1e2e; }}
            QLabel               {{ background: transparent; font-family: Courier; }}
            QSlider::groove:horizontal {{
                height: 4px; background: #45475a; border-radius: 2px;
            }}
            QSlider::handle:horizontal {{
                background: {TEXT_MAIN.name()};
                width: 12px; height: 12px;
                margin: -4px 0; border-radius: 6px;
            }}
            QFrame[frameShape="4"] {{ color: #45475a; }}
        """)

    # ── Construção da UI ──────────────────────────────────────────────────────
    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(8, 8, 8, 6)
        root.setSpacing(6)

        # Barra de info
        top = QHBoxLayout()
        self.lbl_passo  = QLabel("Passo 0/0")
        self.lbl_evento = QLabel("← use ▶ Próximo para avançar")
        self.lbl_passo.setStyleSheet(f"color: {TEXT_MAIN.name()}; font-size: 10px;")
        self.lbl_evento.setStyleSheet("color: #a6e3a1; font-size: 10px;")
        top.addWidget(self.lbl_passo)
        top.addSpacing(16)
        top.addWidget(self.lbl_evento)
        top.addStretch()
        root.addLayout(top)

        # Canvas
        self.scene = QGraphicsScene()
        self.scene.setBackgroundBrush(QBrush(BG_SCENE))
        self.view  = ASTView(self.scene)
        root.addWidget(self.view, stretch=1)

        # Separador
        sep = QFrame()
        sep.setFrameShape(QFrame.Shape.HLine)
        root.addWidget(sep)

        # Controles
        ctrl = QHBoxLayout()
        ctrl.setSpacing(6)

        self.btn_prev  = QPushButton("◀  Anterior")
        self.btn_next  = QPushButton("▶  Próximo")
        self.btn_auto  = QPushButton("▶▶ Auto")
        self.btn_start = QPushButton("⏮ Início")
        self.btn_end   = QPushButton("⏭ Fim")
        btn_fit        = QPushButton("⤢ Ajustar")

        self.btn_prev.clicked.connect(self.passo_anterior)
        self.btn_next.clicked.connect(self.proximo_passo)
        self.btn_auto.clicked.connect(self.toggle_auto)
        self.btn_start.clicked.connect(self.ir_inicio)
        self.btn_end.clicked.connect(self.ir_fim)
        btn_fit.clicked.connect(self.view.fit)

        for btn in (self.btn_prev, self.btn_next, self.btn_auto,
                    self.btn_start, self.btn_end, btn_fit):
            ctrl.addWidget(btn)

        ctrl.addSpacing(16)
        lbl_vel = QLabel("Velocidade:")
        lbl_vel.setStyleSheet(f"color: {TEXT_DIM.name()}; font-size: 9px;")
        self.sld_vel = QSlider(Qt.Orientation.Horizontal)
        self.sld_vel.setRange(50, 1500)
        self.sld_vel.setValue(400)
        self.sld_vel.setFixedWidth(120)
        ctrl.addWidget(lbl_vel)
        ctrl.addWidget(self.sld_vel)

        ctrl.addStretch()
        self._legenda(ctrl)
        root.addLayout(ctrl)

    def _legenda(self, layout):
        itens = [
            ('PROGRAMA/BLOCO', '#1a6fa8'),
            ('DECL',           '#b87c0a'),
            ('COMANDO',        '#c05c1a'),
            ('FOLHA',          '#aa2222'),
            ('EXPRESSÃO',      '#6b3fa0'),
        ]
        for label, cor in reversed(itens):
            dot = QLabel('■')
            dot.setStyleSheet(f"color: {cor}; font-size: 14px;")
            lbl = QLabel(label)
            lbl.setStyleSheet(f"color: {TEXT_DIM.name()}; font-size: 8px;")
            layout.addWidget(dot)
            layout.addWidget(lbl)
            layout.addSpacing(2)

    # ── Teclado ───────────────────────────────────────────────────────────────
    def keyPressEvent(self, event):
        key = event.key()
        if key in (Qt.Key.Key_Right, Qt.Key.Key_Period):
            self.proximo_passo()
        elif key in (Qt.Key.Key_Left, Qt.Key.Key_Comma):
            self.passo_anterior()
        elif key == Qt.Key.Key_Space:
            self.toggle_auto()
        elif key == Qt.Key.Key_Home:
            self.ir_inicio()
        elif key == Qt.Key.Key_End:
            self.ir_fim()
        else:
            super().keyPressEvent(event)

    # ── Lógica de passos ──────────────────────────────────────────────────────
    def _aplica_evento(self, ev):
        if ev['t'] == 'N':
            self.nodes[ev['id']] = {
                'tipo': ev['tipo'], 'linha': ev['linha'], 'val': ev['val'],
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
        self.nodes, self.ultimo_id = {}, None
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
            self.btn_auto.setText('■  Parar')
            self._timer.start(self.sld_vel.value())

    def _tick_auto(self):
        if self.passo >= len(self.events):
            self._parar_auto()
            return
        self.proximo_passo()
        self._timer.setInterval(self.sld_vel.value())

    def _parar_auto(self):
        self.tocando = False
        self.btn_auto.setText('▶▶ Auto')
        self._timer.stop()

    # ── Atualização ───────────────────────────────────────────────────────────
    def _atualiza_tudo(self):
        total = len(self.events)
        self.lbl_passo.setText(f"Passo {self.passo}/{total}")
        self.lbl_evento.setText(self._desc_ultimo_evento())
        self.btn_prev.setEnabled(self.passo > 0)
        self.btn_next.setEnabled(self.passo < total)
        self._desenha()

    def _desc_ultimo_evento(self):
        u = self.ultimo_id
        if u is None:
            return "← use ▶ Próximo para avançar"
        if u[0] == 'N':
            n = self.nodes.get(u[1], {})
            t = n.get('tipo', '?').replace('NO_', '')
            return (f"Criado: {t}({n.get('val','-')})  "
                    f"[id={u[1]}, linha={n.get('linha','?')}]")
        if u[0] == 'L':
            return f"Ligação: nó {u[1]}.{u[2]} → nó {u[3]}"
        if u[0] == 'P':
            return f"Prox: nó {u[1]} → nó {u[2]}"
        return ""

    # ── Desenho ───────────────────────────────────────────────────────────────
    def _desenha(self):
        self.scene.clear()

        if not self.nodes:
            txt = self.scene.addText(
                "Nenhum nó ainda.\nClique em ▶ Próximo.",
                QFont('Courier', 14))
            txt.setDefaultTextColor(TEXT_DIM)
            return

        pos = compute_layout(self.nodes)
        if not pos:
            return

        # Identifica destaque
        destaque_no     = None
        destaque_aresta = None
        u = self.ultimo_id
        if u:
            if u[0] == 'N':
                destaque_no = u[1]
            elif u[0] == 'L':
                destaque_aresta = (u[1], u[2], u[3])
            elif u[0] == 'P':
                destaque_aresta = (u[1], 'prox', u[2])

        # Arestas (abaixo dos nós)
        for nid, n in self.nodes.items():
            if nid not in pos:
                continue
            px, py = pos[nid]
            for slot, fid in [('f1', n.get('f1')), ('f2', n.get('f2')),
                               ('f3', n.get('f3')), ('prox', n.get('prox'))]:
                if not fid or fid not in pos:
                    continue
                cx, cy = pos[fid]
                hl = (destaque_aresta == (nid, slot, fid))
                self._desenha_aresta(px, py, cx, cy, slot, hl)

        # Nós
        for nid, n in self.nodes.items():
            if nid not in pos:
                continue
            x, y = pos[nid]
            item = NodeItem(n['tipo'], n['val'], nid == destaque_no)
            item.setPos(x, y)
            self.scene.addItem(item)

        # Ajusta sceneRect
        xs = [p[0] for p in pos.values()]
        ys = [p[1] for p in pos.values()]
        self.scene.setSceneRect(
            min(xs) - MARGIN - NODE_W,
            min(ys) - MARGIN - NODE_H,
            max(xs) - min(xs) + 2 * MARGIN + 2 * NODE_W,
            max(ys) - min(ys) + 2 * MARGIN + 4 * NODE_H,
        )

    def _desenha_aresta(self, px, py, cx, cy, slot, highlight):
        eh_prox   = (slot == 'prox')
        cor       = HIGHLIGHT if highlight else (
                        QColor('#585b70') if eh_prox else QColor('#6c7086'))
        espessura = 2 if highlight else 1

        pen = QPen(cor, espessura)
        if eh_prox:
            pen.setStyle(Qt.PenStyle.DashLine)

        sx, sy = (px + NODE_W / 2, py) if eh_prox else (px, py + NODE_H / 2)
        ex, ey = (cx - NODE_W / 2, cy) if eh_prox else (cx, cy - NODE_H / 2)

        self.scene.addLine(sx, sy, ex, ey, pen)
        self._desenha_seta(ex, ey, sx, sy, cor, espessura)

        if not eh_prox:
            lbl = self.scene.addText(slot.replace('f', 'filho'),
                                     QFont('Courier', 7))
            lbl.setDefaultTextColor(TEXT_DIM)
            lbl.setPos((sx + ex) / 2 + 8, (sy + ey) / 2 - 8)

    def _desenha_seta(self, tx, ty, fx, fy, cor, espessura):
        dx, dy = tx - fx, ty - fy
        length = math.hypot(dx, dy)
        if length == 0:
            return
        ux, uy = dx / length, dy / length
        sz = 8
        lx = tx - sz * ux + sz * 0.4 * (-uy)
        ly = ty - sz * uy + sz * 0.4 * ux
        rx = tx - sz * ux - sz * 0.4 * (-uy)
        ry = ty - sz * uy - sz * 0.4 * ux

        path = QPainterPath()
        path.moveTo(tx, ty)
        path.lineTo(lx, ly)
        path.lineTo(rx, ry)
        path.closeSubpath()
        self.scene.addPath(path, QPen(cor, espessura), QBrush(cor))


# ── Ponto de entrada ──────────────────────────────────────────────────────────
def main():
    if len(sys.argv) != 2:
        print(f"Uso: python3 {sys.argv[0]} <arquivo.g>", file=sys.stderr)
        sys.exit(1)

    arquivo    = sys.argv[1]
    compilador = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'g-v1')

    app = QApplication(sys.argv)

    if not os.path.isfile(compilador):
        QMessageBox.critical(None, "Erro",
            f"Compilador não encontrado: {compilador}\nRode 'make' primeiro.")
        sys.exit(1)

    try:
        resultado = subprocess.run(
            [compilador, '--step', arquivo],
            capture_output=True, text=True)
    except FileNotFoundError:
        QMessageBox.critical(None, "Erro",
            f"Não foi possível executar: {compilador}")
        sys.exit(1)

    if resultado.returncode != 0:
        QMessageBox.critical(None, "Erro de compilação",
            f"O compilador reportou:\n\n{resultado.stderr.strip()}")
        sys.exit(1)

    events = parse_events(resultado.stdout.splitlines())

    if not events:
        QMessageBox.information(None, "Aviso",
            "Nenhum evento gerado. O arquivo está vazio?")
        sys.exit(0)

    win = Visualizador(events, arquivo)
    win.show()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
