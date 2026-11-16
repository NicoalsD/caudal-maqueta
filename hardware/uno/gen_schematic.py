"""Generates the KiCad schematic of the Arduino UNO maqueta (maqueta-uno.kicad_sch).

Run: python3 gen_schematic.py <kicad symbols dir>
The symbols are copied from the stock KiCad libraries, so the schematic opens without extra
libraries. Every connection is a wire stub ending in a net label; nets with the same name are
the same wire. Unused pins get a no-connect flag, so the ERC stays clean.
"""

from __future__ import annotations

import re
import sys
import uuid
from dataclasses import dataclass
from pathlib import Path

OUT = Path(__file__).resolve().parent / "maqueta-uno.kicad_sch"
STUB_MM = 5.08
NS = uuid.UUID("6f1c3a52-2c7e-4b8e-9d55-0c0a7b1d2e11")


def uid(*parts: object) -> str:
    return str(uuid.uuid5(NS, "/".join(str(p) for p in parts)))


def extract(text: str, start: int) -> str:
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return text[start : i + 1]
    raise ValueError("unbalanced")


def load_symbol(libdir: Path, lib: str, name: str) -> str:
    text = (libdir / f"{lib}.kicad_sym").read_text(encoding="utf-8")
    match = re.search(rf'\n\t*\(symbol "{re.escape(name)}"', text)
    if not match:
        raise KeyError(f"{lib}:{name}")
    body = extract(text, match.start() + text[match.start() :].index("("))
    parent = re.search(r'\(extends "([^"]+)"\)', body)
    if parent:
        base = load_symbol(libdir, lib, parent.group(1)).replace(
            f'(symbol "{lib}:{parent.group(1)}"', f'(symbol "{lib}:{name}"', 1
        )
        base = base.replace(f'(symbol "{parent.group(1)}_', f'(symbol "{name}_')
        for prop in ("Value", "Description", "Datasheet", "Footprint", "ki_keywords"):
            own = re.search(rf'\(property "{prop}" "([^"]*)"', body)
            if own:
                base = re.sub(
                    rf'(\(property "{prop}" ")[^"]*(")', lambda m: m.group(1) + own.group(1) + m.group(2), base, count=1
                )
        return base
    return body.replace(f'(symbol "{name}"', f'(symbol "{lib}:{name}"', 1)


@dataclass(frozen=True)
class Pin:
    number: str
    name: str
    x: float
    y: float
    angle: int
    hidden: bool


def pins_of(symbol: str) -> list[Pin]:
    found = []
    for m in re.finditer(
        r'\(pin \w+ \w+(?P<hide> hide)?\s*\(at (?P<x>[-\d.]+) (?P<y>[-\d.]+) (?P<a>\d+)\).*?'
        r'\(name "(?P<name>[^"]*)".*?\(number "(?P<num>[^"]*)"',
        symbol,
        re.S,
    ):
        hidden = bool(m.group("hide")) or "(hide yes)" in symbol[m.start() : m.start() + 200]
        found.append(
            Pin(m.group("num"), m.group("name"), float(m.group("x")), float(m.group("y")),
                int(m.group("a")), hidden)
        )
    return found


def custom_symbol(name: str, description: str, half_w: float, half_h: float,
                  pins: list[tuple[str, str, str, float, float, int]]) -> str:
    """Symbol of the `caudal` library. Each pin: (type, name, number, x, y, angle)."""
    pin_text = "".join(
        f'(pin {kind} line (at {x} {y} {angle}) (length 2.54) '
        f'(name "{pin_name}" (effects (font (size 1.27 1.27)))) '
        f'(number "{number}" (effects (font (size 1.27 1.27)))))'
        for kind, pin_name, number, x, y, angle in pins
    )
    return (
        f'(symbol "caudal:{name}" (pin_names (offset 0.762)) (exclude_from_sim no) (in_bom yes) (on_board yes) '
        f'(property "Reference" "U" (at 0 {half_h + 3} 0) (effects (font (size 1.27 1.27)))) '
        f'(property "Value" "{name}" (at 0 {-half_h - 3} 0) (effects (font (size 1.27 1.27)))) '
        f'(property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide)) '
        f'(property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide)) '
        f'(property "Description" "{description}" (at 0 0 0) (effects (font (size 1.27 1.27)) hide)) '
        f'(symbol "{name}_0_1" (rectangle (start {-half_w} {half_h}) (end {half_w} {-half_h}) '
        f'(stroke (width 0.254) (type default)) (fill (type background)))) '
        f'(symbol "{name}_1_1" {pin_text}))'
    )


