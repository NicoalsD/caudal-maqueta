"""Draws the breadboard wiring of the Arduino UNO maqueta (Fritzing-like illustration).

Run: python3 gen_breadboard.py  -> maqueta-uno-protoboard.svg
Everything is fed by the USB of the UNO through the breadboard rails; there is no external
supply. Wire colors are the real ones. The routes are chosen so that no two wires cross,
except the GND of the sensor, which passes under its TRIG and ECHO wires.
"""

from __future__ import annotations

from pathlib import Path

OUT = Path(__file__).resolve().parent / "maqueta-uno-protoboard.svg"
W, H = 1500, 1110

# Breadboard (left half of an 830-point board: columns 1-30 never cross the rail split).
BX, BY, P, COLS = 60, 300, 18, 30
ROW_Y = {
    "T-": BY + 28, "T+": BY + 46,
    **{r: BY + 86 + i * P for i, r in enumerate("jihgf")},
    **{r: BY + 198 + i * P for i, r in enumerate("edcba")},
    "B+": BY + 310, "B-": BY + 328,
}
BOARD_W, BOARD_H = 600, 350

# Arduino UNO, USB on the left, digital header on top, power header at the bottom.
AX, AY, AW, AH = 800, 300, 560, 420
DIGITAL_X = {
    "SCL": 150, "SDA": 170, "AREF": 190, "GND": 210, "13": 230, "12": 250, "11": 270,
    "10": 290, "9": 310, "8": 330, "7": 360, "6": 380, "5": 400, "4": 420, "3": 440,
    "2": 460, "1": 480, "0": 500,
}
POWER_X = {"IOREF": 190, "RESET": 210, "3V3": 230, "5V": 250, "GND1": 270, "GND2": 290, "VIN": 310}
ANALOG_X = {f"A{i}": 350 + i * 20 for i in range(6)}
DIG_Y, POW_Y = AY + 22, AY + AH - 22

COLORS = {
    "red": "#d32f2f", "black": "#212121", "brown": "#8d5524", "orange": "#f57c00",
    "yellow": "#f2c200", "blue": "#1e88e5",
}

parts: list[str] = []


def hole(col: int, row: str) -> tuple[float, float]:
    return BX + 39 + (col - 1) * P, ROW_Y[row]


def dpin(name: str) -> tuple[float, float]:
    return AX + DIGITAL_X[name], DIG_Y


def ppin(name: str) -> tuple[float, float]:
    return AX + POWER_X[name], POW_Y


def text(x: float, y: float, s: str, size: int = 13, fill: str = "#222", weight: str = "normal",
         anchor: str = "middle", rotate: int | None = None) -> None:
    transform = f' transform="rotate({rotate} {x} {y})"' if rotate is not None else ""
    parts.append(
        f'<text x="{x}" y="{y}" font-family="Liberation Sans, Arial, sans-serif" font-size="{size}" '
        f'fill="{fill}" font-weight="{weight}" text-anchor="{anchor}"{transform}>{s}</text>'
    )


def wire(points: list[tuple[float, float]], color: str, label: str | None = None,
         label_at: int = 0, width: float = 6) -> None:
    d = " ".join(f"{'M' if i == 0 else 'L'}{x},{y}" for i, (x, y) in enumerate(points))
    parts.append(f'<path d="{d}" fill="none" stroke="#00000055" stroke-width="{width + 3}" '
                 'stroke-linejoin="round" stroke-linecap="round"/>')
    parts.append(f'<path d="{d}" fill="none" stroke="{COLORS[color]}" stroke-width="{width}" '
                 'stroke-linejoin="round" stroke-linecap="round"/>')
    for x, y in (points[0], points[-1]):
        parts.append(f'<circle cx="{x}" cy="{y}" r="5" fill="{COLORS[color]}" stroke="#000" stroke-width="1.5"/>')
    if label:
        (x1, y1), (x2, y2) = points[label_at], points[label_at + 1]
        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
        parts.append(f'<rect x="{mx - len(label) * 3.6 - 6}" y="{my - 10}" width="{len(label) * 7.2 + 12}" '
                     'height="20" rx="4" fill="#fff" stroke="#999"/>')
        text(mx, my + 5, label, 12, "#111", "bold")


