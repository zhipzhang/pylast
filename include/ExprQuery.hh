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
        virtual void init_variables() = 0;
        void add_expr(const std::string& expr)
        {
            if(expr_.empty())
            {
                expr_ = expr;
            }
            else 
            {
                expr_ += " && " + expr;    
            }
        }
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