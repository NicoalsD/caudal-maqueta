// Bench simulator of the CAUDAL UNO firmware, on simavr.
//
// Runs the real firmware.elf on a simulated ATmega328P at 16 MHz, in real time, and adds the
// two parts the firmware talks to:
//   - a virtual HC-SR04: when TRIG (D9 = PB1) falls, ECHO (D8 = PB0) goes high for the time
//     the sound needs to travel the distance written in the distance file (millimeters);
//   - two virtual servos: the pulse width on D10 (PB2) and D11 (PB3) is turned into an angle
//     and printed when it changes.
// The serial port of the UNO is exposed as a pseudo-terminal, so `caudal-sim maqueta-usb`
// (or any serial monitor) can open it as if it were /dev/ttyACM0.
//
// Usage: uno_sim <firmware.elf> <distance file> [pty link path]

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "avr_ioport.h"
#include "avr_uart.h"
#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_time.h"

#define CPU_HZ 16000000UL
#define SOUND_US_PER_MM 5.831f  // round trip: 2 / 0.343 mm/us
#define ECHO_DELAY_US 250
#define SERVO_MIN_US 544.0f     // Servo library defaults
#define SERVO_MAX_US 2400.0f
#define RX_QUEUE 4096

static avr_t *avr;
static int pty_master = -1;
static const char *distance_path;
static volatile int running = 1;

static unsigned char rx_queue[RX_QUEUE];
static int rx_head, rx_tail;
static int uart_xon = 1;
static avr_irq_t *uart_in;

static avr_irq_t *echo_irq;
static int trig_high;

static avr_cycle_count_t servo_rise[2];
static int servo_angle[2] = {-1, -1};

static void on_signal(int sig) {
  (void)sig;
  running = 0;
}

static long read_distance_mm(void) {
  FILE *f = fopen(distance_path, "r");
  long mm = -1;
  if (f) {
    if (fscanf(f, "%ld", &mm) != 1) mm = -1;
    fclose(f);
  }
  return mm;
}

static avr_cycle_count_t echo_low(avr_t *a, avr_cycle_count_t when, void *param) {
  (void)when;
  (void)param;
  avr_raise_irq(echo_irq, 0);
  (void)a;
  return 0;
}

static avr_cycle_count_t echo_high(avr_t *a, avr_cycle_count_t when, void *param) {
  (void)when;
  long mm = (long)(intptr_t)param;
  avr_raise_irq(echo_irq, 1);
  avr_cycle_timer_register_usec(a, (uint32_t)(mm * SOUND_US_PER_MM), echo_low, NULL);
  return 0;
}

static void on_trig(struct avr_irq_t *irq, uint32_t value, void *param) {
  (void)irq;
  (void)param;
  if (value) {
    trig_high = 1;
    return;
  }
  if (!trig_high) return;
  trig_high = 0;
  long mm = read_distance_mm();
  if (mm <= 0) return;  // no echo: the firmware times out and answers D -1
  avr_cycle_timer_register_usec(avr, ECHO_DELAY_US, echo_high, (void *)(intptr_t)mm);
}

static void on_servo(struct avr_irq_t *irq, uint32_t value, void *param) {
  (void)irq;
  int i = (int)(intptr_t)param;
  if (value) {
    servo_rise[i] = avr->cycle;
    return;
  }
  if (!servo_rise[i]) return;
  float us = (float)(avr->cycle - servo_rise[i]) * 1e6f / CPU_HZ;
  int angle = (int)((us - SERVO_MIN_US) * 180.0f / (SERVO_MAX_US - SERVO_MIN_US) + 0.5f);
  if (angle != servo_angle[i] && (servo_angle[i] < 0 || abs(angle - servo_angle[i]) >= 1)) {
    servo_angle[i] = angle;
    fprintf(stderr, "[sim] servo %d (D%d): pulso %.0f us -> %d grados\n", i + 1, 10 + i, us, angle);
  }
}

