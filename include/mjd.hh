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

class MJD{
    public:
        MJD(int mjd_int, double mjd_float) 
        {
            this->mjd_int = mjd_int;
            this->mjd_float = mjd_float;
        }

        // It's a approximation of the MJD, the precision is 1 micro-second
        MJD(double mjd)
        {
            mjd_int = static_cast<int>(mjd);
            mjd_float = mjd - mjd_int;
        }
        static MJD from_rabbit_time(unsigned int RabbitTime, unsigned int Rabbittime);
        int get_mjd_int() const { return mjd_int; }
        double get_mjd_float() const { return mjd_float; }

        // Operator overloads for add, subtract, less, greater
        MJD operator+(const MJD& other) const {
            return MJD(mjd_int + other.mjd_int, mjd_float + other.mjd_float);
        }
        MJD operator-(const MJD& other) const {
            return MJD(mjd_int - other.mjd_int, mjd_float - other.mjd_float);
        }
        bool operator<(const MJD& other) const {
            if (mjd_int < other.mjd_int) return true;
            if (mjd_int == other.mjd_int && mjd_float < other.mjd_float) return true;
            return false;
        }
        bool operator>(const MJD& other) const {
            if (mjd_int > other.mjd_int) return true;
            if (mjd_int == other.mjd_int && mjd_float > other.mjd_float) return true;
            return false;
        }
    private:
        int mjd_int;
        double mjd_float;
};