def breadboard() -> None:
    parts.append(f'<rect x="{BX}" y="{BY}" width="{BOARD_W}" height="{BOARD_H}" rx="10" fill="#f4f4f1" stroke="#c8c8c0" stroke-width="2"/>')
    groove = (ROW_Y["f"] + ROW_Y["e"]) / 2
    parts.append(f'<rect x="{BX + 8}" y="{groove - 7}" width="{BOARD_W - 16}" height="14" fill="#e2e2dc"/>')
    for top, bottom in (("T-", "T+"), ("B+", "B-")):
        for row in (top, bottom):
            y = ROW_Y[row]
            positive = row.endswith("+")
            line_y = y - 11 if row in ("T-", "B+") else y + 11
            parts.append(f'<line x1="{BX + 25}" y1="{line_y}" x2="{BX + BOARD_W - 25}" y2="{line_y}" '
                         f'stroke="{"#e53935" if positive else "#1e88e5"}" stroke-width="2.5"/>')
            text(BX + 14, y + 5, "+" if positive else "−", 16, "#e53935" if positive else "#1e88e5", "bold")
            text(BX + BOARD_W - 14, y + 5, "+" if positive else "−", 16, "#e53935" if positive else "#1e88e5", "bold")
            for col in range(1, COLS + 1):
                if col % 6 == 0:
                    continue
                x, _ = hole(col, row)
                parts.append(f'<rect x="{x - 3.5}" y="{y - 3.5}" width="7" height="7" rx="1.5" fill="#3a3a3a"/>')
    for row in "jihgfedcba":
        y = ROW_Y[row]
        text(BX + 22, y + 4, row, 10, "#777")
        for col in range(1, COLS + 1):
            x, _ = hole(col, row)
            parts.append(f'<rect x="{x - 3.5}" y="{y - 3.5}" width="7" height="7" rx="1.5" fill="#3a3a3a"/>')
    for col in (1, 5, 10, 15, 20, 25, 30):
        x, _ = hole(col, "j")
        text(x, ROW_Y["j"] - 12, str(col), 9, "#777")


def arduino() -> None:
    parts.append(f'<rect x="{AX}" y="{AY}" width="{AW}" height="{AH}" rx="16" fill="#00879a" stroke="#00626f" stroke-width="3"/>')
    parts.append(f'<rect x="{AX - 30}" y="{AY + 70}" width="95" height="85" rx="4" fill="#b0b0b0" stroke="#777" stroke-width="2"/>')
    text(AX + 18, AY + 118, "USB", 13, "#333", "bold")
    parts.append(f'<rect x="{AX - 20}" y="{AY + 300}" width="75" height="70" rx="6" fill="#222"/>')
    parts.append(f'<rect x="{AX + 230}" y="{AY + 190}" width="220" height="55" rx="3" fill="#1d1d1d"/>')
    text(AX + 340, AY + 223, "ATMEGA328P", 12, "#bbb")
    text(AX + 400, AY + 160, "Arduino UNO", 26, "#fff", "bold")
    text(AX + 400, AY + 285, "firmware caudal_uno", 14, "#e0f7fa")
    text(AX + 400, AY + 305, "USB al computador (alimenta todo)", 13, "#e0f7fa")
    for group in ((150, 330), (360, 500)):
        parts.append(f'<rect x="{AX + group[0] - 11}" y="{DIG_Y - 11}" width="{group[1] - group[0] + 22}" height="22" fill="#111"/>')
    for name, dx in DIGITAL_X.items():
        parts.append(f'<rect x="{AX + dx - 4}" y="{DIG_Y - 4}" width="8" height="8" fill="#888"/>')
        text(AX + dx, DIG_Y + 32, name, 10, "#fff", rotate=-90 if len(name) > 2 else None)
    text(AX + 430, DIG_Y + 62, "DIGITAL", 11, "#e0f7fa")
    for xs in (POWER_X, ANALOG_X):
        lo, hi = min(xs.values()), max(xs.values())
        parts.append(f'<rect x="{AX + lo - 11}" y="{POW_Y - 11}" width="{hi - lo + 22}" height="22" fill="#111"/>')
        for name, dx in xs.items():
            parts.append(f'<rect x="{AX + dx - 4}" y="{POW_Y - 4}" width="8" height="8" fill="#888"/>')
            label = "GND" if name.startswith("GND") else name
            text(AX + dx, POW_Y - 20, label, 10, "#fff", rotate=-90 if len(label) > 2 else None)
    text(AX + 250, POW_Y - 62, "POWER", 11, "#e0f7fa")
    text(AX + 400, POW_Y - 62, "ANALOG IN", 11, "#e0f7fa")