static void on_uart_out(struct avr_irq_t *irq, uint32_t value, void *param) {
  (void)irq;
  (void)param;
  unsigned char c = (unsigned char)value;
  if (write(pty_master, &c, 1) < 0 && errno != EAGAIN) perror("write pty");
}

static void on_xon(struct avr_irq_t *irq, uint32_t value, void *param) {
  (void)irq;
  (void)value;
  (void)param;
  uart_xon = 1;
}

static void on_xoff(struct avr_irq_t *irq, uint32_t value, void *param) {
  (void)irq;
  (void)value;
  (void)param;
  uart_xon = 0;
}

static void pump_pty(void) {
  unsigned char buf[256];
  ssize_t n = read(pty_master, buf, sizeof buf);
  for (ssize_t i = 0; i < n; i++) {
    int next = (rx_tail + 1) % RX_QUEUE;
    if (next == rx_head) break;
    rx_queue[rx_tail] = buf[i];
    rx_tail = next;
  }
  if (uart_xon && rx_head != rx_tail) {
    avr_raise_irq(uart_in, rx_queue[rx_head]);
    rx_head = (rx_head + 1) % RX_QUEUE;
  }
}

static double wall_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "uso: %s <firmware.elf> <archivo de distancia> [enlace del pty]\n", argv[0]);
    return 2;
  }
  distance_path = argv[2];
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);

  elf_firmware_t fw;
  memset(&fw, 0, sizeof fw);
  if (elf_read_firmware(argv[1], &fw) != 0) {
    fprintf(stderr, "no se pudo leer %s\n", argv[1]);
    return 1;
  }
  avr = avr_make_mcu_by_name("atmega328p");
  if (!avr) return 1;
  avr_init(avr);
  fw.frequency = CPU_HZ;
  avr_load_firmware(avr, &fw);
  avr->frequency = CPU_HZ;
  avr->log = LOG_WARNING;

  pty_master = posix_openpt(O_RDWR | O_NOCTTY);
  if (pty_master < 0 || grantpt(pty_master) || unlockpt(pty_master)) {
    perror("pty");
    return 1;
  }
  struct termios tio;
  tcgetattr(pty_master, &tio);
  cfmakeraw(&tio);
  tcsetattr(pty_master, TCSANOW, &tio);
  fcntl(pty_master, F_SETFL, O_NONBLOCK);
  const char *slave = ptsname(pty_master);
  if (argc > 3) {
    unlink(argv[3]);
    if (symlink(slave, argv[3]) != 0) perror("symlink");
  }
  fprintf(stderr, "[sim] UNO simulado. Puerto serie: %s%s%s\n", slave, argc > 3 ? " -> " : "",
          argc > 3 ? argv[3] : "");

  uint32_t flags = 0;
  avr_ioctl(avr, AVR_IOCTL_UART_GET_FLAGS('0'), &flags);
  flags &= ~AVR_UART_FLAG_STDIO;
  avr_ioctl(avr, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT),
                          on_uart_out, NULL);
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUT_XON),
                          on_xon, NULL);
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUT_XOFF),
                          on_xoff, NULL);
  uart_in = avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_INPUT);

  echo_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 0);
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 1), on_trig, NULL);
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 2), on_servo,
                          (void *)(intptr_t)0);
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 3), on_servo,
                          (void *)(intptr_t)1);

  double start = wall_seconds();
  avr_cycle_count_t next_pump = 0;
  while (running) {
    int state = avr_run(avr);
    if (state == cpu_Done || state == cpu_Crashed) {
      fprintf(stderr, "[sim] el firmware se detuvo (estado %d)\n", state);
      break;
    }
    if (avr->cycle >= next_pump) {
      next_pump = avr->cycle + CPU_HZ / 20000;  // every 50 us of AVR time
      pump_pty();
      double ahead = (double)avr->cycle / CPU_HZ - (wall_seconds() - start);
      if (ahead > 0.002) usleep((useconds_t)(ahead * 1e6));
    }
  }
  if (argc > 3) unlink(argv[3]);
  return 0;
}