# Power pins on top and bottom, signals on the left: every wire leaves the part on its own side.
CUSTOM = {
    "hc": custom_symbol("HC-SR04", "Ultrasonic distance sensor, 5 V", 7.62, 7.62, [
        ("power_in", "VCC", "1", 0, 10.16, 270),
        ("input", "TRIG", "2", -10.16, 1.27, 0),
        ("output", "ECHO", "3", -10.16, -1.27, 0),
        ("power_in", "GND", "4", 0, -10.16, 90),
    ]),
    "servo": custom_symbol("Servo_MG90S", "Micro servo MG90S: orange PWM, red +5V, brown GND", 6.35, 5.08, [
        ("input", "PWM", "1", -8.89, 0, 0),
        ("power_in", "+", "2", 0, 7.62, 270),
        ("power_in", "-", "3", 0, -7.62, 90),
    ]),
    "jack": custom_symbol("Fuente_5V", "External 5 V supply for the servos, common GND", 6.35, 5.08, [
        ("passive", "+", "1", 0, 7.62, 270),
        ("passive", "-", "2", 0, -7.62, 90),
    ]),
}


def main(libdir: Path) -> None:
    lib_ids = {
        "uno": ("MCU_Module", "Arduino_UNO_R3"),
        "cap": ("Device", "C_Polarized"),
        "p5v": ("power", "+5V"),
        "gnd": ("power", "GND"),
        "flag": ("power", "PWR_FLAG"),
    }
    symbols = {key: load_symbol(libdir, *value) for key, value in lib_ids.items()}
    symbols.update(CUSTOM)
    lib_names = {key: f"{lib}:{name}" for key, (lib, name) in lib_ids.items()}
    for key, text in CUSTOM.items():
        lib_names[key] = re.search(r'\(symbol "([^"]+)"', text).group(1)

    items: list[str] = []
    instances: list[tuple[str, str, str]] = []

    def place(key: str, ref: str, value: str, x: float, y: float, nets: dict[str, str],
              hide_value: bool = False, rot: int = 0, text_dx: float = 0, ref_dy: float = -3,
              value_dy: float = 3) -> None:
        sym_uuid = uid("sym", ref)
        pins = pins_of(symbols[key])
        pin_entries = "".join(f'(pin "{p.number}" (uuid "{uid(ref, p.number)}"))' for p in pins)
        items.append(
            f'(symbol (lib_id "{lib_names[key]}") (at {x} {y} {rot}) (unit 1) (exclude_from_sim no) '
            f'(in_bom yes) (on_board yes) (dnp no) (uuid "{sym_uuid}") '
            f'(property "Reference" "{ref}" (at {x + text_dx} {y + ref_dy} 0) (effects (font (size 1.27 1.27)){" hide" if ref.startswith("#") else ""})) '
            f'(property "Value" "{value}" (at {x + text_dx} {y + value_dy} 0) (effects (font (size 1.27 1.27)){" hide" if hide_value else ""}{" (justify left)" if text_dx else ""})) '
            f'(property "Footprint" "" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide)) '
            f'(property "Datasheet" "" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide)) '
            f"{pin_entries} "
            f'(instances (project "maqueta-uno" (path "/{uid("root")}" (reference "{ref}") (unit 1)))))'
        )
        if key in ("p5v", "gnd", "flag"):
            return
        for p in pins:
            if p.hidden:
                continue
            px, py = x + p.x, y - p.y
            label = nets.get(p.name) or nets.get(p.number)
            if label is None:
                items.append(f'(no_connect (at {px} {py}) (uuid "{uid("nc", ref, p.number)}"))')
                continue
            # Outward direction of the pin end (symbol y axis points up, sheet y points down).
            dx, dy, langle = {0: (-1, 0, 180), 180: (1, 0, 0), 90: (0, 1, 270), 270: (0, -1, 90)}[p.angle]
            ex, ey = px + dx * STUB_MM, py + dy * STUB_MM
            items.append(
                f'(wire (pts (xy {px} {py}) (xy {ex} {ey})) (stroke (width 0) (type default)) '
                f'(uuid "{uid("w", ref, p.number)}"))'
            )
            if label in ("+5V", "GND"):
                power_symbol(label, ex, ey, (dx, dy))
                continue
            justify = "left" if langle in (0, 90) else "right"
            items.append(
                f'(label "{label}" (at {ex} {ey} {langle}) (fields_autoplaced yes) '
                f'(effects (font (size 1.27 1.27)) (justify {justify} bottom)) (uuid "{uid("l", ref, p.number)}"))'
            )

    counter = {"pwr": 0, "flag": False}

    def power_symbol(net: str, x: float, y: float, direction: tuple[int, int]) -> None:
        """+5V or GND symbol at the end of a stub, turned to point away from the pin."""
        counter["pwr"] += 1
        ref = f"#PWR{counter['pwr']:02d}"
        if net == "+5V":
            rot = {(0, -1): 0, (0, 1): 180, (-1, 0): 90, (1, 0): 270}[direction]
            place("p5v", ref, "+5V", x, y, {}, rot=rot, ref_dy=-6, value_dy=-4)
        else:
            rot = {(0, 1): 0, (0, -1): 180, (-1, 0): 270, (1, 0): 90}[direction]
            place("gnd", ref, "GND", x, y, {}, rot=rot, hide_value=True)

    uno_nets = {"D8": "TRIG", "D9": "ECHO", "D5": "SERVO1_PWM", "D6": "SERVO2_PWM",
                "+5V": "+5V", "GND": "GND"}
    place("uno", "A1", "Arduino UNO R3 (USB al computador)", 101.6, 101.6, uno_nets,
          text_dx=-12.7, ref_dy=43.18, value_dy=45.72)
    side = {"text_dx": 10.16, "ref_dy": -1.27, "value_dy": 1.27}
    place("hc", "U1", "HC-SR04 (arriba de la botella)", 190.5, 50.8,
          {"VCC": "+5V", "TRIG": "TRIG", "ECHO": "ECHO", "GND": "GND"}, **side)
    place("servo", "M1", "MG90S llave salida 1", 190.5, 88.9,
          {"1": "SERVO1_PWM", "2": "+5V", "3": "GND"}, **side)
    place("servo", "M2", "MG90S llave salida 2", 190.5, 127.0,
          {"1": "SERVO2_PWM", "2": "+5V", "3": "GND"}, **side)
    place("cap", "C1", "1000uF 16V (+ al riel 5V)", 251.46, 88.9, {"1": "+5V", "2": "GND"}, **side)
    place("jack", "J1", "Fuente externa 5V 2A (opcional)", 251.46, 127.0, {"1": "+5V", "2": "GND"}, **side)
    # The PWR_FLAG says the +5V/GND nets are fed (by the USB of the UNO).
    place("flag", "#FLG01", "PWR_FLAG", 251.46, 97.79, {}, rot=180)

    notes = [
        (20.32, 160.02, "Cables reales: HC-SR04 Vcc=cafe, Trig=naranja, Echo=amarillo, Gnd=ROJO (colores invertidos: revisar antes de conectar)."),
        (20.32, 165.1, "Servos MG90S: senal=naranja, +5V=rojo, GND=cafe. Los rieles +5V y GND son los de la protoboard (unir las dos mitades en la fila 30)."),
        (20.32, 170.18, "Con la fuente externa J1: quitar el cable del pin 5V del Arduino al riel + y dejar el GND comun. Nunca juntar dos fuentes de 5 V."),
        (20.32, 175.26, "C1 entre los rieles (pata + al riel +5V) para los picos de los servos. El UNO va por USB al computador que corre caudal-sim maqueta-usb."),
    ]
    for x, y, text in notes:
        items.append(f'(text "{text}" (exclude_from_sim no) (at {x} {y} 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{uid("t", y)}"))')

    lib_block = "\n".join(symbols[k] for k in symbols)
    doc = (
        '(kicad_sch (version 20231120) (generator "eeschema") (generator_version "8.0") '
        f'(uuid "{uid("root")}") (paper "A4") '
        '(title_block (title "Maqueta CAUDAL con Arduino UNO") (rev "1.0") (company "CAUDAL") '
        '(comment 1 "HC-SR04 + 2 servos MG90S + puente USB caudal-sim maqueta-usb")) '
        f"(lib_symbols {lib_block}) " + "\n".join(items) + ' (sheet_instances (path "/" (page "1"))))'
    )
    OUT.write_text(doc, encoding="utf-8")
    (OUT.parent / "caudal.kicad_sym").write_text(
        '(kicad_symbol_lib (version 20241209) (generator "caudal") '
        + " ".join(text.replace('(symbol "caudal:', '(symbol "', 1) for text in CUSTOM.values())
        + ")",
        encoding="utf-8",
    )
    print(OUT)


if __name__ == "__main__":
    main(Path(sys.argv[1]))