def hc_sr04(x: float, y: float, pin_xs: list[float]) -> None:
    parts.append(f'<rect x="{x}" y="{y}" width="270" height="120" rx="8" fill="#1565c0" stroke="#0d47a1" stroke-width="2"/>')
    for cx in (x + 62, x + 208):
        parts.append(f'<circle cx="{cx}" cy="{y + 55}" r="44" fill="#cfd8dc" stroke="#90a4ae" stroke-width="4"/>')
        parts.append(f'<circle cx="{cx}" cy="{y + 55}" r="32" fill="#37474f"/>')
    text(x + 135, y + 30, "HC-SR04", 15, "#fff", "bold")
    for px, name in zip(pin_xs, ("Vcc", "Trig", "Echo", "Gnd"), strict=True):
        text(px + 4, y + 114, name, 10, "#fff", "bold", "start", rotate=-90)
        parts.append(f'<rect x="{px - 3}" y="{y + 120}" width="6" height="18" fill="#aaa"/>')
    text(x + 290, y + 50, "Va arriba de la botella,", 13, "#333", "normal", "start")
    text(x + 290, y + 68, "mirando al agua", 13, "#333", "normal", "start")


def servo(x: float, y: float, title: str, plug: tuple[float, float]) -> None:
    parts.append(f'<rect x="{x}" y="{y}" width="150" height="80" rx="6" fill="#2b2b2b" stroke="#000" stroke-width="2"/>')
    parts.append(f'<rect x="{x - 18}" y="{y + 30}" width="186" height="16" rx="4" fill="#2b2b2b" stroke="#000"/>')
    parts.append(f'<rect x="{x + 28}" y="{y + 18}" width="94" height="40" fill="#c2185b"/>')
    text(x + 75, y + 37, "MG90S", 13, "#fff", "bold")
    text(x + 75, y + 52, "Micro servo", 10, "#fff")
    parts.append(f'<circle cx="{x + 120}" cy="{y + 2}" r="12" fill="#444" stroke="#000"/>')
    parts.append(f'<rect x="{x + 100}" y="{y - 6}" width="56" height="12" rx="6" fill="#ddd" stroke="#777"/>')
    text(x + 75, y + 105, title, 13, "#222", "bold")
    px, py = plug
    # Three-wire cable: brown, red, orange, into a black 3-pin plug on row a.
    for i, color in enumerate(("brown", "red", "orange")):
        cx = px + i * P
        sx = x + 50 + i * 10
        parts.append(f'<path d="M{sx},{y} C{sx},{y - 60} {cx},{py + 90} {cx},{py + 22}" fill="none" '
                     f'stroke="{COLORS[color]}" stroke-width="5"/>')
    parts.append(f'<rect x="{px - 11}" y="{py - 8}" width="{2 * P + 22}" height="30" rx="3" fill="#111"/>')


def legend(x: float, y: float) -> None:
    rows = [
        ("brown", "HC-SR04 Vcc (café)", "riel + de arriba"),
        ("yellow", "HC-SR04 Echo (amarillo)", "pin D8"),
        ("orange", "HC-SR04 Trig (naranja)", "pin D9"),
        ("red", "HC-SR04 Gnd (rojo)", "riel − de arriba"),
        ("orange", "Servo 1 señal (naranja)", "pin D10"),
        ("orange", "Servo 2 señal (naranja)", "pin D11"),
        ("red", "Servos rojo  /  Arduino 5V", "riel + de abajo"),
        ("black", "Servos café  /  Arduino GND", "riel − de abajo"),
        ("red", "Puente rojo y negro (col. 1-2)", "une rieles de arriba y abajo"),
    ]
    parts.append(f'<rect x="{x}" y="{y}" width="760" height="{58 + 24 * len(rows)}" rx="10" fill="#fafafa" stroke="#bbb"/>')
    text(x + 18, y + 28, "Conexiones", 16, "#111", "bold", "start")
    for i, (color, what, where) in enumerate(rows):
        yy = y + 54 + i * 24
        parts.append(f'<line x1="{x + 20}" y1="{yy - 4}" x2="{x + 56}" y2="{yy - 4}" stroke="{COLORS[color]}" stroke-width="6" stroke-linecap="round"/>')
        text(x + 70, yy, what, 13, "#222", "normal", "start")
        text(x + 360, yy, "→  " + where, 13, "#222", "bold", "start")


