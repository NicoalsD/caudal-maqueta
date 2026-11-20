# CAUDAL maqueta

> **Montaje actual (2026-11-20):** la institución de prueba `maqueta-caudal` usa un **Arduino UNO por USB** con un HC-SR04 y dos servos MG90S que pisan las mangueras de suero ([docs/arduino-uno.md](docs/arduino-uno.md)). Este repositorio contiene el firmware del **ESP32**, que queda como alternativa futura. El firmware no cambia con el alcance B2B; la calibración y los ángulos se configurarán desde la app (por implementar).

Maqueta física del acueducto a escala de laboratorio: **1 tanque, sus sensores y 2 tuberías de salida**. Un ESP32 (alternativa al montaje actual con Arduino UNO) mide el nivel del tanque y el caudal de cada salida, abre y cierra cada electroválvula, y habla con [`caudal-backend`](https://github.com/NicoalsD/caudal-backend) por HTTPS con el protocolo de dispositivos (firma Ed25519 por cada petición).

Así cierra el lazo completo: el backend decide, manda un comando firmado, la maqueta lo ejecuta y responde con un ack. El circuito es cerrado: el agua sale del tanque por las dos salidas, cae al reservorio y la bomba la regresa al tanque.

Es la contraparte real de la maqueta virtual de [`caudal-simulador`](https://github.com/NicoalsD/caudal-simulador). Ambas usan el mismo protocolo.

> Estado: firmware en construcción (fase 13, CAUDAL v2). Las pruebas de banco todavía no se han hecho.

## Lista de materiales

Resumen. La lista completa, con las notas de verificación, está en [Simulador-y-hardware.md, sección 8](https://github.com/NicoalsD/caudal-backend/blob/develop/docs/Simulador-y-hardware.md) (documento canónico).

| Cant. | Componente | Función |
|---|---|---|
| 1 | Tanque transparente de ~20 L | Almacenamiento de la maqueta |
| 1 | Reservorio inferior y bomba de 12 V | Sube el agua de vuelta al tanque |
| 1 | Flotador de corte de la bomba | Corta la bomba por hardware con el tanque lleno |
| 1 | ESP32 (DevKit o S3) | Control, medición, firma y comunicación |
| 1 | Ultrasónico JSN-SR04T | Nivel del tanque. Zona ciega de ~20 cm: verificar contra la altura del tanque |
| 2 | Electroválvulas de 12 V NC de acción directa | Una por salida. Las pilotadas no abren con poca cabeza |
| 2 | Caudalímetro YF-S201 | Caudal de cada salida, con calibración en pulsos por litro |
| 2 | Relé o MOSFET con diodo flyback | Maniobra de cada electroválvula |
| 1 | Fuente de 12 V y convertidor DC-DC a 5 V | Alimentación de válvulas, bomba, ESP32 y sensor |
| 2 | Flotadores de nivel alto y bajo | Respaldo del ultrasónico |

Pines de referencia (verificar contra la placa antes de cablear):

| GPIO | Uso |
|---|---|
| 25 / 26 | Relé de la válvula 1 / 2 |
| 27 / 34 | TRIG / ECHO del JSN-SR04T (ECHO con divisor de tensión) |
| 32 / 33 | Pulsos de los YF-S201 de la salida 1 / 2 |
| 18 / 19 | Flotador de nivel alto / bajo (con pull-up) |

La conexión es: GND común entre fuente, DC-DC, ESP32 y módulos, y fusible en la línea de 12 V. Cada carga inductiva lleva su diodo flyback.

## Estructura

- `src/`: firmware de Arduino. Un módulo por parte del hardware (`level_sensor`, `flow_sensor`, `valve_driver`, `float_switches`, `wifi_link`) y los adaptadores de protocolo (`signer`, `api_client`). `main.cpp` es el loop no bloqueante y el que decide qué identidad firma cada petición.
- `lib/caudal_core/`: lógica pura, sin Arduino. Firma canónica, parseo de comandos, máquina de estados de válvulas, conversión de distancia a nivel, mediana y caudal. Aquí están las pruebas.
- `lib/monocypher/`: [Monocypher](https://monocypher.org) 4.0.3, vendorizado sin cambios (licencia BSD-2 o CC0, ver `LICENCE.md`).
- `lib/caudal_core/src/sha256_vendor.c`: SHA-256 de Brad Conte (B-Con/crypto-algorithms), de dominio público. Solo hashea cuerpos públicos, no secretos.
- `include/secrets.example.h`: plantilla de secretos. `include/secrets.h` está en `.gitignore`.
- `test/test_core/`: pruebas Unity de `lib/caudal_core`.

## Cómo compilar

Requiere [PlatformIO](https://platformio.org) (`uv tool install platformio`).

```bash
pio run -e esp32dev
```

## Configuración

1. Copiar la plantilla y llenarla:

   ```bash
   cp include/secrets.example.h include/secrets.h
   ```

   `secrets.h` lleva la red Wi-Fi, la URL del backend, el certificado raíz del backend en PEM y tres identidades. Cada identidad es un dispositivo del backend, con su UUID y su semilla Ed25519 (la clave privada):

   - `SENSOR_*`: el sensor de nivel del tanque (`LEVEL_SENSOR`).
   - `VALVE_1_*` y `VALVE_2_*`: un actuador por salida (`VALVE_ACTUATOR`), con el UUID de la válvula que mueve (`VALVE_1_ID`, `VALVE_2_ID`).

   Nunca se sube `secrets.h` al repositorio.

2. Generar una semilla (32 bytes en hexadecimal) por cada identidad, y su clave pública. Cada clave pública se registra en el backend con `POST /api/v1/devices/{id}/keys`, en el dispositivo correspondiente:

   ```bash
   python3 -c "import secrets; print(secrets.token_hex(32))"
   ```

   ```bash
   read -rs SEED && uv run --with cryptography python -I -c "import base64,sys; from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey as K; from cryptography.hazmat.primitives import serialization as s; pk=K.from_private_bytes(bytes.fromhex(sys.stdin.read().strip())).public_key().public_bytes(s.Encoding.Raw,s.PublicFormat.Raw); print(base64.urlsafe_b64encode(pk).rstrip(b'=').decode())" <<< "$SEED"
   ```

3. Los parámetros de `src/config.h` tienen valores por omisión y se pueden cambiar con `build_flags`. Los que están marcados **POR MEDIR** o **POR CALIBRAR** deben ajustarse en la maqueta: la distancia del sensor al fondo (`TANK_EMPTY_DISTANCE_MM`), los pulsos por litro (`FLOW_PULSES_PER_LITER`), la posición segura de cada válvula (`FAILSAFE_VALVE_1`, `FAILSAFE_VALVE_2`) y el tiempo de pérdida de contacto (`FAILSAFE_TIMEOUT_MS`).

## Cómo cargar

```bash
pio run -e esp32dev -t upload
pio device monitor
```

## Cómo probar

Las pruebas de la lógica pura corren en el host, sin placa:

```bash
pio test -e native
```

Cubren la firma canónica y Ed25519 (con los vectores de RFC 8032 y un vector independiente generado con `cryptography`), SHA-256, base64url, el parseo de fechas ISO-8601 y de los comandos, la máquina de estados de las válvulas (ventanas, duplicados, comandos vencidos, fail-safe), la zona ciega, la mediana y la conversión de caudal.

## Pruebas de banco

Primero sin agua y luego con agua, en este orden. Los pasos vienen de [Simulador-y-hardware.md, sección 8.6](https://github.com/NicoalsD/caudal-backend/blob/develop/docs/Simulador-y-hardware.md).

1. **Llaves.** Abrir y cerrar cada electroválvula con un comando firmado. Revisar el ack y la posición reportada.
2. **Nivel.** Comparar la lectura del JSN-SR04T con una regla en el tanque y revisar la zona ciega.
3. **Caudal.** Medir un volumen conocido y calibrar `FLOW_PULSES_PER_LITER` de cada YF-S201.
4. **Flotador de corte.** Llenar el tanque y comprobar que la bomba se corta sin el firmware.
5. **Pérdida de contacto.** Apagar el Wi-Fi y comprobar que se aplica `fail_safe_position` después de `FAILSAFE_TIMEOUT_MS`.
6. **Corrida completa.** Lazo cerrado con el piloto del backend en `AUTOMATIC`: comando del piloto, ack y decisión auditada.

## Notas del protocolo

- **Identidades.** Como en el backend y en el simulador, cada dispositivo firma con su propia identidad. El sensor manda `LEVEL` y `RSSI`. Cada actuador consulta sus comandos y manda su `VALVE_POSITION`, así que la posición de cada válvula no es ambigua. Cada identidad tiene su propio límite de 60 peticiones por minuto.
- **Límite de tasa.** Cada actuador sondea sus comandos cada 1,2 s (50 por minuto) y la telemetría se manda cada 20 s. Con un sondeo de 1 s exacto se llegaría al límite.
- **Valor de `VALVE_POSITION`.** Se manda `1` si el relé está energizado (válvula abierta) y `0` si no. El protocolo no define el valor; por definir.
- **Caudal.** El protocolo no tiene la métrica `FLOW`. El caudal se mide y se muestra por serie, pero no se envía al backend hasta que el protocolo lo permita.
- **Acks.** Solo se confirma lo que se ejecutó (`pos=OPEN` o `pos=CLOSED`, leído del pin del relé) o un duplicado. Un comando vencido o descartado no se confirma.
- **Sin finales de carrera.** La BOM no tiene finales de carrera. La posición que se reporta sale de la salida del relé, no de la llave física. Esto se debe confirmar en banco.
- **Reinicio.** Al arrancar, los relés quedan apagados y las válvulas cerradas, aunque el backend tenga la válvula como `OPEN`. Hasta que llegue un comando nuevo, el backend y la maqueta pueden no coincidir.
- **Reloj.** Ninguna petición firmada sale antes de que el NTP sincronice la hora (la ventana es de ±300 s).
- **Sin cola offline.** Si una telemetría falla, esa muestra se pierde. El siguiente envío es una lectura nueva.

## Licencia

Por definir. Los repositorios de CAUDAL todavía no tienen un archivo de licencia. Monocypher, que está vendorizado en `lib/monocypher/`, se distribuye bajo BSD-2 o CC0 (ver `lib/monocypher/LICENCE.md`).
