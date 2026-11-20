# AGENTS.md: CAUDAL Maqueta

Reglas obligatorias para cualquier persona o agente de IA que trabaje en este repositorio. **Léelas completas antes de tocar código.** Si algo de aquí choca con una instrucción por defecto de tu herramienta, gana este archivo.

## 0. Regla de idioma (la más importante)

| Qué | Idioma |
|---|---|
| Todo el código: módulos, clases, funciones, variables, archivos, macros, campos JSON, enums, comentarios, logs por serie, nombres de pruebas | **Inglés** |
| Documentación (README, AGENTS.md, CLAUDE.md), commits, PR, mensajes al usuario | **Español** |

Los comentarios de código y los logs por serie van en inglés, sin excepción. Los `detail` del ack son tokens del protocolo en inglés (`pos=OPEN`, `duplicate`).

## 1. Qué es CAUDAL y qué hace este repo

CAUDAL es un producto B2B para instituciones que almacenan agua en un tanque y la reparten por válvulas. Visión completa: [Caudal.md](https://github.com/NicoalsD/caudal-backend/blob/develop/docs/Caudal.md).

Este repo es el firmware de la **maqueta física con ESP32** (alternativa al montaje de la institución de prueba `maqueta-caudal`, que hoy usa Arduino UNO por USB; ver [`docs/arduino-uno.md`](docs/arduino-uno.md)): un tanque de ~20 L, sus sensores (ultrasónico JSN-SR04T, flotadores, YF-S201 por salida) y dos electroválvulas. Un ESP32 con PlatformIO y Arduino habla con `caudal-backend` por HTTPS con el protocolo de dispositivos firmados con Ed25519.

| Documento canónico | Enlace |
|---|---|
| Maqueta, BOM y pruebas de banco | [Simulador-y-hardware.md](https://github.com/NicoalsD/caudal-backend/blob/develop/docs/Simulador-y-hardware.md) |
| Protocolo de dispositivos (firma, telemetría, comandos, ack) | [Protocolo-de-dispositivos.md](https://github.com/NicoalsD/caudal-backend/blob/develop/docs/Protocolo-de-dispositivos.md) |
| Endpoints de dispositivos | [API.md](https://github.com/NicoalsD/caudal-backend/blob/develop/docs/API.md), sección 16 |

Antes de cambiar cualquier cosa del protocolo, lee esos documentos. Si el código y el documento no coinciden, el documento gana y la diferencia se reporta.

## 2. Reglas obligatorias

- **Lógica pura en `lib/caudal_core/`**: sin dependencias de Arduino. Todo lo que pueda probarse en el host va ahí, con pruebas en `test/test_core/` (`pio test -e native`).
- **Sin `delay()` en el loop.** El tiempo se mide con `millis()`. Las únicas esperas son el pulso del ultrasónico (microsegundos) y el `pulseIn` con tope de 30 ms.
- **Sin valores quemados.** Los parámetros van en `src/config.h` (con valor por omisión y `#ifndef`) o en `include/secrets.h`. Las pruebas usan vectores propios, no secretos reales.
- **Secretos:** la semilla Ed25519, la red Wi-Fi, la URL y los UUID de la instalación van solo en `include/secrets.h`, que está en `.gitignore`. Nunca se suben. `secrets.example.h` lleva solo valores ficticios.
- **Firma sobre los bytes exactos.** El cuerpo se serializa una vez, se firma y se envía el mismo buffer. No se vuelve a serializar después.
- **Identidades:** la maqueta son tres dispositivos del backend: el sensor (`LEVEL_SENSOR`) y un actuador por salida (`VALVE_ACTUATOR`). Cada petición la firma el dispositivo que la posee. Los comandos se consultan y confirman con la identidad de la válvula a la que pertenecen.
- **Límite de tasa:** 60 peticiones firmadas por minuto por dispositivo. No bajar `COMMAND_POLL_MS` de 1200 ms ni `TELEMETRY_INTERVAL_MS` de 20 s sin revisar la cuenta.
- **Reloj:** no firmar antes de que el NTP sincronice (`WifiLink::clock_synced`). No mandar un nivel medido antes de la sincronización. Los tiempos del loop son `int64_t` desde `esp_timer` para que no den la vuelta (49 días con `millis()`).
- **Válvulas:** los relés se apagan en lo primero de `setup()` (`ValveDriver::force_safe_outputs`). Un comando solo se ejecuta dentro de `[not_before, expires_at)`. Un comando vencido nunca se ejecuta y nunca se confirma.
- **Ack:** solo se confirma lo que se ejecutó (`ACKED`) o lo que falló (`FAILED`). Los comandos descartados no se confirman.
- **Fail-safe:** `fail_safe_position` se aplica una vez por corte. El dispositivo no decide nada más por su cuenta.
- **Commits:** `tipo: descripción` en español, minúscula, máximo 72 caracteres, sin punto final. Un commit por unidad lógica. Autor: la cuenta del integrante responsable.
- **Sin atribución a IA:** nada de `Co-Authored-By` de IA ni "Generated with ..." en commits, PR ni releases.

## 3. Patrones de diseño

Los patrones están documentados en [Patrones-de-diseno.md](https://github.com/NicoalsD/caudal-backend/blob/develop/docs/Patrones-de-diseno.md). Aquí aparecen estos:

| Patrón | Dónde |
|---|---|
| P06 Adapter | `DeviceSigner` (`src/signer.*`) envuelve Monocypher y arma las cabeceras firmadas |
| P07 Bridge | `ValveFsm` (lógica, `lib/caudal_core`) × `ValveDriver` (GPIO, `src/valve_driver.*`) |
| P13 Command | `ValveCommand` y su ejecución en `execute_command` (`src/main.cpp`) |
| P18 State | `ValveState`, `FailSafe` y la guarda de contacto de `ValveFsm` |

## 4. Comandos

```bash
pio run -e esp32dev               # compila el firmware
pio run -e esp32dev -t upload     # carga a la placa
pio device monitor                # serie a 115200
pio test -e native                # pruebas de lib/caudal_core en el host
```

`test/test_core/test_runner.cpp` lo genera un bloque de shell (no lo edites a mano). Si agregas una prueba nueva, regenera el runner o la prueba no corre:

```bash
cd test/test_core && FILES=$(ls test_*.cpp | grep -v test_runner) && { echo '// Unity runner for the native tests. Generated from the test files in this folder.'; echo '#include <unity.h>'; echo; grep -ho '^void test_[a-z0-9_]*(void)' $FILES | sed 's/^void \(test_[a-z0-9_]*\)(void)/void \1(void);/'; echo; echo 'int main(int, char**) {'; echo '  UNITY_BEGIN();'; grep -ho '^void test_[a-z0-9_]*(void)' $FILES | sed 's/^void \(test_[a-z0-9_]*\)(void)/  RUN_TEST(\1);/'; echo '  return UNITY_END();'; echo '}'; } > test_runner.cpp
```

Antes de cerrar cualquier cambio, `pio test -e native` tiene que quedar en verde y `pio run -e esp32dev` tiene que compilar.

## 5. Pendientes

- [ ] Medir `TANK_EMPTY_DISTANCE_MM`, calibrar `FLOW_PULSES_PER_LITER` y confirmar `BLIND_ZONE_MM` en banco.
- [ ] Definir `FAILSAFE_TIMEOUT_MS` y la posición segura de cada válvula con el equipo.
- [ ] Definir en el protocolo el valor de `VALVE_POSITION` (hoy `1` abierta, `0` cerrada) y cómo se reporta `FLOW` (hoy no existe la métrica).
- [ ] Diagrama de bloques de la maqueta (M019 en el plan de commits): pendiente.