def notes(x: float, y: float) -> None:
    lines = [
        "<tspan font-weight='bold'>Sin fuente externa:</tspan> todo se alimenta del USB del computador (unos 500 mA).",
        "El firmware mueve un servo a la vez y despacio, así que alcanza para el sensor y los dos MG90S.",
        "Si el Arduino se reinicia al mover un servo: ajusta el ángulo cerrado para que no fuerce la",
        "manguera y, si tienes uno, pon un condensador de 470-1000 µF entre los rieles de abajo (+ al +).",
        "<tspan font-weight='bold'>¡Ojo!</tspan> En el sensor el café es Vcc y el rojo es GND (al revés de lo normal).",
        "Revisa esos dos cables antes de conectar el USB: invertidos dañan el sensor.",
        "<tspan font-weight='bold'>Protoboard:</tspan> usa solo la mitad izquierda (columnas 1 a 30); los rieles se cortan en la mitad.",
    ]
    parts.append(f'<rect x="{x}" y="{y}" width="640" height="{30 + 22 * len(lines)}" rx="10" fill="#fff8e1" stroke="#f9a825"/>')
    for i, line in enumerate(lines):
        parts.append(f'<text x="{x + 16}" y="{y + 28 + i * 22}" font-family="Liberation Sans, Arial, sans-serif" '
                     f'font-size="13" fill="#333">{line}</text>')


def main() -> None:
    parts.append(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
    text(40, 42, "Maqueta CAUDAL con Arduino UNO: conexiones en la protoboard", 24, "#111", "bold", "start")
    text(40, 66, "Colores reales de los cables. Todo se alimenta del USB del Arduino.", 14, "#555", "normal", "start")
    breadboard()
    arduino()

    # Bridges: top rails joined to the bottom rails at the left end.
    wire([hole(1, "T-"), hole(1, "B-")], "black", width=5)
    wire([hole(2, "T+"), hole(2, "B+")], "red", width=5)

    # Arduino power into the bottom rails.
    x5, y5 = ppin("5V")
    xg, yg = ppin("GND1")
    wire([(x5, y5), (x5, 745), (hole(29, "B+")[0], 745), hole(29, "B+")], "red", "5V", 1)
    wire([(xg, yg), (xg, 768), (hole(28, "B-")[0], 768), hole(28, "B-")], "black", "GND", 1)

    # HC-SR04 above the board. Its GND goes first, so the signal wires pass over it.
    pin_xs = [hole(c, "j")[0] for c in (20, 21, 22, 23)]
    hc_sr04(pin_xs[0] - 103, 82, pin_xs)
    pin_y = 82 + 138
    wire([(pin_xs[3], pin_y), hole(23, "T-")], "red")
    wire([(pin_xs[0], pin_y), hole(20, "T+")], "brown")
    xd9, yd9 = dpin("9")
    xd8, yd8 = dpin("8")
    wire([(pin_xs[1], pin_y), (pin_xs[1], 268), (xd9, 268), (xd9, yd9)], "orange", "Trig → D9", 1)
    wire([(pin_xs[2], pin_y), (pin_xs[2], 252), (xd8, 252), (xd8, yd8)], "yellow", "Echo → D8", 1)

    # Servo 1 plugged in columns 6-8, servo 2 in columns 15-17 (row a).
    servo(90, 830, "Servo 1: llave salida 1", hole(6, "a"))
    servo(330, 830, "Servo 2: llave salida 2", hole(15, "a"))
    for brown_col, red_col, sig_col, gnd_col, plus_col in ((6, 7, 8, 4, 10), (15, 16, 17, 13, 19)):
        wire([hole(brown_col, "c"), (hole(gnd_col, "c")[0], ROW_Y["c"]), hole(gnd_col, "B-")], "black", width=5)
        wire([hole(red_col, "b"), (hole(plus_col, "b")[0], ROW_Y["b"]), hole(plus_col, "B+")], "red", width=5)
    groove = (ROW_Y["f"] + ROW_Y["e"]) / 2
    x10, y10 = dpin("10")
    x11, y11 = dpin("11")
    s1x = hole(8, "e")[0]
    s2x = hole(17, "e")[0]
    wire([hole(8, "e"), (s1x, groove - 8), (700, groove - 8), (700, 286), (x10, 286), (x10, y10)],
         "orange", "Servo 1 → D10", 3)
    wire([hole(17, "e"), (s2x, groove + 8), (720, groove + 8), (720, 302), (x11, 302), (x11, y11)],
         "orange", "Servo 2 → D11", 2)

    legend(700, 830)
    notes(40, 975)
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H + 60}" viewBox="0 0 {W} {H + 60}">'
           + "".join(parts) + "</svg>")
    OUT.write_text(svg, encoding="utf-8")
    print(OUT)


if __name__ == "__main__":
    main()
