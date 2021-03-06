#ifndef __solverEhqp_primal_h__
#define __solverEhqp_primal_h__

#include <Eigen/Dense>
#include <vector>
#include <iostream>
#include "weighted_hqp/HCod.hpp"

namespace hcod{
    class Ehqp_primal{
        public:
            Ehqp_primal(const std::vector<h_structure> &h, const Eigen::MatrixXd & Y);
            Ehqp_primal(const std::vector<h_structure> &h);
            ~Ehqp_primal(){};
        
        private: 

        public:
            void setProblem(const std::vector<h_structure> &h, const Eigen::MatrixXd & Y){
                h_ = h;
                Y_ = Y;

                nh_ = Y.cols();
                p_ = h.size();
                int num=0;

                for (int i =0; i<p_; i++)
                    num += h_[i].m;

                y_.setZero(num);
            }
            void compute();
            Eigen::VectorXd & getx(){
                return x_;
            }
            Eigen::VectorXd & gety(){
                return y_;
            }
            const Eigen::VectorXd & getx() const{
                return x_;
            }
            const Eigen::VectorXd & gety() const{
                return y_;
            }
            void setWProblem(const std::vector<h_structure> &h){
                h_ = h;
            
                p_ = h.size();
                int num=0;

                for (int i =0; i<p_; i++)
                    num += h_[i].m;

                y_.setZero(num);
            }
           
        private:
           std::vector<h_structure> h_;
           Eigen::MatrixXd Y_, L_, M1_, W1_;
           int p_, nh_;
           Eigen::VectorXd y_, b_, e_, x_;
           bool _isweighted;

    };
}

#endif