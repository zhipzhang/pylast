/**
 * @file ExprQuery.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief use muparser to evaluate the expression
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #pragma once


 #include "muParser.h"

 class ExprQuery
 {
    public:
    ExprQuery() = default;
    virtual ~ExprQuery() = default;
    protected:
        /**
         * @brief Append a new expression with AND logic
         * @param expr The expression to append
         * @details If current expression is empty, sets it directly.
         *          Otherwise, appends the new expression with && operator.
         */
        void add_expr(const std::string& expr) {
            expr_ = expr_.empty() ? expr : expr_ + " && " + expr;
        }
        /**
         * @brief Set the expr object, the string is like "var1 > 0 && var2 < 10"
         * 
         * @param expr 
         */
        void set_expr(const std::string& expr)
        {
            expr_ = expr;
            parser_.SetExpr(expr_.c_str());
        }
        void set_expr()
        {
            parser_.SetExpr(expr_.c_str());
        }
    mu::Parser parser_;
    private:
    std::string expr_;
    
 };