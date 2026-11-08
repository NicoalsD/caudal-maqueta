#include "caudal/iso8601.h"

namespace caudal {

namespace {

// Days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant's algorithm).
int64_t days_from_civil(int64_t y, int64_t m, int64_t d) {
  y -= m <= 2 ? 1 : 0;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const int64_t yoe = y - era * 400;
  const int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

void civil_from_days(int64_t z, int64_t* y, int64_t* m, int64_t* d) {
  z += 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const int64_t doe = z - era * 146097;
  const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const int64_t mp = (5 * doy + 2) / 153;
  *d = doy - (153 * mp + 2) / 5 + 1;
  *m = mp < 10 ? mp + 3 : mp - 9;
  *y = yoe + era * 400 + (*m <= 2 ? 1 : 0);
}

bool digits(const char* text, size_t len, size_t pos, size_t count, int64_t* value) {
  if (pos + count > len) return false;
  int64_t v = 0;
  for (size_t i = pos; i < pos + count; ++i) {
    if (text[i] < '0' || text[i] > '9') return false;
    v = v * 10 + (text[i] - '0');
  }
  *value = v;
  return true;
}

bool is_leap(int64_t y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

int64_t days_in_month(int64_t y, int64_t m) {
  static const int64_t kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return (m == 2 && is_leap(y)) ? 29 : kDays[m - 1];
}

}  // namespace

bool parse_iso8601_ms(const char* text, size_t len, int64_t* epoch_ms) {
  if (text == nullptr || epoch_ms == nullptr) return false;
  // Minimum: "YYYY-MM-DDTHH:MM:SSZ" (20 characters).
  if (len < 20 || text[4] != '-' || text[7] != '-' || text[10] != 'T' || text[13] != ':' ||
      text[16] != ':') {
    return false;
  }
  int64_t year, month, day, hour, minute, second;
  if (!digits(text, len, 0, 4, &year) || !digits(text, len, 5, 2, &month) ||
      !digits(text, len, 8, 2, &day) || !digits(text, len, 11, 2, &hour) ||
      !digits(text, len, 14, 2, &minute) || !digits(text, len, 17, 2, &second)) {
    return false;
  }
  if (month < 1 || month > 12 || day < 1 || day > days_in_month(year, month) || hour > 23 ||
      minute > 59 || second > 60) {
    return false;
  }

  size_t pos = 19;
  int64_t millis = 0;
  if (pos < len && text[pos] == '.') {
    ++pos;
    size_t start = pos;
    int64_t scale = 100;
    while (pos < len && text[pos] >= '0' && text[pos] <= '9') {
      if (scale > 0) {
        millis += (text[pos] - '0') * scale;
        scale /= 10;
      }
      ++pos;
    }
    if (pos == start || pos - start > 9) return false;
  }

  int64_t offset_min = 0;
  if (pos >= len) return false;
  if (text[pos] == 'Z') {
    if (pos + 1 != len) return false;
  } else if (text[pos] == '+' || text[pos] == '-') {
    const int64_t sign = text[pos] == '-' ? -1 : 1;
    int64_t oh, om;
    if (!digits(text, len, pos + 1, 2, &oh)) return false;
    size_t after;
    if (pos + 3 < len && text[pos + 3] == ':') {
      if (!digits(text, len, pos + 4, 2, &om)) return false;
      after = pos + 6;
    } else {
      if (!digits(text, len, pos + 3, 2, &om)) return false;
      after = pos + 5;
    }
    if (after != len || oh > 23 || om > 59) return false;
    offset_min = sign * (oh * 60 + om);
  } else {
    return false;
  }

  const int64_t days = days_from_civil(year, month, day);
  const int64_t seconds = days * 86400 + hour * 3600 + minute * 60 + second;
  *epoch_ms = seconds * 1000 + millis - offset_min * 60000;
  return true;
}

bool format_iso8601_utc(int64_t epoch_seconds, char* out) {
  const int64_t days = epoch_seconds >= 0 ? epoch_seconds / 86400
                                          : -((-epoch_seconds + 86399) / 86400);
  int64_t rem = epoch_seconds - days * 86400;
  int64_t y, m, d;
  civil_from_days(days, &y, &m, &d);
  if (y < 0 || y > 9999) return false;
  const int64_t hh = rem / 3600;
  const int64_t mm = (rem % 3600) / 60;
  const int64_t ss = rem % 60;
  // Fixed layout, written digit by digit so no printf is needed.
  size_t pos = 0;
  auto put_digits = [&out, &pos](int64_t value, int count) {
    for (int i = count - 1; i >= 0; --i) {
      int64_t divisor = 1;
      for (int k = 0; k < i; ++k) divisor *= 10;
      out[pos++] = static_cast<char>('0' + (value / divisor) % 10);
    }
  };
  put_digits(y, 4);
  out[pos++] = '-';
  put_digits(m, 2);
  out[pos++] = '-';
  put_digits(d, 2);
  out[pos++] = 'T';
  put_digits(hh, 2);
  out[pos++] = ':';
  put_digits(mm, 2);
  out[pos++] = ':';
  put_digits(ss, 2);
  out[pos++] = 'Z';
  out[pos] = '\0';
  return true;
}

}  // namespace caudal
