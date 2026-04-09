/**
 * @file mjd.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Define the MJD class
 * @version 0.1
 * @date 2026-04-04
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <utility>
constexpr int MJD19700101 = 40587;
constexpr int TAI2UTC = 37;
constexpr int SECONDS_PER_DAY = 86400;
class MJD {
public:
  MJD(int mjd_int, double mjd_float) {
    this->mjd_int = mjd_int;
    this->mjd_float = mjd_float;
  }

  // It's a approximation of the MJD, the precision is 1 micro-second
  MJD(double mjd) {
    mjd_int = static_cast<int>(mjd);
    mjd_float = mjd - mjd_int;
  }
  static MJD from_rabbit_time(unsigned int RabbitTime,
                              unsigned int Rabbittime) {

      
    // 1. 计算以秒为单位的 UTC 时间
    long long utc_seconds = static_cast<long long>(RabbitTime) - TAI2UTC;

    long long days = utc_seconds / SECONDS_PER_DAY;
    long long remaining_seconds = utc_seconds % SECONDS_PER_DAY;

    if (remaining_seconds < 0) {
      days -= 1;
      remaining_seconds += SECONDS_PER_DAY;
    }

    // 3. 计算 MJD 整数部分
    int mjd_int = MJD19700101 + static_cast<int>(days);

    // 4. 计算 MJD 小数部分：(当天秒数 + 纳秒换算成的秒数) / 每天总秒数
    double mjd_double =
        (remaining_seconds + Rabbittime * 8.0 / 1e9) / SECONDS_PER_DAY;
    return MJD(mjd_int, mjd_double);
  }
  std::pair<unsigned int, unsigned int> to_rabbit_time() const {
    constexpr int64_t TICKS_PER_SECOND = 125000000LL;

    int64_t ticks_in_day = static_cast<int64_t>(
        std::round(mjd_float * SECONDS_PER_DAY * TICKS_PER_SECOND));

    // 2. 完美分离出当天的“完整秒数”和“不足一秒的小尾巴”
    int64_t remaining_seconds = ticks_in_day / TICKS_PER_SECOND;
    unsigned int fractional_ticks =
        static_cast<unsigned int>(ticks_in_day % TICKS_PER_SECOND);

    // 引入局部变量，避免修改 this->mjd_int
    int current_mjd_int = this->mjd_int;

    // 防御性编程：如果浮点数极端接近 1.0 导致进位，将秒数进位到天数
    if (remaining_seconds >= SECONDS_PER_DAY) {
        remaining_seconds -= SECONDS_PER_DAY;
        current_mjd_int += 1; // 修改局部变量，对象本身不受影响
    }

    // 3. 计算自 1970-01-01 以来的总整天数
    int64_t days = static_cast<int64_t>(current_mjd_int) - MJD19700101;

    // 4. 组装 UTC 总秒数：整天秒数 + 当天剩余秒数
    int64_t utc_seconds = days * SECONDS_PER_DAY + remaining_seconds;

    unsigned int rabbitTime = static_cast<unsigned int>(utc_seconds + TAI2UTC);
    return {rabbitTime, fractional_ticks};
}
  int get_mjd_int() const { return mjd_int; }
  double get_mjd_float() const { return mjd_float; }
  double to_float() const { return mjd_int + mjd_float; }

  // Operator overloads for add, subtract, less, greater
  MJD operator+(const MJD &other) const {
    return MJD(mjd_int + other.mjd_int, mjd_float + other.mjd_float);
  }
  MJD operator-(const MJD &other) const {
    return MJD(mjd_int - other.mjd_int, mjd_float - other.mjd_float);
  }
  bool operator<(const MJD &other) const {
    if (mjd_int < other.mjd_int)
      return true;
    if (mjd_int == other.mjd_int && mjd_float < other.mjd_float)
      return true;
    return false;
  }
  bool operator>(const MJD &other) const {
    if (mjd_int > other.mjd_int)
      return true;
    if (mjd_int == other.mjd_int && mjd_float > other.mjd_float)
      return true;
    return false;
  }

private:
  int mjd_int;
  double mjd_float;
};
